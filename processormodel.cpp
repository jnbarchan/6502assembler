#include <QBrush>
#include <QCoreApplication>
#include <QDebug>

#include "processormodel.h"

//
// Assembler Class
//

Assembler::OpcodesInfo Assembler::opcodesInfo[]
{
    { LDA, ImmZPXAbsXYIndXY },
    { LDX, ImmZPYAbsY },
    { LDY, ImmZPYAbsX },
    { STA, ZPXAbsXYIndXY },
    { STX, ZPYAbs },
    { STY, ZPXAbs },
    { TAX, Implicit },
    { TAY, Implicit },
    { TXA, Implicit },
    { TYA, Implicit },
    { TSX, Implicit },
    { TXS, Implicit },
    { PHA, Implicit },
    { PHP, Implicit },
    { PLA, Implicit },
    { PLP, Implicit },
    { AND, ImmZPXAbsXYIndXY },
    { EOR, ImmZPXAbsXYIndXY },
    { ORA, ImmZPXAbsXYIndXY },
    { BIT, ZPAbs },
    { ADC, ImmZPXAbsXYIndXY },
    { SBC, ImmZPXAbsXYIndXY },
    { CMP, ImmZPXAbsXYIndXY },
    { CPX, ImmZPAbs },
    { CPY, ImmZPAbs },
    { INC, ZPXAbsX },
    { INX, Implicit },
    { INY, Implicit },
    { DEC, ZPXAbsX },
    { DEX, Implicit },
    { DEY, Implicit },
    { ASL, AccZPXAbsX },
    { LSR, AccZPXAbsX },
    { ROL, AccZPXAbsX },
    { ROR, AccZPXAbsX },
    { JMP, AbsInd },
    { JSR, Absolute },
    { RTS, Implicit },
    { BCC, Relative },
    { BCS, Relative },
    { BEQ, Relative },
    { BMI, Relative },
    { BNE, Relative },
    { BPL, Relative },
    { BVC, Relative },
    { BVS, Relative },
    { CLC, Implicit },
    { CLD, Implicit },
    { CLI, Implicit },
    { CLV, Implicit },
    { SEC, Implicit },
    { SED, Implicit },
    { SEI, Implicit },
    { BRK, Implicit },
    { NOP, Implicit },
    { RTI, Implicit }
};

/*static*/ const Assembler::OpcodesInfo &Assembler::getOpcodeInfo(const Opcodes opcode)
{
    Q_ASSERT(opcode >= 0 && opcode < sizeof(opcodesInfo) / sizeof(opcodesInfo[0]));
    const OpcodesInfo &opcodeInfo(opcodesInfo[opcode]);
    return opcodesInfo[opcode];
}

/*static*/ bool Assembler::opcodeSupportsAddressingMode(const Opcodes opcode, AddressingMode mode)
{
    const OpcodesInfo &opcodeInfo(getOpcodeInfo(opcode));
    return opcodeInfo.modes & mode;
}


//
// ProcessorModel Class
//

#define __JSR_outch 0xefff

ProcessorModel::ProcessorModel(QObject *parent)
    : QObject{parent}
{
    _codeStream = nullptr;
    _memory.resize(64 * 1024);
    _memoryModel = new MemoryModel(this);
    resetModel();
    doneAssemblePass1 = false;
    _stopRun = true;
    _isRunning = false;
}

uint8_t ProcessorModel::accumulator() const
{
    return _accumulator;
}

void ProcessorModel::setAccumulator(uint8_t newAccumulator)
{
    _accumulator = newAccumulator;
    emit accumulatorChanged();
}

uint8_t ProcessorModel::xregister() const
{
    return _xregister;
}

void ProcessorModel::setXregister(uint8_t newXregister)
{
    _xregister = newXregister;
    emit xregisterChanged();
}

uint8_t ProcessorModel::yregister() const
{
    return _yregister;
}

void ProcessorModel::setYregister(uint8_t newYregister)
{
    _yregister = newYregister;
    emit yregisterChanged();
}

uint8_t ProcessorModel::stackRegister() const
{
    return _stackRegister;
}

void ProcessorModel::setStackRegister(uint8_t newStackRegister)
{
    _stackRegister = newStackRegister;
    emit stackRegisterChanged();
}

