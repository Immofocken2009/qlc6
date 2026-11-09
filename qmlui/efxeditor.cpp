/*
  Q Light Controller Plus
  efxeditor.cpp

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

#include <QTimer>
#include <QRegularExpression>
#include <QDebug>
#include <QQmlEngine>
#include <QRandomGenerator>

#include "qlcfixturemode.h"
#include "fixturemanager.h"
#include "efxeditor.h"
#include "listmodel.h"
#include "tardis.h"
#include "efxlibrary.h"
#include "efxpath.h"
#include "efx.h"
#include "doc.h"
#include "app.h"

EFXEditor::EFXEditor(QQuickView *view, Doc *doc, QObject *parent)
    : FunctionEditor(view, doc, parent)
    , m_efx(nullptr)
    , m_fixtureTree(nullptr)
    , m_maxPanDegrees(360.0)
    , m_maxTiltDegrees(270.0)
    , m_pathEditing(false)
    , m_draftClosed(true)
    , m_draftEasing(EFXPath::ConstantSpeed)
{
    m_editorObjectName = "efxEditorRoot";

    refreshEffectList();
    connect(EFXLibrary::instance(), &EFXLibrary::effectsChanged, this, [this]()
    {
        refreshEffectList();
        emit effectListChanged();
        emit algorithmIndexChanged();
        emit effectParamsChanged();
        updateAlgorithmData();
    });

    m_fixtureList = new ListModel(this);
    QStringList listRoles;
    listRoles << "name" << "fxID" << "head" << "isSelected" << "mode" << "reverse" << "offset";
    m_fixtureList->setRoleNames(listRoles);

    // publish to QML only once fully constructed: bindings of an already
    // loaded editor view re-evaluate immediately and read the models
    m_view->rootContext()->setContextProperty("efxEditor", this);
}

EFXEditor::~EFXEditor()
{
    m_view->rootContext()->setContextProperty("efxEditor", nullptr);
}

void EFXEditor::refreshEffectList()
{
    m_effectIds.clear();
    const QList<EFXEffectPtr> list = EFXLibrary::instance()->effects();
    for (const EFXEffectPtr &fx : list)
        m_effectIds << fx->id();
}

void EFXEditor::setFunctionID(quint32 id)
{
    if (m_pathEditing)
        cancelPathEdit();

    if (id == Function::invalidId())
    {
        disconnect(m_efx, &EFX::attributeChanged, this, &EFXEditor::slotAttributeChanged);
        m_efx = nullptr;
        return;
    }

    m_efx = qobject_cast<EFX *>(m_doc->function(id));
    if (m_efx != nullptr)
    {
        connect(m_efx, &EFX::attributeChanged, this, &EFXEditor::slotAttributeChanged);
        updateAlgorithmData();
        emit algorithmIndexChanged();
        emit effectParamsChanged();
    }

    FunctionEditor::setFunctionID(id);
    updateFixtureList();
}

void EFXEditor::slotAttributeChanged(int attrIndex, qreal fraction)
{
    Q_UNUSED(fraction)

    switch (attrIndex)
    {
        case EFX::Width: emit algorithmWidthChanged(); break;
        case EFX::Height: emit algorithmHeightChanged(); break;
        case EFX::Rotation: emit algorithmRotationChanged(); break;
        case EFX::XOffset: emit algorithmXOffsetChanged(); break;
        case EFX::YOffset: emit algorithmYOffsetChanged(); break;
        case EFX::StartOffset: emit algorithmStartOffsetChanged(); break;
        default: break;
    }

    updateAlgorithmData();
}

/************************************************************************
 * Algorithm
 ************************************************************************/

QStringList EFXEditor::algorithms() const
{
    QStringList names;
    const QList<EFXEffectPtr> list = EFXLibrary::instance()->effects();
    for (const EFXEffectPtr &fx : list)
        names << fx->name();
    return names;
}

int EFXEditor::algorithmIndex() const
{
    if (m_efx == nullptr)
        return 0;

    return m_effectIds.indexOf(m_efx->effectId());
}

void EFXEditor::setAlgorithmIndex(int algoIndex)
{
    if (m_efx == nullptr || algoIndex < 0 || algoIndex >= m_effectIds.size())
        return;

    const QString newId = m_effectIds.at(algoIndex);
    if (newId == m_efx->effectId())
        return;

    Tardis::instance()->enqueueAction(Tardis::EFXSetEffect, m_efx->id(), m_efx->effectId(), newId);
    m_efx->setEffect(newId);
    emit algorithmIndexChanged();
    emit effectParamsChanged();
    updateAlgorithmData();
}

