/*
  Q Light Controller Plus
  efx.cpp

  Copyright (C) Heikki Junnila
                Massimo Callegari

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

#include <QVector>
#include <QDebug>
#include <QList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <math.h>

#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>

#include "efxlibrary.h"
#include "genericfader.h"
#include "mastertimer.h"
#include "qlcmacros.h"
#include "doc.h"
#include "efx.h"
#include "bus.h"

/*****************************************************************************
 * Initialization
 *****************************************************************************/

EFX::EFX(Doc* doc)
    : Function(doc, Function::EFXType)
    , m_dimmerControl(false)
    , m_isRelative(false)
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    , m_fixturesMutex(QMutex::Recursive)
#endif
    , m_propagationMode(Parallel)
    , m_legacyFadeBus(Bus::invalid())
    , m_legacyHoldBus(Bus::invalid())
{
    updateRotationCache();
    setName(tr("New EFX"));
    setDuration(20000); // 20s

    registerAttribute(tr("Width"), Function::LastWins, 0.0, 127.0, 127.0);
    registerAttribute(tr("Height"), Function::LastWins, 0.0, 127.0, 127.0);
    registerAttribute(tr("Rotation"), Function::LastWins, 0.0, 359.0, 0.0);
    registerAttribute(tr("X Offset"), Function::LastWins, 0.0, 255.0, 127.0);
    registerAttribute(tr("Y Offset"), Function::LastWins, 0.0, 255.0, 127.0);
    registerAttribute(tr("Start Offset"), Function::LastWins, 0.0, 359.0, 0.0);

    connect(EFXLibrary::instance(), &EFXLibrary::effectsChanged, this, &EFX::slotLibraryChanged);

    // Same default as the former algorithm; stays empty if the library has no "circle"
    setEffect(QStringLiteral("circle"));
}

EFX::~EFX()
{
    while (m_fixtures.isEmpty() == false)
        delete m_fixtures.takeFirst();
}

QIcon EFX::getIcon() const
{
    return QIcon(":/efx.png");
}

/*****************************************************************************
 * Copying
 *****************************************************************************/

Function* EFX::createCopy(Doc* doc, bool addToDoc)
{
    Q_ASSERT(doc != NULL);

    Function* copy = new EFX(doc);
    if (copy->copyFrom(this) == false)
    {
        delete copy;
        copy = NULL;
    }
    if (addToDoc == true && doc->addFunction(copy) == false)
    {
        delete copy;
        copy = NULL;
    }

    return copy;
}

bool EFX::copyFrom(const Function* function)
{
    const EFX* efx = qobject_cast<const EFX*> (function);
    if (efx == NULL)
        return false;

    while (m_fixtures.isEmpty() == false)
        delete m_fixtures.takeFirst();

    QListIterator <EFXFixture*> it(efx->m_fixtures);
    while (it.hasNext() == true)
    {
        EFXFixture* ef = new EFXFixture(this);
        ef->copyFrom(it.next());
        m_fixtures.append(ef);
    }

    m_propagationMode = efx->m_propagationMode;

    for (int i = 0; i < efx->attributes().count(); i++)
        adjustAttribute(efx->attributes().at(i).m_value, i);

    m_isRelative = efx->m_isRelative;

    updateRotationCache();

    if (efx != this)
    {
        QMutexLocker src(&efx->m_effectMutex);
        QMutexLocker dst(&m_effectMutex);
        m_effectId = efx->m_effectId;
        m_effect = efx->m_effect;
        m_embedded = efx->m_embedded;
        m_paramMap = efx->m_paramMap;
        m_paramVector = efx->m_paramVector;
    }

    m_dimmerControl = efx->m_dimmerControl;

    return Function::copyFrom(function);
}

void EFX::setDuration(uint ms)
{
    Function::setDuration(ms);

    for (int i = 0; i < m_fixtures.size(); ++i)
        m_fixtures[i]->durationChanged();

    emit durationChanged(ms);
}

uint EFX::loopDuration() const
{
    uint fadeIn = overrideFadeInSpeed() == defaultSpeed() ? fadeInSpeed() : overrideFadeInSpeed();

    // With a fade in not shorter than the duration, the subtraction would
    // wrap around to a huge loop and the EFX would look frozen: use the
    // whole duration for the loop instead
    if (fadeIn >= duration())
        return duration();

    return duration() - fadeIn;
}

/*****************************************************************************
 * Effect (JSON definition)
 *****************************************************************************/

