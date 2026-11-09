/*
  Q Light Controller Plus
  simulationclient.cpp

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

#include "simulationclient.h"

#include <QNetworkReply>
#include <QJsonDocument>
#include <QSettings>
#include <QDebug>

SimulationClient::SimulationClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    load();
}

// ---------------------------------------------------------------------------
// Properties
// ---------------------------------------------------------------------------

QString SimulationClient::apiUrl() const
{
    return m_apiUrl;
}

void SimulationClient::setApiUrl(const QString& url)
{
    QString trimmed = url.trimmed();
    // Strip trailing slash for consistent path building
    while (trimmed.endsWith('/'))
        trimmed.chop(1);
    if (m_apiUrl == trimmed)
        return;
    m_apiUrl = trimmed;
    emit apiUrlChanged();
}

QString SimulationClient::apiToken() const
{
    return m_apiToken;
}

void SimulationClient::setApiToken(const QString& token)
{
    if (m_apiToken == token)
        return;
    m_apiToken = token;
    emit apiTokenChanged();
}

bool SimulationClient::busy() const
{
    return m_busy;
}

// ---------------------------------------------------------------------------
// Persistence
// ---------------------------------------------------------------------------

void SimulationClient::save()
{
    QSettings settings;
    settings.beginGroup(SETTINGS_GROUP);
    settings.setValue(SETTINGS_URL, m_apiUrl);
    // Store token obfuscated (XOR with 0xAA) – not cryptographically secure,
    // just prevents casual plaintext exposure in the settings file.
    QByteArray tok = m_apiToken.toUtf8();
    for (char& c : tok)
        c ^= static_cast<char>(0xAA);
    settings.setValue(SETTINGS_TOKEN, QString::fromLatin1(tok.toBase64()));
    settings.endGroup();
}

void SimulationClient::load()
{
    QSettings settings;
    settings.beginGroup(SETTINGS_GROUP);
    m_apiUrl = settings.value(SETTINGS_URL, QString()).toString().trimmed();
    while (m_apiUrl.endsWith('/'))
        m_apiUrl.chop(1);
    QString b64 = settings.value(SETTINGS_TOKEN, QString()).toString();
    if (!b64.isEmpty())
    {
        QByteArray tok = QByteArray::fromBase64(b64.toLatin1());
        for (char& c : tok)
            c ^= static_cast<char>(0xAA);
        m_apiToken = QString::fromUtf8(tok);
    }
    settings.endGroup();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

QNetworkRequest SimulationClient::buildRequest(const QString& path) const
{
    QUrl url(m_apiUrl + path);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_apiToken.isEmpty())
        req.setRawHeader("Authorization", ("Bearer " + m_apiToken).toUtf8());
    return req;
}

void SimulationClient::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

// ---------------------------------------------------------------------------
// Public slots
// ---------------------------------------------------------------------------

void SimulationClient::testConnection()
{
    if (m_apiUrl.isEmpty())
    {
        emit connectionTested(false, tr("API-URL ist nicht konfiguriert."), QJsonObject());
        return;
    }
    setBusy(true);
    QNetworkReply *reply = m_nam->get(buildRequest("/api/v1/fx/health"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        setBusy(false);
        handleHealthReply(reply);
    });
}

void SimulationClient::fetchScenarios()
{
    if (m_apiUrl.isEmpty())
    {
        emit requestFailed("fetchScenarios", tr("API-URL ist nicht konfiguriert."));
        return;
    }
    setBusy(true);
    QNetworkReply *reply = m_nam->get(buildRequest("/api/v1/fx/scenarios"));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        setBusy(false);
        handleScenariosReply(reply);
    });
}

void SimulationClient::fetchScenarioSpots(int scenarioId)
{
    if (m_apiUrl.isEmpty())
    {
        emit requestFailed("fetchScenarioSpots", tr("API-URL ist nicht konfiguriert."));
        return;
    }
    setBusy(true);
    QNetworkReply *reply = m_nam->get(buildRequest(
        QString("/api/v1/fx/scenarios/%1/spots").arg(scenarioId)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, scenarioId]() {
        reply->deleteLater();
        setBusy(false);
        handleSpotsReply(reply, scenarioId);
    });
}

void SimulationClient::calculatePanTilt(const QJsonObject& request)
{
    if (m_apiUrl.isEmpty())
    {
        emit requestFailed("calculatePanTilt", tr("API-URL ist nicht konfiguriert."));
        return;
    }
    QJsonObject req = request;
    req["api_version"] = API_VERSION;
    QByteArray body = QJsonDocument(req).toJson(QJsonDocument::Compact);

    QNetworkRequest netReq = buildRequest("/api/v1/fx/calculate");
    setBusy(true);
    QNetworkReply *reply = m_nam->post(netReq, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        setBusy(false);
        handleCalculateReply(reply);
    });
}

// ---------------------------------------------------------------------------
// Reply handlers
// ---------------------------------------------------------------------------

void SimulationClient::handleHealthReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        emit connectionTested(false, reply->errorString(), QJsonObject());
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();
    bool ok = obj.value("ok").toBool(false);
    QString msg = ok ? tr("Verbindung erfolgreich.")
                     : tr("Server antwortete, aber 'ok' ist nicht true.");
    emit connectionTested(ok, msg, obj);
}

void SimulationClient::handleScenariosReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        emit requestFailed("fetchScenarios", reply->errorString());
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isArray())
    {
        emit requestFailed("fetchScenarios", tr("Unerwartetes Antwortformat (kein Array)."));
        return;
    }
    emit scenariosFetched(doc.array());
}

void SimulationClient::handleSpotsReply(QNetworkReply *reply, int scenarioId)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        emit requestFailed("fetchScenarioSpots", reply->errorString());
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject())
    {
        emit requestFailed("fetchScenarioSpots", tr("Unerwartetes Antwortformat (kein Objekt)."));
        return;
    }
    emit spotsFetched(scenarioId, doc.object());
}

void SimulationClient::handleCalculateReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        // Distinguish 401 (auth error) for a better user message
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpStatus == 401)
            emit requestFailed("calculatePanTilt", tr("Authentifizierung fehlgeschlagen. Bitte API-Token prüfen."));
        else
            emit requestFailed("calculatePanTilt", reply->errorString());
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    if (!doc.isObject())
    {
        emit requestFailed("calculatePanTilt", tr("Unerwartetes Antwortformat."));
        return;
    }
    QJsonObject obj = doc.object();
    if (obj.value("status").toString() != "ok")
    {
        emit requestFailed("calculatePanTilt",
            obj.value("error").toString(tr("Unbekannter Fehler vom Server.")));
        return;
    }
    emit panTiltCalculated(obj);
}