QVariantList EFXEditor::effectParams() const
{
    QVariantList list;
    if (m_efx == nullptr || !m_efx->hasEffect())
        return list;

    const QList<EFXParam> params = m_efx->effect()->params();
    for (const EFXParam &p : params)
    {
        QVariantMap m;
        m["id"] = p.id;
        m["label"] = p.label;
        m["type"] = p.type;
        m["min"] = p.min;
        m["max"] = p.max;
        m["step"] = p.step;
        m["value"] = m_efx->effectParam(p.id);
        list << m;
    }
    return list;
}

void EFXEditor::setEffectParam(const QString &id, double value)
{
    if (m_efx == nullptr)
        return;

    const double oldValue = m_efx->effectParam(id);
    m_efx->setEffectParam(id, value);
    const double newValue = m_efx->effectParam(id);
    if (oldValue == newValue)
        return;

    Tardis::instance()->enqueueAction(Tardis::EFXSetEffectParam, m_efx->id(),
                                      QVariantList() << id << oldValue, QVariantList() << id << newValue);
    emit effectParamsChanged();
    updateAlgorithmData();
}

bool EFXEditor::isRelative() const
{
    if (m_efx == nullptr)
        return false;

    return m_efx->isRelative();
}

void EFXEditor::setIsRelative(bool relative)
{
    if (m_efx == nullptr || relative == m_efx->isRelative())
        return;

    Tardis::instance()->enqueueAction(Tardis::EFXSetRelative, m_efx->id(), m_efx->isRelative(), relative);
    m_efx->setIsRelative(relative);

    if (relative)
    {
        setAlgorithmXOffset(127);
        setAlgorithmYOffset(127);
    }
    emit isRelativeChanged();
    updateAlgorithmData();
}

int EFXEditor::algorithmWidth() const
{
    if (m_efx == nullptr)
        return 0;

    return m_efx->width();
}

void EFXEditor::setAlgorithmWidth(int algorithmWidth)
{
    if (m_efx == nullptr || algorithmWidth == m_efx->width())
        return;

    Tardis::instance()->enqueueAction(Tardis::EFXSetWidth, m_efx->id(), m_efx->width(), algorithmWidth);
    m_efx->setWidth(algorithmWidth);
    emit algorithmWidthChanged();
    updateAlgorithmData();
}

int EFXEditor::algorithmHeight() const
{
    if (m_efx == nullptr)
        return 0;

    return m_efx->height();
}

void EFXEditor::setAlgorithmHeight(int algorithmHeight)
{
    if (m_efx == nullptr || algorithmHeight == m_efx->height())
        return;

    Tardis::instance()->enqueueAction(Tardis::EFXSetHeight, m_efx->id(), m_efx->height(), algorithmHeight);
    m_efx->setHeight(algorithmHeight);
    emit algorithmHeightChanged();
    updateAlgorithmData();
}

int EFXEditor::algorithmXOffset() const
{
    if (m_efx == nullptr)
        return 0;

    return m_efx->xOffset();
}

void EFXEditor::setAlgorithmXOffset(int algorithmXOffset)
{
    if (m_efx == nullptr || algorithmXOffset == m_efx->xOffset())
        return;

    Tardis::instance()->enqueueAction(Tardis::EFXSetXOffset, m_efx->id(), m_efx->xOffset(), algorithmXOffset);
    m_efx->setXOffset(algorithmXOffset);
    emit algorithmXOffsetChanged();
    updateAlgorithmData();
}

int EFXEditor::algorithmYOffset() const
{
    if (m_efx == nullptr)
        return 0;

    return m_efx->yOffset();
}

void EFXEditor::setAlgorithmYOffset(int algorithmYOffset)
{
    if (m_efx == nullptr || algorithmYOffset == m_efx->yOffset())
        return;

    Tardis::instance()->enqueueAction(Tardis::EFXSetYOffset, m_efx->id(), m_efx->yOffset(), algorithmYOffset);
    m_efx->setYOffset(algorithmYOffset);
    emit algorithmYOffsetChanged();
    updateAlgorithmData();
}

int EFXEditor::algorithmRotation() const
{
    if (m_efx == nullptr)
        return 0;

    return m_efx->rotation();
}

void EFXEditor::setAlgorithmRotation(int algorithmRotation)
{
    if (m_efx == nullptr || algorithmRotation == m_efx->rotation())
        return;

    Tardis::instance()->enqueueAction(Tardis::EFXSetRotation, m_efx->id(), m_efx->rotation(), algorithmRotation);
    m_efx->setRotation(algorithmRotation);
    emit algorithmRotationChanged();
    updateAlgorithmData();
}

