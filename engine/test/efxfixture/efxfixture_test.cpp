/*
  Q Light Controller Plus - Unit test
  efxfixture_test.cpp

  Copyright (c) Heikki Junnila
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

#include <QtTest>
#include <QList>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#define protected public
#define private public
#include "mastertimer_stub.h"
#include "efxfixture_test.h"
#include "qlcfixturemode.h"
#include "qlcfixturedef.h"
#include "genericfader.h"
#include "fadechannel.h"
#include "efxfixture.h"
#include "qlcchannel.h"
#include "universe.h"
#include "function.h"
#include "fixture.h"
#include "qlcfile.h"
#include "efx.h"
#include "doc.h"
#undef private
#undef protected

#include "../common/resource_paths.h"

void EFXFixture_Test::initTestCase()
{
    m_doc = new Doc(this);

    QDir dir(INTERNAL_FIXTUREDIR);
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList() << QString("*%1").arg(KExtFixture));
    QVERIFY(m_doc->fixtureDefCache()->loadMap(dir) == true);
}

void EFXFixture_Test::init()
{
    int address = 0;
    {
        QLCFixtureDef* def = m_doc->fixtureDefCache()->fixtureDef("Futurelight", "DJScan250");
        QVERIFY(def != NULL);
        QLCFixtureMode* mode = def->modes().first();
        QVERIFY(mode != NULL);

        Fixture* fxi = new Fixture(m_doc);
        fxi->setFixtureDefinition(def, mode);
        fxi->setAddress(address);
        m_fixture8bitAddress = address;
        address += fxi->channels();
        m_doc->addFixture(fxi);
        m_fixture8bit = fxi->id();
    }

    {
        QLCFixtureDef* def = m_doc->fixtureDefCache()->fixtureDef("Futurelight", "MH-440");
        QVERIFY(def != NULL);
        QLCFixtureMode* mode = def->modes().first();
        QVERIFY(mode != NULL);

        Fixture* fxi = new Fixture(m_doc);
        fxi->setFixtureDefinition(def, mode);
        fxi->setAddress(address);
        m_fixture16bitAddress = address;
        address += fxi->channels();
        m_doc->addFixture(fxi);
        m_fixture16bit = fxi->id();
    }

    {
        QLCFixtureDef* def = m_doc->fixtureDefCache()->fixtureDef("Futurelight", "CY-200");
        QVERIFY(def != NULL);
        QLCFixtureMode* mode = def->modes().first();
        QVERIFY(mode != NULL);

        Fixture* fxi = new Fixture(m_doc);
        fxi->setFixtureDefinition(def, mode);
        fxi->setAddress(address);
        m_fixturePanOnlyAddress = address;
        address += fxi->channels();
        m_doc->addFixture(fxi);
        m_fixturePanOnly = fxi->id();
    }

    {
        QLCFixtureDef* def = m_doc->fixtureDefCache()->fixtureDef("American DJ", "Sweeper Beam Quad LED");
        QVERIFY(def != NULL);
        QLCFixtureMode* mode = def->modes().last(); // 39 Channel mode
        QVERIFY(mode != NULL);

        Fixture* fxi = new Fixture(m_doc);
        fxi->setFixtureDefinition(def, mode);
        fxi->setAddress(address);
        m_fixtureLedBarAddress = address;
        m_doc->addFixture(fxi);
        m_fixtureLedBar = fxi->id();
    }
}

void EFXFixture_Test::cleanupTestCase()
{
    delete m_doc;
}

void EFXFixture_Test::cleanup()
{
    m_doc->clearContents();
}

void EFXFixture_Test::initial()
{
    EFX e(m_doc);

    EFXFixture ef(&e);
    QVERIFY(ef.head().fxi == Fixture::invalidId());
    QVERIFY(ef.head().head == -1);
    QVERIFY(ef.direction() == EFX::Forward);
    QVERIFY(ef.serialNumber() == 0);
    QVERIFY(ef.isValid() == false);
    QVERIFY(ef.isDone() == false);

    QVERIFY(ef.m_runTimeDirection == EFX::Forward);
    QVERIFY(ef.m_done == false);
    QVERIFY(ef.m_elapsed == 0);
}

void EFXFixture_Test::copyFrom()
{
    EFX e(m_doc);

    EFXFixture ef(&e);
    ef.m_head.fxi = 15;
    ef.m_head.head = 16;
    ef.m_direction = EFX::Backward;
    ef.m_serialNumber = 25;
    ef.m_runTimeDirection = EFX::Backward;
    ef.m_done = true;
    ef.m_elapsed = 31337;

    EFXFixture copy(&e);
    copy.copyFrom(&ef);
    QVERIFY(copy.m_head.fxi == 15);
    QVERIFY(copy.m_head.head == 16);
    QVERIFY(copy.m_direction == EFX::Backward);
    QVERIFY(copy.m_serialNumber == 25);
    QVERIFY(copy.m_runTimeDirection == EFX::Backward);
    QVERIFY(copy.m_done == true);
    QVERIFY(copy.m_elapsed == 31337);
}

void EFXFixture_Test::publicProperties()
{
    EFX e(m_doc);
    EFXFixture ef(&e);

    ef.setHead(GroupHead(19, 5));
    QVERIFY(ef.head().fxi == 19);
    QVERIFY(ef.head().head == 5);

    ef.setHead(GroupHead());
    QVERIFY(ef.head().fxi == Fixture::invalidId());

    ef.setDirection(EFX::Backward);
    QVERIFY(ef.direction() == EFX::Backward);
    QVERIFY(ef.m_runTimeDirection == EFX::Backward);

    ef.setDirection(EFX::Forward);
    QVERIFY(ef.direction() == EFX::Forward);
    QVERIFY(ef.m_runTimeDirection == EFX::Forward);
}

void EFXFixture_Test::loadSuccess()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("Fixture");

    xmlWriter.writeTextElement("ID", "83");
    xmlWriter.writeTextElement("Head", "76");
    xmlWriter.writeTextElement("Direction", "Backward");

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    EFXFixture ef(&e);
    QVERIFY(ef.loadXML(xmlReader) == true);
    QVERIFY(ef.head().fxi == 83);
    QVERIFY(ef.head().head == 76);
    QVERIFY(ef.direction() == EFX::Backward);
}

void EFXFixture_Test::loadWrongRoot()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("EFXFixture");

    xmlWriter.writeTextElement("ID", "189");
    xmlWriter.writeTextElement("Direction", "Backward");

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    EFXFixture ef(&e);
    QVERIFY(ef.loadXML(xmlReader) == false);
    QVERIFY(!ef.head().isValid());
    QVERIFY(ef.direction() == EFX::Forward);
}

void EFXFixture_Test::loadWrongDirection()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("Fixture");

    xmlWriter.writeTextElement("ID", "97");
    xmlWriter.writeTextElement("Direction", "Phorrwarrd");

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    EFXFixture ef(&e);
    QVERIFY(ef.loadXML(xmlReader) == true);
    QVERIFY(ef.head().fxi == 97);
    QVERIFY(ef.direction() == EFX::Forward);
}

void EFXFixture_Test::loadExtraTag()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("Fixture");

    xmlWriter.writeTextElement("ID", "108");
    xmlWriter.writeTextElement("Direction", "Forward");
    xmlWriter.writeTextElement("Foobar", "Just testing");

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    EFXFixture ef(&e);
    QVERIFY(ef.loadXML(xmlReader) == true);
    QVERIFY(ef.head().fxi == 108);
    QVERIFY(ef.direction() == EFX::Forward);
}

void EFXFixture_Test::save()
{
    EFX e(m_doc);
    EFXFixture ef(&e);
    ef.setHead(GroupHead(56, 7));
    ef.setDirection(EFX::Backward);

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("EFX");

    QVERIFY(ef.saveXML(&xmlWriter) == true);

    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();
    QVERIFY(xmlReader.name().toString() == "EFX");

    xmlReader.readNextStartElement();
    QVERIFY(xmlReader.name().toString() == "Fixture");

    xmlReader.readNextStartElement();
    QVERIFY(xmlReader.name().toString() == "ID");
    QVERIFY(xmlReader.readElementText() == "56");

    xmlReader.readNextStartElement();
    QVERIFY(xmlReader.name().toString() == "Head");
    QVERIFY(xmlReader.readElementText() == "7");

    xmlReader.readNextStartElement();
    QVERIFY(xmlReader.name().toString() == "Mode");
    QVERIFY(xmlReader.readElementText() == "0");

    xmlReader.readNextStartElement();
    QVERIFY(xmlReader.name().toString() == "Direction");
    QVERIFY(xmlReader.readElementText() == "Backward");
}

void EFXFixture_Test::serialNumber()
{
    EFX e(m_doc);
    EFXFixture ef(&e);

    ef.setSerialNumber(15);
    QVERIFY(ef.serialNumber() == 15);
}

void EFXFixture_Test::isValid()
{
    EFX e(m_doc);
    EFXFixture ef(&e);

    QVERIFY(ef.isValid() == false);

    ef.setHead(GroupHead(0,0));
    QVERIFY(ef.isValid() == true);
}

void EFXFixture_Test::reset()
{
    EFX e(m_doc);

    EFXFixture* ef1 = new EFXFixture(&e);
    ef1->setHead(GroupHead(1,0));
    ef1->setSerialNumber(0);
    ef1->m_runTimeDirection = EFX::Forward;
    ef1->m_done = true;
    ef1->m_elapsed = 1337;
    e.addFixture(ef1);

    EFXFixture* ef2 = new EFXFixture(&e);
    ef2->setHead(GroupHead(2,0));
    ef2->setSerialNumber(1);
    ef2->m_runTimeDirection = EFX::Forward;
    ef2->m_done = true;
    ef2->m_elapsed = 13;
    e.addFixture(ef2);

    EFXFixture* ef3 = new EFXFixture(&e);
    ef3->setHead(GroupHead(3,0));
    ef3->setSerialNumber(2);
    ef3->setDirection(EFX::Forward);
    ef3->m_runTimeDirection = EFX::Backward;
    ef3->m_done = true;
    ef3->m_elapsed = 69;
    e.addFixture(ef3);

    EFXFixture* ef4 = new EFXFixture(&e);
    ef4->setHead(GroupHead(4,0));
    ef4->setSerialNumber(3);
    ef4->setDirection(EFX::Forward);
    ef4->m_runTimeDirection = EFX::Backward;
    ef4->m_done = true;
    ef4->m_elapsed = 42;
    e.addFixture(ef4);

    ef1->reset();
    QVERIFY(ef1->m_head.fxi == 1);
    QVERIFY(ef1->m_direction == EFX::Forward);
    QVERIFY(ef1->m_serialNumber == 0);
    QVERIFY(ef1->m_runTimeDirection == EFX::Forward);
    QVERIFY(ef1->m_done == false);
    QVERIFY(ef1->m_elapsed == 0);

    ef2->reset();
    QVERIFY(ef2->m_head.fxi == 2);
    QVERIFY(ef2->m_direction == EFX::Forward);
    QVERIFY(ef2->m_serialNumber == 1);
    QVERIFY(ef2->m_runTimeDirection == EFX::Forward);
    QVERIFY(ef2->m_done == false);
    QVERIFY(ef2->m_elapsed == 0);

    ef3->reset();
    QVERIFY(ef3->m_head.fxi == 3);
    QVERIFY(ef3->m_direction == EFX::Forward);
    QVERIFY(ef3->m_serialNumber == 2);
    QVERIFY(ef3->m_runTimeDirection == EFX::Forward);
    QVERIFY(ef3->m_done == false);
    QVERIFY(ef3->m_elapsed == 0);

    ef4->reset();
    QVERIFY(ef4->m_head.fxi == 4);
    QVERIFY(ef4->m_direction == EFX::Forward);
    QVERIFY(ef4->m_serialNumber == 3);
    QVERIFY(ef4->m_runTimeDirection == EFX::Forward);
    QVERIFY(ef4->m_done == false);
    QVERIFY(ef4->m_elapsed == 0);
}

void EFXFixture_Test::startOffset()
{
    EFX e(m_doc);
    EFXFixture ef(&e);
    ef.setHead(GroupHead(0,0));

    QCOMPARE(0, ef.startOffset());
    for (int i = 0; i < 360; i += 90)
    {
        ef.setStartOffset(i);
        QCOMPARE(i, ef.startOffset());
    }
}

void EFXFixture_Test::setPoint8bit()
{
    EFX e(m_doc);
    EFXFixture ef(&e);
    ef.setHead(GroupHead(m_fixture8bit, 0));

    QList<Universe*> ua = m_doc->inputOutputMap()->universes();
    Universe *universe = ua[0];
    QSharedPointer<GenericFader> fader = universe->requestFader();

    ef.start(fader);
    ef.setPointPanTilt(ua, fader, 5.4, 1.5); // PMSB: 5, PLSB: 0.4, TMSB: 1 (102), TLSB: 0.5(127)
    QCOMPARE(fader->channels().count(), 2);
    universe->processFaders(MasterTimer::tick());

    QCOMPARE((int)universe->preGMValues()[m_fixture8bitAddress + 0], 5);
    QCOMPARE((int)universe->preGMValues()[m_fixture8bitAddress + 1], 1);
    QCOMPARE((int)universe->preGMValues()[m_fixture8bitAddress + 2], 0); /* No LSB channels */
    QCOMPARE((int)universe->preGMValues()[m_fixture8bitAddress + 3], 0); /* No LSB channels */
}

