#include <QBrush>
#include <QDateTime>
#include <QCoreApplication>
#include <QDebug>

#include "processormodel.h"


//
// ProcessorModel Class
//

ProcessorModel::ProcessorModel(QObject *parent)
    : QObject{parent}
{
    _memory = QByteArray::fromRawData(_memoryData, sizeof(_memoryData));
    _memory.fill(0xa5);
    _memoryModel = new MemoryModel(this);
    resetModel();
    _instructions = nullptr;
    _startNewRun = true;
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
    return _memory.at(address) | (_memory.at(static_cast<uint16_t>(address + 1)) << 8);
}

uint16_t ProcessorModel::memoryZPWordAt(uint8_t address) const
{
    return _memory.at(address) | (_memory.at(static_cast<uint8_t>(address + 1)) << 8);
}

unsigned int ProcessorModel::memorySize() const
{
    return _memory.size();
}

const QList<Instruction> *ProcessorModel::instructions() const
{
    return _instructions;
}

void ProcessorModel::setInstructions(const QList<Instruction> *newInstructions)
{
    _instructions = newInstructions;
    setStartNewRun(true);
}

int ProcessorModel::currentInstructionNumber() const
{
    return _currentInstructionNumber;
}

void ProcessorModel::setCurrentInstructionNumber(int newCurrentInstructionNumber)
{
    _currentInstructionNumber = newCurrentInstructionNumber;
    emit currentInstructionNumberChanged(_currentInstructionNumber);
}

const QList<uint16_t> *ProcessorModel::breakpoints() const
{
    return _breakpoints;
}

void ProcessorModel::setBreakpoints(const QList<uint16_t> *newBreakpoints)
{
    _breakpoints = newBreakpoints;
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
    if (newStopRun != _stopRun)
    {
        _stopRun = newStopRun;
        emit stopRunChanged();
    }
    if (_stopRun)
        setStartNewRun(true);
}

bool ProcessorModel::startNewRun() const
{
    return _startNewRun;
}

void ProcessorModel::setStartNewRun(bool newStartNewRun)
{
    _startNewRun = newStartNewRun;
}

const Instruction *ProcessorModel::nextInstructionToExecute() const
{
    int instructionNumber;
    if (startNewRun() || stopRun())
        instructionNumber = 0;
    else
        instructionNumber = _currentInstructionNumber;
    if (instructionNumber < 0 || instructionNumber >= _instructions->size())
        return nullptr;
    return &_instructions->at(instructionNumber);
}


void ProcessorModel::debugMessage(const QString &message) const
{
    QString message2(message);
    QString location;
    if (_currentInstructionNumber >= 0)
        location = QString("[instruction #%2]").arg(_currentInstructionNumber);
    if (!location.isEmpty())
        message2 = location + " " + message2;
    qDebug() << message2;
    emit sendMessageToConsole(message2, Qt::red);
}

void ProcessorModel::executionError(const QString &message) const
{
    debugMessage("Execution Error: " + message);
}


/*slot*/ void ProcessorModel::restart()
{
    if (_isRunning)
        return;

    resetModel();
    setStopRun(false);
    setStartNewRun(true);
}

/*slot*/ void ProcessorModel::endRun()
{
    if (_isRunning)
        return;

    restart();
}

/*slot*/ void ProcessorModel::stop()
{
    setStopRun(true);
}

/*slot*/ void ProcessorModel::run()
{
    run(Run);
}

void ProcessorModel::continueRun()
{
    run(Continue);
}

/*slot*/ void ProcessorModel::stepInto()
{
    run(StepInto);
}

/*slot*/ void ProcessorModel::stepOver()
{
    run(StepOver);
}

/*slot*/ void ProcessorModel::stepOut()
{
    run(StepOut);
}


void ProcessorModel::run(RunMode runMode)
{
    if (_isRunning)
        return;

    bool startedNewRun = false;
    bool step = runMode == StepInto || runMode == StepOut || runMode == StepOver;
    if (runMode == Run || startNewRun() || stopRun())
    {
        restart();
        elapsedTimer.start();
        setCurrentInstructionNumber(0);
        setStartNewRun(false);
        startedNewRun = true;
        if (runMode == Continue)
            runMode = Run;
    }
    if (stopRun())
        return;

    int count = 0;
    const int processEventsEverySoOften = 1000;

    setIsRunning(true);

    int stopAtInstructionNumber = -1;
    bool keepGoing = true;
    if (startedNewRun)
        if (_breakpoints->contains(_currentInstructionNumber))
            keepGoing = false;
    while (!stopRun() && keepGoing && _currentInstructionNumber < _instructions->size())
    {
        const Instruction &instruction(_instructions->at(_currentInstructionNumber));
        const Opcodes &opcode(instruction.opcode);
        const OpcodeOperand &operand(instruction.operand);
        keepGoing = !step || stopAtInstructionNumber >= 0;
        if (step && stopAtInstructionNumber < 0)
        {
            if (runMode == StepOver && opcode == Opcodes::JSR)
            {
                stopAtInstructionNumber = _currentInstructionNumber + 1;
                keepGoing = true;
            }
            else if (runMode == StepOut)
            {
                uint16_t rtsAddress = memoryByteAt(_stackBottom + static_cast<uint8_t>(_stackRegister + 1)) << 8;
                rtsAddress |= memoryByteAt(_stackBottom + static_cast<uint8_t>(_stackRegister + 2));
                stopAtInstructionNumber = rtsAddress;
                keepGoing = true;
            }
        }

        runNextInstruction(opcode, operand);

        if (_currentInstructionNumber == stopAtInstructionNumber || _breakpoints->contains(_currentInstructionNumber))
            keepGoing = false;

        count++;
        if (processEventsEverySoOften != 0 && count % processEventsEverySoOften == 0)
            QCoreApplication::processEvents();//TEMPORARY
    }

    setIsRunning(false);
}