int EFXEditor::algorithmStartOffset() const
{
    if (m_efx == nullptr)
        return 0;

    return m_efx->startOffset();
}

void EFXEditor::setAlgorithmStartOffset(int algorithmStartOffset)
{
    if (m_efx == nullptr || algorithmStartOffset == m_efx->startOffset())
        return;

    Tardis::instance()->enqueueAction(Tardis::EFXSetStartOffset, m_efx->id(), m_efx->startOffset(), algorithmStartOffset);
    m_efx->setStartOffset(algorithmStartOffset);
    emit algorithmStartOffsetChanged();
    updateAlgorithmData();
}

bool EFXEditor::dimmerControl() const
{
    if (m_efx == nullptr)
        return false;

    return m_efx->dimmerControlEnabled();
}

void EFXEditor::setDimmerControl(bool dimmerControl)
{
    if (m_efx == nullptr || dimmerControl == m_efx->dimmerControlEnabled())
        return;

    Tardis::instance()->enqueueAction(Tardis::EFXSetDimmerControl, m_efx->id(), m_efx->dimmerControlEnabled(), dimmerControl);
    m_efx->setDimmerControlEnabled(dimmerControl);
    emit dimmerControlChanged();
}

/************************************************************************
 * Fixtures
 ************************************************************************/

int EFXEditor::propagation() const
{
    if (m_efx == nullptr)
        return 0;

    return m_efx->propagationMode();
}

void EFXEditor::setPropagation(int newPropagation)
{
    if (m_efx == nullptr || newPropagation == m_efx->propagationMode())
        return;

    Tardis::instance()->enqueueAction(Tardis::EFXFixturePropagation, m_efx->id(), m_efx->propagationMode(), newPropagation);
    m_efx->setPropagationMode(EFX::PropagationMode(newPropagation));
    emit propagationChanged();
    updateAlgorithmData();
}

QVariant EFXEditor::fixtureList() const
{
    return QVariant::fromValue(m_fixtureList);
}

QVariant EFXEditor::groupsTreeModel()
{
    if (m_fixtureTree == nullptr)
    {
        m_fixtureTree = new TreeModel(this);
        QQmlEngine::setObjectOwnership(m_fixtureTree, QQmlEngine::CppOwnership);
        QStringList treeColumns;
        treeColumns << "classRef" << "type" << "id" << "subid" << "head";
        m_fixtureTree->setColumnNames(treeColumns);
        m_fixtureTree->enableSorting(false);
        FixtureManager::updateGroupsTree(m_doc, m_fixtureTree); // TODO: search filter ?
    }

    return QVariant::fromValue(m_fixtureTree);
}

qreal EFXEditor::maxPanDegrees() const
{
    return m_maxPanDegrees;
}

qreal EFXEditor::maxTiltDegrees() const
{
    return m_maxTiltDegrees;
}

void EFXEditor::addGroup(QVariant reference)
{
    //qDebug() << "[EFXEditor::addGroup]" << reference;
    bool listChanged = false;

    if (m_efx == nullptr)
        return;

    if (reference.canConvert<Universe *>())
    {
        Universe *uni = reference.value<Universe *>();

        if (uni != nullptr)
        {
            qDebug() << "Adding a universe";
            for (Fixture *fixture : m_doc->fixtures()) // C++11
            {
                if (fixture->universe() != uni->id())
                    continue;

                for (int headIdx = 0; headIdx < fixture->heads(); headIdx++)
                {
                    quint32 panCh = fixture->channelNumber(QLCChannel::Pan, QLCChannel::MSB, headIdx);
                    quint32 tiltCh = fixture->channelNumber(QLCChannel::Tilt, QLCChannel::MSB, headIdx);

                    if (panCh != QLCChannel::invalid() || tiltCh != QLCChannel::invalid())
                    {
                        EFXFixture *ef = new EFXFixture(m_efx);
                        GroupHead head(fixture->id(), headIdx);
                        ef->setHead(head);

                        if (m_efx->addFixture(ef) == false)
                            delete ef;
                        else
                            listChanged = true;
                    }
                }
            }
        }
    }
    else if (reference.canConvert<FixtureGroup *>())
    {
        FixtureGroup *group = reference.value<FixtureGroup *>();

        if (group != nullptr)
        {
            qDebug() << "Adding a fixture group";
            for (GroupHead head : group->headList())
            {
                Fixture *fixture = m_doc->fixture(head.fxi);
                if (fixture == nullptr)
                    continue;

                quint32 panCh = fixture->channelNumber(QLCChannel::Pan, QLCChannel::MSB, head.head);
                quint32 tiltCh = fixture->channelNumber(QLCChannel::Tilt, QLCChannel::MSB, head.head);

                if (panCh != QLCChannel::invalid() || tiltCh != QLCChannel::invalid())
                {
                    EFXFixture *ef = new EFXFixture(m_efx);
                    ef->setHead(head);
                    if (m_efx->addFixture(ef) == false)
                    {
                        delete ef;
                    }
                    else
                    {
                        Tardis::instance()->enqueueAction(Tardis::EFXAddFixture, m_efx->id(), QVariant(),
                                                          Tardis::instance()->actionToByteArray(Tardis::EFXAddFixture, m_efx->id(),
                                                                                                QVariant::fromValue((void *)ef)));
                        listChanged = true;
                    }
                }
            }
        }
    }

    if (listChanged)
        updateFixtureList();
}