void EFXFixture_Test::setPoint16bit()
{
    EFX e(m_doc);
    EFXFixture ef(&e);
    ef.setHead(GroupHead(m_fixture16bit, 0));

    QList<Universe*> ua = m_doc->inputOutputMap()->universes();
    Universe *universe = ua[0];
    QSharedPointer<GenericFader> fader = universe->requestFader();

    ef.start(fader);
    ef.setPointPanTilt(ua, fader, 5.4, 1.5); // PMSB: 5, PLSB: 0.4, TMSB: 1 (102), TLSB: 0.5(127)
    QCOMPARE(fader->channels().count(), 4);
    universe->processFaders(MasterTimer::tick());
    QCOMPARE((int)universe->preGMValues()[m_fixture16bitAddress + 0], 5);
    QCOMPARE((int)universe->preGMValues()[m_fixture16bitAddress + 1], 1);
    QCOMPARE((int)universe->preGMValues()[m_fixture16bitAddress + 2], 102); /* 255 * 0.4 */
    QCOMPARE((int)universe->preGMValues()[m_fixture16bitAddress + 3], 127); /* 255 * 0.5 */
}

void EFXFixture_Test::setPointPanOnly()
{
    EFX e(m_doc);
    EFXFixture ef(&e);
    ef.setHead(GroupHead(m_fixturePanOnly, 0));

    QList<Universe*> ua = m_doc->inputOutputMap()->universes();
    Universe *universe = ua[0];
    QSharedPointer<GenericFader> fader = universe->requestFader();

    ef.start(fader);
    ef.setPointPanTilt(ua, fader, 5.4, 1.5); // PMSB: 5, PLSB: 0.4, TMSB: 1 (102), TLSB: 0.5(127)
    QCOMPARE(fader->channels().count(), 1);
    universe->processFaders(MasterTimer::tick());
    QCOMPARE((int)universe->preGMValues()[m_fixturePanOnlyAddress + 0], 5); /* Pan */
    QCOMPARE((int)universe->preGMValues()[m_fixturePanOnlyAddress + 1], 0);
    QCOMPARE((int)universe->preGMValues()[m_fixturePanOnlyAddress + 2], 0);
    QCOMPARE((int)universe->preGMValues()[m_fixturePanOnlyAddress + 3], 0);
}