bool EFX::setEffect(const QString& id)
{
    EFXEffectPtr fx = EFXLibrary::instance()->effect(id);
    if (fx.isNull())
        return false;

    {
        QMutexLocker locker(&m_effectMutex);
        m_paramMap.clear();
    }
    setEffect(fx);
    return true;
}

void EFX::setEffect(EFXEffectPtr effect)
{
    if (effect.isNull())
        return;

    {
        QMutexLocker locker(&m_effectMutex);
        m_effect = effect;
        m_effectId = effect->id();
        m_embedded = effect->json();
        rebuildParamVector();
    }
    emit changed(this->id());
}

EFXEffectPtr EFX::effect() const
{
    QMutexLocker locker(&m_effectMutex);
    return m_effect;
}

QString EFX::effectId() const
{
    QMutexLocker locker(&m_effectMutex);
    return m_effectId;
}

bool EFX::hasEffect() const
{
    QMutexLocker locker(&m_effectMutex);
    return !m_effect.isNull();
}

bool EFX::hasPanTilt() const
{
    QMutexLocker locker(&m_effectMutex);
    return !m_effect.isNull() && m_effect->hasPanTilt();
}

void EFX::setEffectParam(const QString& id, double value)
{
    {
        QMutexLocker locker(&m_effectMutex);
        if (!m_effect.isNull())
        {
            bool known = false;
            for (const EFXParam& p : m_effect->params())
            {
                if (p.id == id)
                {
                    value = EFXEffect::sanitizeParam(p, value);
                    known = true;
                    break;
                }
            }
            if (!known)
                return;
        }
        m_paramMap[id] = value;
        rebuildParamVector();
    }
    emit changed(this->id());
}

double EFX::effectParam(const QString& id) const
{
    QMutexLocker locker(&m_effectMutex);
    if (m_paramMap.contains(id))
        return m_paramMap.value(id);
    if (!m_effect.isNull())
    {
        for (const EFXParam& p : m_effect->params())
        {
            if (p.id == id)
                return p.def;
        }
    }
    return 0.0;
}

QVector<double> EFX::effectParamValues() const
{
    QMutexLocker locker(&m_effectMutex);
    return m_paramVector;
}

void EFX::resetEffectParams()
{
    {
        QMutexLocker locker(&m_effectMutex);
        m_paramMap.clear();
        rebuildParamVector();
    }
    emit changed(this->id());
}

// caller holds m_effectMutex
void EFX::rebuildParamVector()
{
    m_paramVector.clear();
    if (m_effect.isNull())
        return;

    for (const EFXParam& p : m_effect->params())
    {
        double v = m_paramMap.contains(p.id) ? EFXEffect::sanitizeParam(p, m_paramMap.value(p.id)) : p.def;
        m_paramVector << v;
    }
}

/* Library first, so an updated library effect applies to every project that
 * uses it; the embedded copy is the fallback when the library file is missing. */
void EFX::resolveEffect()
{
    EFXEffectPtr fx = EFXLibrary::instance()->effect(m_effectId);

    if (fx.isNull() && !m_embedded.isEmpty())
    {
        QString err;
        fx = EFXEffect::fromJson(m_embedded, &err);
        if (fx.isNull())
            qWarning() << "EFX" << name() << ": embedded effect is invalid:" << err;
    }

    if (fx.isNull())
    {
        qWarning() << "EFX" << name() << ": effect" << m_effectId << "not found in the library and not embedded";
        m_effect.clear();
    }
    else
    {
        m_effect = fx;
        m_embedded = fx->json();
    }
    rebuildParamVector();
}

void EFX::slotLibraryChanged()
{
    bool changedEffect = false;
    {
        QMutexLocker locker(&m_effectMutex);
        if (m_effectId.isEmpty())
            return;
        EFXEffectPtr fx = EFXLibrary::instance()->effect(m_effectId);
        if (fx.isNull() || fx == m_effect)
            return;
        m_effect = fx;
        m_embedded = fx->json();
        rebuildParamVector();
        changedEffect = true;
    }
    if (changedEffect)
        emit changed(this->id());
}

bool EFX::dimmerControlEnabled() const
{
    return m_dimmerControl;
}

void EFX::setDimmerControlEnabled(bool enable)
{
    m_dimmerControl = enable;
}

void EFX::preview(QPolygonF &polygon) const
{
    preview(polygon, Function::Forward, 0);
}

