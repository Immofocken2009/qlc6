/*
  Q Light Controller Plus
  efxeffect_test.cpp

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

#include <QtTest>
#include <QTemporaryDir>
#include <QtMath>
#include <cmath>

#include "efxeffect_test.h"
#include "efxexpression.h"
#include "efxlibrary.h"
#include "efxeffect.h"

static double ev(const QString& src, const QStringList& vars = QStringList(),
                 const QVector<double>& vals = QVector<double>(), bool *ok = nullptr)
{
    EFXExpression e;
    bool good = e.compile(src, vars);
    if (ok)
        *ok = good;
    if (!good)
        return qQNaN();
    return e.evaluate(vals.constData());
}

static bool near(double a, double b, double eps = 1e-9)
{
    return std::fabs(a - b) < eps;
}

void EFXEffect_Test::exprArithmetic()
{
    QVERIFY(near(ev("1 + 2 * 3"), 7));
    QVERIFY(near(ev("(1 + 2) * 3"), 9));
    QVERIFY(near(ev("10 / 4"), 2.5));
    QVERIFY(near(ev("7 % 4"), 3));
    QVERIFY(near(ev("1.5e2"), 150));
    QVERIFY(near(ev(".5 + .25"), 0.75));
    QVERIFY(near(ev("1 / 0"), 0));      // never inf/nan
    QVERIFY(near(ev("5 % 0"), 0));
}

void EFXEffect_Test::exprPrecedence()
{
    QVERIFY(near(ev("-2 ^ 2"), -4));
    QVERIFY(near(ev("2 ^ 3 ^ 2"), 512));    // right associative
    QVERIFY(near(ev("2 ^ -1"), 0.5));
    QVERIFY(near(ev("- - 3"), 3));
    QVERIFY(near(ev("1 < 2"), 1));
    QVERIFY(near(ev("2 <= 1"), 0));
    QVERIFY(near(ev("3 == 3"), 1));
    QVERIFY(near(ev("3 != 3"), 0));
    QVERIFY(near(ev("1 + 1 > 1"), 1));
}

void EFXEffect_Test::exprVariablesAndConstants()
{
    QStringList vars = QStringList() << "t" << "foo";
    QVector<double> vals = QVector<double>() << 0.25 << 4.0;
    QVERIFY(near(ev("t * foo", vars, vals), 1.0));
    QVERIFY(near(ev("pi", vars, vals), M_PI));
    QVERIFY(near(ev("tau", vars, vals), 2 * M_PI));
    QVERIFY(near(ev("e", vars, vals), M_E));
    // a variable named like a constant wins, it was declared explicitly
    QVERIFY(near(ev("pi", QStringList() << "pi", QVector<double>() << 3.0), 3.0));
}

void EFXEffect_Test::exprFunctions()
{
    QVERIFY(near(ev("sin(0)"), 0));
    QVERIFY(near(ev("cos(0)"), 1));
    QVERIFY(near(ev("atan2(1, 1)"), M_PI / 4));
    QVERIFY(near(ev("abs(-3)"), 3));
    QVERIFY(near(ev("sqrt(16)"), 4));
    QVERIFY(near(ev("pow(2, 10)"), 1024));
    QVERIFY(near(ev("pow(-2, 3)"), -8));
    QVERIFY(near(ev("floor(-1.5)"), -2));
    QVERIFY(near(ev("ceil(1.2)"), 2));
    QVERIFY(near(ev("round(2.5)"), 3));
    QVERIFY(near(ev("round(-2.5)"), -3));
    QVERIFY(near(ev("sign(-9)"), -1));
    QVERIFY(near(ev("min(1, 2)"), 1));
    QVERIFY(near(ev("max(1, 2)"), 2));
    QVERIFY(near(ev("clamp(5, 0, 1)"), 1));
    QVERIFY(near(ev("mix(10, 20, 0.5)"), 15));
    QVERIFY(near(ev("mod(-1, 3)"), 2));     // floored modulo, always positive
    QVERIFY(near(ev("smoothstep(0, 1, 0.5)"), 0.5));
    QVERIFY(near(ev("smoothstep(0, 1, 2)"), 1));
    QVERIFY(near(ev("if(1, 7, 8)"), 7));
    QVERIFY(near(ev("if(0, 7, 8)"), 8));
    QVERIFY(near(ev("sin(cos(0) * pi / 2)"), 1));
}

void EFXEffect_Test::exprWaveforms()
{
    QVERIFY(near(ev("tri(0)"), -1));
    QVERIFY(near(ev("tri(0.25)"), 0));
    QVERIFY(near(ev("tri(0.5)"), 1));
    QVERIFY(near(ev("tri(1.75)"), 0));
    QVERIFY(near(ev("saw(0)"), -1));
    QVERIFY(near(ev("saw(0.5)"), 0));
    QVERIFY(near(ev("square(0.1)"), 1));
    QVERIFY(near(ev("square(0.6)"), -1));
    QVERIFY(near(ev("pulse(0.1, 0.5)"), 1));
    QVERIFY(near(ev("pulse(0.6, 0.5)"), 0));
    QVERIFY(near(ev("pulse(-0.1, 0.5)"), 0));   // frac(-0.1) = 0.9

    // noise: deterministic, bounded and continuous
    double prev = ev("noise(0)");
    for (int i = 1; i <= 2000; i++)
    {
        QString s = QString("noise(%1)").arg(i * 0.01, 0, 'f', 4);
        double v = ev(s);
        QVERIFY(v >= -1.0 && v <= 1.0);
        QVERIFY(std::fabs(v - prev) < 0.2);
        prev = v;
    }
    QCOMPARE(ev("noise(3.7)"), ev("noise(3.7)"));
}

void EFXEffect_Test::exprSafety()
{
    // deep nesting must be rejected at compile time, not overflow the eval stack
    QString deep;
    for (int i = 0; i < 200; i++)
        deep += "1+(";
    deep += "1";
    for (int i = 0; i < 200; i++)
        deep += ")";
    bool ok = true;
    ev(deep, QStringList(), QVector<double>(), &ok);
    QVERIFY(!ok);

    // right-leaning chains need a stack proportional to their depth
    QString chain = "1";
    for (int i = 0; i < 100; i++)
        chain += "^1";
    ok = false;
    ev(chain, QStringList(), QVector<double>(), &ok);
    QVERIFY(!ok);

    // invalid expressions evaluate to 0
    EFXExpression bad;
    QVERIFY(!bad.compile("1 +", QStringList()));
    QVERIFY(!bad.isValid());
    QCOMPARE(bad.evaluate(nullptr), 0.0);

    // results are always finite
    QVERIFY(std::isfinite(ev("exp(1000)")));
    QVERIFY(std::isfinite(ev("log(-1)")));
    QVERIFY(std::isfinite(ev("sqrt(-1)")));
}

void EFXEffect_Test::exprErrors()
{
    bool ok;
    ev("", QStringList(), QVector<double>(), &ok);          QVERIFY(!ok);
    ev("   ", QStringList(), QVector<double>(), &ok);       QVERIFY(!ok);
    ev("1 +", QStringList(), QVector<double>(), &ok);       QVERIFY(!ok);
    ev("(1 + 2", QStringList(), QVector<double>(), &ok);    QVERIFY(!ok);
    ev("1 + 2)", QStringList(), QVector<double>(), &ok);    QVERIFY(!ok);
    ev("foo", QStringList(), QVector<double>(), &ok);       QVERIFY(!ok);
    ev("foo(1)", QStringList(), QVector<double>(), &ok);    QVERIFY(!ok);
    ev("sin()", QStringList(), QVector<double>(), &ok);     QVERIFY(!ok);
    ev("sin(1, 2)", QStringList(), QVector<double>(), &ok); QVERIFY(!ok);
    ev("1 $ 2", QStringList(), QVector<double>(), &ok);     QVERIFY(!ok);
    ev("1..2", QStringList(), QVector<double>(), &ok);      QVERIFY(!ok);

    EFXExpression e;
    QVERIFY(!e.compile("1 + foo", QStringList()));
    QVERIFY(e.error().contains("foo"));
    QVERIFY(e.error().contains("position"));
}

static const char *minimalJson =
    "{ \"schema\": 1, \"id\": \"mini\", \"name\": \"Mini\","
    "  \"params\": [ { \"id\": \"amp\", \"type\": \"float\", \"min\": 0, \"max\": 2, \"default\": 1 } ],"
    "  \"channels\": [ { \"target\": \"pan\", \"expr\": \"amp * sin(theta)\" },"
    "                  { \"target\": \"tilt\", \"expr\": \"cos(theta)\" },"
    "                  { \"target\": \"Dimmer\", \"expr\": \"i / max(n - 1, 1)\" },"
    "                  { \"target\": { \"channel\": 7 }, \"expr\": \"t\" } ] }";

void EFXEffect_Test::effectMinimal()
{
    QString err;
    EFXEffectPtr fx = EFXEffect::fromJson(minimalJson, &err);
    QVERIFY2(!fx.isNull(), qPrintable(err));
    QCOMPARE(fx->id(), QString("mini"));
    QCOMPARE(fx->name(), QString("Mini"));
    QCOMPARE(fx->channels().size(), 4);
    QCOMPARE(fx->channelIndex("dimmer"), 2);    // targets are case-insensitive
    QCOMPARE(fx->channels().at(3).rawChannel, 7);
    QCOMPARE(fx->json(), QByteArray(minimalJson));

    QVector<double> p = fx->defaultParams();
    QCOMPARE(p.size(), 1);

    double pan = 0, tilt = 0;
    QVERIFY(fx->evaluatePanTilt(M_PI / 2, 0, 1, 0, p, &pan, &tilt));
    QVERIFY(near(pan, 1.0));
    QVERIFY(near(tilt, 0.0, 1e-9));

    // per-fixture variables
    QVERIFY(near(fx->evaluateChannel(2, 0, 3, 4, 0, p), 1.0));
    QVERIFY(near(fx->evaluateChannel(2, 0, 0, 4, 0, p), 0.0));
    QVERIFY(near(fx->evaluateChannel(3, 0.5, 0, 1, 0, p), 0.5));

    // parameter values are honoured and sanitised
    QVERIFY(fx->evaluatePanTilt(M_PI / 2, 0, 1, 0, QVector<double>() << 0.5, &pan, &tilt));
    QVERIFY(near(pan, 0.5));
    QVERIFY(fx->evaluatePanTilt(M_PI / 2, 0, 1, 0, QVector<double>() << 99.0, &pan, &tilt));
    QVERIFY(near(pan, 2.0));    // clamped to max
    QVERIFY(fx->evaluatePanTilt(M_PI / 2, 0, 1, 0, QVector<double>(), &pan, &tilt));
    QVERIFY(near(pan, 1.0));    // missing -> default
}

void EFXEffect_Test::effectInvalid()
{
    QString err;
    QVERIFY(EFXEffect::fromJson("not json", &err).isNull());
    QVERIFY(!err.isEmpty());
    QVERIFY(EFXEffect::fromJson("[1,2]", &err).isNull());
    QVERIFY(EFXEffect::fromJson("{}", &err).isNull());                         // no id
    QVERIFY(EFXEffect::fromJson("{\"id\":\"a\"}", &err).isNull());             // no channels
    QVERIFY(EFXEffect::fromJson("{\"id\":\"../evil\",\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}]}", &err).isNull());
    QVERIFY(EFXEffect::fromJson("{\"id\":\"a/b\",\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}]}", &err).isNull());
    QVERIFY(EFXEffect::fromJson("{\"schema\":2,\"id\":\"a\",\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}]}", &err).isNull());
    QVERIFY(err.contains("schema"));
    QVERIFY(EFXEffect::fromJson("{\"id\":\"a\",\"channels\":[{\"target\":\"pan\",\"expr\":\"foo\"}]}", &err).isNull());
    QVERIFY(err.contains("foo"));
    QVERIFY(EFXEffect::fromJson("{\"id\":\"a\",\"channels\":[{\"expr\":\"0\"}]}", &err).isNull());
    QVERIFY(EFXEffect::fromJson("{\"id\":\"a\",\"channels\":[{\"target\":{\"channel\":9999},\"expr\":\"0\"}]}", &err).isNull());
    // an empty path is rejected with a clear message
    QVERIFY(EFXEffect::fromJson("{\"id\":\"a\",\"path\":{},\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}]}", &err).isNull());
    QVERIFY(err.contains("path"));
}

void EFXEffect_Test::effectParams()
{
    QString err;
    const QByteArray head = "{\"id\":\"a\",\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}],\"params\":";
    QVERIFY(EFXEffect::fromJson(head + "[{\"id\":\"sin\"}]}", &err).isNull());          // function name
    QVERIFY(EFXEffect::fromJson(head + "[{\"id\":\"theta\"}]}", &err).isNull());        // builtin
    QVERIFY(EFXEffect::fromJson(head + "[{\"id\":\"1x\"}]}", &err).isNull());           // not an identifier
    QVERIFY(EFXEffect::fromJson(head + "[{\"id\":\"x\"},{\"id\":\"x\"}]}", &err).isNull());
    QVERIFY(EFXEffect::fromJson(head + "[{\"id\":\"x\",\"min\":5,\"max\":1}]}", &err).isNull());
    QVERIFY(EFXEffect::fromJson(head + "[{\"id\":\"x\",\"type\":\"string\"}]}", &err).isNull());

    EFXEffectPtr fx = EFXEffect::fromJson(head + "[{\"id\":\"n1\",\"type\":\"int\",\"min\":0,\"max\":10,\"default\":99},"
                                                 "{\"id\":\"b\",\"type\":\"bool\",\"default\":true}]}", &err);
    QVERIFY2(!fx.isNull(), qPrintable(err));
    QCOMPARE(fx->defaultParams().at(0), 10.0);          // default clamped
    QCOMPARE(fx->defaultParams().at(1), 1.0);
    QCOMPARE(EFXEffect::sanitizeParam(fx->params().at(0), 3.6), 4.0);   // int rounds
    QCOMPARE(EFXEffect::sanitizeParam(fx->params().at(0), qQNaN()), 0.0);
}

void EFXEffect_Test::effectReverse()
{
    QString err;
    EFXEffectPtr plain = EFXEffect::fromJson("{\"id\":\"a\",\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}]}", &err);
    QVERIFY(!plain.isNull());
    QVERIFY(near(plain->reverseTheta(1.0, QVector<double>()), 2 * M_PI - 1.0));

    EFXEffectPtr custom = EFXEffect::fromJson("{\"id\":\"a\",\"reverse\":\"theta + 1\","
                                              "\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}]}", &err);
    QVERIFY2(!custom.isNull(), qPrintable(err));
    QVERIFY(near(custom->reverseTheta(1.0, QVector<double>()), 2.0));

    QVERIFY(EFXEffect::fromJson("{\"id\":\"a\",\"reverse\":\"nope\","
                                "\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}]}", &err).isNull());
}

void EFXEffect_Test::libraryUserEffects()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    EFXLibrary lib;
    lib.setUserDirectory(QDir(tmp.path()));

    QSignalSpy spy(&lib, SIGNAL(effectsChanged()));

    QString err, id;
    QVERIFY2(lib.addUserEffect(minimalJson, &err, &id), qPrintable(err));
    QCOMPARE(id, QString("mini"));
    QVERIFY(QFile::exists(tmp.path() + "/mini.json"));
    QVERIFY(!lib.effect("mini").isNull());
    QVERIFY(lib.isUserEffect("mini"));
    QVERIFY(spy.count() >= 1);

    // replacing keeps one effect
    QVERIFY(lib.addUserEffect(minimalJson, &err));
    QCOMPARE(lib.effects().size(), 1);

    // survives a reload from disk
    EFXLibrary lib2;
    QCOMPARE(lib2.load(QDir(tmp.path()), true), 1);
    QVERIFY(!lib2.effect("mini").isNull());

    // invalid input is rejected and leaves no file behind
    QVERIFY(!lib.addUserEffect("{\"id\":\"broken\"}", &err));
    QVERIFY(!QFile::exists(tmp.path() + "/broken.json"));

    QVERIFY(lib.removeUserEffect("mini", &err));
    QVERIFY(!QFile::exists(tmp.path() + "/mini.json"));
    QVERIFY(!lib.removeUserEffect("mini", &err));
}

void EFXEffect_Test::libraryRejectsBadIds()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    EFXLibrary lib;
    lib.setUserDirectory(QDir(tmp.path()));

    QString err;
    QVERIFY(!lib.addUserEffect("{\"id\":\"../../escape\",\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}]}", &err));
    QVERIFY(!lib.addUserEffect("{\"id\":\"/abs\",\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}]}", &err));
    QVERIFY(!lib.addUserEffect("{\"id\":\"a\\\\b\",\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}]}", &err));
    QCOMPARE(QDir(tmp.path()).entryList(QDir::Files).size(), 0);
    QVERIFY(!QFile::exists(tmp.path() + "/../escape.json"));
}

void EFXEffect_Test::libraryLoadsPresets()
{
    EFXLibrary lib;
    QCOMPARE(lib.load(QDir(EFX_PRESET_DIR), false), 13);
    QVERIFY2(lib.loadErrors().isEmpty(), qPrintable(lib.loadErrors().join("\n")));

    // a system effect cannot be removed
    QString err;
    QVERIFY(!lib.removeUserEffect("circle", &err));
    QVERIFY(!lib.isUserEffect("circle"));

    // a user effect with the same id overrides it
    QTemporaryDir tmp;
    lib.setUserDirectory(QDir(tmp.path()));
    QVERIFY(lib.addUserEffect("{\"id\":\"circle\",\"name\":\"Mine\",\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}]}", &err));
    QCOMPARE(lib.effect("circle")->name(), QString("Mine"));
    QVERIFY(lib.isUserEffect("circle"));
}

/* The old EFX algorithms were captured to a CSV before the enum was removed.
 * This reproduces the old pipeline (direction, start offset, rotate and scale)
 * around the JSON presets and compares every sample. */
