/*
  Q Light Controller Plus - Unit test
  efx_test.cpp

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
#include "qlcfixturemode.h"
#include "qlcfixturedef.h"
#include "genericfader.h"
#include "efxfixture.h"
#include "qlcchannel.h"
#include "universe.h"
#include "efx_test.h"
#include "function.h"
#include "qlcfile.h"
#include "fixture.h"
#include "scene.h"
#include "doc.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "efx.h"
#include "efxlibrary.h"
#include "bus.h"
#undef private
#undef protected

#include "../common/resource_paths.h"

void EFX_Test::initTestCase()
{
    m_doc = new Doc(this);

    QDir dir(INTERNAL_FIXTUREDIR);
    dir.setFilter(QDir::Files);
    dir.setNameFilters(QStringList() << QString("*%1").arg(KExtFixture));
    QVERIFY(m_doc->fixtureDefCache()->loadMap(dir));

    EFXLibrary::instance()->setUserDirectory(QDir(QDir::tempPath() + "/efx_test_user"));
    QVERIFY(EFXLibrary::instance()->load(QDir(EFX_PRESET_DIR), false) > 0);
}

void EFX_Test::cleanupTestCase()
{
    delete m_doc;
}

void EFX_Test::init()
{
}

void EFX_Test::cleanup()
{
    m_doc->clearContents();
}

void EFX_Test::initial()
{
    EFX e(m_doc);
    QCOMPARE(e.type(), Function::EFXType);
    QCOMPARE(e.name(), QString("New EFX"));
    QCOMPARE(e.id(), Function::invalidId());

    QCOMPARE(e.effectId(), QString("circle"));
    QVERIFY(e.hasEffect());
    QCOMPARE(e.width(), 127);
    QCOMPARE(e.height(), 127);
    QCOMPARE(e.rotation(), 0);
    QCOMPARE(e.xOffset(), 127);
    QCOMPARE(e.yOffset(), 127);


    QCOMPARE(e.fixtures().size(), 0);
    QCOMPARE(e.propagationMode(), EFX::Parallel);

    QCOMPARE(e.m_fadersMap.count(), 0);
    QCOMPARE(e.m_legacyFadeBus, Bus::invalid());
    QCOMPARE(e.m_legacyHoldBus, Bus::invalid());
}

void EFX_Test::loopDurationWithLongFadeIn()
{
    EFX e(m_doc);
    e.setDuration(2000);

    e.setFadeInSpeed(500);
    QCOMPARE(e.loopDuration(), uint(1500));

    // a fade in not shorter than the duration must not wrap around
    e.setFadeInSpeed(3000);
    QCOMPARE(e.loopDuration(), uint(2000));
    e.setFadeInSpeed(2000);
    QCOMPARE(e.loopDuration(), uint(2000));
}

void EFX_Test::width()
{
    EFX e(m_doc);

    e.setWidth(300);
    QCOMPARE(e.width(), 127);

    e.setWidth(0);
    QCOMPARE(e.width(), 0);

    e.setWidth(128);
    QCOMPARE(e.width(), 127);

    e.setWidth(52);
    QCOMPARE(e.width(), 52);

    e.setWidth(-4);
    QCOMPARE(e.width(), 0);
}

void EFX_Test::height()
{
    EFX e(m_doc);

    e.setHeight(300);
    QCOMPARE(e.height(), 127);

    e.setHeight(0);
    QCOMPARE(e.height(), 0);

    e.setHeight(128);
    QCOMPARE(e.height(), 127);

    e.setHeight(12);
    QCOMPARE(e.height(), 12);

    e.setHeight(-4);
    QCOMPARE(e.height(), 0);
}

void EFX_Test::rotation()
{
    EFX e(m_doc);
    QCOMPARE(e.m_cosR, cos(M_PI/180 * 0));
    QCOMPARE(e.m_sinR, sin(M_PI/180 * 0));

    e.setRotation(400);
    QCOMPARE(e.rotation(), 359);
    QCOMPARE(e.m_cosR, cos(M_PI/180 * 359));
    QCOMPARE(e.m_sinR, sin(M_PI/180 * 359));

    e.setRotation(360);
    QCOMPARE(e.rotation(), 359);
    QCOMPARE(e.m_cosR, cos(M_PI/180 * 359));
    QCOMPARE(e.m_sinR, sin(M_PI/180 * 359));

    e.setRotation(0);
    QCOMPARE(e.rotation(), 0);
    QCOMPARE(e.m_cosR, cos(M_PI/180 * 0));
    QCOMPARE(e.m_sinR, sin(M_PI/180 * 0));

    e.setRotation(12);
    QCOMPARE(e.rotation(), 12);
    QCOMPARE(e.m_cosR, cos(M_PI/180 * 12));
    QCOMPARE(e.m_sinR, sin(M_PI/180 * 12));

    e.setRotation(-4);
    QCOMPARE(e.rotation(), 0);
    QCOMPARE(e.m_cosR, cos(M_PI/180 * 0));
    QCOMPARE(e.m_sinR, sin(M_PI/180 * 0));
}

void EFX_Test::xOffset()
{
    EFX e(m_doc);

    e.setXOffset(300);
    QCOMPARE(e.xOffset(), UCHAR_MAX);

    e.setXOffset(0);
    QCOMPARE(e.xOffset(), 0);

    e.setXOffset(256);
    QCOMPARE(e.xOffset(), UCHAR_MAX);

    e.setXOffset(12);
    QCOMPARE(e.xOffset(), 12);

    e.setXOffset(-4);
    QCOMPARE(e.xOffset(), 0);
}

void EFX_Test::yOffset()
{
    EFX e(m_doc);

    e.setYOffset(300);
    QCOMPARE(e.yOffset(), UCHAR_MAX);

    e.setYOffset(0);
    QCOMPARE(e.yOffset(), 0);

    e.setYOffset(256);
    QCOMPARE(e.yOffset(), UCHAR_MAX);

    e.setYOffset(12);
    QCOMPARE(e.yOffset(), 12);

    e.setYOffset(-4);
    QCOMPARE(e.yOffset(), 0);
}

void EFX_Test::fixtures()
{
    EFX* e = new EFX(m_doc);
    QCOMPARE(e->fixtures().size(), 0);

    /* Add first fixture */
    EFXFixture* ef1 = new EFXFixture(e);
    ef1->setHead(GroupHead(12, 0));
    QVERIFY(e->addFixture(ef1));
    QCOMPARE(e->fixtures().size(), 1);

    /* Add second fixture */
    EFXFixture* ef2 = new EFXFixture(e);
    ef2->setHead(GroupHead(34, 0));
    QVERIFY(e->addFixture(ef2));
    QCOMPARE(e->fixtures().size(), 2);

    /* Must not be able to add the same fixture twice */
    //QVERIFY(!e->addFixture(ef1));
    //QVERIFY(!e->addFixture(ef2));
    //QCOMPARE(e->fixtures().size(), 2);

    /* Try to remove a non-member fixture */
    EFXFixture* ef3 = new EFXFixture(e);
    ef3->setHead(GroupHead(56, 0));
    QVERIFY(!e->removeFixture(ef3));
    QCOMPARE(e->fixtures().size(), 2);

    /* Add third fixture */
    e->addFixture(ef3);
    QCOMPARE(e->fixtures().size(), 3);
    QCOMPARE(e->fixtures().at(0), ef1);
    QCOMPARE(e->fixtures().at(1), ef2);
    QCOMPARE(e->fixtures().at(2), ef3);
    QCOMPARE(e->fixture(12, 0), ef1);
    QCOMPARE(e->fixture(34, 0), ef2);
    QCOMPARE(e->fixture(56, 0), ef3);
    /* Test a non existent fixture */
    QVERIFY(e->fixture(11, 22) == NULL);

    /* Add fourth fixture */
    EFXFixture* ef4 = new EFXFixture(e);
    ef4->setHead(GroupHead(78, 0));
    e->addFixture(ef4);
    QCOMPARE(e->fixtures().size(), 4);
    QCOMPARE(e->fixtures().at(0), ef1);
    QCOMPARE(e->fixtures().at(1), ef2);
    QCOMPARE(e->fixtures().at(2), ef3);
    QCOMPARE(e->fixtures().at(3), ef4);

    QVERIFY(e->removeFixture(ef4));
    QCOMPARE(e->fixtures().at(0), ef1);
    QCOMPARE(e->fixtures().at(1), ef2);
    QCOMPARE(e->fixtures().at(2), ef3);
    delete ef4;

    /* Raising the first and lowering the last must not succeed */
    QVERIFY(!e->raiseFixture(ef1));
    QVERIFY(!e->lowerFixture(ef3));
    QCOMPARE(e->fixtures().at(0), ef1);
    QCOMPARE(e->fixtures().at(1), ef2);
    QCOMPARE(e->fixtures().at(2), ef3);

    QVERIFY(e->raiseFixture(ef2));
    QCOMPARE(e->fixtures().at(0), ef2);
    QCOMPARE(e->fixtures().at(1), ef1);
    QCOMPARE(e->fixtures().at(2), ef3);
    QVERIFY(!e->raiseFixture(ef2));

    QVERIFY(e->lowerFixture(ef1));
    QCOMPARE(e->fixtures().at(0), ef2);
    QCOMPARE(e->fixtures().at(1), ef3);
    QCOMPARE(e->fixtures().at(2), ef1);
    QVERIFY(!e->lowerFixture(ef1));

    /* Uninteresting fixture is removed */
    e->slotFixtureRemoved(99);
    QCOMPARE(e->fixtures().size(), 3);
    QCOMPARE(e->fixtures().at(0), ef2);
    QCOMPARE(e->fixtures().at(1), ef3);
    QCOMPARE(e->fixtures().at(2), ef1);

    /* Member fixture is removed */
    e->slotFixtureRemoved(34);
    QCOMPARE(e->fixtures().size(), 2);
    QCOMPARE(e->fixtures().at(0), ef3);
    QCOMPARE(e->fixtures().at(1), ef1);

    /* Non-Member fixture is removed */
    e->slotFixtureRemoved(34);
    QCOMPARE(e->fixtures().size(), 2);
    QCOMPARE(e->fixtures().at(0), ef3);
    QCOMPARE(e->fixtures().at(1), ef1);

    /* Member fixture is removed */
    e->slotFixtureRemoved(12);
    QCOMPARE(e->fixtures().size(), 1);
    QCOMPARE(e->fixtures().at(0), ef3);

    /* Member fixture is removed */
    e->slotFixtureRemoved(56);
    QCOMPARE(e->fixtures().size(), 0);

    EFXFixture* ef5 = new EFXFixture(e);
    ef5->setHead(GroupHead(18, 0));
    QVERIFY(e->addFixture(ef5));

    /* Remove by fixture ID and head number */
    QVERIFY(e->removeFixture(18, 0));
    QCOMPARE(e->fixtures().size(), 0);

    QVERIFY(!e->removeFixture(98, 99));

    delete e;
}