void EFXFixture_Test::setPointLedBar()
{
    EFX e(m_doc);
    EFXFixture ef(&e);
    ef.setHead(GroupHead(m_fixtureLedBar, 0));

    QList<Universe*> ua = m_doc->inputOutputMap()->universes();
    Universe *universe = ua[0];
    QSharedPointer<GenericFader> fader = universe->requestFader();

    ef.start(fader);
    ef.setPointPanTilt(ua, fader, 5.4, 1.5); // PMSB: 5, PLSB: 0.4, TMSB: 1 (102), TLSB: 0.5(127)
    QCOMPARE(fader->channels().count(), 1);
    universe->processFaders(MasterTimer::tick());

    QCOMPARE((int)universe->preGMValues()[m_fixtureLedBarAddress + 0], 1); /* Tilt */
    QCOMPARE((int)universe->preGMValues()[m_fixtureLedBarAddress + 1], 0);
    QCOMPARE((int)universe->preGMValues()[m_fixtureLedBarAddress + 2], 0);
    QCOMPARE((int)universe->preGMValues()[m_fixtureLedBarAddress + 3], 0);
}


static EFXEffectPtr testEffect(const QString& channels)
{
    QString err;
    EFXEffectPtr fx = EFXEffect::fromJson(
        QString("{\"id\":\"zz-test\",\"channels\":[%1]}").arg(channels).toUtf8(), &err);
    if (fx.isNull())
        qWarning() << "test effect rejected:" << err;
    return fx;
}