void EFX::previewFixtures(QVector <QPolygonF>& polygons) const
{
    polygons.resize(m_fixtures.size());
    for (int i = 0; i < m_fixtures.size(); ++i)
    {
        int propagationOffset =
            (m_propagationMode == EFX::Asymmetric || m_propagationMode == EFX::Serial)
                ? 360 / m_fixtures.size() * i
                : 0;

        preview(polygons[i], m_fixtures[i]->m_direction,
                m_fixtures[i]->m_startOffset + propagationOffset, i, m_fixtures.size());
    }
}

void EFX::preview(QPolygonF &polygon, Function::Direction direction, int startOffset,
                  int fixtureIndex, int fixtureCount) const
{
    float stepCount = 512.0;
    int step = 0;
    float stepSize = 1.0 / (stepCount / (M_PI * 2.0));

    float i = 0;
    float x = 0;
    float y = 0;

    /* Reset the polygon to fill it with new values */
    polygon.clear();

    /* Draw a preview of the effect */
    for (step = 0; step < stepCount; step++)
    {
        calculatePoint(direction, startOffset, i, &x, &y, fixtureIndex, fixtureCount);
        polygon << QPointF(x, y);
        i += stepSize;
    }
}

void EFX::calculatePoint(Function::Direction direction, int startOffset, float iterator, float *x, float *y,
                         int fixtureIndex, int fixtureCount, int head) const
{
    calculatePoint(effectiveAngle(direction, startOffset, iterator), x, y, fixtureIndex, fixtureCount, head);
}

float EFX::effectiveAngle(Function::Direction direction, int startOffset, float iterator) const
{
    iterator = calculateDirection(direction, iterator);
    iterator += convertOffset(startOffset + getAttributeValue(StartOffset));

    if (iterator >= M_PI * 2.0)
        iterator -= M_PI * 2.0;

    return iterator;
}

EFXEffectPtr EFX::calculateChannelValues(Function::Direction direction, int startOffset, float iterator,
                                         int fixtureIndex, int fixtureCount, int head,
                                         QVector<double> *values) const
{
    EFXEffectPtr fx;
    QVector<double> params;
    {
        QMutexLocker locker(&m_effectMutex);
        fx = m_effect;
        params = m_paramVector;
    }

    if (fx.isNull())
        return fx;

    const double t = double(effectiveAngle(direction, startOffset, iterator)) / (M_PI * 2.0);
    const int count = fx->channels().size();
    values->resize(count);
    for (int i = 0; i < count; i++)
        (*values)[i] = fx->evaluateChannel(i, t, fixtureIndex, fixtureCount, head, params);
    return fx;
}

void EFX::rotateAndScale(float *x, float *y) const
{
    float xx = *x;
    float yy = *y;
    float w = getAttributeValue(Width);
    float h = getAttributeValue(Height);
    float fadeScale = 1.0;

    if (isRunning())
    {
        uint fadeIn = overrideFadeInSpeed() == defaultSpeed() ? fadeInSpeed() : overrideFadeInSpeed();
        if (fadeIn > 0 && elapsed() <= fadeIn)
        {
            fadeScale = SCALE(float(elapsed()),
                              float(0), float(fadeIn),
                              float(0), float(1.0));
        }
    }

    *x = (getAttributeValue(XOffset)) + xx * m_cosR * (w * fadeScale) + yy * m_sinR * (h * fadeScale);
    *y = (getAttributeValue(YOffset)) + -xx * m_sinR * (w * fadeScale) + yy * m_cosR * (h * fadeScale);
}

float EFX::calculateDirection(Function::Direction direction, float iterator) const
{
    if (direction == this->direction())
        return iterator;

    EFXEffectPtr fx;
    QVector<double> params;
    {
        QMutexLocker locker(&m_effectMutex);
        fx = m_effect;
        params = m_paramVector;
    }

    if (fx.isNull())
        return (M_PI * 2.0) - iterator;

    return float(fx->reverseTheta(iterator, params));
}

float EFX::dimmerLevel(float startOffset, float iterator) const
{
    /* The dimmer follows the tilt itself, so it fades at the same RATE as the
       movement (not linearly), reaching full/0 exactly when the tilt reaches its
       extremes. The angle is phased per fixture by its StartOffset (both in
       radians). Over the first half [0, PI) the tilt goes from its +1 extreme to
       its -1 extreme; the second half [PI, 2PI) (the return stroke) stays black.
       The EFX Direction is handled by the movement itself, so it is not applied
       here. */
    const float PI = float(M_PI);
    const float TWO_PI = float(M_PI * 2.0);

    float angle = iterator + startOffset;
    angle = fmodf(angle, TWO_PI);
    if (angle < 0.0f)
        angle += TWO_PI;

    if (angle >= PI)
        return 0.0f; // return stroke: black

    /* cos(angle): +1 at angle 0 (tilt extreme) -> -1 at angle PI (other extreme).
       Map to 0..1 so brightness tracks the tilt curve: full at +1, dark at -1. */
    return (cosf(angle) + 1.0f) * 0.5f;
}