void EFX_Test::propagationMode()
{
    EFX e(m_doc);
    QCOMPARE(e.propagationMode(), EFX::Parallel);

    e.setPropagationMode(EFX::Serial);
    QCOMPARE(e.propagationMode(), EFX::Serial);

    e.setPropagationMode(EFX::Asymmetric);
    QCOMPARE(e.propagationMode(), EFX::Asymmetric);

    QCOMPARE(EFX::propagationModeToString(EFX::Serial), QString("Serial"));
    QCOMPARE(EFX::propagationModeToString(EFX::Parallel), QString("Parallel"));
    QCOMPARE(EFX::propagationModeToString(EFX::Asymmetric), QString("Asymmetric"));
    QCOMPARE(EFX::propagationModeToString(EFX::PropagationMode(7)), QString("Parallel"));

    QCOMPARE(EFX::stringToPropagationMode("Serial"), EFX::Serial);
    QCOMPARE(EFX::stringToPropagationMode("Parallel"), EFX::Parallel);
    QCOMPARE(EFX::stringToPropagationMode("Asymmetric"), EFX::Asymmetric);
    QCOMPARE(EFX::stringToPropagationMode("Foobar"), EFX::Parallel);
}

void EFX_Test::dimmerEnvelope()
{
    EFX e(m_doc);

    /* Default: dimmer control off */
    QCOMPARE(e.dimmerControlEnabled(), false);

    e.setDimmerControlEnabled(true);
    QCOMPARE(e.dimmerControlEnabled(), true);

    /* The dimmer is driven from the EFX cycle angle, phased by the fixture's
       StartOffset. Both are in radians. The first half [0, PI) fades from full
       (at 0) to 0 (at PI); the second half [PI, 2PI) is black. */
    const float pi = float(M_PI);

    QCOMPARE(e.dimmerLevel(0.0f, 0.0f), 1.0f);         // 0     -> full
    QCOMPARE(e.dimmerLevel(0.0f, pi * 0.5f), 0.5f);    // PI/2  -> half
    QVERIFY(e.dimmerLevel(0.0f, pi * 0.99f) < 0.02f);  // ~PI   -> near 0
    QCOMPARE(e.dimmerLevel(0.0f, pi), 0.0f);           // PI    -> 0 (start of black half)
    QCOMPARE(e.dimmerLevel(0.0f, pi * 1.5f), 0.0f);    // 3PI/2 -> black
    QCOMPARE(e.dimmerLevel(0.0f, pi * 1.99f), 0.0f);   // ~2PI  -> black

    /* StartOffset (radians) shifts the phase: a PI offset moves the bright point
       (0) to where PI was (black), and vice-versa. */
    QCOMPARE(e.dimmerLevel(pi, 0.0f), 0.0f);           // 0 + PI -> black
    QCOMPARE(e.dimmerLevel(pi, pi), 1.0f);             // PI + PI = 2PI -> wraps to 0 -> full
}