static quint32 addTestFixture(Doc *doc, const QString& manufacturer, const QString& model,
                              const QString& modeName, quint32 address)
{
    QLCFixtureDef* def = doc->fixtureDefCache()->fixtureDef(manufacturer, model);
    if (def == NULL)
        return Fixture::invalidId();
    QLCFixtureMode* mode = def->mode(modeName);
    if (mode == NULL)
        return Fixture::invalidId();

    Fixture* fxi = new Fixture(doc);
    fxi->setFixtureDefinition(def, mode);
    fxi->setAddress(address);
    doc->addFixture(fxi);
    return fxi->id();
}

void EFXFixture_Test::effectChannelsColour()
{
    const quint32 id = addTestFixture(m_doc, "Generic", "Generic RGB", "RGB", 400);
    QVERIFY(id != Fixture::invalidId());

    QList<Universe*> ua = m_doc->inputOutputMap()->universes();
    Universe *universe = ua[0];
    QSharedPointer<GenericFader> fader = universe->requestFader();

    EFX e(m_doc);
    e.setDuration(1000);
    EFXEffectPtr fx = testEffect("{\"target\":\"red\",\"expr\":\"0.5\"},{\"target\":\"green\",\"expr\":\"0\"},"
                                 "{\"target\":\"blue\",\"expr\":\"2\"}"); // 2 is clamped to 1
    QVERIFY(!fx.isNull());
    e.setEffect(fx);

    EFXFixture* ef = new EFXFixture(&e);
    ef->setHead(GroupHead(id, 0));
    e.addFixture(ef);

    // a colour effect keeps a fixture valid that has neither pan nor tilt, even in Position mode
    ef->setMode(EFXFixture::PanTilt);
    QVERIFY(ef->isValid());
    EFX plain(m_doc);
    EFXFixture pf(&plain);
    pf.setHead(GroupHead(id, 0));
    pf.setMode(EFXFixture::PanTilt);
    QVERIFY(!pf.isValid()); // default circle: nothing to drive on an RGB lamp in Position mode

    ef->start(fader);
    ef->setPointChannels(ua, fader);
    QCOMPARE(fader->channels().count(), 3);
    universe->processFaders(MasterTimer::tick());

    QCOMPARE(uchar(universe->preGMValues()[400]), uchar(127));
    QCOMPARE(uchar(universe->preGMValues()[401]), uchar(0));
    QCOMPARE(uchar(universe->preGMValues()[402]), uchar(255));
}

