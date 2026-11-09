/*
  Q Light Controller Plus
  efxeditor.h

  Copyright (c) Massimo Callegari

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

#ifndef EFXEDITOR_H
#define EFXEDITOR_H

#include "functioneditor.h"
#include "treemodel.h"
#include "efxeffect.h"
#include "simulationclient.h"

class Doc;
class EFX;
class ListModel;
class FixtureGroup;

class EFXEditor final : public FunctionEditor
{
    Q_OBJECT

    Q_PROPERTY(QStringList algorithms READ algorithms NOTIFY effectListChanged)
    Q_PROPERTY(int algorithmIndex READ algorithmIndex WRITE setAlgorithmIndex NOTIFY algorithmIndexChanged)
    Q_PROPERTY(QVariantList effectParams READ effectParams NOTIFY effectParamsChanged)

    Q_PROPERTY(bool isRelative READ isRelative WRITE setIsRelative NOTIFY isRelativeChanged)
    Q_PROPERTY(int algorithmWidth READ algorithmWidth WRITE setAlgorithmWidth NOTIFY algorithmWidthChanged)
    Q_PROPERTY(int algorithmHeight READ algorithmHeight WRITE setAlgorithmHeight NOTIFY algorithmHeightChanged)

    Q_PROPERTY(int algorithmXOffset READ algorithmXOffset WRITE setAlgorithmXOffset NOTIFY algorithmXOffsetChanged)
    Q_PROPERTY(int algorithmYOffset READ algorithmYOffset WRITE setAlgorithmYOffset NOTIFY algorithmYOffsetChanged)
    Q_PROPERTY(int algorithmRotation READ algorithmRotation WRITE setAlgorithmRotation NOTIFY algorithmRotationChanged)
    Q_PROPERTY(int algorithmStartOffset READ algorithmStartOffset WRITE setAlgorithmStartOffset NOTIFY algorithmStartOffsetChanged)

    Q_PROPERTY(int propagation READ propagation WRITE setPropagation NOTIFY propagationChanged)

    Q_PROPERTY(bool dimmerControl READ dimmerControl WRITE setDimmerControl NOTIFY dimmerControlChanged)

    Q_PROPERTY(QVariant fixtureList READ fixtureList NOTIFY fixtureListChanged)
    Q_PROPERTY(QVariant groupsTreeModel READ groupsTreeModel NOTIFY groupsTreeModelChanged)
    Q_PROPERTY(qreal maxPanDegrees READ maxPanDegrees NOTIFY maxPanDegreesChanged)
    Q_PROPERTY(qreal maxTiltDegrees READ maxTiltDegrees NOTIFY maxTiltDegreesChanged)

    Q_PROPERTY(QVariantList algorithmData READ algorithmData NOTIFY algorithmDataChanged)
    Q_PROPERTY(QVariantList fixturesData READ fixturesData WRITE setFixturesData NOTIFY fixturesDataChanged)

    Q_PROPERTY(bool pathEditing READ pathEditing NOTIFY pathEditingChanged)
    Q_PROPERTY(QVariantList pathPoints READ pathPoints NOTIFY pathPointsChanged)
    Q_PROPERTY(bool pathClosed READ pathClosed WRITE setPathClosed NOTIFY pathPointsChanged)

    // Simulation API
    Q_PROPERTY(QString simApiUrl READ simApiUrl WRITE setSimApiUrl NOTIFY simApiUrlChanged)
    Q_PROPERTY(QString simApiToken READ simApiToken WRITE setSimApiToken NOTIFY simApiTokenChanged)
    Q_PROPERTY(bool simBusy READ simBusy NOTIFY simBusyChanged)
    Q_PROPERTY(QVariantList simScenarios READ simScenarios NOTIFY simScenariosChanged)
    Q_PROPERTY(QVariantList simSpots READ simSpots NOTIFY simSpotsChanged)
    Q_PROPERTY(QString simLastError READ simLastError NOTIFY simLastErrorChanged)
    Q_PROPERTY(QString simLastStatus READ simLastStatus NOTIFY simLastStatusChanged)

public:
    EFXEditor(QQuickView *view, Doc *doc, QObject *parent = 0);
    ~EFXEditor();

    /** Set the ID of the EFX to edit */
    void setFunctionID(quint32 id) override;

    /** Rebuild the selector list from the effect library */
    void refreshEffectList();

protected slots:
    void slotAttributeChanged(int attrIndex, qreal fraction);

private:
    /** Reference of the EFX currently being edited */
    EFX *m_efx;

    /** Effect ids shown in the selector, sorted; same order as algorithms() */
    QStringList m_effectIds;

    /************************************************************************
     * Algorithm
     ************************************************************************/