void EFX_Test::widthHeightOffset()
{
    EFX e(m_doc);
    int i = 0;
    int max = 0;

    QPolygonF poly;
    e.preview(poly);
    QCOMPARE(poly.size(), 512);

    /* Check that width affects the pattern */
    e.setWidth(50);
    poly.clear();
    e.preview(poly);
    QVERIFY(poly.size() == 512);

    /* Width of 50 means actually 50px left of center (127-50) and
       50px right of center (127+50). +1 because the bound coordinates
       are OUTSIDE the actual points. */
    QVERIFY(poly.boundingRect().width() == 50 + 50);
    QVERIFY(poly.boundingRect().height() == 127 + 127);

    /* Check that height affects the pattern */
    e.setHeight(87);
    poly.clear();
    e.preview(poly);
    QVERIFY(poly.size() == 512);

    /* Height of 87 means actually 87px down of center (127-87) and
       87px up of center (127+87). And +1 because the bound coordinates
       are OUTSIDE the actual points. */
    QVERIFY(poly.boundingRect().height() == 87 + 87);
    QVERIFY(poly.boundingRect().width() == 100);

    /* X Offset is at center */
    max = 0;
    poly.clear();
    e.preview(poly);
    QVERIFY(poly.size() == 512);
    for (i = 0; i < 512; i++)
        if (poly[i].x() > max)
            max = poly[i].x();
    QVERIFY(max == 177);

    /* X offset + 20 */
    max = 0;
    e.setXOffset(127 + 20);
    poly.clear();
    e.preview(poly);
    QVERIFY(poly.size() == 512);
    for (i = 0; i < 512; i++)
        if (poly[i].x() > max)
            max = poly[i].x();
    QVERIFY(max == 197);

    /* Y Offset is at center */
    max = 0;
    poly.clear();
    e.preview(poly);
    QVERIFY(poly.size() == 512);
    for (i = 0; i < 512; i++)
        if (poly[i].y() > max)
            max = poly[i].y();
    QVERIFY(max == 214);

    /* Y offset - 25 */
    max = 0;
    e.setYOffset(127 - 25);
    poly.clear();
    e.preview(poly);
    QVERIFY(poly.size() == 512);
    for (i = 0; i < 512; i++)
        if (poly[i].y() > max)
            max = poly[i].y();
    QVERIFY(max == 189);
}

void EFX_Test::rotateAndScale()
{
    EFX efx(m_doc);

    /* The X & Y params used here represent the first point in a circle
       algorithm as calculated by EFX::circlePoint(), based on iterator
       value 0, before calling rotateAndScale(). */
    float x, y;

    /* Rotation */
    x = -0.125333;
    y = 0.992115;
    efx.setWidth(127);
    efx.setHeight(127);
    efx.setXOffset(127);
    efx.setYOffset(127);
    efx.setRotation(0);
    efx.rotateAndScale(&x, &y);
    QVERIFY(floor(x + 0.5) == 111);
    QVERIFY(floor(y + 0.5) == 253);

    x = -0.125333;
    y = 0.992115;
    efx.setWidth(127);
    efx.setHeight(127);
    efx.setXOffset(127);
    efx.setYOffset(127);
    efx.setRotation(90);
    efx.rotateAndScale(&x, &y);
    QVERIFY(floor(x + 0.5) == 253);
    QVERIFY(floor(y + 0.5) == 143);

    x = -0.125333;
    y = 0.992115;
    efx.setWidth(127);
    efx.setHeight(127);
    efx.setXOffset(127);
    efx.setYOffset(127);
    efx.setRotation(180);
    efx.rotateAndScale(&x, &y);
    QVERIFY(floor(x + 0.5) == 143);
    QVERIFY(floor(y + 0.5) == 1);

    x = -0.125333;
    y = 0.992115;
    efx.setWidth(127);
    efx.setHeight(127);
    efx.setXOffset(127);
    efx.setYOffset(127);
    efx.setRotation(270);
    efx.rotateAndScale(&x, &y);
    QVERIFY(floor(x + 0.5) == 1);
    QVERIFY(floor(y + 0.5) == 111);

    x = -0.125333;
    y = 0.992115;
    efx.setWidth(127);
    efx.setHeight(127);
    efx.setXOffset(127);
    efx.setYOffset(127);
    efx.setRotation(45);
    efx.rotateAndScale(&x, &y);
    QVERIFY(floor(x + 0.5) == 205);
    QVERIFY(floor(y + 0.5) == 227);

    /* Offset */
    x = -0.125333;
    y = 0.992115;
    efx.setWidth(127);
    efx.setHeight(127);
    efx.setXOffset(128);
    efx.setYOffset(128);
    efx.setRotation(0);
    efx.rotateAndScale(&x, &y);
    QVERIFY(floor(x + 0.5) == 112);
    QVERIFY(floor(y + 0.5) == 254);

    x = -0.125333;
    y = 0.992115;
    efx.setWidth(127);
    efx.setHeight(127);
    efx.setXOffset(0);
    efx.setYOffset(0);
    efx.setRotation(0);
    efx.rotateAndScale(&x, &y);
    QVERIFY(floor(x + 0.5) == -16);
    QVERIFY(floor(y + 0.5) == 126);

    x = -0.125333;
    y = 0.992115;
    efx.setWidth(127);
    efx.setHeight(127);
    efx.setXOffset(0);
    efx.setYOffset(127);
    efx.setRotation(0);
    efx.rotateAndScale(&x, &y);
    QVERIFY(floor(x + 0.5) == -16);
    QVERIFY(floor(y + 0.5) == 253);

    x = -0.125333;
    y = 0.992115;
    efx.setWidth(127);
    efx.setHeight(127);
    efx.setXOffset(127);
    efx.setYOffset(0);
    efx.setRotation(0);
    efx.rotateAndScale(&x, &y);
    QVERIFY(floor(x + 0.5) == 111);
    QVERIFY(floor(y + 0.5) == 126);

    x = -0.125333;
    y = 0.992115;
    efx.setWidth(127);
    efx.setHeight(127);
    efx.setXOffset(1);
    efx.setYOffset(UCHAR_MAX);
    efx.setRotation(0);
    efx.rotateAndScale(&x, &y);
    QVERIFY(floor(x + 0.5) == -15);
    QVERIFY(floor(y + 0.5) == 381);

    /* Width & height (rotate also by 90 degrees to get more tangible
       results, because x&y point to the bottom-center, where width & height
       params have very little effect without offset or rotation). */
    x = -0.125333;
    y = 0.992115;
    efx.setWidth(64);
    efx.setHeight(64);
    efx.setXOffset(127);
    efx.setYOffset(127);
    efx.setRotation(90);
    efx.rotateAndScale(&x, &y);
    QCOMPARE(floor(x + 0.5), double(190));
    QCOMPARE(floor(y + 0.5), double(135));

    x = -0.125333;
    y = 0.992115;
    efx.setWidth(10);
    efx.setHeight(127);
    efx.setXOffset(127);
    efx.setYOffset(127);
    efx.setRotation(90);
    efx.rotateAndScale(&x, &y);
    QCOMPARE(floor(x + 0.5), double(253));
    QCOMPARE(floor(y + 0.5), double(128));

    x = -0.125333;
    y = 0.992115;
    efx.setWidth(64);
    efx.setHeight(127);
    efx.setXOffset(127);
    efx.setYOffset(127);
    efx.setRotation(90);
    efx.rotateAndScale(&x, &y);
    QCOMPARE(floor(x + 0.5), double(253));
    QCOMPARE(floor(y + 0.5), double(135));

    x = -0.125333;
    y = 0.992115;
    efx.setWidth(64);
    efx.setHeight(0);
    efx.setXOffset(127);
    efx.setYOffset(127);
    efx.setRotation(90);
    efx.rotateAndScale(&x, &y);
    QCOMPARE(floor(x + 0.5), double(127));
    QCOMPARE(floor(y + 0.5), double(135));

    x = -0.125333;
    y = 0.992115;
    efx.setWidth(127);
    efx.setHeight(0);
    efx.setXOffset(127);
    efx.setYOffset(127);
    efx.setRotation(90);
    efx.rotateAndScale(&x, &y);
    QCOMPARE(floor(x + 0.5), double(127));
    QCOMPARE(floor(y + 0.5), double(143));
}