// this function should map from 0..M_PI * 2 -> -1..1
void EFX::calculatePoint(float iterator, float *x, float *y, int fixtureIndex, int fixtureCount, int head) const
{
    EFXEffectPtr fx;
    QVector<double> params;
    {
        QMutexLocker locker(&m_effectMutex);
        fx = m_effect;
        params = m_paramVector;
    }

    double pan = 0.0, tilt = 0.0;
    if (!fx.isNull())
        fx->evaluatePanTilt(iterator, fixtureIndex, fixtureCount, head, params, &pan, &tilt);

    *x = float(pan);
    *y = float(tilt);

    rotateAndScale(x, y);
}

/*****************************************************************************
 * Width
 *****************************************************************************/

void EFX::setWidth(int width)
{
    adjustAttribute(static_cast<double> (CLAMP(width, 0, 127)), Width);
    emit changed(this->id());
}

int EFX::width() const
{
    return static_cast<int> (attributes().at(Width).m_value);
}

/*****************************************************************************
 * Height
 *****************************************************************************/

void EFX::setHeight(int height)
{
    adjustAttribute(static_cast<double> (CLAMP(height, 0, 127)), Height);
    emit changed(this->id());
}

int EFX::height() const
{
    return static_cast<int> (attributes().at(Height).m_value);
}

/*****************************************************************************
 * Rotation
 *****************************************************************************/

void EFX::setRotation(int rot)
{
    adjustAttribute(CLAMP(rot, 0, 359), Rotation);
    updateRotationCache();
    emit changed(this->id());
}

int EFX::rotation() const
{
    return static_cast<int> (attributes().at(Rotation).m_value);
}

void EFX::updateRotationCache()
{
    double r = M_PI/180 * getAttributeValue(Rotation);
    m_cosR = cos(r);
    m_sinR = sin(r);
}

/*****************************************************************************
 * Start Offset
 *****************************************************************************/

void EFX::setStartOffset(int startOffset)
{
    adjustAttribute(CLAMP(startOffset, 0, 359), StartOffset);
    emit changed(this->id());
}

int EFX::startOffset() const
{
    return static_cast<int> (attributes().at(StartOffset).m_value);
}

float EFX::convertOffset(int offset) const
{
    return M_PI/180 * (offset % 360);
}

/*****************************************************************************
 * Is Relative
 *****************************************************************************/

void EFX::setIsRelative(bool isRelative)
{
    m_isRelative = isRelative;
    emit changed(this->id());
}

bool EFX::isRelative() const
{
    return m_isRelative;
}

/*****************************************************************************
 * Offset
 *****************************************************************************/

void EFX::setXOffset(int offset)
{
    adjustAttribute(static_cast<double> (CLAMP(offset, 0, (int)UCHAR_MAX)), XOffset);
    emit changed(this->id());
}

int EFX::xOffset() const
{
    return static_cast<int> (attributes().at(XOffset).m_value);
}

void EFX::setYOffset(int offset)
{
    adjustAttribute(static_cast<double> (CLAMP(offset, 0, (int)UCHAR_MAX)), YOffset);
    emit changed(this->id());
}

int EFX::yOffset() const
{
    return static_cast<int> (attributes().at(YOffset).m_value);
}

/*****************************************************************************
 * Fixtures
 *****************************************************************************/

bool EFX::addFixture(EFXFixture* ef)
{
    Q_ASSERT(ef != NULL);

    /* Search for an existing fixture with the same ID and append at last but do
     * not prevent multiple entries because a fixture can have multiple efx. */
    //! @todo Prevent multiple entries using head & mode
    {
        QMutexLocker locker(&m_fixturesMutex);
        int i;
        for (i = 0; i < m_fixtures.size(); i++)
        {
            if (m_fixtures[i]->head() == ef->head())
            {
                m_fixtures.insert(i, ef);
                break;
            }
        }

        /* If not inserted, put the EFXFixture object into our list */
        if (i >= m_fixtures.size())
            m_fixtures.append(ef);
    }

    emit changed(this->id());

    return true;
}