static QByteArray keyEffect(const QString& channels, const QString& extra = QString())
{
    return QString("{\"id\":\"k\",%1\"channels\":[%2]}").arg(extra, channels).toUtf8();
}

void EFXEffect_Test::keyframes()
{
    QString err;
    EFXEffectPtr fx = EFXEffect::fromJson(keyEffect(
        "{\"target\":\"dimmer\",\"keys\":[{\"t\":0.25,\"v\":0},{\"t\":0.5,\"v\":1,\"ease\":\"smooth\"},"
        "{\"t\":0.75,\"v\":0.5,\"ease\":\"step\"},{\"t\":1,\"v\":0}]}"), &err);
    QVERIFY2(!fx.isNull(), qPrintable(err));
    QVERIFY(fx->channels().at(0).isKeyframed());

    const QVector<double> p;
    auto v = [&](double t) { return fx->evaluateChannel(0, t, 0, 1, 0, p); };
    QVERIFY(near(v(0.0), 0.0));      // held before the first key
    QVERIFY(near(v(0.25), 0.0));
    QVERIFY(near(v(0.375), 0.5));    // linear midpoint
    QVERIFY(near(v(0.5), 1.0));
    QVERIFY(near(v(0.625), 0.75));   // smoothstep midpoint = linear midpoint
    QVERIFY(v(0.5625) > 0.85 && v(0.5625) < 0.95); // smoothstep(0.25) = 0.156 -> eases away from the key
    QVERIFY(near(v(0.75), 0.5));
    QVERIFY(near(v(0.9), 0.5));      // step holds the key value for the whole interval
    QVERIFY(near(v(0.9999), 0.5));
    QVERIFY(near(v(1.0), v(0.0)));   // t wraps
    QVERIFY(near(v(1.375), v(0.375)));
    QVERIFY(near(v(-0.625), v(0.375)));

    // a single key is a constant
    EFXEffectPtr one = EFXEffect::fromJson(keyEffect("{\"target\":\"dimmer\",\"keys\":[{\"t\":0.3,\"v\":0.7}]}"), &err);
    QVERIFY2(!one.isNull(), qPrintable(err));
    QVERIFY(near(one->evaluateChannel(0, 0.9, 0, 1, 0, p), 0.7));
}