void EFX_Test::copyFrom()
{
    EFX e1(m_doc);
    e1.setName("First");
    e1.setDirection(EFX::Backward);
    e1.setRunOrder(EFX::SingleShot);
    e1.setEffect("lissajous");
    e1.setWidth(13);
    e1.setHeight(42);
    e1.setRotation(78);
    e1.setXOffset(34);
    e1.setYOffset(27);
    e1.setEffectParam("xfreq", 5);
    e1.setEffectParam("yfreq", 4);
    e1.setEffectParam("xphase", 163);
    e1.setEffectParam("yphase", 94);
    e1.setPropagationMode(EFX::Serial);
    e1.setFadeInSpeed(42);
    e1.setFadeOutSpeed(69);
    e1.setDuration(1337);

    EFXFixture* ef1 = new EFXFixture(&e1);
    ef1->setHead(GroupHead(12, 3));
    e1.addFixture(ef1);
    EFXFixture* ef2 = new EFXFixture(&e1);
    ef2->setHead(GroupHead(34, 7));
    e1.addFixture(ef2);

    /* Verify that EFX contents are copied */
    EFX e2(m_doc);
    QSignalSpy spy(&e2, SIGNAL(changed(quint32)));
    QVERIFY(e2.copyFrom(&e1) == true);
    QCOMPARE(spy.size(), 1);
    QVERIFY(e2.name() == e1.name());
    QVERIFY(e2.direction() == EFX::Backward);
    QVERIFY(e2.runOrder() == EFX::SingleShot);
    QVERIFY(e2.effectId() == QString("lissajous"));
    QVERIFY(e2.width() == 13);
    QVERIFY(e2.height() == 42);
    QVERIFY(e2.rotation() == 78);
    QVERIFY(e2.xOffset() == 34);
    QVERIFY(e2.yOffset() == 27);
    QVERIFY(e2.effectParam("xfreq") == 5);
    QVERIFY(e2.effectParam("yfreq") == 4);
    QVERIFY(e2.effectParam("xphase") == 163);
    QVERIFY(e2.effectParam("yphase") == 94);
    QVERIFY(e2.propagationMode() == EFX::Serial);
    QCOMPARE(e2.fadeInSpeed(), uint(42));
    QCOMPARE(e2.fadeOutSpeed(), uint(69));
    QCOMPARE(e2.duration(), uint(1337));
    QVERIFY(e2.fixtures().size() == 2);
    QVERIFY(e2.fixtures().at(0)->head().fxi == 12);
    QVERIFY(e2.fixtures().at(0)->head().head == 3);
    QVERIFY(e2.fixtures().at(1)->head().fxi == 34);
    QVERIFY(e2.fixtures().at(1)->head().head == 7);
    QVERIFY(e2.fixtures().at(0) != ef1);
    QVERIFY(e2.fixtures().at(1) != ef2);

    /* Verify that an EFX gets a copy only from another EFX */
    Scene s(m_doc);
    QVERIFY(e2.copyFrom(&s) == false);

    /* Make a third EFX */
    EFX e3(m_doc);
    e3.setName("Third");
    e3.setDirection(EFX::Forward);
    e3.setRunOrder(EFX::Loop);
    e3.setEffect("lissajous");
    e3.setWidth(31);
    e3.setHeight(24);
    e3.setRotation(87);
    e3.setXOffset(43);
    e3.setYOffset(72);
    e3.setEffectParam("xfreq", 3);
    e3.setEffectParam("yfreq", 2);
    e3.setEffectParam("xphase", 136);
    e3.setEffectParam("yphase", 49);
    e3.setPropagationMode(EFX::Parallel);
    e3.setFadeInSpeed(69);
    e3.setFadeOutSpeed(1337);
    e3.setDuration(42);
    EFXFixture* ef3 = new EFXFixture(&e3);
    ef3->setHead(GroupHead(56, 8));
    e3.addFixture(ef3);
    EFXFixture* ef4 = new EFXFixture(&e3);
    ef4->setHead(GroupHead(78, 9));
    e3.addFixture(ef4);

    /* Verify that copying to the same EFX a second time succeeds */
    QVERIFY(e2.copyFrom(&e3) == true);
    QVERIFY(e2.name() == e3.name());
    QVERIFY(e2.direction() == EFX::Forward);
    QVERIFY(e2.runOrder() == EFX::Loop);
    QVERIFY(e2.effectId() == QString("lissajous"));
    QVERIFY(e2.width() == 31);
    QVERIFY(e2.height() == 24);
    QVERIFY(e2.rotation() == 87);
    QVERIFY(e2.xOffset() == 43);
    QVERIFY(e2.yOffset() == 72);
    QVERIFY(e2.effectParam("xfreq") == 3);
    QVERIFY(e2.effectParam("yfreq") == 2);
    QVERIFY(e2.effectParam("xphase") == 136);
    QVERIFY(e2.effectParam("yphase") == 49);
    QVERIFY(e2.propagationMode() == EFX::Parallel);
    QCOMPARE(e2.fadeInSpeed(), uint(69));
    QCOMPARE(e2.fadeOutSpeed(), uint(1337));
    QCOMPARE(e2.duration(), uint(42));
    QVERIFY(e2.fixtures().size() == 2);
    QVERIFY(e2.fixtures().at(0)->head().fxi == 56);
    QVERIFY(e2.fixtures().at(0)->head().head == 8);
    QVERIFY(e2.fixtures().at(1)->head().fxi == 78);
    QVERIFY(e2.fixtures().at(1)->head().head == 9);
    QVERIFY(e2.fixtures().at(0) != ef1);
    QVERIFY(e2.fixtures().at(0) != ef3);
    QVERIFY(e2.fixtures().at(1) != ef2);
    QVERIFY(e2.fixtures().at(1) != ef4);
}

void EFX_Test::createCopy()
{
    Doc doc(this);

    EFX* e1 = new EFX(m_doc);
    e1->setName("First");
    e1->setDirection(EFX::Forward);
    e1->setRunOrder(EFX::PingPong);
    e1->setEffect("lissajous");
    e1->setWidth(13);
    e1->setHeight(42);
    e1->setRotation(78);
    e1->setXOffset(34);
    e1->setYOffset(27);
    e1->setEffectParam("xfreq", 5);
    e1->setEffectParam("yfreq", 4);
    e1->setEffectParam("xphase", 163);
    e1->setEffectParam("yphase", 94);
    e1->setPropagationMode(EFX::Serial);
    e1->setFadeInSpeed(42);
    e1->setFadeOutSpeed(69);
    e1->setDuration(1337);
    EFXFixture* ef1 = new EFXFixture(e1);
    ef1->setHead(GroupHead(12, 3));
    e1->addFixture(ef1);
    EFXFixture* ef2 = new EFXFixture(e1);
    ef2->setHead(GroupHead(34, 7));
    e1->addFixture(ef2);

    doc.addFunction(e1);
    QVERIFY(e1->id() != Function::invalidId());

    Function* f = e1->createCopy(&doc);
    QVERIFY(f != NULL);
    QVERIFY(f != e1);
    QVERIFY(f->id() != e1->id());

    EFX* copy = qobject_cast<EFX*> (f);
    QVERIFY(copy != NULL);
    QVERIFY(copy->name() == e1->name());
    QVERIFY(copy->direction() == EFX::Forward);
    QVERIFY(copy->runOrder() == EFX::PingPong);
    QVERIFY(copy->effectId() == QString("lissajous"));
    QVERIFY(copy->width() == 13);
    QVERIFY(copy->height() == 42);
    QVERIFY(copy->rotation() == 78);
    QVERIFY(copy->xOffset() == 34);
    QVERIFY(copy->yOffset() == 27);
    QVERIFY(copy->effectParam("xfreq") == 5);
    QVERIFY(copy->effectParam("yfreq") == 4);
    QVERIFY(copy->effectParam("xphase") == 163);
    QVERIFY(copy->effectParam("yphase") == 94);
    QVERIFY(copy->propagationMode() == EFX::Serial);
    QCOMPARE(copy->fadeInSpeed(), uint(42));
    QCOMPARE(copy->fadeOutSpeed(), uint(69));
    QCOMPARE(copy->duration(), uint(1337));
    QVERIFY(copy->fixtures().size() == 2);
    QVERIFY(copy->fixtures().at(0)->head().fxi == 12);
    QVERIFY(copy->fixtures().at(0)->head().head == 3);
    QVERIFY(copy->fixtures().at(1)->head().fxi == 34);
    QVERIFY(copy->fixtures().at(1)->head().head == 7);
    QVERIFY(copy->fixtures().at(0) != ef1);
    QVERIFY(copy->fixtures().at(1) != ef2);
}