bool EFX::addFixture(quint32 fxi, int head)
{
    EFXFixture *ef = new EFXFixture(this);
    GroupHead gHead(fxi, head);
    ef->setHead(gHead);

    return addFixture(ef);
}

bool EFX::removeFixture(EFXFixture* ef)
{
    Q_ASSERT(ef != NULL);

    int removed = 0;
    {
        QMutexLocker locker(&m_fixturesMutex);
        removed = m_fixtures.removeAll(ef);
    }

    if (removed > 0)
    {
        emit changed(this->id());
        return true;
    }
    else
    {
        return false;
    }
}

bool EFX::removeFixture(quint32 fxi, int head)
{
    QMutexLocker locker(&m_fixturesMutex);
    for (int i = 0; i < m_fixtures.count(); i++)
    {
        EFXFixture *ef = m_fixtures.at(i);
        if (ef->head().fxi == fxi && ef->head().head == head)
        {
            m_fixtures.removeAt(i);
            return true;
        }
    }

    return false;
}

void EFX::removeAllFixtures()
{
    {
        QMutexLocker locker(&m_fixturesMutex);
        m_fixtures.clear();
    }
    emit changed(this->id());
}

bool EFX::raiseFixture(EFXFixture* ef)
{
    Q_ASSERT(ef != NULL);

    QMutexLocker locker(&m_fixturesMutex);
    int index = m_fixtures.indexOf(ef);
    if (index > 0)
    {
        m_fixtures.move(index, index - 1);
        locker.unlock();
        emit changed(this->id());
        return true;
    }
    else
    {
        return false;
    }
}

bool EFX::lowerFixture(EFXFixture* ef)
{
    QMutexLocker locker(&m_fixturesMutex);
    int index = m_fixtures.indexOf(ef);
    if (index < (m_fixtures.count() - 1))
    {
        m_fixtures.move(index, index + 1);
        locker.unlock();
        emit changed(this->id());
        return true;
    }
    else
    {
        return false;
    }
}

const QList <EFXFixture*> EFX::fixtures() const
{
    return m_fixtures;
}

EFXFixture *EFX::fixture(quint32 id, int headIndex)
{
    foreach (EFXFixture *ef, m_fixtures)
    {
        if (ef->head().fxi == id && ef->head().head == headIndex)
            return ef;
    }

    return NULL;
}

QList<quint32> EFX::components() const
{
    QList<quint32> ids;

    foreach (EFXFixture *ef, m_fixtures)
    {
        if (ids.contains(ef->head().fxi) == false)
            ids.append(ef->head().fxi);
    }

    return ids;
}

void EFX::slotFixtureRemoved(quint32 fxi_id)
{
    bool removed = false;

    {
        QMutexLocker locker(&m_fixturesMutex);

        /* Remove the destroyed fixture from our list. A fixture with
         * more than one head has an entry for each of them */
        QMutableListIterator <EFXFixture*> it(m_fixtures);
        while (it.hasNext() == true)
        {
            it.next();

            if (it.value()->head().fxi == fxi_id)
            {
                delete it.value();
                it.remove();
                removed = true;
            }
        }
    }

    if (removed)
        emit changed(this->id());
}

/*****************************************************************************
 * Fixture propagation mode
 *****************************************************************************/

void EFX::setPropagationMode(PropagationMode mode)
{
    m_propagationMode = mode;
    emit changed(this->id());
}

EFX::PropagationMode EFX::propagationMode() const
{
    return m_propagationMode;
}

QString EFX::propagationModeToString(PropagationMode mode)
{
    if (mode == Serial)
        return QString(KXMLQLCEFXPropagationModeSerial);
    else if (mode == Asymmetric)
        return QString(KXMLQLCEFXPropagationModeAsymmetric);
    else
        return QString(KXMLQLCEFXPropagationModeParallel);
}

EFX::PropagationMode EFX::stringToPropagationMode(const QString& str)
{
    if (str == QString(KXMLQLCEFXPropagationModeSerial))
        return Serial;
    else if (str == QString(KXMLQLCEFXPropagationModeAsymmetric))
        return Asymmetric;
    else
        return Parallel;
}

/*****************************************************************************
 * Load & Save
 *****************************************************************************/