void ProcessorModel::runNextInstruction(const Opcodes &opcode, const OpcodeOperand &operand)
{
    if (stopRun())
        return;

    //
    // EXECUTION PHASE
    //
    executeNextInstruction(opcode, operand);
}


void ProcessorModel::executeNextInstruction(const Opcodes &opcode, const OpcodeOperand &operand)
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
        _argAddress = _currentInstructionNumber + int8_t(operand.arg);
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
            _argAddress = static_cast<uint8_t>(_argAddress);
        else if (operand.mode == AddressingMode::AbsoluteX)
            _argAddress += _xregister;
        else if (operand.mode == AddressingMode::ZeroPageX)
            _argAddress = static_cast<uint8_t>(_argAddress + _xregister);
        else if (operand.mode == AddressingMode::AbsoluteY)
            _argAddress += _yregister;
        else if (operand.mode == AddressingMode::ZeroPageY)
            _argAddress = static_cast<uint8_t>(_argAddress + _yregister);
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
        executionError(QString("Unimplemented operand addressing mode: %1").arg(Assembly::AddressingModeValueToString(operand.mode)));
        setStopRun(true);
        return;
    }

    if (!Assembly::opcodeSupportsAddressingMode(opcode, operand.mode))
    {
        executionError(QString("Opcode does not support operand addressing mode: %1 %2")
                         .arg(Assembly::OpcodesValueToString(opcode))
                         .arg(Assembly::AddressingModeValueToString(operand.mode)));
        setStopRun(true);
        return;
    }

    setCurrentInstructionNumber(_currentInstructionNumber + 1);

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
        tempValue16 = _currentInstructionNumber;
        pushToStack(static_cast<uint8_t>(tempValue16));
        pushToStack(static_cast<uint8_t>(tempValue16 >> 8));
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
        executionError(QString("Unimplemented opcode: %1").arg(Assembly::OpcodesValueToString(opcode)));
        setStopRun(true);
        return;
    }
}

void ProcessorModel::setNZStatusFlags(uint8_t value)
{
    setStatusFlag(StatusFlags::Negative, value & 0x80);
    setStatusFlag(StatusFlags::Zero, value == 0);
}

void ProcessorModel::jumpTo(uint16_t instructionNumber)
{
    bool internal = false;
    if (instructionNumber == InternalJSRs::__JSR_outch)
    {
        jsr_outch();
        internal = true;
    }
    else if (instructionNumber == InternalJSRs::__JSR_get_time)
    {
        jsr_get_time();
        internal = true;
    }
    else if (instructionNumber == InternalJSRs::__JSR_get_elapsed_time)
    {
        jsr_get_elapsed_time();
        internal = true;
    }
    if (internal)
        instructionNumber = (pullFromStack() << 8) | pullFromStack();
    if (instructionNumber < 0 || instructionNumber > _instructions->size())
    {
        executionError(QString("Jump to instruction number out of range: %1").arg(instructionNumber));
        setStopRun(true);
        return;
    }
    setCurrentInstructionNumber(instructionNumber);
}

void ProcessorModel::jsr_outch()
{
    emit sendCharToConsole(accumulator());
}

void ProcessorModel::jsr_get_time()
{
    uint16_t milliseconds = static_cast<uint16_t>(QDateTime::currentMSecsSinceEpoch());
    setAccumulator(static_cast<uint8_t>(milliseconds));
    setXregister(static_cast<uint8_t>(milliseconds >> 8));
}

void ProcessorModel::jsr_get_elapsed_time()
{
    uint16_t milliseconds = static_cast<uint16_t>(elapsedTimer.elapsed());
    setAccumulator(static_cast<uint8_t>(milliseconds));
    setXregister(static_cast<uint8_t>(milliseconds >> 8));
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