void EFX_Test::loadXAxis()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("Axis");
    xmlWriter.writeAttribute("Name", "X");

    xmlWriter.writeTextElement("Offset", "1");
    xmlWriter.writeTextElement("Frequency", "2");
    xmlWriter.writeTextElement("Phase", "3");
    // Unknown tag
    xmlWriter.writeTextElement("Foo", "Bar");

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    QVERIFY(e.loadXMLAxis(xmlReader) == true);
}

void EFX_Test::loadYAxis()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("Axis");
    xmlWriter.writeAttribute("Name", "Y");

    xmlWriter.writeTextElement("Offset", "1");
    xmlWriter.writeTextElement("Frequency", "2");
    xmlWriter.writeTextElement("Phase", "3");

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    QVERIFY(e.loadXMLAxis(xmlReader) == true);
}

void EFX_Test::loadYAxisWrongRoot()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("sixA");
    xmlWriter.writeAttribute("Name", "Y");

    xmlWriter.writeTextElement("Offset", "1");
    xmlWriter.writeTextElement("Frequency", "2");
    xmlWriter.writeTextElement("Phase", "3");

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    QVERIFY(e.loadXMLAxis(xmlReader) == false);
}

void EFX_Test::loadAxisNoXY()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("Axis");
    xmlWriter.writeAttribute("Name", "Not X nor Y");

    xmlWriter.writeTextElement("Offset", "1");
    xmlWriter.writeTextElement("Frequency", "5");
    xmlWriter.writeTextElement("Phase", "333");

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    QVERIFY(e.loadXMLAxis(xmlReader) == false);
    QVERIFY(e.xOffset() != 1);
    QVERIFY(e.effectParam("xfreq") != 5);
    QVERIFY(e.effectParam("xphase") != 333);
}

void EFX_Test::loadSuccessLegacy()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("Function");
    xmlWriter.writeAttribute("Type", "EFX");
    xmlWriter.writeAttribute("Name", "Test EFX");

    xmlWriter.writeTextElement("PropagationMode", "Serial");

    xmlWriter.writeStartElement("Bus");
    xmlWriter.writeAttribute("Role", "Fade");
    xmlWriter.writeCharacters("12");
    xmlWriter.writeEndElement();

    xmlWriter.writeStartElement("Bus");
    xmlWriter.writeAttribute("Role", "Hold");
    xmlWriter.writeCharacters("13");
    xmlWriter.writeEndElement();

    xmlWriter.writeTextElement("Direction", "Forward");
    xmlWriter.writeTextElement("RunOrder", "Loop");
    xmlWriter.writeTextElement("Algorithm", "Diamond");
    xmlWriter.writeTextElement("Width", "100");
    xmlWriter.writeTextElement("Height", "90");
    xmlWriter.writeTextElement("Rotation", "310");

    /* X Axis */
    xmlWriter.writeStartElement("Axis");
    xmlWriter.writeAttribute("Name", "X");
    xmlWriter.writeTextElement("Offset", "10");
    xmlWriter.writeTextElement("Frequency", "2");
    xmlWriter.writeTextElement("Phase", "270");
    xmlWriter.writeEndElement();

    /* Y Axis */
    xmlWriter.writeStartElement("Axis");
    xmlWriter.writeAttribute("Name", "Y");
    xmlWriter.writeTextElement("Offset", "20");
    xmlWriter.writeTextElement("Frequency", "3");
    xmlWriter.writeTextElement("Phase", "80");
    xmlWriter.writeEndElement();

    /* Fixture 1 */
    xmlWriter.writeStartElement("Fixture");
    xmlWriter.writeTextElement("ID", "33");
    xmlWriter.writeTextElement("Direction", "Forward");
    xmlWriter.writeEndElement();

    /* Fixture 2 */
    xmlWriter.writeStartElement("Fixture");
    xmlWriter.writeTextElement("ID", "11");
    xmlWriter.writeTextElement("Direction", "Backward");
    xmlWriter.writeEndElement();

    /* Fixture 3 */
    xmlWriter.writeStartElement("Fixture");
    xmlWriter.writeTextElement("ID", "45");
    xmlWriter.writeTextElement("Direction", "Backward");
    xmlWriter.writeEndElement();

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    QVERIFY(e.loadXML(xmlReader) == true);

    QVERIFY(e.m_legacyFadeBus == 12);
    QVERIFY(e.m_legacyHoldBus == 13);
    QVERIFY(e.direction() == EFX::Forward);
    QVERIFY(e.runOrder() == EFX::Loop);

    // Legacy algorithms are not converted: the default effect stays
    QVERIFY(e.effectId() == QString("circle"));
    QVERIFY(e.width() == 100);
    QVERIFY(e.height() == 90);
    QVERIFY(e.rotation() == 310);

    QVERIFY(e.xOffset() == 10);
    QVERIFY(e.yOffset() == 20);

    QVERIFY(e.propagationMode() == EFX::Serial);
    QCOMPARE(e.fixtures().size(), 3);
    QCOMPARE(e.fixtures().at(0)->head().fxi, quint32(33));
    QCOMPARE(e.fixtures().at(0)->direction(), EFX::Forward);
    QCOMPARE(e.fixtures().at(1)->head().fxi, quint32(11));
    QCOMPARE(e.fixtures().at(1)->direction(), EFX::Backward);
    QCOMPARE(e.fixtures().at(2)->head().fxi, quint32(45));
    QCOMPARE(e.fixtures().at(2)->direction(), EFX::Backward);

    Bus::instance()->setValue(12, 50);
    Bus::instance()->setValue(13, 100);
    e.postLoad();

    QVERIFY(e.fadeInSpeed() == uint(1000));
    QVERIFY(e.fadeOutSpeed() == uint(1000));
    QVERIFY(e.duration() == uint(2000));
}