void EFXEffect_Test::keyframesRejectBad()
{
    QString err;
    const QStringList bad = QStringList()
        << "{\"target\":\"dimmer\",\"keys\":[]}"
        << "{\"target\":\"dimmer\",\"keys\":{}}"
        << "{\"target\":\"dimmer\",\"keys\":[{\"t\":0.5,\"v\":0},{\"t\":0.5,\"v\":1}]}"      // not increasing
        << "{\"target\":\"dimmer\",\"keys\":[{\"t\":0.6,\"v\":0},{\"t\":0.5,\"v\":1}]}"      // decreasing
        << "{\"target\":\"dimmer\",\"keys\":[{\"t\":-0.1,\"v\":0}]}"
        << "{\"target\":\"dimmer\",\"keys\":[{\"t\":1.1,\"v\":0}]}"
        << "{\"target\":\"dimmer\",\"keys\":[{\"t\":\"x\",\"v\":0}]}"
        << "{\"target\":\"dimmer\",\"keys\":[{\"t\":0.1}]}"
        << "{\"target\":\"dimmer\",\"keys\":[{\"t\":0.1,\"v\":0,\"ease\":\"bounce\"}]}"
        << "{\"target\":\"dimmer\",\"expr\":\"1\",\"keys\":[{\"t\":0.1,\"v\":0}]}";          // both
    for (const QString& ch : bad)
        QVERIFY2(EFXEffect::fromJson(keyEffect(ch), &err).isNull(), qPrintable(ch));

    // too many keys
    QStringList keys;
    for (int i = 0; i < 1025; i++)
        keys << QString("{\"t\":%1,\"v\":0}").arg(double(i) / 2000.0);
    QVERIFY(EFXEffect::fromJson(keyEffect("{\"target\":\"dimmer\",\"keys\":[" + keys.join(",") + "]}"), &err).isNull());
}