void EFXFixture_Test::effectChannelsNoMovement()
{
    QList<Universe*> ua = m_doc->inputOutputMap()->universes();
    Universe *universe = ua[0];
    QSharedPointer<GenericFader> fader = universe->requestFader();

    EFX e(m_doc);
    e.setDuration(1000);
    // a raw channel offset: DJScan250 channel 2, no pan/tilt in the effect
    EFXEffectPtr fx = testEffect("{\"target\":{\"channel\":2},\"expr\":\"1\"}");
    QVERIFY(!fx.isNull());
    e.setEffect(fx);
    QVERIFY(!e.hasPanTilt());

    EFXFixture* ef = new EFXFixture(&e);
    ef->setHead(GroupHead(m_fixture8bit, 0));
    e.addFixture(ef);
    ef->setSerialNumber(0);

    // faders of earlier tests may still hold pan/tilt targets: settle them first
    universe->processFaders(MasterTimer::tick());
    const uchar panBefore = uchar(universe->preGMValues()[m_fixture8bitAddress + 0]);
    const uchar tiltBefore = uchar(universe->preGMValues()[m_fixture8bitAddress + 1]);

    ef->nextStep(ua, fader);
    // only the raw channel is written, the head is not sent to the centre
    QCOMPARE(fader->channels().count(), 1);
    universe->processFaders(MasterTimer::tick());

    QCOMPARE(uchar(universe->preGMValues()[m_fixture8bitAddress + 0]), panBefore);
    QCOMPARE(uchar(universe->preGMValues()[m_fixture8bitAddress + 1]), tiltBefore);
    QCOMPARE(uchar(universe->preGMValues()[m_fixture8bitAddress + 2]), uchar(255));
}

