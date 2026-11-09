/*
  Q Light Controller Plus
  efxexpression.cpp

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

#include <QtMath>
#include <cmath>

#include "efxexpression.h"

namespace
{

const int MaxStack = 64;
const int MaxCodeSize = 4096;

struct FuncDef
{
    const char *name;
    int arity;
};

enum FuncId
{
    FSin, FCos, FTan, FAsin, FAcos, FAtan, FAtan2, FAbs, FSqrt, FPow, FExp,
    FLog, FFloor, FCeil, FRound, FSign, FMin, FMax, FClamp, FMix, FMod,
    FSmoothstep, FTri, FSaw, FSquare, FPulse, FNoise, FIf,
    FuncCount
};

const FuncDef funcs[FuncCount] =
{
    {"sin", 1}, {"cos", 1}, {"tan", 1}, {"asin", 1}, {"acos", 1}, {"atan", 1},
    {"atan2", 2}, {"abs", 1}, {"sqrt", 1}, {"pow", 2}, {"exp", 1},
    {"log", 1}, {"floor", 1}, {"ceil", 1}, {"round", 1}, {"sign", 1},
    {"min", 2}, {"max", 2}, {"clamp", 3}, {"mix", 3}, {"mod", 2},
    {"smoothstep", 3}, {"tri", 1}, {"saw", 1}, {"square", 1}, {"pulse", 2},
    {"noise", 1}, {"if", 3}
};

inline double frac(double x)
{
    return x - std::floor(x);
}

double hashNoise(qint64 n)
{
    quint64 h = quint64(n) * 0x9E3779B97F4A7C15ULL;
    h ^= h >> 32;
    h *= 0xD6E8FEB86659FD93ULL;
    h ^= h >> 32;
    return double(h & 0xFFFFFF) / double(0xFFFFFF) * 2.0 - 1.0;
}

double valueNoise(double x)
{
    double fl = std::floor(x);
    double f = x - fl;
    double s = f * f * (3.0 - 2.0 * f);
    double a = hashNoise(qint64(fl));
    double b = hashNoise(qint64(fl) + 1);
    return a + (b - a) * s;
}

double callFunc(int id, const double *a)
{
    switch (id)
    {
        case FSin: return std::sin(a[0]);
        case FCos: return std::cos(a[0]);
        case FTan: return std::tan(a[0]);
        case FAsin: return std::asin(a[0]);
        case FAcos: return std::acos(a[0]);
        case FAtan: return std::atan(a[0]);
        case FAtan2: return std::atan2(a[0], a[1]);
        case FAbs: return std::fabs(a[0]);
        case FSqrt: return std::sqrt(a[0]);
        case FPow: return std::pow(a[0], a[1]);
        case FExp: return std::exp(a[0]);
        case FLog: return std::log(a[0]);
        case FFloor: return std::floor(a[0]);
        case FCeil: return std::ceil(a[0]);
        case FRound: return std::round(a[0]);
        case FSign: return (a[0] > 0) - (a[0] < 0);
        case FMin: return std::min(a[0], a[1]);
        case FMax: return std::max(a[0], a[1]);
        case FClamp: return std::min(std::max(a[0], a[1]), a[2]);
        case FMix: return a[0] + (a[1] - a[0]) * a[2];
        case FMod: return a[1] == 0.0 ? 0.0 : a[0] - a[1] * std::floor(a[0] / a[1]);
        case FSmoothstep:
        {
            if (a[0] == a[1])
                return a[2] < a[0] ? 0.0 : 1.0;
            double t = std::min(std::max((a[2] - a[0]) / (a[1] - a[0]), 0.0), 1.0);
            return t * t * (3.0 - 2.0 * t);
        }
        case FTri: return 1.0 - 4.0 * std::fabs(frac(a[0]) - 0.5);
        case FSaw: return 2.0 * frac(a[0]) - 1.0;
        case FSquare: return frac(a[0]) < 0.5 ? 1.0 : -1.0;
        case FPulse: return frac(a[0]) < a[1] ? 1.0 : 0.0;
        case FNoise: return valueNoise(a[0]);
        case FIf: return a[0] != 0.0 ? a[1] : a[2];
    }
    return 0.0;
}

} // namespace

/****************************************************************************
 * Parser (recursive descent, emits RPN directly)
 ****************************************************************************/

class EFXExpressionParser
{
public:
    EFXExpressionParser(const QString& src, const QStringList& vars, EFXExpression& out)
        : m_s(src), m_vars(vars), m_out(out), m_pos(0), m_depth(0), m_maxDepth(0)
    {
    }

    bool run()
    {
        skipWs();
        if (m_pos >= m_s.size())
            return fail(QStringLiteral("empty expression"));
        if (!parseComparison())
            return false;
        skipWs();
        if (m_pos < m_s.size())
            return fail(QStringLiteral("unexpected '%1'").arg(m_s.at(m_pos)));
        if (m_out.m_code.size() > MaxCodeSize)
            return fail(QStringLiteral("expression too long"));
        m_out.m_maxStack = m_maxDepth;
        return true;
    }

private:
    bool fail(const QString& msg)
    {
        m_out.m_error = QStringLiteral("%1 (at position %2)").arg(msg).arg(m_pos + 1);
        return false;
    }