uint8_t ProcessorModel::pullFromStack()
{
    setStackRegister(_stackRegister + 1);
    return memoryByteAt(_stackBottom + _stackRegister);
}

void ProcessorModel::pushToStack(uint8_t value)
{
    setMemoryByteAt(_stackBottom + _stackRegister, value);
    setStackRegister(_stackRegister - 1);
}

bool ProcessorModel::isStackAddress(uint16_t address)
{
    return address >= _stackBottom && address < _stackBottom + 0x0100;
}

uint8_t ProcessorModel::statusFlags() const
{
    return _statusFlags;
}

void ProcessorModel::setStatusFlags(uint8_t newStatusFlags)
{
    _statusFlags = newStatusFlags;
    emit statusFlagsChanged();
}

uint8_t ProcessorModel::statusFlag(uint8_t flagBit) const
{
    return _statusFlags & flagBit;
}

void ProcessorModel::clearStatusFlag(uint8_t newFlagBit)
{
    _statusFlags &= ~newFlagBit;
    emit statusFlagsChanged();
}

void ProcessorModel::setStatusFlag(uint8_t newFlagBit)
{
    _statusFlags |= newFlagBit;
    emit statusFlagsChanged();
}

void ProcessorModel::setStatusFlag(uint8_t newFlagBit, bool on)
{
    if (on)
        setStatusFlag(newFlagBit);
    else
        clearStatusFlag(newFlagBit);
}

uint8_t ProcessorModel::memoryByteAt(uint16_t address) const
{
    return _memory.at(address);
}

void ProcessorModel::setMemoryByteAt(uint16_t address, uint8_t value)
{
    _memory[address] = value;
    emit memoryChanged(address);
}

uint16_t ProcessorModel::memoryWordAt(uint16_t address) const
{
    return _memory.at(address) | (_memory.at(uint16_t(address + 1)) << 8);
}

uint16_t ProcessorModel::memoryZPWordAt(uint8_t address) const
{
    return _memory.at(address) | (_memory.at(uint8_t(address + 1)) << 8);
}

unsigned int ProcessorModel::memorySize() const
{
    return _memory.size();
}

int ProcessorModel::currentCodeLineNumber() const
{
    return _currentCodeLineNumber;
}

void ProcessorModel::setCurrentCodeLineNumber(int newCurrentCodeLineNumber)
{
    _currentCodeLineNumber = newCurrentCodeLineNumber;
    emit currentCodeLineNumberChanged(_currentCodeLineNumber);
}

void ProcessorModel::setCodeLabel(const QString &key, int value)
{
    _codeLabels[key] = value;
}

void ProcessorModel::setCode(QTextStream *codeStream)
{
    _codeStream = codeStream;
}

void ProcessorModel::resetModel()
{
    _stackRegister = 0xFD;
    _statusFlags = 0x00;
    _accumulator = 5;
    _xregister = 10;
    _yregister = 15;
    emit modelReset();
    _memoryModel->clearLastMemoryChanged();
}


bool ProcessorModel::isRunning() const
{
    return _isRunning;
}

void ProcessorModel::setIsRunning(bool newIsRunning)
{
    _isRunning = newIsRunning;
}

bool ProcessorModel::stopRun() const
{
    return _stopRun;
}

void ProcessorModel::setStopRun(bool newStopRun)
{
    _stopRun = newStopRun;
    emit stopRunChanged();
}


void ProcessorModel::debugMessage(const QString &message)
{
    qDebug() << message;
    emit sendMessageToConsole(message);
}


/*slot*/ void ProcessorModel::restart(bool assemblePass2 /*= false*/)
{
    if (_isRunning)
        return;
    Q_ASSERT(_codeStream);
    setCurrentCodeLineNumber(0);
    _currentToken.clear();

    if (!assemblePass2)
    {
        _codeLines.clear();
        _codeStream->seek(0);
        _codeLabels.clear();
        _codeLabels["__outch"] = __JSR_outch;
        doneAssemblePass1 = false;
    }
    resetModel();
    setStopRun(false);
}

/*slot*/ void ProcessorModel::stop()
{
    setStopRun(true);
}