void EFXFixture_Test::effectChannels16bit()
{
    const quint32 id = addTestFixture(m_doc, "Acme", "Ginamp", "6 Channel", 420);
    QVERIFY(id != Fixture::invalidId());

    QList<Universe*> ua = m_doc->inputOutputMap()->universes();
    Universe *universe = ua[0];
    QSharedPointer<GenericFader> fader = universe->requestFader();

    EFX e(m_doc);
    e.setDuration(1000);
    EFXEffectPtr fx = testEffect("{\"target\":\"dimmer\",\"expr\":\"0.5\"},{\"target\":\"white\",\"expr\":\"1\"}");
    QVERIFY(!fx.isNull());
    e.setEffect(fx);

    EFXFixture* ef = new EFXFixture(&e);
    ef->setHead(GroupHead(id, 0));
    e.addFixture(ef);

    ef->start(fader);
    ef->setPointChannels(ua, fader);
    universe->processFaders(MasterTimer::tick());

    QCOMPARE(uchar(universe->preGMValues()[420 + 3]), uchar(255)); // white
    QCOMPARE(uchar(universe->preGMValues()[420 + 4]), uchar(127)); // dimmer MSB (0.5 * 255)
    QCOMPARE(uchar(universe->preGMValues()[420 + 5]), uchar(127)); // dimmer LSB
}

void EFXFixture_Test::effectChannelsMissingTargets()
{
    QList<Universe*> ua = m_doc->inputOutputMap()->universes();
    Universe *universe = ua[0];
    QSharedPointer<GenericFader> fader = universe->requestFader();

    EFX e(m_doc);
    e.setDuration(1000);
    // the fixture has none of these: gobo, zoom, raw channel beyond its channel count
    EFXEffectPtr fx = testEffect("{\"target\":\"gobo\",\"expr\":\"1\"},{\"target\":\"zoom\",\"expr\":\"1\"},"
                                 "{\"target\":{\"channel\":400},\"expr\":\"1\"},{\"target\":\"bogus\",\"expr\":\"1\"}");
    QVERIFY(!fx.isNull());
    e.setEffect(fx);

    const quint32 id = addTestFixture(m_doc, "Generic", "Generic RGB", "RGB", 440);
    QVERIFY(id != Fixture::invalidId());
    EFXFixture* ef = new EFXFixture(&e);
    ef->setHead(GroupHead(id, 0));
    e.addFixture(ef);

    ef->start(fader);
    QVERIFY(ef->m_bindings.isEmpty());
    ef->setPointChannels(ua, fader); // must not write or crash
    QCOMPARE(fader->channels().count(), 0);
}