static EFXPath circlePath(EFXPath::Easing easing)
{
    const double k = 0.5522847498;
    QVector<EFXPathPoint> pts;
    auto add = [&](double x, double y, double inx, double iny, double outx, double outy)
    {
        EFXPathPoint p;
        p.pos = QPointF(x, y);
        p.in = QPointF(inx, iny);
        p.out = QPointF(outx, outy);
        pts << p;
    };
    add(0, -1, -k, -1, k, -1);
    add(1, 0, 1, -k, 1, k);
    add(0, 1, k, 1, -k, 1);
    add(-1, 0, -1, k, -1, -k);
    EFXPath path;
    QString err;
    path.setPoints(pts, true, true, easing, &err);
    return path;
}

void EFXEffect_Test::pathCircle()
{
    const EFXPath path = circlePath(EFXPath::ConstantSpeed);
    QVERIFY(path.isValid());
    QVERIFY(std::fabs(path.length() - 2 * M_PI) < 0.01);

    // starts at the first anchor, quarter turns land on the anchors, and every sample is on the unit circle
    QVERIFY(std::fabs(path.evaluate(0).x()) < 1e-9 && std::fabs(path.evaluate(0).y() + 1) < 1e-9);
    QVERIFY(std::fabs(path.evaluate(0.25).x() - 1) < 0.01 && std::fabs(path.evaluate(0.25).y()) < 0.01);
    QVERIFY(std::fabs(path.evaluate(0.5).x()) < 0.01 && std::fabs(path.evaluate(0.5).y() - 1) < 0.01);
    for (int i = 0; i < 400; i++)
    {
        const QPointF p = path.evaluate(i / 400.0);
        QVERIFY(std::fabs(std::hypot(p.x(), p.y()) - 1) < 0.002);
    }

    // closed and wrapping
    QVERIFY(std::fabs(path.evaluate(1.0).y() - path.evaluate(0.0).y()) < 1e-9);
    QVERIFY(std::fabs(path.evaluate(1.25).x() - path.evaluate(0.25).x()) < 1e-9);
    QVERIFY(std::fabs(path.evaluate(-0.75).x() - path.evaluate(0.25).x()) < 1e-9);
}