    void skipWs()
    {
        while (m_pos < m_s.size() && m_s.at(m_pos).isSpace())
            m_pos++;
    }

    bool peekIs(QChar c)
    {
        skipWs();
        return m_pos < m_s.size() && m_s.at(m_pos) == c;
    }

    void addInstr(EFXExpression::Op op, int arg = 0, double value = 0.0)
    {
        EFXExpression::Instr i = { op, arg, value };
        m_out.m_code.append(i);
    }

    bool push()
    {
        if (++m_depth > m_maxDepth)
            m_maxDepth = m_depth;
        if (m_maxDepth > MaxStack)
            return fail(QStringLiteral("expression too deeply nested"));
        return true;
    }

    // comparison := additive [(< > <= >= == !=) additive]
    bool parseComparison()
    {
        if (!parseAdditive())
            return false;
        skipWs();
        if (m_pos >= m_s.size())
            return true;

        EFXExpression::Op op;
        QChar c = m_s.at(m_pos);
        QChar n = m_pos + 1 < m_s.size() ? m_s.at(m_pos + 1) : QChar();
        int len = 1;
        if (c == '<' && n == '=') { op = EFXExpression::Le; len = 2; }
        else if (c == '>' && n == '=') { op = EFXExpression::Ge; len = 2; }
        else if (c == '=' && n == '=') { op = EFXExpression::Eq; len = 2; }
        else if (c == '!' && n == '=') { op = EFXExpression::Ne; len = 2; }
        else if (c == '<') op = EFXExpression::Lt;
        else if (c == '>') op = EFXExpression::Gt;
        else return true;

        m_pos += len;
        if (!parseAdditive())
            return false;
        addInstr(op);
        m_depth--;
        return true;
    }

    bool parseAdditive()
    {
        if (!parseTerm())
            return false;
        for (;;)
        {
            skipWs();
            if (m_pos >= m_s.size())
                return true;
            QChar c = m_s.at(m_pos);
            if (c != '+' && c != '-')
                return true;
            m_pos++;
            if (!parseTerm())
                return false;
            addInstr(c == '+' ? EFXExpression::Add : EFXExpression::Sub);
            m_depth--;
        }
    }

    bool parseTerm()
    {
        if (!parseUnary())
            return false;
        for (;;)
        {
            skipWs();
            if (m_pos >= m_s.size())
                return true;
            QChar c = m_s.at(m_pos);
            if (c != '*' && c != '/' && c != '%')
                return true;
            m_pos++;
            if (!parseUnary())
                return false;
            addInstr(c == '*' ? EFXExpression::Mul : (c == '/' ? EFXExpression::Div : EFXExpression::Mod));
            m_depth--;
        }
    }

    bool parseUnary()
    {
        skipWs();
        if (m_pos < m_s.size() && (m_s.at(m_pos) == '-' || m_s.at(m_pos) == '+'))
        {
            bool neg = m_s.at(m_pos) == '-';
            m_pos++;
            if (!parseUnary())
                return false;
            if (neg)
                addInstr(EFXExpression::Neg);
            return true;
        }
        return parsePower();
    }

    // power := primary ['^' unary]   (right associative, binds tighter than unary minus on the left)
    bool parsePower()
    {
        if (!parsePrimary())
            return false;
        skipWs();
        if (m_pos < m_s.size() && m_s.at(m_pos) == '^')
        {
            m_pos++;
            if (!parseUnary())
                return false;
            addInstr(EFXExpression::Pow);
            m_depth--;
        }
        return true;
    }

    bool parsePrimary()
    {
        skipWs();
        if (m_pos >= m_s.size())
            return fail(QStringLiteral("unexpected end of expression"));

        QChar c = m_s.at(m_pos);

        if (c == '(')
        {
            m_pos++;
            if (!parseComparison())
                return false;
            if (!peekIs(')'))
                return fail(QStringLiteral("missing ')'"));
            m_pos++;
            return true;
        }

        if (c.isDigit() || c == '.')
            return parseNumber();

        if (c.isLetter() || c == '_')
            return parseIdentifier();

        return fail(QStringLiteral("unexpected '%1'").arg(c));
    }

    bool parseNumber()
    {
        int start = m_pos;
        while (m_pos < m_s.size() && (m_s.at(m_pos).isDigit() || m_s.at(m_pos) == '.'))
            m_pos++;
        if (m_pos < m_s.size() && (m_s.at(m_pos) == 'e' || m_s.at(m_pos) == 'E'))
        {
            int save = m_pos;
            m_pos++;
            if (m_pos < m_s.size() && (m_s.at(m_pos) == '+' || m_s.at(m_pos) == '-'))
                m_pos++;
            if (m_pos < m_s.size() && m_s.at(m_pos).isDigit())
            {
                while (m_pos < m_s.size() && m_s.at(m_pos).isDigit())
                    m_pos++;
            }
            else
            {
                m_pos = save;
            }
        }
        bool ok = false;
        double v = m_s.mid(start, m_pos - start).toDouble(&ok);
        if (!ok)
        {
            m_pos = start;
            return fail(QStringLiteral("invalid number"));
        }
        addInstr(EFXExpression::PushConst, 0, v);
        return push();
    }