bool EFX::saveXML(QXmlStreamWriter *doc) const
{
    Q_ASSERT(doc != NULL);

    /* Function tag */
    doc->writeStartElement(KXMLQLCFunction);

    /* Common attributes */
    saveXMLCommon(doc);

    /* Fixtures */
    QListIterator <EFXFixture*> it(m_fixtures);
    while (it.hasNext() == true)
        it.next()->saveXML(doc);

    /* Propagation mode */
    doc->writeTextElement(KXMLQLCEFXPropagationMode, propagationModeToString(m_propagationMode));

    /* Tempo type */
    saveXMLTempoType(doc);
    /* Speeds */
    saveXMLSpeed(doc);
    /* Direction */
    saveXMLDirection(doc);
    /* Run order */
    saveXMLRunOrder(doc);

    /* Effect: library reference, parameter overrides by id and an embedded copy */
    {
        QMutexLocker locker(&m_effectMutex);
        doc->writeStartElement(KXMLQLCEFXEffect);
        doc->writeAttribute(KXMLQLCEFXEffectId, m_effectId);
        doc->writeAttribute(KXMLQLCEFXEffectVersion, QString::number(m_effect.isNull() ? 1 : m_effect->version()));

        QJsonObject params;
        for (auto it = m_paramMap.constBegin(); it != m_paramMap.constEnd(); ++it)
            params.insert(it.key(), it.value());
        doc->writeTextElement(KXMLQLCEFXEffectParams,
                              QString::fromUtf8(QJsonDocument(params).toJson(QJsonDocument::Compact)));
        doc->writeTextElement(KXMLQLCEFXEffectEmbedded, QString::fromUtf8(m_embedded));
        doc->writeEndElement();
    }
    /* Width */
    doc->writeTextElement(KXMLQLCEFXWidth, QString::number(width()));
    /* Height */
    doc->writeTextElement(KXMLQLCEFXHeight, QString::number(height()));
    /* Rotation */
    doc->writeTextElement(KXMLQLCEFXRotation, QString::number(rotation()));
    /* StartOffset */
    doc->writeTextElement(KXMLQLCEFXStartOffset, QString::number(startOffset()));
    /* IsRelative */
    doc->writeTextElement(KXMLQLCEFXIsRelative, QString::number(isRelative() ? 1 : 0));
    /* DimmerControl */
    doc->writeTextElement(KXMLQLCEFXDimmerControl, QString::number(dimmerControlEnabled() ? 1 : 0));

    /********************************************
     * X-Axis
     ********************************************/
    doc->writeStartElement(KXMLQLCEFXAxis);
    doc->writeAttribute(KXMLQLCFunctionName, KXMLQLCEFXX);

    /* Offset */
    doc->writeTextElement(KXMLQLCEFXOffset, QString::number(xOffset()));

    /* End the (X) <Axis> tag */
    doc->writeEndElement();

    /********************************************
     * Y-Axis
     ********************************************/
    doc->writeStartElement(KXMLQLCEFXAxis);
    doc->writeAttribute(KXMLQLCFunctionName, KXMLQLCEFXY);

    /* Offset */
    doc->writeTextElement(KXMLQLCEFXOffset, QString::number(yOffset()));

    /* End the (Y) <Axis> tag */
    doc->writeEndElement();

    /* End the <Function> tag */
    doc->writeEndElement();

    return true;
}