void EFXEffect_Test::pathEasing()
{
    // two segments of very different length
    QVector<EFXPathPoint> pts;
    for (double x : { -1.0, -0.8, 1.0 })
    {
        EFXPathPoint p;
        p.pos = p.in = p.out = QPointF(x, 0);
        pts << p;
    }
    EFXPath constant, segments;
    QVERIFY(constant.setPoints(pts, false, false, EFXPath::ConstantSpeed));
    QVERIFY(segments.setPoints(pts, false, false, EFXPath::EqualSegments));

    // constant speed: x grows linearly with t; equal segments: half the time is spent on the short one
    QVERIFY(std::fabs(constant.evaluate(0.5).x() - 0.0) < 1e-3);
    QVERIFY(std::fabs(segments.evaluate(0.5).x() - (-0.8)) < 1e-3);
    QVERIFY(std::fabs(segments.evaluate(0.25).x() - (-0.9)) < 1e-3);
}

void EFXEffect_Test::pathOpenAndPolyline()
{
    QString err;
    EFXEffectPtr fx = EFXEffect::fromJson(
        "{\"id\":\"p\",\"path\":{\"type\":\"polyline\",\"points\":[{\"x\":-1,\"y\":-1},{\"x\":1,\"y\":1}]}}", &err);
    QVERIFY2(!fx.isNull(), qPrintable(err));
    QVERIFY(fx->hasPath());
    QVERIFY(fx->hasPanTilt());
    QVERIFY(!fx->path().isClosed());

    // open path: t 0 -> first point, t -> 1 approaches the last point
    double pan, tilt;
    const QVector<double> p;
    QVERIFY(fx->evaluatePanTilt(0, 0, 1, 0, p, &pan, &tilt));
    QVERIFY(near(pan, -1) && near(tilt, -1));
    QVERIFY(fx->evaluatePanTilt(M_PI, 0, 1, 0, p, &pan, &tilt));
    QVERIFY(near(pan, 0) && near(tilt, 0));
    QVERIFY(fx->evaluatePanTilt(2 * M_PI * 0.999, 0, 1, 0, p, &pan, &tilt));
    QVERIFY(pan > 0.99 && tilt > 0.99);

    // JSON round trip keeps the geometry
    EFXPath copy;
    QVERIFY(copy.fromJson(fx->path().toJson(), &err));
    QCOMPARE(copy.points().size(), 2);
    QCOMPARE(copy.isBezier(), false);
    QVERIFY(near(copy.evaluate(0.5).x(), 0));

    // values outside -1..1 are clamped
    EFXPath wide;
    QVector<EFXPathPoint> pts;
    for (double x : { -3.0, 3.0 })
    {
        EFXPathPoint pt;
        pt.pos = pt.in = pt.out = QPointF(x, 0);
        pts << pt;
    }
    QVERIFY(wide.setPoints(pts, false, false, EFXPath::ConstantSpeed));
    QVERIFY(near(wide.evaluate(0.01).x(), -1) && near(wide.evaluate(0.99).x(), 1));
}