void EFX_Test::loadSuccess()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("Function");
    xmlWriter.writeAttribute("Type", "EFX");
    xmlWriter.writeAttribute("Name", "Test EFX");

    xmlWriter.writeTextElement("PropagationMode", "Serial");

    xmlWriter.writeStartElement("Speed");
    xmlWriter.writeAttribute("FadeIn", "1300");
    xmlWriter.writeAttribute("FadeOut", "1400");
    xmlWriter.writeAttribute("Duration", "1500");
    xmlWriter.writeEndElement();

    xmlWriter.writeTextElement("Direction", "Forward");
    xmlWriter.writeTextElement("RunOrder", "Loop");
    xmlWriter.writeStartElement("Effect");
    xmlWriter.writeAttribute("Id", "lissajous");
    xmlWriter.writeAttribute("Version", "1");
    xmlWriter.writeTextElement("Params", "{\"xfreq\":2,\"xphase\":270,\"yfreq\":3,\"yphase\":80}");
    xmlWriter.writeEndElement();
    xmlWriter.writeTextElement("Width", "100");
    xmlWriter.writeTextElement("Height", "90");
    xmlWriter.writeTextElement("Rotation", "310");

    /* X Axis */
    xmlWriter.writeStartElement("Axis");
    xmlWriter.writeAttribute("Name", "X");
    xmlWriter.writeTextElement("Offset", "10");
    xmlWriter.writeTextElement("Frequency", "2");
    xmlWriter.writeTextElement("Phase", "270");
    xmlWriter.writeEndElement();

    /* Y Axis */
    xmlWriter.writeStartElement("Axis");
    xmlWriter.writeAttribute("Name", "Y");
    xmlWriter.writeTextElement("Offset", "20");
    xmlWriter.writeTextElement("Frequency", "3");
    xmlWriter.writeTextElement("Phase", "80");
    xmlWriter.writeEndElement();

    /* Fixture 1 */
    xmlWriter.writeStartElement("Fixture");
    xmlWriter.writeTextElement("ID", "33");
    xmlWriter.writeTextElement("Direction", "Forward");
    xmlWriter.writeEndElement();

    /* Fixture 2 */
    xmlWriter.writeStartElement("Fixture");
    xmlWriter.writeTextElement("ID", "11");
    xmlWriter.writeTextElement("Direction", "Backward");
    xmlWriter.writeEndElement();

    /* Fixture 3 */
    xmlWriter.writeStartElement("Fixture");
    xmlWriter.writeTextElement("ID", "45");
    xmlWriter.writeTextElement("Direction", "Backward");
    xmlWriter.writeEndElement();

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    QVERIFY(e.loadXML(xmlReader) == true);
    QVERIFY(e.direction() == EFX::Forward);
    QVERIFY(e.runOrder() == EFX::Loop);

    QVERIFY(e.effectId() == QString("lissajous"));
    QVERIFY(e.width() == 100);
    QVERIFY(e.height() == 90);
    QVERIFY(e.rotation() == 310);

    QVERIFY(e.xOffset() == 10);
    QVERIFY(e.effectParam("xfreq") == 2);
    QVERIFY(e.effectParam("xphase") == 270);
    QVERIFY(e.yOffset() == 20);
    QVERIFY(e.effectParam("yfreq") == 3);
    QVERIFY(e.effectParam("yphase") == 80);

    QVERIFY(e.propagationMode() == EFX::Serial);
    QVERIFY(e.fixtures().size() == 3);
    QVERIFY(e.fixtures().at(0)->head().fxi == 33);
    QVERIFY(e.fixtures().at(0)->direction() == EFX::Forward);
    QVERIFY(e.fixtures().at(1)->head().fxi == 11);
    QVERIFY(e.fixtures().at(1)->direction() == EFX::Backward);
    QVERIFY(e.fixtures().at(2)->head().fxi == 45);
    QVERIFY(e.fixtures().at(2)->direction() == EFX::Backward);

    QVERIFY(e.fadeInSpeed() == uint(1300));
    QVERIFY(e.fadeOutSpeed() == uint(1400));
    QVERIFY(e.duration() == uint(1500));
}

void EFX_Test::loadWrongType()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("Function");
    xmlWriter.writeAttribute("Type", "Chaser");

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    QVERIFY(e.loadXML(xmlReader) == false);
}

void EFX_Test::loadWrongRoot()
{
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("EFX");
    xmlWriter.writeAttribute("Type", "EFX");

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    QVERIFY(e.loadXML(xmlReader) == false);
}

void EFX_Test::loadDuplicateFixture()
{
    QSKIP("Duplicate fixtures are allowed because can animate different parameters (RGB, dimmer, etc.)");

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("Function");
    xmlWriter.writeAttribute("Type", "EFX");
    xmlWriter.writeAttribute("Name", "Test EFX");

    /* Fixture 1 */
    xmlWriter.writeStartElement("Fixture");
    xmlWriter.writeTextElement("ID", "33");
    xmlWriter.writeTextElement("Direction", "Forward");
    xmlWriter.writeEndElement();

    /* Fixture 2 */
    xmlWriter.writeStartElement("Fixture");
    xmlWriter.writeTextElement("ID", "33");
    xmlWriter.writeTextElement("Direction", "Backward");
    xmlWriter.writeEndElement();

    xmlWriter.writeEndDocument();
    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();

    EFX e(m_doc);
    QVERIFY(e.loadXML(xmlReader) == true);
    QVERIFY(e.fixtures().size() == 1);
    QVERIFY(e.fixtures().at(0)->direction() == EFX::Forward);
}

