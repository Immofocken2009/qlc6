/*
  Q Light Controller Plus
  efxeffect.h

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

#ifndef EFXEFFECT_H
#define EFXEFFECT_H

#include <QSharedPointer>
#include <QByteArray>
#include <QString>
#include <QVector>
#include <QList>

#include "efxexpression.h"
#include "efxpath.h"

/** @addtogroup engine Engine
 * @{
 */

/** A user adjustable parameter of an effect (exposed to expressions by id). */
struct EFXParam
{
    QString id;
    QString label;
    QString type;       // "float", "int" or "bool"
    double min;
    double max;
    double def;
    double step;
};

/** A keyframe of a channel track. @a ease shapes the interval that starts at this key. */
struct EFXKey
{
    enum Ease { Linear, Smooth, Step };

    double t;       // cycle position 0..1
    double v;       // value
    Ease ease;

    EFXKey() : t(0), v(0), ease(Linear) { }
};

/**
 * One output of an effect: a semantic target (pan, tilt, dimmer, red...) or a
 * raw channel, driven either by an expression or by a keyframe track.
 */
struct EFXEffectChannel
{
    QString target;     // semantic name, lower case; empty when rawChannel is used
    int rawChannel;     // fixture channel offset, -1 when target is used
    EFXExpression expr; // used when keys is empty
    QVector<EFXKey> keys;

    EFXEffectChannel() : rawChannel(-1) { }
    bool isKeyframed() const { return !keys.isEmpty(); }
};

/**
 * An effect definition loaded from JSON (schema 1).
 *
 * Instances are immutable once created by fromJson(), so they can be shared
 * between the MasterTimer thread and the UI thread without locking.
 *
 * Expression variables:
 *   t      cycle position 0..1
 *   theta  2*pi*t
 *   i      fixture index within the EFX
 *   n      number of fixtures
 *   h      head index
 *   <id>   every parameter by its id
 */
class EFXEffect
{
public:
    /** Parse and validate @a json. Returns a null pointer and sets @a error on failure. */
    static QSharedPointer<EFXEffect> fromJson(const QByteArray& json, QString *error = nullptr);

    int schema() const { return m_schema; }
    QString id() const { return m_id; }
    QString name() const { return m_name; }
    int version() const { return m_version; }
    QString description() const { return m_description; }

    /** The exact JSON text this effect was created from (for embedding in projects). */
    QByteArray json() const { return m_json; }

    const QList<EFXParam>& params() const { return m_params; }
    const QList<EFXEffectChannel>& channels() const { return m_channels; }

    /** Default value of every parameter, in params() order. */
    QVector<double> defaultParams() const;

    /** Clamp (and for int/bool round) a parameter value into its allowed range. */
    static double sanitizeParam(const EFXParam& p, double value);

    /** Index in channels() of the given semantic target, or -1. */
    int channelIndex(const QString& target) const;

    /**
     * Evaluate a single channel.
     * @param params parameter values in params() order (missing entries use defaults)
     */
    double evaluateChannel(int channel, double t, double fixtureIndex, double fixtureCount,
                           double head, const QVector<double>& params) const;

    /**
     * Map a cycle angle to the angle used for reverse playback. Uses the
     * effect's "reverse" expression when present, otherwise 2*pi - theta.
     */
    double reverseTheta(double theta, const QVector<double>& params) const;

    /**
     * Value of a keyframe track at cycle position @a t (wraps around). Before
     * the first key the first value is held, after the last key the last one.
     */
    static double evaluateKeys(const QVector<EFXKey>& keys, double t);

    /** True if the effect has a Bezier/polyline path that drives pan and tilt. */
    bool hasPath() const { return m_path.isValid(); }
    const EFXPath& path() const { return m_path; }

    /** True if the effect produces pan and tilt (by path or by channels). */
    bool hasPanTilt() const { return hasPath() || (m_panIndex >= 0 && m_tiltIndex >= 0); }

    /** Convenience: pan/tilt pair (both -1..1). Returns false if the effect has no pan or tilt. */
    bool evaluatePanTilt(double theta, double fixtureIndex, double fixtureCount, double head,
                         const QVector<double>& params, double *pan, double *tilt) const;

    /** Names of variables that precede the parameters in the evaluation array. */
    static QStringList builtinVariables();

private:
    EFXEffect();

    void fillVars(double *vars, double t, double i, double n, double h,
                  const QVector<double>& params) const;

private:
    int m_schema;
    QString m_id;
    QString m_name;
    int m_version;
    QString m_description;
    QByteArray m_json;
    QList<EFXParam> m_params;
    QList<EFXEffectChannel> m_channels;
    EFXPath m_path;
    EFXExpression m_reverse;
    bool m_hasReverse;
    int m_panIndex;
    int m_tiltIndex;
};

typedef QSharedPointer<EFXEffect> EFXEffectPtr;

/** @} */

#endif