void EFXEffect_Test::pathRejectBad()
{
    QString err;
    const QStringList bad = QStringList()
        << "{\"id\":\"p\",\"path\":{\"points\":[]}}"
        << "{\"id\":\"p\",\"path\":{\"points\":[{\"x\":0,\"y\":0}]}}"                       // one point
        << "{\"id\":\"p\",\"path\":{\"type\":\"spline\",\"points\":[{\"x\":0,\"y\":0},{\"x\":1,\"y\":1}]}}"
        << "{\"id\":\"p\",\"path\":{\"easing\":\"wobble\",\"points\":[{\"x\":0,\"y\":0},{\"x\":1,\"y\":1}]}}"
        << "{\"id\":\"p\",\"path\":{\"points\":[{\"x\":0},{\"x\":1,\"y\":1}]}}"
        << "{\"id\":\"p\",\"path\":{\"points\":[{\"x\":\"a\",\"y\":0},{\"x\":1,\"y\":1}]}}"
        << "{\"id\":\"p\",\"path\":{\"points\":[{\"x\":99,\"y\":0},{\"x\":1,\"y\":1}]}}"      // out of range
        << "{\"id\":\"p\",\"path\":{\"points\":[{\"x\":0,\"y\":0,\"out\":[1]},{\"x\":1,\"y\":1}]}}"
        << "{\"id\":\"p\",\"path\":{\"points\":[{\"x\":0,\"y\":0,\"in\":[0,99]},{\"x\":1,\"y\":1}]}}"
        << "{\"id\":\"p\",\"path\":[]}"
        << "{\"id\":\"p\"}"                                                                    // neither channels nor path
        << "{\"id\":\"p\",\"path\":{\"points\":[{\"x\":0,\"y\":0},{\"x\":1,\"y\":1}]},"
           "\"channels\":[{\"target\":\"pan\",\"expr\":\"0\"}]}";                                // path + pan
    for (const QString& json : bad)
        QVERIFY2(EFXEffect::fromJson(json.toUtf8(), &err).isNull(), qPrintable(json));

    // too many points
    QStringList pts;
    for (int i = 0; i < EFXPath::MaxPoints + 1; i++)
        pts << "{\"x\":0,\"y\":0}";
    QVERIFY(EFXEffect::fromJson(("{\"id\":\"p\",\"path\":{\"points\":[" + pts.join(",") + "]}}").toUtf8(), &err).isNull());

    // a path may be combined with non pan/tilt channels
    QVERIFY(!EFXEffect::fromJson(
        "{\"id\":\"p\",\"path\":{\"points\":[{\"x\":0,\"y\":0},{\"x\":1,\"y\":1}]},"
        "\"channels\":[{\"target\":\"dimmer\",\"expr\":\"1\"}]}", &err).isNull());
}

