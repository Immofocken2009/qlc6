/*
  Q Light Controller Plus
  efxexpression.h

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

#ifndef EFXEXPRESSION_H
#define EFXEXPRESSION_H

#include <QStringList>
#include <QVector>
#include <QString>

/** @addtogroup engine Engine
 * @{
 */

/**
 * A small arithmetic expression compiled once into bytecode.
 *
 * Variables are bound to slots at compile time (by name), so evaluating never
 * touches strings or the heap. evaluate() is const and uses only a local
 * stack, hence a compiled expression can be evaluated concurrently from the
 * MasterTimer thread and the UI thread.
 *
 * Grammar: numbers, variables, + - * / % ^ (right assoc), unary - +,
 * comparisons < > <= >= == != (yield 1 or 0), parentheses and function calls.
 * Constants: pi, tau, e.
 *
 * Functions: sin cos tan asin acos atan atan2 abs sqrt pow exp log floor ceil
 * round sign min max clamp mix mod smoothstep tri saw square pulse noise if
 *
 * Waveform helpers (all with period 1 in their argument):
 *   tri(x)      triangle, starts at -1 for x=0, peaks +1 at x=0.5
 *   saw(x)      2*frac(x)-1
 *   square(x)   +1 for frac(x) < 0.5, else -1
 *   pulse(x,w)  1 for frac(x) < w, else 0
 *   noise(x)    smooth deterministic value noise in -1..1
 */
class EFXExpression
{
public:
    EFXExpression();

    /**
     * Compile @a source. Identifiers that are not constants or functions must
     * be listed in @a variables; the evaluate() input array uses the same order.
     *
     * @return true on success; on failure error() describes the problem.
     */
    bool compile(const QString& source, const QStringList& variables);

    bool isValid() const { return m_valid; }
    QString error() const { return m_error; }
    QString source() const { return m_source; }

    /** Evaluate with variable values in the order given to compile(). */
    double evaluate(const double* vars) const;

private:
    enum Op
    {
        PushConst, PushVar,
        Add, Sub, Mul, Div, Mod, Pow, Neg,
        Lt, Gt, Le, Ge, Eq, Ne,
        Call
    };

    struct Instr
    {
        Op op;
        int arg;        // variable slot or function id
        double value;   // constant
    };

    friend class EFXExpressionParser;

    bool m_valid;
    QString m_error;
    QString m_source;
    QVector<Instr> m_code;
    int m_maxStack;
};

/** @} */

#endif
