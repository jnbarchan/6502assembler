#include <QDebug>

#include "assembler.h"


//
// Assembler Class
//

#define __JSR_outch 0xefff

Assembler::Assembler(QObject *parent)
    : QObject{parent}
{
    _codeStream = nullptr;
    _currentCodeLineNumber = -1;
    _assembleState = AssembleState::NotStarted;
}

bool Assembler::needsAssembling() const
{
    return _assembleState != Assembled;
}

void Assembler::setNeedsAssembling()
{
    setAssembleState(AssembleState::NotStarted);
}

int Assembler::currentCodeLineNumber() const
{
    return _currentCodeLineNumber;
}

void Assembler::setCurrentCodeLineNumber(int newCurrentCodeLineNumber)
{
    if (_currentCodeLineNumber == newCurrentCodeLineNumber)
        return;
    _currentCodeLineNumber = newCurrentCodeLineNumber;
    emit currentCodeLineNumberChanged(_currentCodeLineNumber);
}

const QList<Instruction> &Assembler::instructions() const
{
    return _instructions;
}

const QList<int> &Assembler::instructionsCodeLineNumbers() const
{
    return _instructionsCodeLineNumbers;
}

int Assembler::currentCodeInstructionNumber() const
{
    return _currentCodeInstructionNumber;
}

void Assembler::setCurrentCodeInstructionNumber(int newCurrentCodeInstructionNumber)
{
    _currentCodeInstructionNumber = newCurrentCodeInstructionNumber;
}

void Assembler::setCodeLabel(const QString &key, int value)
{
    _codeLabels[key] = value;
}

void Assembler::setCode(QTextStream *codeStream)
{
    _codeStream = codeStream;
    restart();
}


void Assembler::debugMessage(const QString &message) const
{
    qDebug() << message;
    emit sendMessageToConsole(message);
}


void Assembler::restart(bool assemblePass2 /*= false*/)
{
    Q_ASSERT(_codeStream);
    setCurrentCodeLineNumber(0);
    setCurrentCodeInstructionNumber(0);
    _currentToken.clear();
    _currentCodeLabelScope = "";

    if (!assemblePass2)
    {
        _codeLines.clear();
        _codeStream->seek(0);
        _codeLabels.clear();
        _codeLabels["__outch"] = __JSR_outch;
        setAssembleState(AssembleState::NotStarted);
    }
}

void Assembler::assemble()
{
    restart();
    setAssembleState(Pass1);
    if (!assemblePass())
        return;
    restart(true);
    setAssembleState(Pass2);
    if (!assemblePass())
        return;
    setAssembleState(Assembled);
}

Assembler::AssembleState Assembler::assembleState() const
{
    return _assembleState;
}

void Assembler::setAssembleState(AssembleState newAssembleState)
{
    _assembleState = newAssembleState;
}


bool Assembler::assemblePass()
{
    _instructions.clear();
    _instructionsCodeLineNumbers.clear();
    Opcodes opcode;
    OpcodeOperand operand;
    bool hasOpcode, blankLine, eof;
    while (assembleNextStatement(opcode, operand, hasOpcode, blankLine, eof))
    {
        if (hasOpcode)
        {
            _instructions.append(Instruction(opcode, operand));
            _instructionsCodeLineNumbers.append(_currentCodeLineNumber);
            setCurrentCodeInstructionNumber(_currentCodeInstructionNumber + 1);
        }
        setCurrentCodeLineNumber(_currentCodeLineNumber + 1);
    }
    if (!eof)
        return false;
    return true;
}