void EFXEditor::addFixture(QVariant reference)
{
    if (m_efx == nullptr)
        return;

    if (reference.canConvert<Fixture *>() == false)
        return;

    Fixture *fixture = reference.value<Fixture *>();
    int count = 0;

    for (int headIdx = 0; headIdx < fixture->heads(); headIdx++)
    {
        EFXFixture *ef = new EFXFixture(m_efx);
        GroupHead head(fixture->id(), headIdx);
        ef->setHead(head);

        if (m_efx->addFixture(ef) == false)
        {
            delete ef;
        }
        else
        {
            Tardis::instance()->enqueueAction(Tardis::EFXAddFixture, m_efx->id(), QVariant(),
                                              Tardis::instance()->actionToByteArray(Tardis::EFXAddFixture, m_efx->id(),
                                                                                    QVariant::fromValue((void *)ef)));
            count++;
        }
    }

    if (count)
        updateFixtureList();
}

void EFXEditor::addHead(int fixtureID, int headIndex)
{
    if (m_efx == nullptr)
        return;

    GroupHead head(fixtureID, headIndex);
    EFXFixture *ef = new EFXFixture(m_efx);
    ef->setHead(head);

    if (m_efx->addFixture(ef) == false)
    {
        delete ef;
    }
    else
    {
        Tardis::instance()->enqueueAction(Tardis::EFXAddFixture, m_efx->id(), QVariant(),
                                          Tardis::instance()->actionToByteArray(Tardis::EFXAddFixture, m_efx->id(),
                                                                                QVariant::fromValue((void *)ef)));
        updateFixtureList();
    }
}

void EFXEditor::removeHeads(QVariantList heads)
{
    if (m_efx == nullptr)
        return;

    for (QVariant &vIdx : heads)
    {
        QModelIndex idx = m_fixtureList->index(vIdx.toInt(), 0, QModelIndex());
        QVariant fixtureID = m_fixtureList->data(idx, "fxID");
        QVariant headIndex = m_fixtureList->data(idx, "head");

        qDebug() << "Removing fixture" << fixtureID << "head" << headIndex;

        EFXFixture *ef = m_efx->fixture(fixtureID.toUInt(), headIndex.toInt());
        if (ef == nullptr)
            continue;

        Tardis::instance()->enqueueAction(Tardis::EFXRemoveFixture, m_efx->id(),
                                          Tardis::instance()->actionToByteArray(Tardis::EFXAddFixture, m_efx->id(),
                                                                                QVariant::fromValue((void *)ef)),
                                          QVariant());

        m_efx->removeFixture(ef);
    }
    updateFixtureList();
}

void EFXEditor::setFixtureMode(quint32 fixtureID, int headIndex, int modeIndex)
{
    if (m_efx == nullptr)
        return;

    EFXFixture *ef = m_efx->fixture(fixtureID, headIndex);

    if (ef != nullptr)
    {
        Tardis::instance()->enqueueAction(Tardis::EFXSetFixtureMode, m_efx->id(),
                                          QVariantList({ int(fixtureID), headIndex, int(ef->mode()) }),
                                          QVariantList({ int(fixtureID), headIndex, modeIndex }));
        ef->setMode(EFXFixture::Mode(modeIndex));
    }
}

void EFXEditor::setFixtureReversed(quint32 fixtureID, int headIndex, bool reversed)
{
    if (m_efx == nullptr)
        return;

    EFXFixture *ef = m_efx->fixture(fixtureID, headIndex);

    if (ef != nullptr)
    {
        Tardis::instance()->enqueueAction(Tardis::EFXSetFixtureReversed, m_efx->id(),
                                          QVariantList({ int(fixtureID), headIndex, ef->direction() == Function::Backward ? 1 : 0 }),
                                          QVariantList({ int(fixtureID), headIndex, reversed ? 1 : 0 }));
        ef->setDirection(reversed ? Function::Backward : Function::Forward);
        updateAlgorithmData();
    }
}