void EFXFixture_Test::nextStepLoop()
{
    QList<Universe*> ua = m_doc->inputOutputMap()->universes();
    Universe *universe = ua[0];
    QSharedPointer<GenericFader> fader = universe->requestFader();
    MasterTimerStub mts(m_doc, ua);

    EFX e(m_doc);
    e.setDuration(1000); // 1s

    EFXFixture* ef = new EFXFixture(&e);
    ef->setHead(GroupHead(0,0));
    e.addFixture(ef);

    /* Initialize the EFXFixture so that it can do the math */
    ef->setSerialNumber(0);
    QVERIFY(ef->isValid() == true);
    QVERIFY(ef->isDone() == false);
    QVERIFY(ef->m_elapsed == 0);

    e.preRun(&mts);

    /* Run two cycles (2 * tickms * freq) to see that Loop never quits */
    uint max = (MasterTimer::tick() * MasterTimer::frequency()) + MasterTimer::tick();
    uint i = MasterTimer::tick();
    for (uint times = 0; times < 2; times++)
    {
        for (; i < max; i += MasterTimer::tick())
        {
            ef->nextStep(ua, fader);
            QVERIFY(ef->isDone() == false); // Loop is never ready
            QCOMPARE(ef->m_elapsed, i);
        }

        i = 0; // m_elapsed is zeroed after a full pass
    }

    e.postRun(&mts, ua);
}

void EFXFixture_Test::nextStepLoopZeroDuration()
{
    QList<Universe*> ua = m_doc->inputOutputMap()->universes();
    Universe *universe = ua[0];
    QSharedPointer<GenericFader> fader = universe->requestFader();
    MasterTimerStub mts(m_doc, ua);

    EFX e(m_doc);
    e.setDuration(0); // 0s

    EFXFixture* ef = new EFXFixture(&e);
    ef->setHead(GroupHead(0,0));
    e.addFixture(ef);

    /* Initialize the EFXFixture so that it can do math */
    ef->setSerialNumber(0);
    QVERIFY(ef->isValid() == true);
    QVERIFY(ef->isDone() == false);
    QVERIFY(ef->m_elapsed == 0);

    e.preRun(&mts);

    /* Run two cycles (2 * tickms * freq) to see that Loop never quits */
    uint max = (MasterTimer::tick() * MasterTimer::frequency()) + MasterTimer::tick();
    uint i = MasterTimer::tick();
    for (uint times = 0; times < 2; times++)
    {
        for (; i < max; i += MasterTimer::tick())
        {
            ef->nextStep(ua, fader);
            QVERIFY(ef->isDone() == false); // Loop is never ready
            QVERIFY(ef->m_elapsed == 0); // elapsed is never increased
        }

        // m_elapsed is NOT zeroed since there are no "rounds" when duration == 0
    }

    e.postRun(&mts, ua);
}

void EFXFixture_Test::nextStepSingleShot()
{
    QList<Universe*> ua = m_doc->inputOutputMap()->universes();
    Universe *universe = ua[0];
    QSharedPointer<GenericFader> fader = universe->requestFader();
    MasterTimerStub mts(m_doc, ua);

    EFX e(m_doc);
    e.setDuration(1000); // 1s
    e.setRunOrder(EFX::SingleShot);

    EFXFixture* ef = new EFXFixture(&e);
    ef->setHead(GroupHead(0,0));
    e.addFixture(ef);

    /* Initialize the EFXFixture so that it can do math */
    ef->setSerialNumber(0);
    QVERIFY(ef->isValid() == true);
    QVERIFY(ef->isDone() == false);
    QVERIFY(ef->m_elapsed == 0);

    e.preRun(&mts);

    ef->reset();

    /* Run one cycle (50 steps) */
    uint max = (MasterTimer::tick() * MasterTimer::frequency()) + MasterTimer::tick();
    for (uint i = MasterTimer::tick(); i < max; i += MasterTimer::tick())
    {
        ef->nextStep(ua, fader);
        QVERIFY(ef->isDone() == false);
        QCOMPARE(ef->m_elapsed, i);
    }

    ef->nextStep(ua, fader);

    /* Single-shot EFX should now be ready */
    QVERIFY(ef->isDone() == true);

    e.postRun(&mts, ua);
}

QTEST_APPLESS_MAIN(EFXFixture_Test)