/*slot*/ void ProcessorModel::run(bool stepOver, bool stepOut)
{
    if (_isRunning)
        return;
    bool step = stepOver || stepOut;
    if (!step || !doneAssemblePass1)
    {
        restart();
        assemblePass1();
        if (!doneAssemblePass1)
            return;
        restart(true);
    }
    if (stopRun())
        return;

    int stopAtLineNumber = -1;
    bool keepGoing = true;
    int count = 0;
    const int processEventsEverySoOften = 1000;
    setIsRunning(true);
    while (!stopRun() && keepGoing)
    {
        Opcodes opcode;
        OpcodeOperand operand;
        bool hasOpcode;
        if (!prepareRunNextStatement(opcode, operand, hasOpcode))
            break;
        if (step && stopAtLineNumber < 0)
        {
            keepGoing = false;
            if (stepOver && hasOpcode && opcode == Opcodes::JSR)
            {
                stopAtLineNumber = _currentCodeLineNumber + 1;
                keepGoing = true;
            }
            else if (stepOut)
            {
                uint16_t rtsAddress = memoryByteAt(_stackBottom + uint8_t(_stackRegister + 1)) << 8;
                rtsAddress |= memoryByteAt(_stackBottom + uint8_t(_stackRegister + 2));
                stopAtLineNumber = rtsAddress;
                keepGoing = true;
            }
        }

        if (!hasOpcode)
            setCurrentCodeLineNumber(_currentCodeLineNumber + 1);
        else
            runNextStatement(opcode, operand);

        if (step)
            if (_currentCodeLineNumber == stopAtLineNumber)
                keepGoing = false;

        count++;
        if (processEventsEverySoOften != 0 && count % processEventsEverySoOften == 0)
            QCoreApplication::processEvents();//TEMPORARY
    }
    setIsRunning(false);
}

/*slot*/ void ProcessorModel::step()
{
    if (_isRunning)
        return;
    if (stopRun())
        return;

    Opcodes opcode;
    OpcodeOperand operand;
    bool hasOpcode;
    if (!prepareRunNextStatement(opcode, operand, hasOpcode))
        return;
    if (!hasOpcode)
        setCurrentCodeLineNumber(_currentCodeLineNumber + 1);
    else
        runNextStatement(opcode, operand);
}


bool ProcessorModel::prepareRunNextStatement(Opcodes &opcode, OpcodeOperand &operand, bool &hasOpcode)
{
    hasOpcode = false;
    if (stopRun())
        return false;

    //
    // ASSEMBLING PHASE
    //
    if (!doneAssemblePass1)
    {
        assemblePass1();
        if (!doneAssemblePass1)
            return false;
        restart(true);
    }
    bool blankLine, eof;
    if (!assembleNextStatement(opcode, operand, hasOpcode, blankLine, eof))
    {
        setStopRun(true);
        return false;
    }
    return true;
}

void ProcessorModel::runNextStatement(const Opcodes &opcode, const OpcodeOperand &operand)
{
    if (stopRun())
        return;

    //
    // EXECUTION PHASE
    //
    executeNextStatement(opcode, operand);
}


void ProcessorModel::assemblePass1()
{
    doneAssemblePass1 = false;
    Opcodes opcode;
    OpcodeOperand operand;
    bool hasOpcode, blankLine, eof;
    while (assembleNextStatement(opcode, operand, hasOpcode, blankLine, eof))
        setCurrentCodeLineNumber(_currentCodeLineNumber + 1);
    if (!eof)
        return;
    doneAssemblePass1 = true;
}

