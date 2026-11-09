/*
  Q Light Controller Plus
  simtoefxconverter.cpp

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

#include "simtoefxconverter.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QVector>
#include <cmath>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static QString sanitizeId(const QString& raw)
{
    // EFX ids must match [a-z0-9_-]+ (lower case, no spaces)
    QString out = raw.toLower();
    QString result;
    result.reserve(out.size() + 4);
    for (QChar c : out)
    {
        if (c.isLetterOrNumber() || c == '-' || c == '_')
            result += c;
        else
            result += '-';
    }
    // Collapse consecutive dashes
    while (result.contains("--"))
        result.replace("--", "-");
    // Strip leading/trailing dashes
    while (result.startsWith('-'))
        result.remove(0, 1);
    while (result.endsWith('-'))
        result.chop(1);
    if (result.isEmpty())
        result = "sim-effect";
    return result;
}

// ---------------------------------------------------------------------------
// SimToEfxConverter
// ---------------------------------------------------------------------------

double SimToEfxConverter::dmx16ToEfxRange(int dmx16)
{
    // DMX 0..65535 → EFX -1..1
    return (static_cast<double>(dmx16) / 65535.0) * 2.0 - 1.0;
}

QVector<SimEfxResult> SimToEfxConverter::convert(const QJsonObject& response,
                                                   const QString& namePrefix)
{
    QVector<SimEfxResult> results;
    QJsonArray fixtures = response.value("results").toArray();
    results.reserve(fixtures.size());
    for (const QJsonValue& fv : fixtures)
        results.append(convertFixture(fv.toObject(), namePrefix));
    return results;
}

SimEfxResult SimToEfxConverter::convertFixture(const QJsonObject& fixtureObj,
                                                 const QString& namePrefix)
{
    SimEfxResult result;
    result.spotId      = fixtureObj.value("spot_id").toInt(-1);
    result.qlcFixtureId = fixtureObj.value("qlc_fixture_id").toString();

    QJsonArray keyframes = fixtureObj.value("keyframes").toArray();
    if (keyframes.isEmpty())
    {
        result.error = QStringLiteral("Keine Keyframes für Spot %1").arg(result.spotId);
        return result;
    }

    // Build effect id from qlc_fixture_id or spot_id
    QString rawId = result.qlcFixtureId.isEmpty()
        ? QString("spot-%1").arg(result.spotId)
        : result.qlcFixtureId;
    result.effectId = "sim-" + sanitizeId(rawId);

    QString displayName = namePrefix
        + (result.qlcFixtureId.isEmpty()
            ? QString("Spot %1").arg(result.spotId)
            : result.qlcFixtureId);

    // Build pan and tilt keyframe arrays
    QJsonArray panKeys, tiltKeys;
    for (const QJsonValue& kv : keyframes)
    {
        QJsonObject kf = kv.toObject();
        double t = kf.value("t").toDouble(-1.0);
        if (t < 0.0 || t > 1.0)
            continue;

        // Skip keyframes where the solver found no solution
        if (kf.value("unreachable").toBool(false))
            continue;

        QJsonValue panVal  = kf.value("pan");
        QJsonValue tiltVal = kf.value("tilt");
        if (panVal.isNull() || tiltVal.isNull())
            continue;

        double panEfx  = dmx16ToEfxRange(panVal.toInt(32767));
        double tiltEfx = dmx16ToEfxRange(tiltVal.toInt(32767));

        // Round to 5 decimal places to keep JSON compact
        panEfx  = std::round(panEfx  * 100000.0) / 100000.0;
        tiltEfx = std::round(tiltEfx * 100000.0) / 100000.0;

        QJsonObject pk, tk;
        pk["t"] = t;
        pk["v"] = panEfx;
        tk["t"] = t;
        tk["v"] = tiltEfx;
        panKeys.append(pk);
        tiltKeys.append(tk);
    }

    if (panKeys.isEmpty())
    {
        result.error = QStringLiteral("Alle Keyframes für Spot %1 sind nicht erreichbar.").arg(result.spotId);
        return result;
    }

    // Assemble the EFX-2.0 JSON
    QJsonObject panChannel, tiltChannel;
    panChannel["target"] = "pan";
    panChannel["keys"]   = panKeys;
    tiltChannel["target"] = "tilt";
    tiltChannel["keys"]   = tiltKeys;

    QJsonObject effect;
    effect["schema"]      = 1;
    effect["id"]          = result.effectId;
    effect["name"]        = displayName;
    effect["version"]     = 1;
    effect["description"] = QStringLiteral("Pan/Tilt computed by dmx_semulator (spot %1)").arg(result.spotId);
    effect["channels"]    = QJsonArray({ panChannel, tiltChannel });

    result.efxJson = QJsonDocument(effect).toJson(QJsonDocument::Indented);
    return result;
}