    bool parseIdentifier()
    {
        int start = m_pos;
        while (m_pos < m_s.size() && (m_s.at(m_pos).isLetterOrNumber() || m_s.at(m_pos) == '_'))
            m_pos++;
        QString name = m_s.mid(start, m_pos - start);

        if (peekIs('('))
            return parseCall(name, start);

        int slot = m_vars.indexOf(name);
        if (slot >= 0)
        {
            addInstr(EFXExpression::PushVar, slot);
            return push();
        }

        if (name == QLatin1String("pi")) { addInstr(EFXExpression::PushConst, 0, M_PI); return push(); }
        if (name == QLatin1String("tau")) { addInstr(EFXExpression::PushConst, 0, M_PI * 2.0); return push(); }
        if (name == QLatin1String("e")) { addInstr(EFXExpression::PushConst, 0, M_E); return push(); }

        m_pos = start;
        return fail(QStringLiteral("unknown identifier '%1'").arg(name));
    }

    bool parseCall(const QString& name, int start)
    {
        int id = -1;
        for (int i = 0; i < FuncCount; i++)
        {
            if (name == QLatin1String(funcs[i].name))
            {
                id = i;
                break;
            }
        }
        if (id < 0)
        {
            m_pos = start;
            return fail(QStringLiteral("unknown function '%1'").arg(name));
        }

        m_pos++; // '('
        int argc = 0;
        if (!peekIs(')'))
        {
            for (;;)
            {
                if (!parseComparison())
                    return false;
                argc++;
                if (peekIs(','))
                {
                    m_pos++;
                    continue;
                }
                break;
            }
        }
        if (!peekIs(')'))
            return fail(QStringLiteral("missing ')' after arguments of '%1'").arg(name));
        m_pos++;

        if (argc != funcs[id].arity)
        {
            return fail(QStringLiteral("function '%1' expects %2 argument(s), got %3")
                        .arg(name).arg(funcs[id].arity).arg(argc));
        }

        addInstr(EFXExpression::Call, id);
        m_depth -= argc - 1;
        return true;
    }

private:
    QString m_s;
    QStringList m_vars;
    EFXExpression& m_out;
    int m_pos;
    int m_depth;
    int m_maxDepth;
};

/****************************************************************************
 * EFXExpression
 ****************************************************************************/

EFXExpression::EFXExpression()
    : m_valid(false)
    , m_maxStack(0)
{
}

bool EFXExpression::compile(const QString& source, const QStringList& variables)
{
    m_valid = false;
    m_error.clear();
    m_code.clear();
    m_source = source;
    m_maxStack = 0;

    EFXExpressionParser parser(source, variables, *this);
    if (!parser.run())
    {
        m_code.clear();
        return false;
    }

    m_valid = true;
    return true;
}

double EFXExpression::evaluate(const double *vars) const
{
    if (!m_valid)
        return 0.0;

    double stack[MaxStack];
    int sp = 0;

    for (const Instr& in : m_code)
    {
        switch (in.op)
        {
            case PushConst: stack[sp++] = in.value; break;
            case PushVar: stack[sp++] = vars[in.arg]; break;
            case Add: sp--; stack[sp - 1] += stack[sp]; break;
            case Sub: sp--; stack[sp - 1] -= stack[sp]; break;
            case Mul: sp--; stack[sp - 1] *= stack[sp]; break;
            case Div:
                sp--;
                stack[sp - 1] = stack[sp] == 0.0 ? 0.0 : stack[sp - 1] / stack[sp];
                break;
            case Mod:
                sp--;
                stack[sp - 1] = stack[sp] == 0.0 ? 0.0 : std::fmod(stack[sp - 1], stack[sp]);
                break;
            case Pow: sp--; stack[sp - 1] = std::pow(stack[sp - 1], stack[sp]); break;
            case Neg: stack[sp - 1] = -stack[sp - 1]; break;
            case Lt: sp--; stack[sp - 1] = stack[sp - 1] < stack[sp]; break;
            case Gt: sp--; stack[sp - 1] = stack[sp - 1] > stack[sp]; break;
            case Le: sp--; stack[sp - 1] = stack[sp - 1] <= stack[sp]; break;
            case Ge: sp--; stack[sp - 1] = stack[sp - 1] >= stack[sp]; break;
            case Eq: sp--; stack[sp - 1] = stack[sp - 1] == stack[sp]; break;
            case Ne: sp--; stack[sp - 1] = stack[sp - 1] != stack[sp]; break;
            case Call:
            {
                int n = funcs[in.arg].arity;
                sp -= n;
                stack[sp] = callFunc(in.arg, &stack[sp]);
                sp++;
                break;
            }
        }
    }

    double r = sp > 0 ? stack[sp - 1] : 0.0;
    return std::isfinite(r) ? r : 0.0;
}