bool ProcessorModel::assembleNextStatement(Opcodes &opcode, OpcodeOperand &operand, bool &hasOpcode, bool &blankLine, bool &eof)
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
    opcode = Assembler::OpcodesKeyToValue(mnemonic.toUpper().toLocal8Bit());
    hasOpcode = Assembler::OpcodesValueIsValid(opcode);
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
                    assignLabelValue(label, value);
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
                    assignLabelValue(label, _currentCodeLineNumber);
            }

            if (!more)
                return true;

            mnemonic = _currentToken;
            opcode = Assembler::OpcodesKeyToValue(mnemonic.toUpper().toLocal8Bit());
            hasOpcode = Assembler::OpcodesValueIsValid(opcode);
        }
    }

    if (!hasOpcode)
    {
        debugMessage(QString("Unrecognized opcode mnemonic: %1").arg(mnemonic));
        return false;
    }

    QString opcodeName(Assembler::OpcodesValueToString(opcode));
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
        if (zpArg && Assembler::opcodeSupportsAddressingMode(opcode, AddressingMode::ZeroPage))
            operand.mode = AddressingMode::ZeroPage;
        break;
    case AddressingMode::AbsoluteX:
        if (zpArg && Assembler::opcodeSupportsAddressingMode(opcode, AddressingMode::ZeroPageX))
            operand.mode = AddressingMode::ZeroPageX;
        break;
    case AddressingMode::AbsoluteY:
        if (zpArg && Assembler::opcodeSupportsAddressingMode(opcode, AddressingMode::ZeroPageY))
            operand.mode = AddressingMode::ZeroPageY;
        break;
    case AddressingMode::IndexedIndirectX:
    case AddressingMode::IndirectIndexedY:
        if (!zpArg)
        {
            debugMessage(QString("Operand argument value must be ZeroPage: %1 %2").arg(Assembler::AddressingModeValueToString(operand.mode)).arg(opcodeName));
            return false;
        }
        break;
    case AddressingMode::Relative:
        relative = operand.arg - _currentCodeLineNumber;
        if (relative < -128 || relative > 127)
            if (doneAssemblePass1)
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

    if (!Assembler::opcodeSupportsAddressingMode(opcode, operand.mode))
    {
        debugMessage(QString("Opcode does not support operand addressing mode: %1 %2")
                         .arg(Assembler::OpcodesValueToString(opcode))
                         .arg(Assembler::AddressingModeValueToString(operand.mode)));
        return false;
    }

    return true;
}

bool ProcessorModel::getNextLine()
{
    _currentLine.clear();
    while (_currentCodeLineNumber >= _codeLines.length())
    {
        if (!_codeStream->readLineInto(&_currentLine))
            return false;
        _codeLines.append(_currentLine);
    }
    if (_currentCodeLineNumber >= _codeLines.length())
        return false;
    _currentLine = _codeLines.at(_currentCodeLineNumber);
    _currentLineStream.setString(&_currentLine, QIODeviceBase::ReadOnly);
    return true;
}

bool ProcessorModel::getNextToken(bool wantOperator /*= false*/)
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
        _currentLineStream >> comment;
        return false;
    }

    _currentToken.append(firstChar);
    if (firstChar.isPunct() || firstChar.isSymbol())
        if (!(firstChar == '%' || firstChar == '$' || firstChar == '\''|| firstChar == '_'
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

int ProcessorModel::getTokensExpressionValueAsInt(bool *ok)
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

bool ProcessorModel::tokenIsLabel() const
{
    if (_currentToken.length() == 0)
        return false;
    QChar ch = _currentToken.at(0);
    if (ch != '_' && !ch.isLetter())
        return false;
    for (int i = 1; i < _currentToken.length(); i++)
    {
        ch = _currentToken.at(i);
        if (ch != '_' && !ch.isLetter() && !ch.isDigit())
            return false;
    }
    return true;
}

bool ProcessorModel::tokenIsInt() const
{
    QChar firstChar = _currentToken.length() > 0 ? _currentToken.at(0) : QChar();
    return firstChar == '%' || firstChar == '$' || firstChar.isDigit() || firstChar == '-' || firstChar == '\'';
}

int ProcessorModel::tokenToInt(bool *ok) const
{
    bool _ok;
    if (ok == nullptr)
        ok = &_ok;
    QChar firstChar = _currentToken.length() > 0 ? _currentToken.at(0) : QChar();
    if (firstChar == '%')
        return _currentToken.mid(1).toInt(ok, 2);
    else if (firstChar == '$')
        return _currentToken.mid(1).toInt(ok, 16);
    else if (firstChar.isDigit() || firstChar == '-')
        return _currentToken.toInt(ok, 10);
    else if (firstChar == '\'')
    {
        QChar nextChar = _currentToken.length() > 1 ? _currentToken.at(1) : QChar();
        QChar nextChar2 = _currentToken.length() > 2 ? _currentToken.at(2) : QChar();
        if (nextChar2 == '\'')
        {
            *ok = true;
            return nextChar.toLatin1();
        }
    }
    *ok = false;
    return -1;
}

int ProcessorModel::tokenValueAsInt(bool *ok) const
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
        if (_codeLabels.contains(_currentToken))
        {
            *ok = true;
            return _codeLabels.value(_currentToken);
        }
        else if (!doneAssemblePass1)
        {
            *ok = true;
            return 0;
        }
        sendMessageToConsole(QString("Label not defined: %1").arg(_currentToken));
    }
    else if (_currentToken == "*")
    {
        *ok = true;
        return _currentCodeLineNumber;
    }
    *ok = false;
    return -1;
}