bool EFX::loadXML(QXmlStreamReader &root)
{
    if (root.name() != KXMLQLCFunction)
    {
        qWarning() << "Function node not found!";
        return false;
    }

    if (root.attributes().value(KXMLQLCFunctionType).toString() != typeToString(Function::EFXType))
    {
        qWarning("Function is not an EFX!");
        return false;
    }

    /* Load EFX contents */
    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCBus)
        {
            /* Bus */
            QString str = root.attributes().value(KXMLQLCBusRole).toString();
            if (str == KXMLQLCBusFade)
                m_legacyFadeBus = root.readElementText().toUInt();
            else if (str == KXMLQLCBusHold)
                m_legacyHoldBus = root.readElementText().toUInt();
        }
        else if (root.name() == KXMLQLCFunctionSpeed)
        {
            loadXMLSpeed(root);
        }
        else if (root.name() == KXMLQLCFunctionTempoType)
        {
            loadXMLTempoType(root);
        }
        else if (root.name() == KXMLQLCEFXFixture)
        {
            EFXFixture* ef = new EFXFixture(this);
            ef->loadXML(root);
            if (ef->head().isValid())
            {
                if (addFixture(ef) == false)
                    delete ef;
            }
        }
        else if (root.name() == KXMLQLCEFXPropagationMode)
        {
            /* Propagation mode */
            setPropagationMode(stringToPropagationMode(root.readElementText()));
        }
        else if (root.name() == KXMLQLCEFXEffect)
        {
            loadXMLEffect(root);
        }
        else if (root.name() == QStringLiteral("Algorithm"))
        {
            /* Projects saved before the JSON effects carry an algorithm name. They are not converted. */
            qWarning() << "EFX" << name() << ": legacy algorithm" << root.readElementText()
                       << "is not supported, select an effect";
        }
        else if (root.name() == KXMLQLCFunctionDirection)
        {
            loadXMLDirection(root);
        }
        else if (root.name() == KXMLQLCFunctionRunOrder)
        {
            loadXMLRunOrder(root);
        }
        else if (root.name() == KXMLQLCEFXWidth)
        {
            /* Width */
            setWidth(root.readElementText().toInt());
        }
        else if (root.name() == KXMLQLCEFXHeight)
        {
            /* Height */
            setHeight(root.readElementText().toInt());
        }
        else if (root.name() == KXMLQLCEFXRotation)
        {
            /* Rotation */
            setRotation(root.readElementText().toInt());
        }
        else if (root.name() == KXMLQLCEFXStartOffset)
        {
            /* StartOffset */
            setStartOffset(root.readElementText().toInt());
        }
        else if (root.name() == KXMLQLCEFXIsRelative)
        {
            /* IsRelative */
            setIsRelative(root.readElementText().toInt() != 0);
        }
        else if (root.name() == KXMLQLCEFXDimmerControl)
        {
            /* DimmerControl */
            setDimmerControlEnabled(root.readElementText().toInt() != 0);
        }
        else if (root.name() == QStringLiteral("DimmerWidth") ||
                 root.name() == QStringLiteral("DimmerAttack") ||
                 root.name() == QStringLiteral("DimmerDecay") ||
                 root.name() == QStringLiteral("DimmerShape") ||
                 root.name() == QStringLiteral("DimmerOrder"))
        {
            /* LEGACY: dimmer sub-parameters that no longer exist. Skip silently. */
            root.skipCurrentElement();
        }
        else if (root.name() == KXMLQLCEFXAxis)
        {
            /* Axes */
            loadXMLAxis(root);
        }
        else
        {
            qWarning() << "Unknown EFX tag:" << root.name();
            root.skipCurrentElement();
        }
    }

    {
        QMutexLocker locker(&m_effectMutex);
        if (!m_effectId.isEmpty() || !m_embedded.isEmpty())
            resolveEffect();
    }

    return true;
}

bool EFX::loadXMLEffect(QXmlStreamReader &root)
{
    QMutexLocker locker(&m_effectMutex);

    m_effectId = root.attributes().value(KXMLQLCEFXEffectId).toString();
    m_embedded.clear();
    m_paramMap.clear();

    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCEFXEffectParams)
        {
            QJsonObject o = QJsonDocument::fromJson(root.readElementText().toUtf8()).object();
            for (auto it = o.constBegin(); it != o.constEnd(); ++it)
                m_paramMap[it.key()] = it.value().toDouble();
        }
        else if (root.name() == KXMLQLCEFXEffectEmbedded)
        {
            m_embedded = root.readElementText().toUtf8();
        }
        else
        {
            qWarning() << "Unknown EFX effect tag:" << root.name();
            root.skipCurrentElement();
        }
    }
    return true;
}

bool EFX::loadXMLAxis(QXmlStreamReader &root)
{
    int offset = 0;
    QString axis;

    if (root.name() != KXMLQLCEFXAxis)
    {
        qWarning() << "EFX axis node not found!";
        return false;
    }

    /* Get the axis name */
    axis = root.attributes().value(KXMLQLCFunctionName).toString();

    /* Load axis contents */
    while (root.readNextStartElement())
    {
        if (root.name() == KXMLQLCEFXOffset)
            offset = root.readElementText().toInt();
        else if (root.name() == QStringLiteral("Frequency") || root.name() == QStringLiteral("Phase"))
            root.skipCurrentElement(); // legacy lissajous values, now effect parameters
        else
        {
            qWarning() << "Unknown EFX axis tag:" << root.name();
            root.skipCurrentElement();
        }
    }

    if (axis == KXMLQLCEFXY)
    {
        setYOffset(offset);
        return true;
    }
    else if (axis == KXMLQLCEFXX)
    {
        setXOffset(offset);
        return true;
    }
    else
    {
        qWarning() << "Unknown EFX axis:" << axis;
        return false;
    }
}