void EFXEditor::setFixtureOffset(quint32 fixtureID, int headIndex, int offset)
{
    if (m_efx == nullptr)
        return;

    EFXFixture *ef = m_efx->fixture(fixtureID, headIndex);

    if (ef != nullptr)
    {
        Tardis::instance()->enqueueAction(Tardis::EFXSetFixtureOffset, m_efx->id(),
                                          QVariantList({ int(fixtureID), headIndex, ef->startOffset() }),
                                          QVariantList({ int(fixtureID), headIndex, offset }));
        ef->setStartOffset(offset);
        updateAlgorithmData();
    }
}

void EFXEditor::setFixturesOffset(int offset, int mode)
{
    if (m_efx == nullptr)
        return;

    QList<EFXFixture *> fixtures = m_efx->fixtures();

    if (mode == OffsetAbsolute)
    {
        // The same absolute offset on every fixture
        for (EFXFixture *ef : fixtures)
        {
            Tardis::instance()->enqueueAction(Tardis::EFXSetFixtureOffset, m_efx->id(),
                                              QVariantList({ int(ef->head().fxi), ef->head().head, ef->startOffset() }),
                                              QVariantList({ int(ef->head().fxi), ef->head().head, offset }));
            ef->setStartOffset(offset);
        }
    }
    else
    {
        // Build the increasing offsets starting from 0: 0, offset, 2*offset, ... (mod 360)
        QList<int> offsets;
        int currentOffset = 0;
        for (int i = 0; i < fixtures.count(); i++)
        {
            offsets.append(currentOffset % 360);
            currentOffset += offset;
        }

        // Random mode: shuffle which fixture gets which of those offsets
        if (mode == OffsetRandom)
        {
            for (int i = offsets.count() - 1; i > 0; i--)
            {
                int j = QRandomGenerator::global()->generate() % (i + 1);
                qSwap(offsets[i], offsets[j]);
            }
        }

        for (int i = 0; i < fixtures.count(); i++)
        {
            EFXFixture *ef = fixtures.at(i);
            Tardis::instance()->enqueueAction(Tardis::EFXSetFixtureOffset, m_efx->id(),
                                              QVariantList({ int(ef->head().fxi), ef->head().head, ef->startOffset() }),
                                              QVariantList({ int(ef->head().fxi), ef->head().head, offsets.at(i) }));
            ef->setStartOffset(offsets.at(i));
        }
    }

    updateFixtureList();
    updateAlgorithmData();
}

void EFXEditor::updateFixtureList()
{
    m_fixtureList->clear();

    if (m_efx == nullptr)
        return;

    qreal oldPanDegrees = m_maxPanDegrees;
    qreal oldTiltDegrees = m_maxTiltDegrees;

    // listRoles << "name" << "fxID" << "head" << "isSelected" << "mode" << "reverse" << "offset";

    for (EFXFixture *ef : m_efx->fixtures()) // C++11
    {
        QVariantMap fxMap;
        GroupHead head = ef->head();
        Fixture *fixture = m_doc->fixture(head.fxi);

        if (fixture == nullptr || head.isValid() == false)
            continue;

        if (fixture->fixtureMode() != nullptr && fixture->fixtureMode()->physical().focusPanMax() > m_maxPanDegrees)
            m_maxPanDegrees = fixture->fixtureMode()->physical().focusPanMax();

        if (fixture->fixtureMode() != nullptr && fixture->fixtureMode()->physical().focusTiltMax() > m_maxTiltDegrees)
            m_maxTiltDegrees = fixture->fixtureMode()->physical().focusTiltMax();

        if (fixture->heads() > 1)
            fxMap.insert("name", QString("%1 [%2]").arg(fixture->name()).arg(ef->head().head));
        else
            fxMap.insert("name", fixture->name());
        fxMap.insert("fxID", head.fxi);
        fxMap.insert("head", head.head);
        fxMap.insert("isSelected", false);
        fxMap.insert("mode", ef->mode());
        fxMap.insert("reverse", ef->direction() == Function::Backward ? true : false);
        fxMap.insert("offset", ef->startOffset());

        m_fixtureList->addDataMap(fxMap);
    }

    emit fixtureListChanged();
    if (oldPanDegrees != m_maxPanDegrees)
        emit maxPanDegreesChanged();
    if (oldTiltDegrees != m_maxTiltDegrees)
        emit maxTiltDegreesChanged();
    updateAlgorithmData();
}

/************************************************************************
 * Preview
 ************************************************************************/