void ProcessorModel::assignLabelValue(const QString &label, int value)
{
    if (_codeLabels.value(label, value) != value)
        debugMessage(QString("Warning: Label redefinition: %1").arg(label));
    setCodeLabel(label, value);
}


void ProcessorModel::executeNextStatement(const Opcodes &opcode, const OpcodeOperand &operand)
{
    //
    // EXECUTION PHASE
    //

    uint8_t _argValue = -1;
    uint16_t _argAddress = -1;
    switch (operand.mode)
    {
    case AddressingMode::Implicit:
        break;
    case AddressingMode::Accumulator:
        _argValue = _accumulator;
        break;
    case AddressingMode::Immediate:
        _argValue = operand.arg;
        break;
    case AddressingMode::Relative:
        _argAddress = _currentCodeLineNumber + int8_t(operand.arg);
        break;
    case AddressingMode::Absolute:
    case AddressingMode::AbsoluteX:
    case AddressingMode::AbsoluteY:
    case AddressingMode::ZeroPage:
    case AddressingMode::ZeroPageX:
    case AddressingMode::ZeroPageY:
    case AddressingMode::Indirect:
    case AddressingMode::IndexedIndirectX:
    case AddressingMode::IndirectIndexedY:
        _argAddress = operand.arg;
        if (operand.mode == AddressingMode::Absolute)
            ;
        else if (operand.mode == AddressingMode::ZeroPage)
            _argAddress = uint8_t(_argAddress);
        else if (operand.mode == AddressingMode::AbsoluteX)
            _argAddress += _xregister;
        else if (operand.mode == AddressingMode::ZeroPageX)
            _argAddress = uint8_t(_argAddress + _xregister);
        else if (operand.mode == AddressingMode::AbsoluteY)
            _argAddress += _yregister;
        else if (operand.mode == AddressingMode::ZeroPageY)
            _argAddress = uint8_t(_argAddress + _yregister);
        else if (operand.mode == AddressingMode::Indirect)
            _argAddress = memoryWordAt(_argAddress);
        else if (operand.mode == AddressingMode::IndexedIndirectX)
            _argAddress = memoryZPWordAt(_argAddress + _xregister);
        else if (operand.mode == AddressingMode::IndirectIndexedY)
            _argAddress = memoryWordAt(_argAddress) + _yregister;
        switch (opcode)
        {
        case Opcodes::STA: case Opcodes::STX: case Opcodes::STY:
        case Opcodes::JMP: case Opcodes::JSR:
            break;
        default:
            _argValue = memoryByteAt(_argAddress);
            break;
        }
        break;
    default:
        debugMessage(QString("EXECUTION: Unimplemented operand addressing mode: %1").arg(Assembler::AddressingModeValueToString(operand.mode)));
        setStopRun(true);
        return;
    }

    if (!Assembler::opcodeSupportsAddressingMode(opcode, operand.mode))
    {
        debugMessage(QString("EXECUTION: Opcode does not support operand addressing mode: %1 %2")
                         .arg(Assembler::OpcodesValueToString(opcode))
                         .arg(Assembler::AddressingModeValueToString(operand.mode)));
        setStopRun(true);
        return;
    }

    setCurrentCodeLineNumber(_currentCodeLineNumber + 1);

    clearStatusFlag(StatusFlags::Break);

    const uint8_t argValue{_argValue};
    const uint16_t argAddress{_argAddress};
    uint8_t tempValue8, origTempValue8;
    uint16_t tempValue16;

    // Opcodes ordered/grouped as per http://www.6502.org/users/obelisk/6502/instructions.html
    switch (opcode)
    {
    case Opcodes::LDA:
        setAccumulator(argValue);
        setNZStatusFlags(_accumulator);
        break;
    case Opcodes::LDX:
        setXregister(argValue);
        setNZStatusFlags(_xregister);
        break;
    case Opcodes::LDY:
        setYregister(argValue);
        setNZStatusFlags(_yregister);
        break;
    case Opcodes::STA:
        setMemoryByteAt(argAddress, _accumulator);
        break;
    case Opcodes::STX:
        setMemoryByteAt(argAddress, _xregister);
        break;
    case Opcodes::STY:
        setMemoryByteAt(argAddress, _yregister);
        break;

    case Opcodes::TAX:
        setXregister(_accumulator);
        setNZStatusFlags(_accumulator);
        break;
    case Opcodes::TAY:
        setYregister(_accumulator);
        setNZStatusFlags(_accumulator);
        break;
    case Opcodes::TXA:
        setAccumulator(_xregister);
        setNZStatusFlags(_xregister);
        break;
    case Opcodes::TYA:
        setAccumulator(_yregister);
        setNZStatusFlags(_yregister);
        break;

    case Opcodes::TSX:
        setXregister(_stackRegister);
        setNZStatusFlags(_xregister);
        break;
    case Opcodes::TXS:
        setStackRegister(_xregister);
        break;
    case Opcodes::PHA:
        pushToStack(_accumulator);
        break;
    case Opcodes::PHP:
        pushToStack(_statusFlags);
        break;
    case Opcodes::PLA:
        setAccumulator(pullFromStack());
        setNZStatusFlags(_xregister);
        break;
    case Opcodes::PLP:
        setStatusFlags(pullFromStack());
        break;

    case Opcodes::AND:
        setAccumulator(_accumulator & argValue);
        setNZStatusFlags(_accumulator);
        break;
    case Opcodes::EOR:
        setAccumulator(_accumulator ^ argValue);
        setNZStatusFlags(_accumulator);
        break;
    case Opcodes::ORA:
        setAccumulator(_accumulator | argValue);
        setNZStatusFlags(_accumulator);
        break;
    case Opcodes::BIT:
        setStatusFlag(StatusFlags::Zero, (_accumulator & argValue) == 0);
        setStatusFlag(StatusFlags::Overflow, argValue & 0x40);
        setStatusFlag(StatusFlags::Negative, argValue & 0x80);
        break;

    case Opcodes::ADC:
        // sum = (A + M + C)
        tempValue16 = _accumulator + argValue + statusFlag(StatusFlags::Carry);
        // C = sum > 0xFF
        setStatusFlag(StatusFlags::Carry, tempValue16 > 0xff);
        // V = (~(A ^ M) & (A ^ R) & 0x80) != 0;
        setStatusFlag(StatusFlags::Overflow, ~(_accumulator ^ argValue) & (_accumulator ^ (tempValue16 & 0xff)) & 0x80);
        tempValue8 = tempValue16;
        setAccumulator(tempValue8);
        setNZStatusFlags(_accumulator);
        break;
    case Opcodes::SBC:
        // sum = (A + ~M + C)
        tempValue16 = _accumulator + (argValue ^ 0xff) + statusFlag(StatusFlags::Carry);
        // C = sum > 0xFF
        setStatusFlag(StatusFlags::Carry, tempValue16 > 0xff);
        // V = ((A ^ M) & (A ^ R) & 0x80) != 0;
        setStatusFlag(StatusFlags::Overflow, (_accumulator ^ argValue) & (_accumulator ^ (tempValue16 & 0xff)) & 0x80);
        tempValue8 = tempValue16;
        setAccumulator(tempValue8);
        setNZStatusFlags(_accumulator);
        break;
    case Opcodes::CMP:
        // (A - M)
        tempValue16 = _accumulator - argValue;
        setStatusFlag(StatusFlags::Carry, tempValue16 <= 0xff);
        tempValue8 = tempValue16;
        setNZStatusFlags(tempValue8);
        break;
    case Opcodes::CPX:
        // (X - M)
        tempValue16 = _xregister - argValue;
        setStatusFlag(StatusFlags::Carry, tempValue16 <= 0xff);
        tempValue8 = tempValue16;
        setNZStatusFlags(tempValue8);
        break;
    case Opcodes::CPY:
        // (Y - M)
        tempValue16 = _yregister - argValue;
        setStatusFlag(StatusFlags::Carry, tempValue16 <= 0xff);
        tempValue8 = tempValue16;
        setNZStatusFlags(tempValue8);
        break;

    case Opcodes::INC:
        tempValue8 = argValue + 1;
        setMemoryByteAt(argAddress, tempValue8);
        setNZStatusFlags(tempValue8);
        break;
    case Opcodes::INX:
        setXregister(_xregister + 1);
        setNZStatusFlags(_xregister);
        break;
    case Opcodes::INY:
        setYregister(_yregister + 1);
        setNZStatusFlags(_yregister);
        break;
    case Opcodes::DEC:
        tempValue8 = argValue - 1;
        setMemoryByteAt(argAddress, tempValue8);
        setNZStatusFlags(tempValue8);
        break;
    case Opcodes::DEX:
        setXregister(_xregister - 1);
        setNZStatusFlags(_xregister);
        break;
    case Opcodes::DEY:
        setYregister(_yregister - 1);
        setNZStatusFlags(_yregister);
        break;

    case Opcodes::ASL:
        origTempValue8 = tempValue8 = (operand.mode == AddressingMode::Accumulator) ? _accumulator : argValue;
        tempValue8 <<= 1;
        setStatusFlag(StatusFlags::Carry, origTempValue8 & 0x80);
        if (operand.mode == AddressingMode::Accumulator)
            setAccumulator(tempValue8);
        else
            setMemoryByteAt(argAddress, tempValue8);
        setNZStatusFlags(tempValue8);
        break;
    case Opcodes::LSR:
        origTempValue8 = tempValue8 = (operand.mode == AddressingMode::Accumulator) ? _accumulator : argValue;
        tempValue8 >>= 1;
        setStatusFlag(StatusFlags::Carry, origTempValue8 & 0x01);
        if (operand.mode == AddressingMode::Accumulator)
            setAccumulator(tempValue8);
        else
            setMemoryByteAt(argAddress, tempValue8);
        setNZStatusFlags(tempValue8);
        break;
    case Opcodes::ROL:
        origTempValue8 = tempValue8 = (operand.mode == AddressingMode::Accumulator) ? _accumulator : argValue;
        tempValue8 <<= 1;
        tempValue8 |= statusFlag(StatusFlags::Carry);
        setStatusFlag(StatusFlags::Carry, origTempValue8 & 0x80);
        if (operand.mode == AddressingMode::Accumulator)
            setAccumulator(tempValue8);
        else
            setMemoryByteAt(argAddress, tempValue8);
        setNZStatusFlags(tempValue8);
        break;
    case Opcodes::ROR:
        origTempValue8 = tempValue8 = (operand.mode == AddressingMode::Accumulator) ? _accumulator : argValue;
        tempValue8 >>= 1;
        tempValue8 |= (statusFlag(StatusFlags::Carry) << 7);
        setStatusFlag(StatusFlags::Carry, origTempValue8 & 0x01);
        if (operand.mode == AddressingMode::Accumulator)
            setAccumulator(tempValue8);
        else
            setMemoryByteAt(argAddress, tempValue8);
        setNZStatusFlags(tempValue8);
        break;

    case Opcodes::JMP:
        jumpTo(argAddress);
        break;
    case Opcodes::JSR:
        tempValue16 = _currentCodeLineNumber;
        pushToStack(uint8_t(tempValue16));
        pushToStack(uint8_t(tempValue16 >> 8));
        jumpTo(argAddress);
        break;
    case Opcodes::RTS:
        tempValue16 = pullFromStack() << 8;
        tempValue16 |= pullFromStack();
        jumpTo(tempValue16);
        break;

    case Opcodes::BCC:
        if (!statusFlag(StatusFlags::Carry))
            jumpTo(argAddress);
        break;
    case Opcodes::BCS:
        if (statusFlag(StatusFlags::Carry))
            jumpTo(argAddress);
        break;
    case Opcodes::BEQ:
        if (statusFlag(StatusFlags::Zero))
            jumpTo(argAddress);
        break;
    case Opcodes::BMI:
        if (statusFlag(StatusFlags::Negative))
            jumpTo(argAddress);
    case Opcodes::BNE:
        if (!statusFlag(StatusFlags::Zero))
            jumpTo(argAddress);
        break;
    case Opcodes::BPL:
        if (!statusFlag(StatusFlags::Negative))
            jumpTo(argAddress);
        break;
    case Opcodes::BVC:
        if (!statusFlag(StatusFlags::Overflow))
            jumpTo(argAddress);
        break;
    case Opcodes::BVS:
        if (statusFlag(StatusFlags::Overflow))
            jumpTo(argAddress);
        break;

    case Opcodes::CLC:
        clearStatusFlag(StatusFlags::Carry);
        break;
    case Opcodes::CLV:
        clearStatusFlag(StatusFlags::Overflow);
        break;
    case Opcodes::SEC:
        setStatusFlag(StatusFlags::Carry);
        break;

    case Opcodes::BRK:
        setStatusFlag(StatusFlags::Break);
        setStopRun(true);
        break;
    case Opcodes::NOP:
        break;

    default:
        debugMessage(QString("EXECUTION: Unimplemented opcode: %1").arg(Assembler::OpcodesValueToString(opcode)));
        setStopRun(true);
        return;
    }
}