void EFX::postLoad()
{
    // Map legacy bus speeds to fixed speed values
    if (m_legacyFadeBus != Bus::invalid())
    {
        quint32 value = Bus::instance()->value(m_legacyFadeBus);
        setFadeInSpeed((value / MasterTimer::frequency()) * 1000);
        setFadeOutSpeed((value / MasterTimer::frequency()) * 1000);
    }

    if (m_legacyHoldBus != Bus::invalid())
    {
        quint32 value = Bus::instance()->value(m_legacyHoldBus);
        setDuration((value / MasterTimer::frequency()) * 1000);
    }
}

/*****************************************************************************
 * Running
 *****************************************************************************/
QSharedPointer<GenericFader> EFX::getFader(QList<Universe *> universes, quint32 universeID)
{
    // get the universe Fader first. If doesn't exist, create it
    QSharedPointer<GenericFader> fader = m_fadersMap.value(universeID, QSharedPointer<GenericFader>());
    if (fader.isNull())
    {
        fader = universes[universeID]->requestFader(isRelative() ? Universe::Override : Universe::Auto);
        fader->adjustIntensity(getAttributeValue(Intensity));
        fader->setBlendMode(blendMode());
        fader->setName(name());
        fader->setParentFunctionID(id());
        fader->setHandleSecondary(true);
        m_fadersMap[universeID] = fader;
    }

    return fader;
}

void EFX::preRun(MasterTimer* timer)
{
    int serialNumber = 0;

    QMutexLocker locker(&m_fixturesMutex);
    QListIterator <EFXFixture*> it(m_fixtures);
    while (it.hasNext() == true)
    {
        EFXFixture *ef = it.next();
        Q_ASSERT(ef != NULL);
        ef->setSerialNumber(serialNumber++);
    }

    locker.unlock();

    Function::preRun(timer);
}

void EFX::write(MasterTimer *timer, QList<Universe*> universes)
{
    Q_UNUSED(timer);

    if (isPaused())
        return;

    int done = 0;
    int count = 0;

    {
        QMutexLocker locker(&m_fixturesMutex);
        count = m_fixtures.count();

        QListIterator <EFXFixture*> it(m_fixtures);
        while (it.hasNext() == true)
        {
            EFXFixture *ef = it.next();
            if (ef->isDone() == false)
            {
                // an entry whose fixture doesn't exist (e.g. it couldn't be
                // loaded) has no universe
                if (ef->universe() >= quint32(universes.count()))
                    continue;

                QSharedPointer<GenericFader> fader = getFader(universes, ef->universe());
                ef->nextStep(universes, fader);
            }
            else
            {
                done++;
            }
        }
    }

    incrementElapsed();

    /* Check for stop condition */
    if (done == count)
        stop(FunctionParent::master());
}

void EFX::postRun(MasterTimer *timer, QList<Universe *> universes)
{
    /* Reset all fixtures */
    {
        QMutexLocker locker(&m_fixturesMutex);
        QListIterator <EFXFixture*> it(m_fixtures);
        while (it.hasNext() == true)
        {
            EFXFixture* ef(it.next());

            /* Run the EFX's stop scene for Loop & PingPong modes */
            if (runOrder() != SingleShot)
                ef->stop();
            ef->reset();
        }
    }

    dismissAllFaders();

    Function::postRun(timer, universes);
}

/*****************************************************************************
 * Intensity
 *****************************************************************************/

int EFX::adjustAttribute(qreal fraction, int attributeId)
{
    int attrIndex = Function::adjustAttribute(fraction, attributeId);

    switch (attrIndex)
    {
        case Intensity:
        {
            foreach (QSharedPointer<GenericFader> fader, m_fadersMap)
            {
                if (!fader.isNull())
                    fader->adjustIntensity(getAttributeValue(Function::Intensity));
            }
        }
        break;

        case Height:
        case Width:
        case XOffset:
        case YOffset:
        case Rotation:
            updateRotationCache();
        break;
    }

    return attrIndex;
}

/*************************************************************************
 * Blend mode
 *************************************************************************/

void EFX::setBlendMode(Universe::BlendMode mode)
{
    if (mode == blendMode())
        return;

    foreach (QSharedPointer<GenericFader> fader, m_fadersMap)
    {
        if (!fader.isNull())
            fader->setBlendMode(mode);
    }

    Function::setBlendMode(mode);
}