QVariantList EFXEditor::algorithmData()
{
    return m_algorithmData;
}

QVariantList EFXEditor::fixturesData()
{
    return m_fixturesData;
}

void EFXEditor::setFixturesData(const QVariantList &data)
{
    if (m_fixturesData != data)
    {
        m_fixturesData = data;
        emit fixturesDataChanged();
    }
}

void EFXEditor::updateAlgorithmData()
{
    if (m_efx == nullptr)
        return;

    QPolygonF polygon;
    m_efx->preview(polygon);

    m_algorithmData.clear();
    m_fixturesData.clear();

    /** 1- fill a QVariantList or XY coordinates in [0 - 255] range
     *  representing the EFX algorithm */
    for (int i = 0; i < polygon.size(); i++)
    {
        QPointF pt = polygon.at(i);
        m_algorithmData.append(pt.x());
        m_algorithmData.append(pt.y());
    }

    /** 2- for each fixture, find the closest XY coordinate
     *  along the EFX algorithm, and store the start index and
     *  the direction of the animation that will happen in the UI
     *  NOTE: the data array is filled backward to display first fixtures on top */
    for (EFXFixture *fixture : m_efx->fixtures()) // C++11
    {
        float x = 0, y = 0;

        /** Append the delta to apply on each animation step */
        if (fixture->direction() == Function::Forward)
            m_fixturesData.prepend(1);
        else
            m_fixturesData.prepend(-1);

        /** Pre-determined case. Easy to solve */
        if (fixture->startOffset() == 0)
        {
            if (fixture->direction() == Function::Forward)
                m_fixturesData.prepend(0);
            else
                m_fixturesData.prepend(polygon.count() - 1);
        }
        else
        {
            int pathIdx = 0;
            float distance = 1000.0;

            /** With a start offset, we need to scan the algorithm points
             *  to find the index of the closest one */
            m_efx->calculatePoint(fixture->direction(), fixture->startOffset(), 0, &x, &y);
            //qDebug() << "Got position:" << x << y << fixture->startOffset();

            for (int i = 0; i < polygon.count(); i++)
            {
                QPointF delta = QPointF(x, y) - polygon.at(i);
                qreal pointsDist = delta.manhattanLength();

                if (pointsDist < distance)
                {
                    pathIdx = i;
                    distance = pointsDist;
                }
            }
            //qDebug() << "Closest point found at index" << pathIdx;
            m_fixturesData.prepend(pathIdx);
        }
    }

    emit algorithmDataChanged();
    emit fixturesDataChanged();
}

/************************************************************************
 * Shape (path) editing
 ************************************************************************/

static const double PathLimit = 3.0; // keep anchors and handles in a sane area

static QPointF clampPoint(const QPointF &p)
{
    return QPointF(qBound(-PathLimit, p.x(), PathLimit), qBound(-PathLimit, p.y(), PathLimit));
}

bool EFXEditor::pathEditing() const
{
    return m_pathEditing;
}

QVariantList EFXEditor::pathPoints() const
{
    QVariantList list;
    for (const EFXPathPoint &p : m_draftPoints)
    {
        QVariantMap m;
        m["x"] = p.pos.x();
        m["y"] = p.pos.y();
        m["ix"] = p.in.x();
        m["iy"] = p.in.y();
        m["ox"] = p.out.x();
        m["oy"] = p.out.y();
        m["corner"] = (p.in == p.pos && p.out == p.pos);
        list << m;
    }
    return list;
}

bool EFXEditor::pathClosed() const
{
    return m_draftClosed;
}

void EFXEditor::setPathClosed(bool closed)
{
    if (!m_pathEditing || closed == m_draftClosed)
        return;

    m_draftClosed = closed;
    applyPathDraft();
}

bool EFXEditor::beginPathEdit()
{
    if (m_efx == nullptr || m_pathEditing)
        return false;

    EFXEffectPtr fx = m_efx->effect();
    if (fx.isNull() || !fx->hasPanTilt())
        return false;

    if (fx->hasPath())
    {
        m_draftPoints = fx->path().points();
        m_draftClosed = fx->path().isClosed();
        m_draftEasing = fx->path().easing();
        if (!fx->path().isBezier())
        {
            // a polyline is a path of corners
            for (EFXPathPoint &p : m_draftPoints)
                p.in = p.out = p.pos;
        }
    }
    else
    {
        // formula effect: sample one cycle and fit a few points to it
        QVector<double> params;
        for (const EFXParam &p : fx->params())
            params << m_efx->effectParam(p.id);
        const QVector<QPointF> samples = fx->samplePanTilt(params, 256);
        m_draftPoints = EFXPath::fromStroke(samples, 0.01, true);
        m_draftClosed = true;
        m_draftEasing = EFXPath::ConstantSpeed;
    }

    if (m_draftPoints.size() < 2)
        return false;

    m_draftOrigin = fx;
    m_pathEditing = true;
    emit pathEditingChanged();
    applyPathDraft();
    return true;
}