void EFXEffect_Test::pathEffect()
{
    EFXLibrary lib;
    QVERIFY(lib.load(QDir(EFX_PRESET_DIR), false) > 0);

    EFXEffectPtr heart = lib.effect("heart");
    QVERIFY(!heart.isNull());
    QVERIFY(heart->hasPath());
    QVERIFY(heart->path().isClosed());
    QVERIFY(heart->channels().isEmpty());

    // the heart's first anchor is the bottom tip
    double pan, tilt;
    QVERIFY(heart->evaluatePanTilt(0, 0, 1, 0, QVector<double>(), &pan, &tilt));
    QVERIFY(near(pan, 0) && near(tilt, -1));

    // reverse playback maps to the mirrored position of the same closed path
    const double rev = heart->reverseTheta(M_PI / 2, QVector<double>());
    QVERIFY(near(rev, 3 * M_PI / 2));

    EFXEffectPtr breathing = lib.effect("breathing-spot");
    QVERIFY(!breathing.isNull());
    QVERIFY(breathing->hasPanTilt());
    QCOMPARE(breathing->channels().size(), 1);
    QVERIFY(near(breathing->evaluateChannel(0, 0.0, 0, 1, 0, QVector<double>()), 1.0));
    QVERIFY(near(breathing->evaluateChannel(0, 0.5, 0, 1, 0, QVector<double>()), 0.1));
}