void ProcessorModel::setNZStatusFlags(uint8_t value)
{
    setStatusFlag(StatusFlags::Negative, value & 0x80);
    setStatusFlag(StatusFlags::Zero, value == 0);
}

void ProcessorModel::jumpTo(uint16_t lineNumber)
{
    if (lineNumber == __JSR_outch)
    {
        jsr_outch();
        lineNumber = (pullFromStack() << 8) | pullFromStack();
    }
    if (lineNumber < 0 || lineNumber > _codeLines.length())
    {
        debugMessage(QString("Jump to line number out of range: %1").arg(lineNumber));
        setStopRun(true);
        return;
    }
    setCurrentCodeLineNumber(lineNumber);
}

void ProcessorModel::jsr_outch()
{
    emit sendCharToConsole(accumulator());
}


//
// MemoryModel Class
//

MemoryModel::MemoryModel(QObject *parent) : QAbstractTableModel(parent)
{
    processorModel = static_cast<ProcessorModel *>(parent);
    connect(processorModel, &ProcessorModel::memoryChanged, this, &MemoryModel::memoryChanged);
    lastMemoryChangedAddress = -1;
}

/*virtual*/ int MemoryModel::rowCount(const QModelIndex &parent /*= QModelIndex()*/) const /*override*/
{
    return (processorModel->memorySize() + columnCount() - 1) / columnCount();
}

