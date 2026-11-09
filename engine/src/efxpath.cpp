/*
  Q Light Controller Plus
  efxpath.cpp

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

#include <QJsonArray>
#include <QJsonValue>
#include <QtMath>
#include <algorithm>
#include <cmath>

#include "efxpath.h"

#define SAMPLES_PER_SEGMENT 64

static bool setError(QString *error, const QString& msg)
{
    if (error)
        *error = msg;
    return false;
}

static bool validCoordinate(double v)
{
    return std::isfinite(v) && std::fabs(v) <= EFXPath::MaxCoordinate;
}

static bool readHandle(const QJsonValue& v, QPointF *out)
{
    QJsonArray a = v.toArray();
    if (a.size() != 2 || !a.at(0).isDouble() || !a.at(1).isDouble())
        return false;
    *out = QPointF(a.at(0).toDouble(), a.at(1).toDouble());
    return validCoordinate(out->x()) && validCoordinate(out->y());
}

EFXPath::EFXPath()
    : m_closed(false)
    , m_bezier(true)
    , m_easing(ConstantSpeed)
    , m_length(0)
{
}

bool EFXPath::fromJson(const QJsonObject& obj, QString *error)
{
    const QString type = obj.value("type").toString("bezier");
    if (type != "bezier" && type != "polyline")
        return setError(error, QStringLiteral("path: unknown type '%1'").arg(type));

    const QString easing = obj.value("easing").toString("linear");
    Easing e;
    if (easing == "linear")
        e = ConstantSpeed;
    else if (easing == "segments")
        e = EqualSegments;
    else
        return setError(error, QStringLiteral("path: unknown easing '%1'").arg(easing));

    const QJsonArray pts = obj.value("points").toArray();
    if (pts.size() < 2)
        return setError(error, QStringLiteral("path: at least 2 points required"));
    if (pts.size() > MaxPoints)
        return setError(error, QStringLiteral("path: too many points (max %1)").arg(MaxPoints));

    QVector<EFXPathPoint> points;
    points.reserve(pts.size());
    for (int i = 0; i < pts.size(); i++)
    {
        const QJsonObject po = pts.at(i).toObject();
        EFXPathPoint p;
        if (!po.value("x").isDouble() || !po.value("y").isDouble())
            return setError(error, QStringLiteral("path point %1: 'x' and 'y' must be numbers").arg(i));
        p.pos = QPointF(po.value("x").toDouble(), po.value("y").toDouble());
        if (!validCoordinate(p.pos.x()) || !validCoordinate(p.pos.y()))
            return setError(error, QStringLiteral("path point %1: coordinate out of range").arg(i));

        p.in = p.out = p.pos;
        if (po.contains("in") && !readHandle(po.value("in"), &p.in))
            return setError(error, QStringLiteral("path point %1: invalid 'in' handle").arg(i));
        if (po.contains("out") && !readHandle(po.value("out"), &p.out))
            return setError(error, QStringLiteral("path point %1: invalid 'out' handle").arg(i));
        points << p;
    }

    return setPoints(points, obj.value("closed").toBool(false), type == "bezier", e, error);
}

static void simplifyRange(const QVector<QPointF>& pts, int first, int last, double tol,
                          QVector<bool>& keep)
{
    if (last <= first + 1)
        return;

    const QPointF a = pts.at(first), b = pts.at(last);
    const double dx = b.x() - a.x(), dy = b.y() - a.y();
    const double len = std::hypot(dx, dy);
    double maxDist = -1;
    int index = -1;
    for (int i = first + 1; i < last; i++)
    {
        const QPointF p = pts.at(i);
        const double d = len > 0.0 ? std::fabs(dy * (p.x() - a.x()) - dx * (p.y() - a.y())) / len
                                   : std::hypot(p.x() - a.x(), p.y() - a.y());
        if (d > maxDist)
        {
            maxDist = d;
            index = i;
        }
    }
    if (maxDist > tol)
    {
        keep[index] = true;
        simplifyRange(pts, first, index, tol, keep);
        simplifyRange(pts, index, last, tol, keep);
    }
}

QVector<EFXPathPoint> EFXPath::fromStroke(const QVector<QPointF>& stroke, double tolerance,
                                          bool closed, double cornerAngle)
{
    QVector<EFXPathPoint> result;

    // drop non finite and consecutive duplicate samples
    QVector<QPointF> pts;
    for (const QPointF& p : stroke)
    {
        if (!std::isfinite(p.x()) || !std::isfinite(p.y()))
            continue;
        if (!pts.isEmpty() && std::hypot(p.x() - pts.last().x(), p.y() - pts.last().y()) < 1e-6)
            continue;
        pts << p;
    }
    if (closed && pts.size() > 2 &&
        std::hypot(pts.first().x() - pts.last().x(), pts.first().y() - pts.last().y()) < 1e-6)
        pts.removeLast();
    if (pts.size() < 2)
        return result;

    // for a closed curve the start point is simplified against itself
    if (closed)
        pts << pts.first();

    QVector<bool> keep(pts.size(), false);
    keep[0] = keep[pts.size() - 1] = true;
    simplifyRange(pts, 0, pts.size() - 1, qMax(tolerance, 1e-6), keep);

    QVector<QPointF> kept;
    for (int i = 0; i < pts.size(); i++)
        if (keep[i])
            kept << pts.at(i);
    if (closed)
        kept.removeLast();
    if (kept.size() > MaxPoints)
        kept.resize(MaxPoints);
    if (kept.size() < 2)
        return result;

    const int n = kept.size();
    const double cornerCos = std::cos(qDegreesToRadians(cornerAngle));
    for (int i = 0; i < n; i++)
    {
        EFXPathPoint pp;
        pp.pos = pp.in = pp.out = kept.at(i);

        const bool hasPrev = closed || i > 0;
        const bool hasNext = closed || i < n - 1;
        const QPointF prev = kept.at((i + n - 1) % n);
        const QPointF next = kept.at((i + 1) % n);
        const QPointF dPrev = pp.pos - prev, dNext = next - pp.pos;
        const double lPrev = std::hypot(dPrev.x(), dPrev.y());
        const double lNext = std::hypot(dNext.x(), dNext.y());

        if (hasPrev && hasNext && lPrev > 0 && lNext > 0)
        {
            const double cosTurn = (dPrev.x() * dNext.x() + dPrev.y() * dNext.y()) / (lPrev * lNext);
            if (cosTurn >= cornerCos) // gentle turn: smooth point
            {
                QPointF dir = next - prev;
                const double l = std::hypot(dir.x(), dir.y());
                if (l > 0)
                {
                    dir /= l;
                    pp.out = pp.pos + dir * (lNext / 3.0);
                    pp.in = pp.pos - dir * (lPrev / 3.0);
                }
            }
        }
        else if (hasNext && lNext > 0 && !hasPrev)
        {
            pp.out = pp.pos + dNext / 3.0;
        }
        else if (hasPrev && lPrev > 0 && !hasNext)
        {
            pp.in = pp.pos - dPrev / 3.0;
        }
        result << pp;
    }

    return result;
}

bool EFXPath::setPoints(const QVector<EFXPathPoint>& points, bool closed, bool bezier,
                        Easing easing, QString *error)
{
    if (points.size() < 2)
        return setError(error, QStringLiteral("path: at least 2 points required"));
    if (points.size() > MaxPoints)
        return setError(error, QStringLiteral("path: too many points (max %1)").arg(MaxPoints));
    for (const EFXPathPoint& p : points)
    {
        if (!validCoordinate(p.pos.x()) || !validCoordinate(p.pos.y()) ||
            !validCoordinate(p.in.x()) || !validCoordinate(p.in.y()) ||
            !validCoordinate(p.out.x()) || !validCoordinate(p.out.y()))
            return setError(error, QStringLiteral("path: coordinate out of range"));
    }

    m_points = points;
    m_closed = closed;
    m_bezier = bezier;
    m_easing = easing;
    rebuild();
    return true;
}

QJsonObject EFXPath::toJson() const
{
    QJsonObject o;
    o.insert("type", m_bezier ? "bezier" : "polyline");
    o.insert("closed", m_closed);
    o.insert("easing", m_easing == ConstantSpeed ? "linear" : "segments");

    QJsonArray pts;
    for (const EFXPathPoint& p : m_points)
    {
        QJsonObject po;
        po.insert("x", p.pos.x());
        po.insert("y", p.pos.y());
        if (m_bezier)
        {
            po.insert("in", QJsonArray() << p.in.x() << p.in.y());
            po.insert("out", QJsonArray() << p.out.x() << p.out.y());
        }
        pts << po;
    }
    o.insert("points", pts);
    return o;
}

QPointF EFXPath::segmentPoint(int segment, double u) const
{
    const EFXPathPoint& a = m_points.at(segment);
    const EFXPathPoint& b = m_points.at((segment + 1) % m_points.size());

    if (!m_bezier)
        return a.pos + (b.pos - a.pos) * u;

    const double v = 1.0 - u;
    const double w0 = v * v * v;
    const double w1 = 3.0 * v * v * u;
    const double w2 = 3.0 * v * u * u;
    const double w3 = u * u * u;
    return a.pos * w0 + a.out * w1 + b.in * w2 + b.pos * w3;
}

void EFXPath::rebuild()
{
    const int segments = segmentCount();
    m_lut.clear();
    m_lut.reserve(segments * SAMPLES_PER_SEGMENT + 1);
    m_lut << 0.0;

    double total = 0.0;
    QPointF prev = segmentPoint(0, 0.0);
    for (int s = 0; s < segments; s++)
    {
        for (int k = 1; k <= SAMPLES_PER_SEGMENT; k++)
        {
            const QPointF cur = segmentPoint(s, double(k) / SAMPLES_PER_SEGMENT);
            total += std::hypot(cur.x() - prev.x(), cur.y() - prev.y());
            m_lut << total;
            prev = cur;
        }
    }
    m_length = total;
}

QPointF EFXPath::evaluate(double t) const
{
    if (!isValid() || !std::isfinite(t))
        return QPointF(0, 0);

    t -= std::floor(t); // wrap into 0..1

    const int segments = segmentCount();
    int segment;
    double u;

    if (m_easing == EqualSegments || m_length <= 0.0)
    {
        const double pos = t * segments;
        segment = qMin(int(pos), segments - 1);
        u = pos - segment;
    }
    else
    {
        const double target = t * m_length;
        // last LUT entry <= target
        int idx = int(std::upper_bound(m_lut.constBegin(), m_lut.constEnd(), target) - m_lut.constBegin()) - 1;
        idx = qBound(0, idx, m_lut.size() - 2);
        const double span = m_lut.at(idx + 1) - m_lut.at(idx);
        const double frac = span > 0.0 ? (target - m_lut.at(idx)) / span : 0.0;
        segment = idx / SAMPLES_PER_SEGMENT;
        u = ((idx % SAMPLES_PER_SEGMENT) + frac) / SAMPLES_PER_SEGMENT;
    }

    const QPointF p = segmentPoint(segment, u);
    return QPointF(qBound(-1.0, p.x(), 1.0), qBound(-1.0, p.y(), 1.0));
}