void EFXEditor::cancelPathEdit()
{
    if (!m_pathEditing)
        return;

    m_pathEditing = false;
    m_draftPoints.clear();
    if (m_efx != nullptr && !m_draftOrigin.isNull())
        m_efx->setEffect(m_draftOrigin);
    m_draftOrigin.clear();

    emit pathEditingChanged();
    emit pathPointsChanged();
    emit algorithmIndexChanged();
    emit effectParamsChanged();
    updateAlgorithmData();
}

QString EFXEditor::commitPathEdit(const QString &name)
{
    if (!m_pathEditing || m_efx == nullptr || m_draftOrigin.isNull())
        return tr("Nothing to save");

    const QString title = name.trimmed().isEmpty() ? tr("My shape") : name.trimmed();

    // build a unique, file name safe id: user-<slug>[-n]
    QString slug = title.toLower();
    slug.replace(QRegularExpression("[^a-z0-9]+"), "-");
    slug.remove(QRegularExpression("^-+|-+$"));
    if (slug.isEmpty())
        slug = "shape";
    slug = slug.left(40);

    EFXLibrary *lib = EFXLibrary::instance();
    QString id = "user-" + slug;
    for (int n = 2; !lib->effect(id).isNull() && n < 1000; n++)
        id = QString("user-%1-%2").arg(slug).arg(n);

    EFXPath path;
    QString error;
    if (!path.setPoints(m_draftPoints, m_draftClosed, true, m_draftEasing, &error))
        return error;

    const QByteArray json = m_draftOrigin->withPath(path.toJson(), id, title, &error);
    if (json.isEmpty())
        return error;

    const QString oldId = m_draftOrigin->id();
    QString savedId;
    if (!lib->addUserEffect(json, &error, &savedId))
        return error; // still editing, nothing was saved

    m_pathEditing = false;
    m_draftPoints.clear();
    m_draftOrigin.clear();

    m_efx->setEffect(savedId);
    Tardis::instance()->enqueueAction(Tardis::EFXSetEffect, m_efx->id(), oldId, savedId);

    emit pathEditingChanged();
    emit pathPointsChanged();
    emit algorithmIndexChanged();
    emit effectParamsChanged();
    updateAlgorithmData();
    return QString();
}

void EFXEditor::applyPathDraft()
{
    if (!m_pathEditing || m_efx == nullptr || m_draftOrigin.isNull())
        return;

    EFXPath path;
    if (path.setPoints(m_draftPoints, m_draftClosed, true, m_draftEasing))
    {
        const QByteArray json = m_draftOrigin->withPath(path.toJson(), "efx-draft", tr("Edited shape"));
        EFXEffectPtr fx = EFXEffect::fromJson(json);
        if (!fx.isNull())
            m_efx->setEffect(fx);
    }

    emit pathPointsChanged();
    emit effectParamsChanged();
    updateAlgorithmData();
}

void EFXEditor::movePathPoint(int index, qreal x, qreal y)
{
    if (!m_pathEditing || index < 0 || index >= m_draftPoints.size())
        return;

    EFXPathPoint &p = m_draftPoints[index];
    const QPointF np = clampPoint(QPointF(x, y));
    const QPointF delta = np - p.pos;
    p.pos = np;
    p.in = clampPoint(p.in + delta);
    p.out = clampPoint(p.out + delta);
    applyPathDraft();
}

void EFXEditor::movePathHandle(int index, bool outgoing, qreal x, qreal y, bool mirror)
{
    if (!m_pathEditing || index < 0 || index >= m_draftPoints.size())
        return;

    EFXPathPoint &p = m_draftPoints[index];
    const QPointF h = clampPoint(QPointF(x, y));
    const QPointF other = clampPoint(p.pos * 2.0 - h);
    if (outgoing)
    {
        p.out = h;
        if (mirror)
            p.in = other;
    }
    else
    {
        p.in = h;
        if (mirror)
            p.out = other;
    }
    applyPathDraft();
}