void EFXEffect_Test::legacyParity()
{
    EFXLibrary lib;
    QCOMPARE(lib.load(QDir(EFX_PRESET_DIR), false), 13);

    QMap<QString, QString> ids;
    ids["Circle"] = "circle";
    ids["Eight"] = "eight";
    ids["Line"] = "line";
    ids["Line2"] = "line2";
    ids["Diamond"] = "diamond";
    ids["Square"] = "square";
    ids["SquareChoppy"] = "square-choppy";
    ids["SquareTrue"] = "square-true";
    ids["Leaf"] = "leaf";
    ids["Lissajous"] = "lissajous";

    QFile csv(EFX_GOLDEN_CSV);
    QVERIFY2(csv.open(QIODevice::ReadOnly | QIODevice::Text), EFX_GOLDEN_CSV);
    csv.readLine(); // header

    int rows = 0, failures = 0;
    QMap<QString, int> perAlgo;
    while (!csv.atEnd())
    {
        const QList<QByteArray> c = csv.readLine().trimmed().split(',');
        QCOMPARE(c.size(), 15);
        const QString algo = c.at(0);
        EFXEffectPtr fx = lib.effect(ids.value(algo));
        QVERIFY2(!fx.isNull(), qPrintable(algo));

        QVector<double> params;
        if (algo == "Lissajous")
            params << c.at(1).toDouble() << c.at(2).toDouble() << c.at(3).toDouble() << c.at(4).toDouble();

        const double w = c.at(5).toDouble(), h = c.at(6).toDouble();
        const double rot = c.at(7).toDouble(), xo = c.at(8).toDouble(), yo = c.at(9).toDouble();
        const bool backward = c.at(10).toInt() == 1;
        const int startOffset = c.at(11).toInt();
        const float iter = c.at(12).toFloat();
        const float ex = c.at(13).toFloat(), ey = c.at(14).toFloat();

        // legacy: calculateDirection(), then += convertOffset(), wrap at 2*pi (all float)
        float theta = iter;
        if (backward)
            theta = float(fx->reverseTheta(theta, params));
        theta += float(M_PI / 180 * (startOffset % 360));
        if (theta >= M_PI * 2.0)
            theta -= M_PI * 2.0;

        double pan = 0, tilt = 0;
        QVERIFY(fx->evaluatePanTilt(theta, 0, 1, 0, params, &pan, &tilt));

        // legacy: rotateAndScale()
        const double r = M_PI / 180 * rot;
        const float xx = float(pan), yy = float(tilt);
        const float x = float(xo + xx * cos(r) * w + yy * sin(r) * h);
        const float y = float(yo + -xx * sin(r) * w + yy * cos(r) * h);

        rows++;
        if (std::fabs(x - ex) > 1e-3 || std::fabs(y - ey) > 1e-3)
        {
            perAlgo[algo]++;
            if (failures++ < 10)
                qWarning("%s dir=%d off=%d it=%.6f: expected (%.4f, %.4f) got (%.4f, %.4f)",
                         qPrintable(algo), backward, startOffset, iter, ex, ey, x, y);
        }
    }

    QVERIFY(rows > 5000);
    QVERIFY2(failures == 0, qPrintable(QString("%1 of %2 samples differ from the legacy algorithms").arg(failures).arg(rows)));
}

QTEST_GUILESS_MAIN(EFXEffect_Test)