bool Assembler::assembleNextStatement(Opcodes &opcode, OpcodeOperand &operand, bool &hasOpcode, bool &blankLine, bool &eof)
{
    //
    // ASSEMBLING PHASE
    //

    hasOpcode = blankLine = eof = false;
    _currentToken.clear();
    if (!getNextLine())
    {
        eof = true;
        return false;
    }
    blankLine = !getNextToken();
    if (blankLine)
        return true;

    QString mnemonic(_currentToken);
    opcode = Assembly::OpcodesKeyToValue(mnemonic.toUpper().toLocal8Bit());
    hasOpcode = Assembly::OpcodesValueIsValid(opcode);
    if (!hasOpcode)
    {
        if (tokenIsLabel())
        {
            QString label(_currentToken);

            bool more = getNextToken();

            if (_currentToken == '=')
            {
                getNextToken();
                bool ok;
                int value = getTokensExpressionValueAsInt(&ok);
                if (ok)
                    assignLabelValue(scopedLabelName(label), value);
                return true;
            }
            else
            {
                bool isCodeLabel = true;
                if (_codeLabelRequiresColon)
                {
                    if (_currentToken == ':')
                        more = getNextToken();
                    else
                    {
                        isCodeLabel = false;
                        more = true;
                        _currentToken = label;
                    }
                }
                if (isCodeLabel)
                    assignLabelValue(scopedLabelName(label), _currentCodeInstructionNumber);
            }

            if (!more)
                return true;

            mnemonic = _currentToken;
            opcode = Assembly::OpcodesKeyToValue(mnemonic.toUpper().toLocal8Bit());
            hasOpcode = Assembly::OpcodesValueIsValid(opcode);
        }

        if (tokenIsDirective())
        {
            QString directive(_currentToken);

            if (directive == ".byte")
            {
                getNextToken();
                bool ok;
                int value = getTokensExpressionValueAsInt(&ok);
                if (!ok)
                {
                    debugMessage(QString("Bad value: %1").arg(_currentToken));
                    return false;
                }
                Q_UNUSED(value)
                debugMessage(QString("Cannot yet implement directive: %1").arg(directive));
                return false;
            }
            else
            {
                debugMessage(QString("Unimplemented directive: %1").arg(directive));
                return false;
            }
        }
    }

    if (!hasOpcode)
    {
        debugMessage(QString("Unrecognized opcode mnemonic: %1").arg(mnemonic));
        return false;
    }

    QString opcodeName(Assembly::OpcodesValueToString(opcode));
    getNextToken();
    if (_currentToken.isEmpty())
    {
        operand.mode = AddressingMode::Implicit;
    }
    else if (_currentToken == '#')
    {
        operand.mode = AddressingMode::Immediate;
        getNextToken();
        bool ok;
        int value = getTokensExpressionValueAsInt(&ok);
        if (!ok)
        {
            debugMessage(QString("Bad value: %1").arg(_currentToken));
            return false;
        }
        if (value < -128 || value > 256)
            debugMessage(QString("Warning: Value out of range for opcode: $%1 %2").arg(value, 2, 16, QChar('0')).arg(opcodeName));
        operand.arg = value;
    }
    else if (_currentToken.toUpper() == "A")
    {
        operand.mode = AddressingMode::Accumulator;
        getNextToken();
    }
    else if (_currentToken == "(")
    {
        getNextToken();
        bool ok;
        int value = getTokensExpressionValueAsInt(&ok);
        if (!ok)
        {
            debugMessage(QString("Bad value: %1").arg(_currentToken));
            return false;
        }
        operand.arg = value;

        bool recognised = false;
        if (_currentToken == ',')
        {
            if (getNextToken() && _currentToken.toUpper() == "X")
                if (getNextToken() && _currentToken == ")")
                {
                    operand.mode = AddressingMode::IndexedIndirectX;
                    recognised = true;
                }
        }
        else if (_currentToken == ")")
        {
            operand.mode = AddressingMode::Indirect;
            recognised = true;
            if (getNextToken())
            {
                recognised = false;
                if (_currentToken == ",")
                    if (getNextToken() && _currentToken.toUpper() == "Y")
                    {
                        operand.mode = AddressingMode::IndirectIndexedY;
                        recognised = true;
                    }
            }
        }
        if (!recognised)
        {
            debugMessage(QString("Unrecognized operand addressing mode for opcode: %1 %2").arg(_currentToken).arg(opcodeName));
            return false;
        }
    }
    else if (tokenIsInt() || tokenIsLabel() || _currentToken == "*")
    {
        operand.mode = AddressingMode::Absolute;
        switch (opcode)
        {
        case Opcodes::BCC: case Opcodes::BCS: case Opcodes::BEQ: case Opcodes::BMI:
        case Opcodes::BNE: case Opcodes::BPL: case Opcodes::BVC: case Opcodes::BVS:
            operand.mode = AddressingMode::Relative;
            break;
        default: break;
        }

        bool ok;
        int value = getTokensExpressionValueAsInt(&ok);
        if (!ok)
        {
            debugMessage(QString("Bad value: %1").arg(_currentToken));
            return false;
        }
        operand.arg = value;

        if (!_currentToken.isEmpty())
        {
            bool recognised = false;
            if (_currentToken == ",")
            {
                recognised = true;
                getNextToken();
                if (_currentToken.toUpper() == "X")
                    operand.mode = AddressingMode::AbsoluteX;
                else if (_currentToken.toUpper() == "Y")
                    operand.mode = AddressingMode::AbsoluteY;
                else
                    recognised = false;
            }
            if (!recognised)
            {
                debugMessage(QString("Unrecognized operand absolute addressing mode for opcode: %1 %2").arg(_currentToken).arg(opcodeName));
                return false;
            }
        }
    }
    else
    {
        debugMessage(QString("Unrecognized operand addressing mode for opcode: %1 %2").arg(_currentToken).arg(opcodeName));
        return false;
    }

    bool zpArg = (operand.arg & 0xff00) == 0;
    int relative;
    switch (operand.mode)
    {
    case AddressingMode::Absolute:
        if (zpArg && Assembly::opcodeSupportsAddressingMode(opcode, AddressingMode::ZeroPage))
            operand.mode = AddressingMode::ZeroPage;
        break;
    case AddressingMode::AbsoluteX:
        if (zpArg && Assembly::opcodeSupportsAddressingMode(opcode, AddressingMode::ZeroPageX))
            operand.mode = AddressingMode::ZeroPageX;
        break;
    case AddressingMode::AbsoluteY:
        if (zpArg && Assembly::opcodeSupportsAddressingMode(opcode, AddressingMode::ZeroPageY))
            operand.mode = AddressingMode::ZeroPageY;
        break;
    case AddressingMode::IndexedIndirectX:
    case AddressingMode::IndirectIndexedY:
        if (!zpArg)
        {
            debugMessage(QString("Operand argument value must be ZeroPage: %1 %2").arg(Assembly::AddressingModeValueToString(operand.mode)).arg(opcodeName));
            return false;
        }
        break;
    case AddressingMode::Relative:
        relative = operand.arg - _currentCodeInstructionNumber;
        if (relative < -128 || relative > 127)
            if (_assembleState == Pass2)
            {
                debugMessage(QString("Relative mode branch address out of range: %1 %2").arg(relative).arg(opcodeName));
                return false;
            }
        operand.arg = relative;
        break;
    default: break;
    }

    if (getNextToken())
    {
        debugMessage(QString("Unexpected token at end of statement: %1 %2").arg(_currentToken).arg(opcodeName));
        return false;
    }

    if (!Assembly::opcodeSupportsAddressingMode(opcode, operand.mode))
    {
        debugMessage(QString("Opcode does not support operand addressing mode: %1 %2")
                         .arg(Assembly::OpcodesValueToString(opcode))
                         .arg(Assembly::AddressingModeValueToString(operand.mode)));
        return false;
    }

    return true;
}


