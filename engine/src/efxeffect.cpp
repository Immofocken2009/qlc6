/*
  Q Light Controller Plus
  efxeffect.cpp

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QtMath>
#include <cmath>
#include <QVarLengthArray>

#include "efxeffect.h"

#define MAX_PARAMS   64
#define MAX_CHANNELS 64
#define MAX_JSON     (1024 * 1024)
#define MAX_KEYS     1024

static const int BuiltinCount = 5; // t, theta, i, n, h

QStringList EFXEffect::builtinVariables()
{
    return QStringList() << "t" << "theta" << "i" << "n" << "h";
}

EFXEffect::EFXEffect()
    : m_schema(1)
    , m_version(1)
    , m_hasReverse(false)
    , m_panIndex(-1)
    , m_tiltIndex(-1)
{
}

static bool setError(QString *error, const QString& msg)
{
    if (error)
        *error = msg;
    return false;
}

static bool isReservedName(const QString& name)
{
    static const QStringList reserved = QStringList()
        << "t" << "theta" << "i" << "n" << "h" << "pi" << "tau" << "e"
        << "sin" << "cos" << "tan" << "asin" << "acos" << "atan" << "atan2"
        << "abs" << "sqrt" << "pow" << "exp" << "log" << "floor" << "ceil"
        << "round" << "sign" << "min" << "max" << "clamp" << "mix" << "mod"
        << "smoothstep" << "tri" << "saw" << "square" << "pulse" << "noise" << "if";
    return reserved.contains(name);
}

static bool parseKeys(const QJsonValue& value, QVector<EFXKey> *out, QString *error)
{
    if (!value.isArray())
        return setError(error, QStringLiteral("'keys' must be an array"));

    const QJsonArray arr = value.toArray();
    if (arr.isEmpty())
        return setError(error, QStringLiteral("'keys' is empty"));
    if (arr.size() > MAX_KEYS)
        return setError(error, QStringLiteral("too many keys (max %1)").arg(MAX_KEYS));

    QVector<EFXKey> keys;
    for (int i = 0; i < arr.size(); i++)
    {
        const QJsonObject ko = arr.at(i).toObject();
        if (!ko.value("t").isDouble() || !ko.value("v").isDouble())
            return setError(error, QStringLiteral("key %1: 't' and 'v' must be numbers").arg(i));

        EFXKey k;
        k.t = ko.value("t").toDouble();
        k.v = ko.value("v").toDouble();
        if (!std::isfinite(k.t) || !std::isfinite(k.v) || k.t < 0.0 || k.t > 1.0)
            return setError(error, QStringLiteral("key %1: 't' must be within 0..1 and 'v' finite").arg(i));
        if (!keys.isEmpty() && k.t <= keys.last().t)
            return setError(error, QStringLiteral("key %1: 't' must be strictly increasing").arg(i));

        const QString ease = ko.value("ease").toString("linear");
        if (ease == "linear")
            k.ease = EFXKey::Linear;
        else if (ease == "smooth")
            k.ease = EFXKey::Smooth;
        else if (ease == "step")
            k.ease = EFXKey::Step;
        else
            return setError(error, QStringLiteral("key %1: unknown ease '%2'").arg(i).arg(ease));
        keys << k;
    }
    *out = keys;
    return true;
}

QSharedPointer<EFXEffect> EFXEffect::fromJson(const QByteArray& json, QString *error)
{
    if (json.size() > MAX_JSON)
    {
        setError(error, QStringLiteral("effect file too large"));
        return QSharedPointer<EFXEffect>();
    }

    QJsonParseError perr;
    QJsonDocument doc = QJsonDocument::fromJson(json, &perr);
    if (doc.isNull() || !doc.isObject())
    {
        setError(error, QStringLiteral("invalid JSON: %1").arg(perr.errorString()));
        return QSharedPointer<EFXEffect>();
    }

    QJsonObject root = doc.object();
    QSharedPointer<EFXEffect> fx(new EFXEffect());
    fx->m_json = json;

    fx->m_schema = root.value("schema").toInt(1);
    if (fx->m_schema != 1)
    {
        setError(error, QStringLiteral("unsupported schema version %1").arg(fx->m_schema));
        return QSharedPointer<EFXEffect>();
    }

    // id: used as file name and project reference, so keep it strict
    fx->m_id = root.value("id").toString();
    static const QRegularExpression idRe("^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$");
    if (!idRe.match(fx->m_id).hasMatch())
    {
        setError(error, QStringLiteral("missing or invalid 'id' (letters, digits, '.', '_', '-', max 64 chars)"));
        return QSharedPointer<EFXEffect>();
    }

    fx->m_name = root.value("name").toString(fx->m_id);
    fx->m_version = root.value("version").toInt(1);
    fx->m_description = root.value("description").toString();

    // Parameters
    QStringList vars = builtinVariables();
    QJsonArray params = root.value("params").toArray();
    if (params.size() > MAX_PARAMS)
    {
        setError(error, QStringLiteral("too many parameters (max %1)").arg(MAX_PARAMS));
        return QSharedPointer<EFXEffect>();
    }
    for (const QJsonValue& pv : params)
    {
        QJsonObject po = pv.toObject();
        EFXParam p;
        p.id = po.value("id").toString();
        static const QRegularExpression varRe("^[A-Za-z_][A-Za-z0-9_]*$");
        if (!varRe.match(p.id).hasMatch() || isReservedName(p.id))
        {
            setError(error, QStringLiteral("invalid parameter id '%1'").arg(p.id));
            return QSharedPointer<EFXEffect>();
        }
        if (vars.contains(p.id))
        {
            setError(error, QStringLiteral("duplicate parameter id '%1'").arg(p.id));
            return QSharedPointer<EFXEffect>();
        }
        p.label = po.value("label").toString(p.id);
        p.type = po.value("type").toString("float");
        if (p.type != "float" && p.type != "int" && p.type != "bool")
        {
            setError(error, QStringLiteral("parameter '%1': unknown type '%2'").arg(p.id, p.type));
            return QSharedPointer<EFXEffect>();
        }
        bool isBool = p.type == "bool";
        p.min = po.value("min").toDouble(0.0);
        p.max = po.value("max").toDouble(isBool ? 1.0 : 1.0);
        if (isBool) { p.min = 0; p.max = 1; }
        if (p.max < p.min)
        {
            setError(error, QStringLiteral("parameter '%1': max < min").arg(p.id));
            return QSharedPointer<EFXEffect>();
        }
        p.step = po.value("step").toDouble(p.type == "float" ? 0.01 : 1.0);
        double d = po.value("default").isBool() ? (po.value("default").toBool() ? 1.0 : 0.0)
                                                : po.value("default").toDouble(p.min);
        p.def = sanitizeParam(p, d);
        vars << p.id;
        fx->m_params << p;
    }

    // Path (drives pan and tilt)
    if (root.contains("path"))
    {
        if (!root.value("path").isObject())
        {
            setError(error, QStringLiteral("'path' must be an object"));
            return QSharedPointer<EFXEffect>();
        }
        QString perr;
        if (!fx->m_path.fromJson(root.value("path").toObject(), &perr))
        {
            setError(error, perr);
            return QSharedPointer<EFXEffect>();
        }
    }

    // Channels
    QJsonArray channels = root.value("channels").toArray();
    if (channels.isEmpty() && !fx->hasPath())
    {
        setError(error, QStringLiteral("effect has no 'channels' and no 'path'"));
        return QSharedPointer<EFXEffect>();
    }
    if (channels.size() > MAX_CHANNELS)
    {
        setError(error, QStringLiteral("too many channels (max %1)").arg(MAX_CHANNELS));
        return QSharedPointer<EFXEffect>();
    }

    for (const QJsonValue& cv : channels)
    {
        QJsonObject co = cv.toObject();
        EFXEffectChannel ch;

        QJsonValue tv = co.value("target");
        if (tv.isString())
        {
            ch.target = tv.toString().toLower();
            static const QRegularExpression tRe("^[a-z][a-z0-9_]{0,31}$");
            if (!tRe.match(ch.target).hasMatch())
            {
                setError(error, QStringLiteral("invalid target '%1'").arg(ch.target));
                return QSharedPointer<EFXEffect>();
            }
        }
        else if (tv.isObject() && tv.toObject().contains("channel"))
        {
            int raw = tv.toObject().value("channel").toInt(-1);
            if (raw < 0 || raw > 511)
            {
                setError(error, QStringLiteral("invalid raw channel %1").arg(raw));
                return QSharedPointer<EFXEffect>();
            }
            ch.rawChannel = raw;
        }
        else
        {
            setError(error, QStringLiteral("channel entry without valid 'target'"));
            return QSharedPointer<EFXEffect>();
        }

        const QString name = ch.target.isEmpty() ? QString::number(ch.rawChannel) : ch.target;

        if (co.contains("keys"))
        {
            if (co.contains("expr"))
            {
                setError(error, QStringLiteral("target '%1': use either 'expr' or 'keys', not both").arg(name));
                return QSharedPointer<EFXEffect>();
            }
            QString kerr;
            if (!parseKeys(co.value("keys"), &ch.keys, &kerr))
            {
                setError(error, QStringLiteral("target '%1': %2").arg(name, kerr));
                return QSharedPointer<EFXEffect>();
            }
        }
        else
        {
            QString expr = co.value("expr").toString();
            if (!ch.expr.compile(expr, vars))
            {
                setError(error, QStringLiteral("target '%1': %2").arg(name, ch.expr.error()));
                return QSharedPointer<EFXEffect>();
            }
        }

        fx->m_channels << ch;
    }

    fx->m_panIndex = fx->channelIndex("pan");
    fx->m_tiltIndex = fx->channelIndex("tilt");

    if (fx->hasPath() && (fx->m_panIndex >= 0 || fx->m_tiltIndex >= 0))
    {
        setError(error, QStringLiteral("an effect with a 'path' cannot also define pan/tilt channels"));
        return QSharedPointer<EFXEffect>();
    }

    // Optional reverse mapping of theta
    if (root.contains("reverse"))
    {
        if (!root.value("reverse").isString() || !fx->m_reverse.compile(root.value("reverse").toString(), vars))
        {
            setError(error, QStringLiteral("reverse: %1").arg(fx->m_reverse.error().isEmpty()
                                                              ? QStringLiteral("must be an expression string")
                                                              : fx->m_reverse.error()));
            return QSharedPointer<EFXEffect>();
        }
        fx->m_hasReverse = true;
    }

    return fx;
}

QVector<double> EFXEffect::defaultParams() const
{
    QVector<double> v;
    v.reserve(m_params.size());
    for (const EFXParam& p : m_params)
        v << p.def;
    return v;
}

double EFXEffect::sanitizeParam(const EFXParam& p, double value)
{
    if (!std::isfinite(value))
        value = p.min;
    value = qBound(p.min, value, p.max);
    if (p.type != "float")
        value = std::round(value);
    return value;
}

int EFXEffect::channelIndex(const QString& target) const
{
    for (int i = 0; i < m_channels.size(); i++)
    {
        if (m_channels.at(i).target == target)
            return i;
    }
    return -1;
}

void EFXEffect::fillVars(double *vars, double t, double i, double n, double h,
                         const QVector<double>& params) const
{
    vars[0] = t;
    vars[1] = t * 2.0 * M_PI;
    vars[2] = i;
    vars[3] = n;
    vars[4] = h;
    for (int p = 0; p < m_params.size(); p++)
        vars[BuiltinCount + p] = p < params.size() ? sanitizeParam(m_params.at(p), params.at(p)) : m_params.at(p).def;
}

double EFXEffect::evaluateChannel(int channel, double t, double fixtureIndex, double fixtureCount,
                                  double head, const QVector<double>& params) const
{
    if (channel < 0 || channel >= m_channels.size())
        return 0.0;
    const EFXEffectChannel& ch = m_channels.at(channel);
    if (ch.isKeyframed())
        return evaluateKeys(ch.keys, t);

    QVarLengthArray<double, BuiltinCount + 16> vars(BuiltinCount + m_params.size());
    fillVars(vars.data(), t, fixtureIndex, fixtureCount, head, params);
    return ch.expr.evaluate(vars.constData());
}

double EFXEffect::evaluateKeys(const QVector<EFXKey>& keys, double t)
{
    if (keys.isEmpty() || !std::isfinite(t))
        return 0.0;

    t -= std::floor(t);

    if (t <= keys.first().t)
        return keys.first().v;
    if (t >= keys.last().t)
        return keys.last().v;

    // first key with a greater t; the interval is [idx-1, idx]
    int idx = 1;
    while (keys.at(idx).t <= t)
        idx++;
    const EFXKey& a = keys.at(idx - 1);
    const EFXKey& b = keys.at(idx);

    if (a.ease == EFXKey::Step)
        return a.v;

    double f = (t - a.t) / (b.t - a.t);
    if (a.ease == EFXKey::Smooth)
        f = f * f * (3.0 - 2.0 * f);
    return a.v + (b.v - a.v) * f;
}

double EFXEffect::reverseTheta(double theta, const QVector<double>& params) const
{
    if (!m_hasReverse)
        return 2.0 * M_PI - theta;
    QVarLengthArray<double, BuiltinCount + 16> vars(BuiltinCount + m_params.size());
    fillVars(vars.data(), theta / (2.0 * M_PI), 0, 1, 0, params);
    vars[1] = theta; // keep theta exact, do not round-trip through t
    return m_reverse.evaluate(vars.constData());
}

bool EFXEffect::evaluatePanTilt(double theta, double fixtureIndex, double fixtureCount, double head,
                                const QVector<double>& params, double *pan, double *tilt) const
{
    if (!hasPanTilt())
        return false;

    const double t = theta / (2.0 * M_PI);
    if (hasPath())
    {
        const QPointF p = m_path.evaluate(t);
        *pan = p.x();
        *tilt = p.y();
        return true;
    }

    QVarLengthArray<double, BuiltinCount + 16> vars(BuiltinCount + m_params.size());
    fillVars(vars.data(), t, fixtureIndex, fixtureCount, head, params);
    vars[1] = theta;

    const EFXEffectChannel& pc = m_channels.at(m_panIndex);
    const EFXEffectChannel& tc = m_channels.at(m_tiltIndex);
    *pan = pc.isKeyframed() ? evaluateKeys(pc.keys, t) : pc.expr.evaluate(vars.constData());
    *tilt = tc.isKeyframed() ? evaluateKeys(tc.keys, t) : tc.expr.evaluate(vars.constData());
    return true;
}

QVector<QPointF> EFXEffect::samplePanTilt(const QVector<double>& params, int count) const
{
    QVector<QPointF> out;
    if (!hasPanTilt() || count < 2)
        return out;

    out.reserve(count);
    for (int i = 0; i < count; i++)
    {
        double pan = 0, tilt = 0;
        evaluatePanTilt(2.0 * M_PI * i / count, 0, 1, 0, params, &pan, &tilt);
        out << QPointF(qBound(-1.0, pan, 1.0), qBound(-1.0, tilt, 1.0));
    }
    return out;
}

QByteArray EFXEffect::withPath(const QJsonObject& pathObject, const QString& id, const QString& name,
                               QString *error) const
{
    QJsonObject root = QJsonDocument::fromJson(m_json).object();
    const bool hadPath = root.contains("path");

    // keep everything except pan/tilt, which now come from the path
    QJsonArray channels;
    const QJsonArray old = root.value("channels").toArray();
    for (const QJsonValue& v : old)
    {
        const QString target = v.toObject().value("target").toString().toLower();
        if (target == "pan" || target == "tilt")
            continue;
        channels << v;
    }
    if (channels.isEmpty())
        root.remove("channels");
    else
        root.insert("channels", channels);

    if (!hadPath)
    {
        root.remove("reverse");

        // keep only parameters that the remaining channels still use
        const QByteArray used = QJsonDocument(channels).toJson(QJsonDocument::Compact);
        QJsonArray params;
        const QJsonArray oldParams = root.value("params").toArray();
        for (const QJsonValue& v : oldParams)
        {
            const QString pid = v.toObject().value("id").toString();
            const QRegularExpression re(QStringLiteral("(?<![A-Za-z0-9_])%1(?![A-Za-z0-9_])")
                                        .arg(QRegularExpression::escape(pid)));
            if (re.match(QString::fromUtf8(used)).hasMatch())
                params << v;
        }
        if (params.isEmpty())
            root.remove("params");
        else
            root.insert("params", params);
    }

    root.insert("path", pathObject);
    root.insert("id", id);
    root.insert("name", name);
    root.insert("version", 1);

    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
    QString err;
    if (fromJson(json, &err).isNull())
    {
        setError(error, err);
        return QByteArray();
    }
    return json;
}