void EFX_Test::save()
{
    EFX e1(m_doc);
    e1.setName("First");
    e1.setDirection(EFX::Backward);
    e1.setRunOrder(EFX::SingleShot);
    e1.setFadeInSpeed(42);
    e1.setFadeOutSpeed(69);
    e1.setDuration(1337);
    e1.setEffect("lissajous");
    e1.setWidth(13);
    e1.setHeight(42);
    e1.setRotation(78);
    e1.setStartOffset(91);
    e1.setIsRelative(false);
    e1.setXOffset(34);
    e1.setYOffset(27);
    e1.setEffectParam("xfreq", 5);
    e1.setEffectParam("yfreq", 4);
    e1.setEffectParam("xphase", 163);
    e1.setEffectParam("yphase", 94);
    e1.setPropagationMode(EFX::Serial);

    EFXFixture* ef1 = new EFXFixture(&e1);
    ef1->setHead(GroupHead(12, 3));
    e1.addFixture(ef1);
    EFXFixture* ef2 = new EFXFixture(&e1);
    ef2->setHead(GroupHead(34, 5));
    ef2->setDirection(EFX::Backward);
    ef2->setStartOffset(27);
    e1.addFixture(ef2);
    EFXFixture* ef3 = new EFXFixture(&e1);
    ef3->setHead(GroupHead(56,7));
    e1.addFixture(ef3);

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter xmlWriter(&buffer);

    xmlWriter.writeStartElement("TestRoot");

    QVERIFY(e1.saveXML(&xmlWriter) == true);

    xmlWriter.setDevice(NULL);
    buffer.close();

    buffer.open(QIODevice::ReadOnly | QIODevice::Text);
    QXmlStreamReader xmlReader(&buffer);
    xmlReader.readNextStartElement();
    QVERIFY(xmlReader.name().toString() == "TestRoot");
    xmlReader.readNextStartElement();
    QVERIFY(xmlReader.name().toString() == "Function");

    QVERIFY(xmlReader.attributes().value("Type").toString() == "EFX");
    QVERIFY(xmlReader.attributes().value("Name").toString() == "First");

    bool dir = false, off = false, run = false, algo = false, w = false,
         h = false, rot = false, isRelative = false, xoff = false, yoff = false,
         prop = false, speed = false;
    int fixtureid = 0, fixturehead = 0, fixturedirection = 0, fixtureStartOffset = 0;
    QList <QString> fixtures;

    while (xmlReader.readNextStartElement())
    {
        if (xmlReader.name().toString() == "Speed")
        {
            QCOMPARE(xmlReader.attributes().value("FadeIn").toString().toUInt(), uint(42));
            QCOMPARE(xmlReader.attributes().value("FadeOut").toString().toUInt(), uint(69));
            QCOMPARE(xmlReader.attributes().value("Duration").toString().toUInt(), uint(1337));
            speed = true;
            xmlReader.skipCurrentElement();
        }
        else if (xmlReader.name().toString() == "Direction")
        {
            QVERIFY(xmlReader.readElementText() == "Backward");
            dir = true;
        }
        else if (xmlReader.name().toString() == "StartOffset")
        {
            QVERIFY(xmlReader.readElementText() == "91");
            off = true;
        }
        else if (xmlReader.name().toString() == "RunOrder")
        {
            QVERIFY(xmlReader.readElementText() == "SingleShot");
            run = true;
        }
        else if (xmlReader.name().toString() == "Bus")
        {
            QFAIL("EFX should not save a Bus tag anymore!");
            xmlReader.skipCurrentElement();
        }
        else if (xmlReader.name().toString() == "Algorithm")
        {
            QFAIL("EFX should not save an Algorithm tag anymore!");
        }
        else if (xmlReader.name().toString() == "Effect")
        {
            QCOMPARE(xmlReader.attributes().value("Id").toString(), QString("lissajous"));
            QByteArray params, embedded;
            while (xmlReader.readNextStartElement())
            {
                if (xmlReader.name().toString() == "Params")
                    params = xmlReader.readElementText().toUtf8();
                else if (xmlReader.name().toString() == "Embedded")
                    embedded = xmlReader.readElementText().toUtf8();
                else
                    xmlReader.skipCurrentElement();
            }
            QJsonObject po = QJsonDocument::fromJson(params).object();
            QCOMPARE(po.value("xfreq").toDouble(), 5.0);
            QCOMPARE(po.value("yfreq").toDouble(), 4.0);
            QCOMPARE(po.value("xphase").toDouble(), 163.0);
            QCOMPARE(po.value("yphase").toDouble(), 94.0);
            QVERIFY(!EFXEffect::fromJson(embedded).isNull());
            algo = true;
        }
        else if (xmlReader.name().toString() == "Width")
        {
            QVERIFY(xmlReader.readElementText() == "13");
            w = true;
        }
        else if (xmlReader.name().toString() == "Height")
        {
            QVERIFY(xmlReader.readElementText() == "42");
            h = true;
        }
        else if (xmlReader.name().toString() == "Rotation")
        {
            QVERIFY(xmlReader.readElementText() == "78");
            rot = true;
        }
        else if (xmlReader.name().toString() == "IsRelative")
        {
            QVERIFY(xmlReader.readElementText() == "0");
            isRelative = true;
        }
        else if (xmlReader.name().toString() == "DimmerControl")
        {
            QVERIFY(xmlReader.readElementText() == "0");
        }
        else if (xmlReader.name().toString() == "PropagationMode")
        {
            QVERIFY(xmlReader.readElementText() == "Serial");
            prop = true;
        }
        else if (xmlReader.name().toString() == "Axis")
        {
            bool axis = true;
            if (xmlReader.attributes().value("Name").toString() == "X")
                axis = true;
            else if (xmlReader.attributes().value("Name").toString() == "Y")
                axis = false;
            else
                QFAIL("Invalid axis!");

            while (xmlReader.readNextStartElement())
            {
                if (xmlReader.name().toString() == "Offset")
                {
                    if (axis == true)
                    {
                        QVERIFY(xmlReader.readElementText() == "34");
                        xoff = true;
                    }
                    else
                    {
                        QVERIFY(xmlReader.readElementText() == "27");
                        yoff = true;
                    }
                }
                else
                {
                    QFAIL("Unexpected axis tag!");
                }
            }
        }
        else if (xmlReader.name().toString() == "Fixture")
        {
            int expectHead = 0;
            bool expectBackward = false;
            int expectedMode = 0;
            int expectStartOffset = 0;

            while (xmlReader.readNextStartElement())
            {
                if (xmlReader.name().toString() == "ID")
                {
                    QString text = xmlReader.readElementText();
                    if (fixtures.contains(text) == true)
                        QFAIL("Same fixture multiple times!");
                    else
                        fixtures.append(text);

                    if (text == "34")
                    {
                        expectHead = 5;
                        expectBackward = true;
                        expectStartOffset = 27;
                    }
                    else if (text == "12")
                    {
                        expectHead = 3;
                        expectBackward = false;
                        expectStartOffset = 0;
                    }
                    else
                    {
                        expectHead = 7;
                        expectBackward = false;
                        expectStartOffset = 0;
                    }

                    fixtureid++;
                }
                else if (xmlReader.name().toString() == "Head")
                {
                    QCOMPARE(xmlReader.readElementText().toInt(), expectHead);
                    fixturehead++;
                }
                else if (xmlReader.name().toString() == "Direction")
                {
                    QString text = xmlReader.readElementText();
                    if (expectBackward == false && text == "Backward")
                    {
                        QFAIL("Not expecting reversal!");
                    }

                    fixturedirection++;
                }
                else if (xmlReader.name().toString() == "StartOffset")
                {
                    QCOMPARE(xmlReader.readElementText().toInt(), expectStartOffset);
                    fixtureStartOffset++;
                }
                else if (xmlReader.name().toString() == "Mode")
                {
                    QCOMPARE(xmlReader.readElementText().toInt(), expectedMode);
                }
                else
                {
                    QFAIL("Unexpected fixture tag!");
                }
            }
        }
        else
        {
            QFAIL("Unexpected EFX tag!");
        }
    }

    QCOMPARE(fixtures.size(), 3);
    QCOMPARE(fixtureid, 3);
    QCOMPARE(fixturehead, 3);
    QCOMPARE(fixturedirection, 3);
    QCOMPARE(fixtureStartOffset, 3);
    QVERIFY(dir == true);
    QVERIFY(off == true);
    QVERIFY(run == true);
    QVERIFY(speed == true);
    QVERIFY(algo == true);
    QVERIFY(w == true);
    QVERIFY(h == true);
    QVERIFY(rot == true);
    QVERIFY(isRelative == true);
    QVERIFY(xoff == true);
    QVERIFY(yoff == true);
                    QVERIFY(prop == true);
}

void EFX_Test::effectParams()
{
    EFX e(m_doc);
    QVERIFY(e.setEffect("lissajous"));
    QVERIFY(!e.setEffect("does-not-exist"));
    QCOMPARE(e.effectId(), QString("lissajous"));

    QCOMPARE(e.effectParam("xfreq"), 2.0);
    e.setEffectParam("xfreq", 3);
    QCOMPARE(e.effectParam("xfreq"), 3.0);

    // Values are clamped to the effect's limits, unknown ids are ignored
    e.setEffectParam("xfreq", 1e9);
    QVERIFY(e.effectParam("xfreq") <= 100.0);
    e.setEffectParam("bogus", 5);
    QCOMPARE(e.effectParam("bogus"), 0.0);

    // Selecting another effect resets the parameters
    e.setEffectParam("xfreq", 4);
    QVERIFY(e.setEffect("lissajous"));
    QCOMPARE(e.effectParam("xfreq"), 2.0);
}