bool Assembler::getNextLine()
{
    _currentLine.clear();
    while (_currentCodeLineNumber >= _codeLines.size())
    {
        if (!_codeStream->readLineInto(&_currentLine))
            return false;
        _codeLines.append(_currentLine);
    }
    if (_currentCodeLineNumber >= _codeLines.size())
        return false;
    _currentLine = _codeLines.at(_currentCodeLineNumber);
    _currentLineStream.setString(&_currentLine, QIODeviceBase::ReadOnly);
    return true;
}

bool Assembler::getNextToken(bool wantOperator /*= false*/)
{
    _currentToken.clear();
    QChar firstChar, nextChar;

    _currentLineStream >> firstChar;
    while (firstChar.isSpace())
        _currentLineStream >> firstChar;

    if (firstChar.isNull())
        return false;
    if (firstChar == ';')
    {
        QString comment;
        comment = _currentLineStream.readLine();
        return false;
    }

    _currentToken.append(firstChar);
    if (firstChar.isPunct() || firstChar.isSymbol())
        if (!(firstChar == '%' || firstChar == '$' || firstChar == '\'' || firstChar == '_' || firstChar == '.'
              || (!wantOperator && (firstChar == '-' || firstChar == '*'))
              || (wantOperator && (firstChar == '<' || firstChar == '>'))
              ))
            return true;
    _currentLineStream >> nextChar;
    if (firstChar == '\'')
    {
        if (!nextChar.isNull())
        {
            _currentToken.append(nextChar);
            _currentLineStream >> nextChar;
            if (nextChar == '\'')
            {
                _currentToken.append(nextChar);
                _currentLineStream >> nextChar;
            }
        }
    }
    else if (firstChar == '<' || firstChar == '>')
    {
        if (nextChar == firstChar)
        {
            _currentToken.append(nextChar);
            _currentLineStream >> nextChar;
        }
    }
    else
    {
        while (!(nextChar.isNull() || nextChar.isSpace() || (nextChar.isPunct() && nextChar != '_') || nextChar.isSymbol()))
        {
            _currentToken.append(nextChar);
            _currentLineStream >> nextChar;
        }
    }
    if (!nextChar.isNull())
        _currentLineStream.seek(_currentLineStream.pos() - 1);
    return true;
}

