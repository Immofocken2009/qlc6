/*
  Q Light Controller Plus
  simtoefxconverter.h

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

#ifndef SIMTOEFXCONVERTER_H
#define SIMTOEFXCONVERTER_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

/**
 * Result of converting one per-fixture set of simulation keyframes to EFX JSON.
 */
struct SimEfxResult
{
    QString effectId;       ///< EFX id as stored in the JSON (and EFX library)
    QString qlcFixtureId;   ///< Round-trip QLC+ fixture identifier from the request
    int     spotId;         ///< Simulation spot id
    QByteArray efxJson;     ///< Complete, valid EFX-2.0 JSON (UTF-8)
    QString error;          ///< Non-empty on failure; efxJson is empty then
};

/**
 * Converts a dmx_semulator /api/v1/fx/calculate response into EFX-2.0 JSON files.
 *
 * The simulation response contains per-fixture pan/tilt keyframes as 16-bit DMX values
 * (0–65535).  Each fixture is converted into a *separate* EFX effect whose channels
 * consist of two keyframe tracks: "pan" and "tilt".  Values are mapped to the EFX
 * range [-1, 1] by  v = (dmx16 / 65535.0) * 2.0 - 1.0  (assumes Width=100 %, Height=100 %).
 *
 * One EFX per fixture allows QLC+ to assign each effect independently to the matching
 * fixture.  The effect ids are derived from the qlc_fixture_id supplied in the request,
 * prefixed with "sim-".
 */
class SimToEfxConverter
{
public:
    /**
     * Convert a full /api/v1/fx/calculate response object.
     *
     * @param response      The parsed JSON object from the simulation API.
     * @param namePrefix    Human-readable prefix for effect names (e.g. "My Show – ").
     * @return              One SimEfxResult per fixture in the response.
     *                      A result with a non-empty @a error field means that
     *                      fixture's conversion failed; others may still succeed.
     */
    static QVector<SimEfxResult> convert(const QJsonObject& response,
                                         const QString& namePrefix = QString());

    /**
     * Convert a single fixture entry from the response.
     *
     * @param fixtureObj    One element of response["results"].
     * @param namePrefix    Human-readable prefix for the effect name.
     */
    static SimEfxResult convertFixture(const QJsonObject& fixtureObj,
                                       const QString& namePrefix = QString());

    /** Map a 16-bit DMX value (0–65535) to EFX channel range [-1, 1]. */
    static double dmx16ToEfxRange(int dmx16);

private:
    SimToEfxConverter() = delete;
};

#endif // SIMTOEFXCONVERTER_H
