/*
  Q Light Controller Plus
  simulationclient.h

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

#ifndef SIMULATIONCLIENT_H
#define SIMULATIONCLIENT_H

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QObject>
#include <QString>

/**
 * HTTP client for communicating with the dmx_semulator FX calculation API.
 *
 * API URL and bearer token are persisted in QSettings under the "SimulationAPI" group.
 * All network calls are asynchronous; results are delivered via signals.
 *
 * API endpoints used:
 *   GET  /api/v1/fx/health                     – connection test (no auth required)
 *   GET  /api/v1/fx/scenarios                  – list available scenarios
 *   GET  /api/v1/fx/scenarios/<id>/spots       – spots in a scenario
 *   POST /api/v1/fx/calculate                  – XY movements → pan/tilt keyframes
 */
class SimulationClient final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString apiUrl READ apiUrl WRITE setApiUrl NOTIFY apiUrlChanged)
    Q_PROPERTY(QString apiToken READ apiToken WRITE setApiToken NOTIFY apiTokenChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit SimulationClient(QObject *parent = nullptr);

    QString apiUrl() const;
    void setApiUrl(const QString& url);

    /** Bearer token; stored obfuscated (not encrypted) in QSettings. */
    QString apiToken() const;
    void setApiToken(const QString& token);

    bool busy() const;

    /** Persist current URL and token to QSettings. */
    void save();
    /** Load URL and token from QSettings. */
    void load();

public slots:
    /** Test reachability. Emits connectionTested(). No auth required. */
    Q_INVOKABLE void testConnection();

    /** Fetch list of scenarios. Emits scenariosFetched() or requestFailed(). */
    Q_INVOKABLE void fetchScenarios();

    /**
     * Fetch spots for a scenario. Emits spotsFetched() or requestFailed().
     * @param scenarioId the scenario_id as returned by fetchScenarios()
     */
    Q_INVOKABLE void fetchScenarioSpots(int scenarioId);

    /**
     * Calculate pan/tilt keyframes from XY movements.
     *
     * @param request JSON object matching the /api/v1/fx/calculate schema:
     *   {
     *     "hall_id": <int>,
     *     "fixtures": [{"spot_id":<int>, "qlc_fixture_id":"<str>"}],
     *     "movements": [{"t":<float>, "x":<float>, "y":<float>}],
     *     "coord_system": "normalized" | "absolute"
     *   }
     * Emits panTiltCalculated() on success, requestFailed() on error.
     */
    Q_INVOKABLE void calculatePanTilt(const QJsonObject& request);

signals:
    void apiUrlChanged();
    void apiTokenChanged();
    void busyChanged();

    /** Emitted after testConnection(). @a ok is true when the server replied with {"ok":true}. */
    void connectionTested(bool ok, const QString& message, const QJsonObject& serverInfo);

    /** Emitted after fetchScenarios(). @a scenarios is an array of scenario objects. */
    void scenariosFetched(const QJsonArray& scenarios);

    /** Emitted after fetchScenarioSpots(). @a spots is the full response object. */
    void spotsFetched(int scenarioId, const QJsonObject& spots);

    /** Emitted after calculatePanTilt(). @a response is the full /calculate response. */
    void panTiltCalculated(const QJsonObject& response);

    /** Emitted on any network or API error. */
    void requestFailed(const QString& operation, const QString& errorMessage);

private:
    QNetworkRequest buildRequest(const QString& path) const;
    void setBusy(bool busy);

    // Handlers bound via lambda in each request method
    void handleHealthReply(QNetworkReply *reply);
    void handleScenariosReply(QNetworkReply *reply);
    void handleSpotsReply(QNetworkReply *reply, int scenarioId);
    void handleCalculateReply(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_nam;
    QString m_apiUrl;
    QString m_apiToken;
    bool m_busy = false;

    static constexpr const char* SETTINGS_GROUP = "SimulationAPI";
    static constexpr const char* SETTINGS_URL   = "apiUrl";
    static constexpr const char* SETTINGS_TOKEN = "apiToken";
    static constexpr const char* API_VERSION    = "1";
};

#endif // SIMULATIONCLIENT_H