static QByteArray saveEfxToXml(EFX& e)
{
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly | QIODevice::Text);
    QXmlStreamWriter w(&buffer);
    w.writeStartElement("TestRoot");
    e.saveXML(&w);
    w.writeEndElement();
    buffer.close();
    return data;
}

static bool loadEfxFromXml(EFX& e, const QByteArray& data)
{
    QXmlStreamReader r(data);
    r.readNextStartElement(); // TestRoot
    r.readNextStartElement(); // Function
    return e.loadXML(r);
}

void EFX_Test::effectEmbeddedFallback()
{
    // An effect that is not in the library travels inside the project
    const QByteArray json = "{\"id\":\"zz-embedded\",\"name\":\"Embedded\","
        "\"params\":[{\"id\":\"k\",\"type\":\"float\",\"min\":0,\"max\":2,\"default\":1}],"
        "\"channels\":[{\"target\":\"pan\",\"expr\":\"k*sin(theta)\"},{\"target\":\"tilt\",\"expr\":\"cos(theta)\"}]}";
    EFXEffectPtr fx = EFXEffect::fromJson(json);
    QVERIFY(!fx.isNull());
    QVERIFY(EFXLibrary::instance()->effect("zz-embedded").isNull());

    EFX e1(m_doc);
    e1.setEffect(fx);
    e1.setEffectParam("k", 1.5);
    const QByteArray xml = saveEfxToXml(e1);

    EFX e2(m_doc);
    QVERIFY(loadEfxFromXml(e2, xml));
    QVERIFY(e2.hasEffect());
    QCOMPARE(e2.effectId(), QString("zz-embedded"));
    QCOMPARE(e2.effectParam("k"), 1.5);
}

void EFX_Test::effectMissing()
{
    // Library id unknown and no embedded copy: loads, but has no usable effect
    const QByteArray xml =
        "<TestRoot><Function Type=\"EFX\" Name=\"x\">"
        "<Effect Id=\"nope\" Version=\"1\"><Params>{\"a\":1}</Params></Effect>"
        "</Function></TestRoot>";
    EFX e(m_doc);
    QVERIFY(loadEfxFromXml(e, xml));
    QVERIFY(!e.hasEffect());
    QCOMPARE(e.effectId(), QString("nope"));

    // Saving keeps the reference so the project is not corrupted
    EFX e2(m_doc);
    QVERIFY(loadEfxFromXml(e2, saveEfxToXml(e)));
    QCOMPARE(e2.effectId(), QString("nope"));
}

void EFX_Test::preRunPostRun()
{
    QList<Universe*> ua;
    ua.append(new Universe(0, new GrandMaster()));
    MasterTimerStub timer(m_doc, ua);

    EFX* e = new EFX(m_doc);
    e->setName("Test EFX");
    QVERIFY(e->m_fadersMap.count() == 0);

    QSignalSpy spy(e, SIGNAL(running(quint32)));
    e->preRun(&timer);
    QVERIFY(e->m_fadersMap.count() == 0);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy[0].size(), 1);
    QCOMPARE(spy[0][0].toUInt(), e->id());
    e->postRun(&timer, ua);
    QVERIFY(e->m_fadersMap.count() == 0);
}

void EFX_Test::adjustIntensity()
{
    /* Basically any fixture with 16bit pan & tilt channels will do, but
       then the exact channel numbers and mode name has to be changed
       below. */
    QLCFixtureDef* def = m_doc->fixtureDefCache()->fixtureDef("Martin", "MAC250+");
    QVERIFY(def != NULL);

    QLCFixtureMode* mode = def->mode("Mode 4");
    QVERIFY(mode != NULL);

    Fixture* fxi1 = new Fixture(m_doc);
    fxi1->setFixtureDefinition(def, mode);
    fxi1->setName("Test Scanner");
    fxi1->setAddress(0);
    fxi1->setUniverse(0);
    m_doc->addFixture(fxi1);

    Fixture* fxi2 = new Fixture(m_doc);
    fxi2->setFixtureDefinition(def, mode);
    fxi2->setName("Test Scanner");
    fxi2->setAddress(0);
    fxi2->setUniverse(1);
    m_doc->addFixture(fxi2);

    EFX* e = new EFX(m_doc);
    e->setName("Test EFX");

    EFXFixture* ef1 = new EFXFixture(e);
    ef1->setHead(GroupHead(fxi1->id(), 0));
    e->addFixture(ef1);

    EFXFixture* ef2 = new EFXFixture(e);
    ef2->setHead(GroupHead(fxi2->id(), 0));
    e->addFixture(ef2);

    e->adjustAttribute(0.2);
    QCOMPARE(e->getAttributeValue(Function::Intensity), 0.2);

    e->preRun(m_doc->masterTimer());

    e->adjustAttribute(0.5);
    QCOMPARE(e->getAttributeValue(Function::Intensity), 0.5);

    QList<Universe*> ua;
    ua.append(new Universe(0, new GrandMaster()));
    e->postRun(m_doc->masterTimer(), ua);
}

void EFX_Test::removeMultiHeadFixture()
{
    EFX *e = new EFX(m_doc);

    for (int h = 0; h < 3; h++)
    {
        EFXFixture *ef = new EFXFixture(e);
        ef->setHead(GroupHead(7, h));
        QVERIFY(e->addFixture(ef));
    }
    EFXFixture *other = new EFXFixture(e);
    other->setHead(GroupHead(8, 0));
    QVERIFY(e->addFixture(other));
    QCOMPARE(e->fixtures().size(), 4);

    // every head of the removed fixture must go
    e->slotFixtureRemoved(7);
    QCOMPARE(e->fixtures().size(), 1);
    QCOMPARE(e->fixtures().at(0), other);

    delete e;
}

void EFX_Test::missingFixture()
{
    QList<Universe*> ua;
    ua.append(new Universe(0, new GrandMaster()));
    MasterTimerStub timer(m_doc, ua);

    QLCFixtureDef* def = m_doc->fixtureDefCache()->fixtureDef("Martin", "MAC250+");
    QLCFixtureMode* mode = def->mode("Mode 4");
    Fixture* fxi = new Fixture(m_doc);
    fxi->setFixtureDefinition(def, mode);
    fxi->setAddress(0);
    fxi->setUniverse(0);
    m_doc->addFixture(fxi);

    EFX *e = new EFX(m_doc);
    m_doc->addFunction(e);

    // an entry for a fixture that doesn't exist (e.g. it couldn't be loaded)
    EFXFixture *missing = new EFXFixture(e);
    missing->setHead(GroupHead(1234, 0));
    QVERIFY(e->addFixture(missing));
    QCOMPARE(missing->universe(), Universe::invalid());

    EFXFixture *ef = new EFXFixture(e);
    ef->setHead(GroupHead(fxi->id(), 0));
    QVERIFY(e->addFixture(ef));

    // it is skipped, and the other fixture still runs
    e->preRun(&timer);
    for (int i = 0; i < 10; i++)
        e->write(&timer, ua);
    QCOMPARE(ef->m_elapsed, uint(10 * MasterTimer::tick()));
    e->postRun(&timer, ua);
}

QTEST_APPLESS_MAIN(EFX_Test)