/*virtual*/ int MemoryModel::columnCount(const QModelIndex &parent /*= QModelIndex()*/) const /*override*/
{
    return 16;
}

/*virtual*/ QVariant MemoryModel::data(const QModelIndex &index, int role) const /*override*/
{
    if (!index.isValid())
        return QVariant();
    int offset = index.row() * columnCount(index) + index.column();
    if (offset < 0 || offset >= processorModel->memorySize())
        return QVariant();
    switch (role)
    {
    case Qt::TextAlignmentRole:
        return QVariant(Qt::AlignRight | Qt::AlignVCenter);
    case Qt::DisplayRole:
    case Qt::EditRole:
        return processorModel->memoryByteAt(offset);
    case Qt::ForegroundRole:
        if (lastMemoryChangedAddress >= 0 && indexToAddress(index) == lastMemoryChangedAddress)
            return QBrush(Qt::red);
        break;
    }
    return QVariant();
}

/*virtual*/ QVariant MemoryModel::headerData(int section, Qt::Orientation orientation, int role) const /*override*/
{
    if (role == Qt::TextAlignmentRole)
        return Qt::AlignCenter;
    else if (role == Qt::DisplayRole)
    {
        if (orientation == Qt::Orientation::Horizontal)
            return QStringLiteral("%1").arg(section, 2, 16, QChar('0')).toUpper();
        else if (orientation == Qt::Orientation::Vertical)
            return QStringLiteral("%1").arg(section * columnCount(), 4, 16, QChar('0')).toUpper();
    }
    return QAbstractTableModel::headerData(section, orientation, role);
}

/*slot*/ void MemoryModel::clearLastMemoryChanged()
{
    if (lastMemoryChangedAddress >= 0)
    {
        QModelIndex ix = addressToIndex(lastMemoryChangedAddress);
        emit dataChanged(ix, ix, { Qt::ForegroundRole });
        lastMemoryChangedAddress = -1;
    }
}

/*slot*/ void MemoryModel::memoryChanged(uint16_t address)
{
    clearLastMemoryChanged();
    lastMemoryChangedAddress = address;
    QModelIndex ix = addressToIndex(address);
    emit dataChanged(ix, ix, { Qt::DisplayRole, Qt::EditRole, Qt::ForegroundRole });
}