public:
    QStringList algorithms() const;

    /** Get/set the EFX selected algorithm index*/
    int algorithmIndex() const;
    void setAlgorithmIndex(int algoIndex);

    /** Get/set if the EFX has relative movement */
    bool isRelative() const;
    void setIsRelative(bool relative);

    /** Get/set the current algorithm width */
    int algorithmWidth() const;
    void setAlgorithmWidth(int algorithmWidth);

    /** Get/set the current algorithm height */
    int algorithmHeight() const;
    void setAlgorithmHeight(int algorithmHeight);

    /** Get/set the current algorithm X offset */
    int algorithmXOffset() const;
    void setAlgorithmXOffset(int algorithmXOffset);

    /** Get/set the current algorithm Y offset */
    int algorithmYOffset() const;
    void setAlgorithmYOffset(int algorithmYOffset);

    /** Get/set the current algorithm rotation */
    int algorithmRotation() const;
    void setAlgorithmRotation(int algorithmRotation);

    /** Get/set the current algorithm start offset */
    int algorithmStartOffset() const;
    void setAlgorithmStartOffset(int algorithmStartOffset);

    /** The parameters of the selected effect, one map per parameter:
     *  id, label, type, min, max, step, value */
    QVariantList effectParams() const;

    /** Set a parameter of the selected effect by its id */
    Q_INVOKABLE void setEffectParam(const QString &id, double value);

    /** Get/set whether the EFX drives the fixtures' dimmer from their tilt */
    bool dimmerControl() const;
    void setDimmerControl(bool dimmerControl);

signals:
    void algorithmIndexChanged();
    void isRelativeChanged();
    void dimmerControlChanged();
    void algorithmWidthChanged();
    void algorithmHeightChanged();
    void algorithmXOffsetChanged();
    void algorithmYOffsetChanged();
    void algorithmRotationChanged();
    void algorithmStartOffsetChanged();
    void effectListChanged();
    void effectParamsChanged();

    /************************************************************************
     * Fixtures
     ************************************************************************/
public:
    /** Get/Set the EFX propagation mode */
    int propagation() const;
    void setPropagation(int newPropagation);

    /** Get the EFX fixture list for the UI */
    QVariant fixtureList() const;

    /** Returns the data model to display a tree of FixtureGroups/Fixtures */
    QVariant groupsTreeModel();

    /** Return the maximum Pan/Tilt degrees gotten from a fixture list update */
    qreal maxPanDegrees() const;
    qreal maxTiltDegrees() const;

    /** Add a Fixture Group or a Universe to this EFX */
    Q_INVOKABLE void addGroup(QVariant reference);

    /** Add all the heads of a single Fixture to this EFX */
    Q_INVOKABLE void addFixture(QVariant reference);

    /** Add a single head to this EFX */
    Q_INVOKABLE void addHead(int fixtureID, int headIndex);

    /** Remove a head from this EFX */
    Q_INVOKABLE void removeHeads(QVariantList heads);

    /** Set the working mode for the provided Fixture */
    Q_INVOKABLE void setFixtureMode(quint32 fixtureID, int headIndex, int modeIndex);

    /** Reverse the provided Fixture */
    Q_INVOKABLE void setFixtureReversed(quint32 fixtureID, int headIndex, bool reversed);

    /** How to distribute an offset across all the EFX fixtures */
    enum FixturesOffsetMode
    {
        OffsetAbsolute = 0, //!< The same offset on every fixture
        OffsetIncreasing,   //!< Increasing offset: offset, 2*offset, 3*offset, ...
        OffsetRandom        //!< Increasing offsets, randomly assigned to the fixtures
    };
    Q_ENUM(FixturesOffsetMode)

    /** Set an offset in degrees to the provided fixture and head */
    Q_INVOKABLE void setFixtureOffset(quint32 fixtureID, int headIndex, int offset);

    /** Apply an $offset in degrees to all the EFX fixtures, distributed per $mode
        (see FixturesOffsetMode) */
    Q_INVOKABLE void setFixturesOffset(int offset, int mode);

protected:
    void updateFixtureList();

signals:
    /** Notify the listeners that the fixture propagation mode has changed */
    void propagationChanged();

    /** Notify the listeners that the fixture list model has changed */
    void fixtureListChanged();

    /** Notify the listeners that the fixture tree model has changed */
    void groupsTreeModelChanged();

    /** Notify the listeners that the Pan/Tilt degrees have changed */
    void maxPanDegreesChanged();
    void maxTiltDegreesChanged();

private:
    /** Reference to a ListModel representing the fixtures list for the QML UI */
    ListModel *m_fixtureList;
    /** Data model used by the QML UI to represent groups/fixtures/heads */
    TreeModel *m_fixtureTree;

    qreal m_maxPanDegrees, m_maxTiltDegrees;

    /************************************************************************
     * Algorithm preview
     ************************************************************************/
public:
    QVariantList algorithmData();
    QVariantList fixturesData();
    void setFixturesData(const QVariantList &data);

private:
    void updateAlgorithmData();

