/*
  Q Light Controller - Unit test
  efx_test.h

  Copyright (c) Heikki Junnila

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

#ifndef EFX_TEST_H
#define EFX_TEST_H

#include <QObject>

class Doc;
class EFX_Test final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    void initial();
    void loopDurationWithLongFadeIn();
    void width();
    void height();
    void rotation();
    void xOffset();
    void yOffset();
    void fixtures();
    void propagationMode();
    void dimmerEnvelope();



    void rotateAndScale();
    void widthHeightOffset();

    void copyFrom();
    void createCopy();

    void loadXAxis();
    void loadYAxis();
    void loadYAxisWrongRoot();
    void loadAxisNoXY();
    void loadSuccessLegacy();
    void loadSuccess();
    void loadWrongType();
    void loadWrongRoot();
    void loadDuplicateFixture();
    void save();
    void effectParams();
    void effectEmbeddedFallback();
    void effectMissing();

    void preRunPostRun();
    void adjustIntensity();
    void removeMultiHeadFixture();
    void missingFixture();

private:
    Doc* m_doc;
};

#endif