int Assembler::getTokensExpressionValueAsInt(bool *ok)
{
    const QList<QString> operators{ "+", "-", "&", "|", "<<", ">>", "*", "/" };

    bool _ok;
    if (ok == nullptr)
        ok = &_ok;
    *ok = false;
    if (_currentToken.isEmpty())
        return -1;
    int value = tokenValueAsInt(ok);
    if (!*ok)
        return -1;
    while (getNextToken(true))
    {
        QString _operator(_currentToken);
        if (!operators.contains(_operator))
            return value;

        getNextToken();
        int value2 = tokenValueAsInt(ok);
        if (!*ok)
            return -1;

        if (_operator == "+")
            value += value2;
        else if (_operator == "-")
            value -= value2;
        else if (_operator == "&")
            value &= value2;
        else if (_operator == "|")
            value |= value2;
        else if (_operator == "<<")
            value <<= value2;
        else if (_operator == ">>")
            value >>= value2;
        else if (_operator == "*")
            value *= value2;
        else if (_operator == "/")
        {
            if (value2 == 0)
            {
                debugMessage(QString("Warning: Division by zero"));
                *ok = false;
                return -1;
            }
            value /= value2;
        }
        else
        {
            *ok = false;
            return -1;
        }
    }
    return value;
}

bool Assembler::tokenIsDirective() const
{
    if (!_currentToken.startsWith('.'))
        return false;
    return _directives.contains(_currentToken);
}

bool Assembler::tokenIsLabel() const
{
    if (_currentToken.size() == 0)
        return false;
    QChar ch = _currentToken.at(0);
    if (!(ch == '.' || ch == '_' || ch.isLetter()))
        return false;
    if (ch == '.')
        if (tokenIsDirective())
            return false;
    for (int i = 1; i < _currentToken.size(); i++)
    {
        ch = _currentToken.at(i);
        if (!(ch == '_' || ch.isLetter() || ch.isDigit()))
            return false;
    }
    return true;
}

bool Assembler::tokenIsInt() const
{
    QChar firstChar = _currentToken.size() > 0 ? _currentToken.at(0) : QChar();
    return firstChar == '%' || firstChar == '$' || firstChar.isDigit() || firstChar == '-' || firstChar == '\'';
}

int Assembler::tokenToInt(bool *ok) const
{
    bool _ok;
    if (ok == nullptr)
        ok = &_ok;
    QChar firstChar = _currentToken.size() > 0 ? _currentToken.at(0) : QChar();
    if (firstChar == '%')
        return _currentToken.mid(1).toInt(ok, 2);
    else if (firstChar == '$')
        return _currentToken.mid(1).toInt(ok, 16);
    else if (firstChar.isDigit() || firstChar == '-')
        return _currentToken.toInt(ok, 10);
    else if (firstChar == '\'')
    {
        QChar nextChar = _currentToken.size() > 1 ? _currentToken.at(1) : QChar();
        QChar nextChar2 = _currentToken.size() > 2 ? _currentToken.at(2) : QChar();
        if (nextChar2 == '\'')
        {
            *ok = true;
            return nextChar.toLatin1();
        }
    }
    *ok = false;
    return -1;
}

int Assembler::tokenValueAsInt(bool *ok) const
{
    bool _ok;
    if (ok == nullptr)
        ok = &_ok;
    int value;
    value = tokenToInt(ok);
    if (*ok)
        return value;
    if (tokenIsLabel())
    {
        QString label = scopedLabelName(_currentToken);
        if (_codeLabels.contains(label))
        {
            *ok = true;
            return _codeLabels.value(label);
        }
        else if (_assembleState == Pass1)
        {
            *ok = true;
            return 0xffff;
        }
        sendMessageToConsole(QString("Label not defined: %1").arg(_currentToken));
    }
    else if (_currentToken == "*")
    {
        *ok = true;
        return _currentCodeInstructionNumber;
    }
    *ok = false;
    return -1;
}

QString Assembler::scopedLabelName(const QString &label) const
{
    if (!label.startsWith('.'))
        return label;
    if (_currentCodeLabelScope.isEmpty())
        debugMessage(QString("Warning: Local label not in any other label scope: %1").arg(label));
    return _currentCodeLabelScope + label;
}

void Assembler::assignLabelValue(const QString &scopedLabel, int value)
{
    if (scopedLabel.isEmpty())
        debugMessage(QString("Warning: Defining label with no name"));
    if (_codeLabels.value(scopedLabel, value) != value)
        debugMessage(QString("Warning: Label redefinition: %1").arg(scopedLabel));
    setCodeLabel(scopedLabel, value);
    if (!scopedLabel.contains('.'))
        _currentCodeLabelScope = scopedLabel;
}