signals:
    void algorithmDataChanged();
    void fixturesDataChanged();

    /************************************************************************
     * Shape (path) editing
     ************************************************************************/
public:
    /** True while a shape is being edited (not saved yet) */
    bool pathEditing() const;

    /** The points of the shape being edited, in the -1..1 pan/tilt space. One map per
     *  point: x, y (anchor), ix, iy (incoming handle), ox, oy (outgoing handle) and
     *  corner (true when both handles sit on the anchor) */
    QVariantList pathPoints() const;

    bool pathClosed() const;
    void setPathClosed(bool closed);

    /** Start editing the current effect as a path. Formula effects are converted to points
     *  (the original stays untouched). @return false if there is nothing to edit */
    Q_INVOKABLE bool beginPathEdit();

    /** Throw the changes away and go back to the effect that was selected before */
    Q_INVOKABLE void cancelPathEdit();

    /** Save the edited shape as a new user effect named @a name and select it.
     *  @return an empty string on success, otherwise an error message */
    Q_INVOKABLE QString commitPathEdit(const QString &name);

    /** Move an anchor; its handles move with it */
    Q_INVOKABLE void movePathPoint(int index, qreal x, qreal y);

    /** Move one handle of an anchor. With @a mirror the opposite handle follows symmetrically */
    Q_INVOKABLE void movePathHandle(int index, bool outgoing, qreal x, qreal y, bool mirror);

    /** Add a point at (x, y): on the path it splits the nearest segment (the shape is kept),
     *  away from the path it is appended to an open path. @return the index of the new point */
    Q_INVOKABLE int insertPathPoint(qreal x, qreal y);

    /** Remove a point (a path keeps at least 2) */
    Q_INVOKABLE void deletePathPoint(int index);

    /** Make a point a sharp corner, or smooth it with automatic handles */
    Q_INVOKABLE void setPathPointCorner(int index, bool corner);

    /** Replace the shape with a freehand stroke: a flat list x0, y0, x1, y1... */
    Q_INVOKABLE void setPathStroke(const QVariantList &xy);

signals:
    void pathEditingChanged();
    void pathPointsChanged();

private:
    /** Rebuild the previewed effect from the draft points */
    void applyPathDraft();
    void autoHandles(int index);

private:
    bool m_pathEditing;
    QVector<EFXPathPoint> m_draftPoints;
    bool m_draftClosed;
    EFXPath::Easing m_draftEasing;
    EFXEffectPtr m_draftOrigin;

private:
    /** EFX algorithm data exchanged with the QML world */
    QVariantList m_algorithmData;
    /** Start index and direction of each fixture of the EFX */
    QVariantList m_fixturesData;

    /************************************************************************
     * Simulation API integration
     ************************************************************************/
public:
    QString simApiUrl() const;
    void setSimApiUrl(const QString& url);
    QString simApiToken() const;
    void setSimApiToken(const QString& token);
    bool simBusy() const;
    QVariantList simScenarios() const;
    QVariantList simSpots() const;
    QString simLastError() const;
    QString simLastStatus() const;

    /** Test connection to the simulation server.  Result via simConnectionTested(). */
    Q_INVOKABLE void simTestConnection();
    /** Fetch scenario list.  Result via simScenariosChanged(). */
    Q_INVOKABLE void simFetchScenarios();
    /** Fetch spots for @a scenarioId.  Result via simSpotsChanged(). */
    Q_INVOKABLE void simFetchSpots(int scenarioId);
    /**
     * Send @a request to the simulation and import the result as user effects.
     * @param request QVariantMap with keys hall_id, fixtures ([{spot_id, qlc_fixture_id}]),
     *                movements ([{t,x,y}]), coord_system.
     * Result: emits simImportDone(importedCount, errorCount).
     */
    Q_INVOKABLE void simCalculateAndImport(const QVariantMap& request);

    /** Save API URL and token to QSettings */
    Q_INVOKABLE void simSaveSettings();

signals:
    void simApiUrlChanged();
    void simApiTokenChanged();
    void simBusyChanged();
    void simScenariosChanged();
    void simSpotsChanged();
    void simLastErrorChanged();
    void simLastStatusChanged();

    /** Emitted when testConnection() completes. */
    void simConnectionTested(bool ok, const QString& message);
    /** Emitted after calculateAndImport() – how many effects were imported / failed. */
    void simImportDone(int imported, int failed);

private:
    void onSimConnectionTested(bool ok, const QString& msg, const QJsonObject& info);
    void onSimScenariosFetched(const QJsonArray& arr);
    void onSimSpotsFetched(int scenarioId, const QJsonObject& obj);
    void onSimPanTiltCalculated(const QJsonObject& response);
    void onSimRequestFailed(const QString& op, const QString& err);

private:
    SimulationClient *m_simClient;
    QVariantList m_simScenarios;
    QVariantList m_simSpots;
    QString m_simLastError;
    QString m_simLastStatus;
};

#endif // EFXEDITOR_H
