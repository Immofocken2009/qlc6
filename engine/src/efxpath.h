/*
  Q Light Controller Plus
  efxpath.h

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

#ifndef EFXPATH_H
#define EFXPATH_H

#include <QPointF>
#include <QVector>
#include <QString>
#include <QJsonObject>

/** @addtogroup engine Engine
 * @{
 */

/** One anchor of a path. @a in and @a out are absolute control points. */
struct EFXPathPoint
{
    QPointF pos;
    QPointF in;     // control point of the segment that ends at this point
    QPointF out;    // control point of the segment that starts at this point
};

/**
 * A pan/tilt path made of cubic Bezier (or straight) segments, in the
 * normalised -1..1 pan/tilt space of an effect.
 *
 * The path is built once by fromJson() and never modified, so evaluate() can
 * be called from the MasterTimer thread and the UI thread at the same time.
 */
class EFXPath
{
public:
    enum Easing
    {
        ConstantSpeed,  // "linear": the point moves with constant speed along the path
        EqualSegments   // "segments": every segment takes the same time
    };

    static const int MaxPoints = 256;
    static const int MaxCoordinate = 10;    // anchors and handles must stay within +/-10

    EFXPath();

    /** Parse the "path" object of an effect. @return false and sets @a error if invalid. */
    bool fromJson(const QJsonObject& obj, QString *error);

    /** Serialise to the "path" object (the inverse of fromJson()). */
    QJsonObject toJson() const;

    /** Build a path directly (used by the visual editor). */
    bool setPoints(const QVector<EFXPathPoint>& points, bool closed, bool bezier,
                   Easing easing, QString *error = nullptr);

    bool isValid() const { return m_points.size() >= 2; }
    bool isClosed() const { return m_closed; }
    bool isBezier() const { return m_bezier; }
    Easing easing() const { return m_easing; }
    const QVector<EFXPathPoint>& points() const { return m_points; }

    /**
     * Position at cycle position @a t. Values outside 0..1 wrap around. The
     * result is clamped to -1..1 on both axes.
     */
    QPointF evaluate(double t) const;

    /** Total length of the path in pan/tilt units. */
    double length() const { return m_length; }

private:
    int segmentCount() const { return m_closed ? m_points.size() : m_points.size() - 1; }
    QPointF segmentPoint(int segment, double u) const;
    void rebuild();

private:
    QVector<EFXPathPoint> m_points;
    bool m_closed;
    bool m_bezier;
    Easing m_easing;

    // cumulative arc length at every LUT sample, SamplesPerSegment per segment + 1
    QVector<double> m_lut;
    double m_length;
};

/** @} */

#endif
