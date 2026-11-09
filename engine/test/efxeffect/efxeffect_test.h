/*
  Q Light Controller Plus
  efxeffect_test.h

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

#ifndef EFXEFFECT_TEST_H
#define EFXEFFECT_TEST_H

#include <QObject>

class EFXEffect_Test final : public QObject
{
    Q_OBJECT

private slots:
    void exprArithmetic();
    void exprPrecedence();
    void exprVariablesAndConstants();
    void exprFunctions();
    void exprWaveforms();
    void exprSafety();
    void exprErrors();

    void effectMinimal();
    void effectInvalid();
    void effectParams();
    void effectReverse();

    void libraryUserEffects();
    void libraryRejectsBadIds();
    void libraryLoadsPresets();

    void keyframes();
    void keyframesRejectBad();
    void pathCircle();
    void pathEasing();
    void pathOpenAndPolyline();
    void pathRejectBad();
    void pathEffect();
    void legacyParity();
};

#endif