int EFXEditor::insertPathPoint(qreal x, qreal y)
{
    if (!m_pathEditing || m_draftPoints.size() >= EFXPath::MaxPoints)
        return -1;

    const QPointF click = clampPoint(QPointF(x, y));
    const int n = m_draftPoints.size();
    const int segments = m_draftClosed ? n : n - 1;

    // find the nearest point on the curve
    const int steps = 48;
    double best = 1e9, bestU = 0;
    int bestSeg = 0;
    for (int s = 0; s < segments; s++)
    {
        const EFXPathPoint &a = m_draftPoints.at(s);
        const EFXPathPoint &b = m_draftPoints.at((s + 1) % n);
        for (int k = 0; k <= steps; k++)
        {
            const double u = double(k) / steps, v = 1.0 - u;
            const QPointF c = a.pos * (v * v * v) + a.out * (3 * v * v * u) +
                              b.in * (3 * v * u * u) + b.pos * (u * u * u);
            const double d = std::hypot(c.x() - click.x(), c.y() - click.y());
            if (d < best)
            {
                best = d;
                bestU = u;
                bestSeg = s;
            }
        }
    }

    // away from the curve of an open path: append a corner at the click position
    if (!m_draftClosed && best > 0.08)
    {
        EFXPathPoint np;
        np.pos = np.in = np.out = click;
        m_draftPoints.append(np);
        applyPathDraft();
        return m_draftPoints.size() - 1;
    }

    // split the segment at bestU (de Casteljau), the shape does not change
    EFXPathPoint &a = m_draftPoints[bestSeg];
    EFXPathPoint &b = m_draftPoints[(bestSeg + 1) % n];
    const double u = bestU;
    auto lerp = [u](const QPointF &p, const QPointF &q) { return p + (q - p) * u; };
    const QPointF q0 = lerp(a.pos, a.out), q1 = lerp(a.out, b.in), q2 = lerp(b.in, b.pos);
    const QPointF r0 = lerp(q0, q1), r1 = lerp(q1, q2);
    EFXPathPoint np;
    np.pos = lerp(r0, r1);
    np.in = r0;
    np.out = r1;
    a.out = q0;
    b.in = q2;

    const int at = bestSeg + 1;
    m_draftPoints.insert(at, np);
    applyPathDraft();
    return at;
}

void EFXEditor::deletePathPoint(int index)
{
    if (!m_pathEditing || index < 0 || index >= m_draftPoints.size() || m_draftPoints.size() <= 2)
        return;

    m_draftPoints.remove(index);
    applyPathDraft();
}

void EFXEditor::autoHandles(int index)
{
    const int n = m_draftPoints.size();
    EFXPathPoint &p = m_draftPoints[index];
    const bool hasPrev = m_draftClosed || index > 0;
    const bool hasNext = m_draftClosed || index < n - 1;
    const QPointF prev = m_draftPoints.at((index + n - 1) % n).pos;
    const QPointF next = m_draftPoints.at((index + 1) % n).pos;

    QPointF dir = hasPrev && hasNext ? next - prev : (hasNext ? next - p.pos : p.pos - prev);
    const double l = std::hypot(dir.x(), dir.y());
    if (l <= 0)
        return;
    dir /= l;

    const double lPrev = hasPrev ? std::hypot(p.pos.x() - prev.x(), p.pos.y() - prev.y()) : 0;
    const double lNext = hasNext ? std::hypot(next.x() - p.pos.x(), next.y() - p.pos.y()) : 0;
    p.out = hasNext ? clampPoint(p.pos + dir * (lNext / 3.0)) : p.pos;
    p.in = hasPrev ? clampPoint(p.pos - dir * (lPrev / 3.0)) : p.pos;
}

void EFXEditor::setPathPointCorner(int index, bool corner)
{
    if (!m_pathEditing || index < 0 || index >= m_draftPoints.size())
        return;

    if (corner)
        m_draftPoints[index].in = m_draftPoints[index].out = m_draftPoints[index].pos;
    else
        autoHandles(index);
    applyPathDraft();
}

void EFXEditor::setPathStroke(const QVariantList &xy)
{
    if (!m_pathEditing)
        return;

    QVector<QPointF> stroke;
    for (int i = 0; i + 1 < xy.size(); i += 2)
        stroke << clampPoint(QPointF(xy.at(i).toDouble(), xy.at(i + 1).toDouble()));
    if (stroke.size() < 4)
        return;

    // a stroke that ends near its start is a closed shape
    const QPointF d = stroke.last() - stroke.first();
    const bool closed = stroke.size() >= 8 && std::hypot(d.x(), d.y()) < 0.15;

    const QVector<EFXPathPoint> pts = EFXPath::fromStroke(stroke, 0.03, closed);
    if (pts.size() < 2)
        return;

    m_draftPoints = pts;
    m_draftClosed = closed;
    applyPathDraft();
}
