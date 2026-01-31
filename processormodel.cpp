#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QTimer>

#include "processormodel.h"


//
// ProcessorModel Class
//

ProcessorModel::ProcessorModel(QObject *parent)
    : QObject{parent}
{
    _memory = _memoryData;
    std::memset(_memoryData, 0xa5, memorySize());
    _memoryModel = new MemoryModel(this);
    resetModel();
    _instructions = reinterpret_cast<Instruction *>(_memoryData);
    processorBreakpointProvider = nullptr;
    _programCounter = 0;
    _currentRunMode = NotRunning;
    _startNewRun = true;
    _stopRun = true;
    _isRunning = false;
}

ProcessorModel::~ProcessorModel()
{
    if (userFile.isOpen())
        userFile.close();
}

void ProcessorModel::setProcessorBreakpointProvider(IProcessorBreakpointProvider *provider)
{
    processorBreakpointProvider = provider;
}

uint8_t ProcessorModel::accumulator() const
{
    return _accumulator;
}

void ProcessorModel::setAccumulator(uint8_t newAccumulator)
{
    _accumulator = newAccumulator;
    if (!suppressSignalsForSpeed())
        emit accumulatorChanged();
}

uint8_t ProcessorModel::xregister() const
{
    return _xregister;
}

void ProcessorModel::setXregister(uint8_t newXregister)
{
    _xregister = newXregister;
    if (!suppressSignalsForSpeed())
        emit xregisterChanged();
}

uint8_t ProcessorModel::yregister() const
{
    return _yregister;
}

void ProcessorModel::setYregister(uint8_t newYregister)
{
    _yregister = newYregister;
    if (!suppressSignalsForSpeed())
        emit yregisterChanged();
}

uint8_t ProcessorModel::stackRegister() const
{
    return _stackRegister;
}

void ProcessorModel::setStackRegister(uint8_t newStackRegister)
{
    _stackRegister = newStackRegister;
    if (!suppressSignalsForSpeed())
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
}

uint8_t ProcessorModel::statusFlag(uint8_t flagBit) const
{
    return _statusFlags & flagBit;
}

void ProcessorModel::clearStatusFlag(uint8_t newFlagBit)
{
    _statusFlags &= ~newFlagBit;
}

void ProcessorModel::setStatusFlag(uint8_t newFlagBit)
{
    _statusFlags |= newFlagBit;
}

void ProcessorModel::setStatusFlag(uint8_t newFlagBit, bool on)
{
    if (on)
        setStatusFlag(newFlagBit);
    else
        clearStatusFlag(newFlagBit);
}

uint8_t *ProcessorModel::memory()
{
    return _memory;
}

unsigned int ProcessorModel::memorySize() const
{
    return sizeof(_memoryData);
}

uint8_t ProcessorModel::memoryByteAt(uint16_t address) const
{
    return _memory[address];
}

void ProcessorModel::setMemoryByteAt(uint16_t address, uint8_t value)
{
    _memory[address] = value;
    if (!suppressSignalsForSpeed())
        emit memoryChanged(address);
}

uint16_t ProcessorModel::memoryWordAt(uint16_t address) const
{
    return _memory[address] | (_memory[static_cast<uint16_t>(address + 1)] << 8);
}

uint16_t ProcessorModel::memoryZPWordAt(uint8_t address) const
{
    return _memory[address] | (_memory[static_cast<uint8_t>(address + 1)] << 8);
}

Instruction *ProcessorModel::instructions() const
{
    return _instructions;
}

uint16_t ProcessorModel::programCounter() const
{
    return _programCounter;
}

void ProcessorModel::setProgramCounter(uint16_t newProgramCounter)
{
    _programCounter = newProgramCounter;
    if (!suppressSignalsForSpeed())
    {
        emit programCounterChanged();
        emit currentInstructionAddressChanged(_programCounter);
    }
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


void ProcessorModel::setCurrentRunMode(RunMode newCurrentRunMode)
{
    _currentRunMode = newCurrentRunMode;
}

bool ProcessorModel::suppressSignalsForSpeed() const
{
    return _currentRunMode == TurboRun;
}


bool ProcessorModel::isRunning() const
{
    return _isRunning;
}

void ProcessorModel::setIsRunning(bool newIsRunning)
{
    if (newIsRunning != _isRunning)
    {
        _isRunning = newIsRunning;
        emit isRunningChanged();
    }
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
    if (_startNewRun)
        setCurrentRunMode(NotRunning);
}

const Instruction *ProcessorModel::nextInstructionToExecute() const
{
    const uint8_t *address = reinterpret_cast<const uint8_t *>(_instructions) + _programCounter;
    return reinterpret_cast<const Instruction *>(address);
}


void ProcessorModel::debugMessage(const QString &message) const
{
    QString message2(message);
    QString location(QString("[instruction $%1]").arg(_programCounter, 4, 16, QChar('0')));
    message2 = location + " " + message2;
    qDebug() << message2;
    emit sendMessageToConsole(message2, Qt::red);
}

void ProcessorModel::executionErrorMessage(const QString &message) const
{
    debugMessage("Execution Error: " + message);
}


/*slot*/ void ProcessorModel::restart()
{
    if (_isRunning)
        return;

    if (userFile.isOpen())
        userFile.close();
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
    if (userFile.isOpen())
        userFile.close();
    setStopRun(true);
}

/*slot*/ void ProcessorModel::turboRun()
{
    runInstructions(TurboRun);
    emit modelReset();
    emit currentInstructionAddressChanged(_programCounter);
}

/*slot*/ void ProcessorModel::run()
{
    runInstructions(Run);
}

/*slot*/void ProcessorModel::continueRun()
{
    runInstructions(Continue);
}

/*slot*/ void ProcessorModel::stepInto()
{
    runInstructions(StepInto);
}

/*slot*/ void ProcessorModel::stepOver()
{
    runInstructions(StepOver);
}

/*slot*/ void ProcessorModel::stepOut()
{
    runInstructions(StepOut);
}


void ProcessorModel::runInstructions(RunMode runMode)
{
    if (_isRunning)
        return;
    Q_ASSERT(runMode != NotRunning);

    try
    {
        bool startedNewRun = false;
        bool step = runMode == StepInto || runMode == StepOut || runMode == StepOver;
        if (runMode == Run || runMode == TurboRun || startNewRun() || stopRun())
        {
            restart();
            elapsedTimer.start();
            elapsedCycles = 0;
            setStartNewRun(false);
            startedNewRun = true;
            if (runMode == Continue)
                runMode = Run;
            pushToStack(static_cast<uint8_t>(Assembly::__JSR_terminate));
            pushToStack(static_cast<uint8_t>(Assembly::__JSR_terminate >> 8));
        }
        if (stopRun())
        {
            setCurrentRunMode(NotRunning);
            return;
        }
        setCurrentRunMode(runMode);
        if (startedNewRun && runMode == StepInto)
            return;

        int count = 0;
        const int processEventsEverySoOften = 10000;

        setIsRunning(true);
        QCoreApplication::processEvents();
        QCoreApplication::processEvents();  // extra time, allows proper redraw

        int stopAtInstructionAddress = -1;
        bool keepGoing = true;
        if (startedNewRun)
            if (runMode != TurboRun && processorBreakpointProvider->breakpointAt(_programCounter))
                keepGoing = false;
        while (!stopRun() && keepGoing)
        {
            const Instruction &instruction(*nextInstructionToExecute());
            const InstructionInfo &instructionInfo(instruction.getInstructionInfo());
            const Operation operation(instructionInfo.operation);
            keepGoing = !step || stopAtInstructionAddress >= 0;
            if (step && stopAtInstructionAddress < 0)
            {
                if (runMode == StepOver && operation == Operation::JSR)
                {
                    stopAtInstructionAddress = _programCounter + instructionInfo.bytes;
                    keepGoing = true;
                }
                else if (runMode == StepOut)
                {
                    uint16_t rtsAddress = memoryByteAt(_stackBottom + static_cast<uint8_t>(_stackRegister + 1)) << 8;
                    rtsAddress |= memoryByteAt(_stackBottom + static_cast<uint8_t>(_stackRegister + 2));
                    stopAtInstructionAddress = rtsAddress;
                    keepGoing = true;
                }
            }

            runNextInstruction(instruction);
            count++;

            if (runMode == TurboRun)
                continue;

            if (_programCounter == stopAtInstructionAddress || processorBreakpointProvider->breakpointAt(_programCounter))
                keepGoing = false;

            if (processEventsEverySoOften != 0 && count % processEventsEverySoOften == 0)
                QCoreApplication::processEvents();
        }
    }
    catch (const ExecutionError &e)
    {
        stop();
        executionErrorMessage(QString(e.what()));
    }

    setCurrentRunMode(NotRunning);
    setIsRunning(false);
}

void ProcessorModel::runNextInstruction(const Instruction &instruction)
{
    if (stopRun())
        return;

    //
    // EXECUTION PHASE
    //
    executeNextInstruction(instruction);
}


void ProcessorModel::executeNextInstruction(const Instruction &instruction)
{
    //
    // EXECUTION PHASE
    //

    const InstructionInfo &instructionInfo(instruction.getInstructionInfo());
    const Operation operation(instructionInfo.operation);
    const AddressingMode mode(instructionInfo.addrMode);
    const uint16_t operand(instruction.operand);
    if (!instructionInfo.isValid())
        throw ExecutionError(QString("Illegal opcode: %1 %2 %3")
                                 .arg(instructionInfo.opcodeByte, 2, 16, QChar('0'))
                                 .arg(Assembly::OperationValueToString(operation))
                                 .arg(Assembly::AddressingModeValueToString(mode)));

    uint8_t _argValue = -1;
    uint16_t _argAddress = -1;
    switch (mode)
    {
    case AddressingMode::Implied:
        break;
    case AddressingMode::Accumulator:
        _argValue = _accumulator;
        break;
    case AddressingMode::Immediate:
        _argValue = operand;
        break;
    case AddressingMode::Relative:
        _argAddress = _programCounter + 2 + static_cast<int8_t>(operand);
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
        _argAddress = operand;
        if (mode == AddressingMode::Absolute)
            ;
        else if (mode == AddressingMode::ZeroPage)
            _argAddress = static_cast<uint8_t>(_argAddress);
        else if (mode == AddressingMode::AbsoluteX)
            _argAddress += _xregister;
        else if (mode == AddressingMode::ZeroPageX)
            _argAddress = static_cast<uint8_t>(_argAddress + _xregister);
        else if (mode == AddressingMode::AbsoluteY)
            _argAddress += _yregister;
        else if (mode == AddressingMode::ZeroPageY)
            _argAddress = static_cast<uint8_t>(_argAddress + _yregister);
        else if (mode == AddressingMode::Indirect)
            _argAddress = memoryWordAt(_argAddress);
        else if (mode == AddressingMode::IndexedIndirectX)
            _argAddress = memoryZPWordAt(_argAddress + _xregister);
        else if (mode == AddressingMode::IndirectIndexedY)
            _argAddress = memoryZPWordAt(_argAddress) + _yregister;
        switch (operation)
        {
        case Operation::STA: case Operation::STX: case Operation::STY:
        case Operation::JMP: case Operation::JSR:
            break;
        default:
            _argValue = memoryByteAt(_argAddress);
            break;
        }
        break;
    default:
        throw ExecutionError(QString("Unimplemented operand addressing mode: %1").arg(Assembly::AddressingModeValueToString(mode)));
    }

    setProgramCounter(_programCounter + instructionInfo.bytes);

    clearStatusFlag(StatusFlags::Break);

    const uint8_t argValue{_argValue};
    const uint16_t argAddress{_argAddress};
    uint8_t tempValue8, origTempValue8;
    uint16_t tempValue16;

    elapsedCycles += instructionInfo.cycles;
    if (mode == AddressingMode::AbsoluteX || mode == AddressingMode::AbsoluteY || mode == AddressingMode::IndirectIndexedY)
        switch (operation)
        {
        case Operation::ADC: case Operation::AND: case Operation::CMP:
        case Operation::EOR: case Operation::LDA: case Operation::LDX:
        case Operation::LDY: case Operation::ORA: case Operation::SBC: {
            uint16_t baseAddress = operand;
            if (mode == AddressingMode::IndirectIndexedY)
                baseAddress = memoryZPWordAt(baseAddress);
            if ((argAddress & 0xff00) != (baseAddress & 0xff00))
                elapsedCycles++;
            break;
        }
        default: break;
        }

    // Operations ordered/grouped as per http://www.6502.org/users/obelisk/6502/instructions.html
    switch (operation)
    {
    case Operation::LDA:
        setAccumulator(argValue);
        setNZStatusFlags(_accumulator);
        break;
    case Operation::LDX:
        setXregister(argValue);
        setNZStatusFlags(_xregister);
        break;
    case Operation::LDY:
        setYregister(argValue);
        setNZStatusFlags(_yregister);
        break;
    case Operation::STA:
        setMemoryByteAt(argAddress, _accumulator);
        break;
    case Operation::STX:
        setMemoryByteAt(argAddress, _xregister);
        break;
    case Operation::STY:
        setMemoryByteAt(argAddress, _yregister);
        break;

    case Operation::TAX:
        setXregister(_accumulator);
        setNZStatusFlags(_xregister);
        break;
    case Operation::TAY:
        setYregister(_accumulator);
        setNZStatusFlags(_yregister);
        break;
    case Operation::TXA:
        setAccumulator(_xregister);
        setNZStatusFlags(_accumulator);
        break;
    case Operation::TYA:
        setAccumulator(_yregister);
        setNZStatusFlags(_accumulator);
        break;

    case Operation::TSX:
        setXregister(_stackRegister);
        setNZStatusFlags(_xregister);
        break;
    case Operation::TXS:
        setStackRegister(_xregister);
        break;
    case Operation::PHA:
        pushToStack(_accumulator);
        break;
    case Operation::PHP:
        pushToStack(_statusFlags);
        break;
    case Operation::PLA:
        setAccumulator(pullFromStack());
        setNZStatusFlags(_accumulator);
        break;
    case Operation::PLP:
        setStatusFlags(pullFromStack());
        break;

    case Operation::AND:
        setAccumulator(_accumulator & argValue);
        setNZStatusFlags(_accumulator);
        break;
    case Operation::EOR:
        setAccumulator(_accumulator ^ argValue);
        setNZStatusFlags(_accumulator);
        break;
    case Operation::ORA:
        setAccumulator(_accumulator | argValue);
        setNZStatusFlags(_accumulator);
        break;
    case Operation::BIT:
        setStatusFlag(StatusFlags::Zero, (_accumulator & argValue) == 0);
        setStatusFlag(StatusFlags::Overflow, argValue & 0x40);
        setStatusFlag(StatusFlags::Negative, argValue & 0x80);
        break;

    case Operation::ADC:
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
    case Operation::SBC:
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
    case Operation::CMP:
        // (A - M)
        tempValue16 = _accumulator - argValue;
        setStatusFlag(StatusFlags::Carry, tempValue16 <= 0xff);
        tempValue8 = tempValue16;
        setNZStatusFlags(tempValue8);
        break;
    case Operation::CPX:
        // (X - M)
        tempValue16 = _xregister - argValue;
        setStatusFlag(StatusFlags::Carry, tempValue16 <= 0xff);
        tempValue8 = tempValue16;
        setNZStatusFlags(tempValue8);
        break;
    case Operation::CPY:
        // (Y - M)
        tempValue16 = _yregister - argValue;
        setStatusFlag(StatusFlags::Carry, tempValue16 <= 0xff);
        tempValue8 = tempValue16;
        setNZStatusFlags(tempValue8);
        break;

    case Operation::INC:
        tempValue8 = argValue + 1;
        setMemoryByteAt(argAddress, tempValue8);
        setNZStatusFlags(tempValue8);
        break;
    case Operation::INX:
        setXregister(_xregister + 1);
        setNZStatusFlags(_xregister);
        break;
    case Operation::INY:
        setYregister(_yregister + 1);
        setNZStatusFlags(_yregister);
        break;
    case Operation::DEC:
        tempValue8 = argValue - 1;
        setMemoryByteAt(argAddress, tempValue8);
        setNZStatusFlags(tempValue8);
        break;
    case Operation::DEX:
        setXregister(_xregister - 1);
        setNZStatusFlags(_xregister);
        break;
    case Operation::DEY:
        setYregister(_yregister - 1);
        setNZStatusFlags(_yregister);
        break;

    case Operation::ASL:
        origTempValue8 = tempValue8 = (mode == AddressingMode::Accumulator) ? _accumulator : argValue;
        tempValue8 <<= 1;
        setStatusFlag(StatusFlags::Carry, origTempValue8 & 0x80);
        if (mode == AddressingMode::Accumulator)
            setAccumulator(tempValue8);
        else
            setMemoryByteAt(argAddress, tempValue8);
        setNZStatusFlags(tempValue8);
        break;
    case Operation::LSR:
        origTempValue8 = tempValue8 = (mode == AddressingMode::Accumulator) ? _accumulator : argValue;
        tempValue8 >>= 1;
        setStatusFlag(StatusFlags::Carry, origTempValue8 & 0x01);
        if (mode == AddressingMode::Accumulator)
            setAccumulator(tempValue8);
        else
            setMemoryByteAt(argAddress, tempValue8);
        setNZStatusFlags(tempValue8);
        break;
    case Operation::ROL:
        origTempValue8 = tempValue8 = (mode == AddressingMode::Accumulator) ? _accumulator : argValue;
        tempValue8 <<= 1;
        tempValue8 |= statusFlag(StatusFlags::Carry);
        setStatusFlag(StatusFlags::Carry, origTempValue8 & 0x80);
        if (mode == AddressingMode::Accumulator)
            setAccumulator(tempValue8);
        else
            setMemoryByteAt(argAddress, tempValue8);
        setNZStatusFlags(tempValue8);
        break;
    case Operation::ROR:
        origTempValue8 = tempValue8 = (mode == AddressingMode::Accumulator) ? _accumulator : argValue;
        tempValue8 >>= 1;
        tempValue8 |= (statusFlag(StatusFlags::Carry) << 7);
        setStatusFlag(StatusFlags::Carry, origTempValue8 & 0x01);
        if (mode == AddressingMode::Accumulator)
            setAccumulator(tempValue8);
        else
            setMemoryByteAt(argAddress, tempValue8);
        setNZStatusFlags(tempValue8);
        break;

    case Operation::JMP:
        jumpTo(argAddress);
        break;
    case Operation::JSR:
        tempValue16 = _programCounter;
        pushToStack(static_cast<uint8_t>(tempValue16));
        pushToStack(static_cast<uint8_t>(tempValue16 >> 8));
        jumpTo(argAddress);
        break;
    case Operation::RTS:
        tempValue16 = pullFromStack() << 8;
        tempValue16 |= pullFromStack();
        jumpTo(tempValue16);
        break;

    case Operation::BCC:
        if (!statusFlag(StatusFlags::Carry))
            branchTo(argAddress);
        break;
    case Operation::BCS:
        if (statusFlag(StatusFlags::Carry))
            branchTo(argAddress);
        break;
    case Operation::BEQ:
        if (statusFlag(StatusFlags::Zero))
            branchTo(argAddress);
        break;
    case Operation::BMI:
        if (statusFlag(StatusFlags::Negative))
            branchTo(argAddress);
        break;
    case Operation::BNE:
        if (!statusFlag(StatusFlags::Zero))
            branchTo(argAddress);
        break;
    case Operation::BPL:
        if (!statusFlag(StatusFlags::Negative))
            branchTo(argAddress);
        break;
    case Operation::BVC:
        if (!statusFlag(StatusFlags::Overflow))
            branchTo(argAddress);
        break;
    case Operation::BVS:
        if (statusFlag(StatusFlags::Overflow))
            branchTo(argAddress);
        break;

    case Operation::CLC:
        clearStatusFlag(StatusFlags::Carry);
        break;
    case Operation::CLV:
        clearStatusFlag(StatusFlags::Overflow);
        break;
    case Operation::SEC:
        setStatusFlag(StatusFlags::Carry);
        break;

    case Operation::BRK:
        tempValue16 = _programCounter;
        pushToStack(static_cast<uint8_t>(tempValue16));
        pushToStack(static_cast<uint8_t>(tempValue16 >> 8));
        setStatusFlag(StatusFlags::Break);
        jumpTo(Assembly::__JSR_brk_handler);
        break;
    case Operation::NOP:
        break;

    default:
        throw ExecutionError(QString("Unimplemented operation: %1").arg(Assembly::OperationValueToString(operation)));
    }

    switch (operation)
    {
    case Operation::STA: case Operation::STX: case Operation::STY:
    case Operation::TXS: case Operation::PHA: case Operation::PHP:
    case Operation::JMP: case Operation::JSR: case Operation::RTS:
    case Operation::NOP:
        break;
    default:
        if (!suppressSignalsForSpeed())
            emit statusFlagsChanged();
        break;
    }
}

void ProcessorModel::setNZStatusFlags(uint8_t value)
{
    setStatusFlag(StatusFlags::Negative, value & 0x80);
    setStatusFlag(StatusFlags::Zero, value == 0);
}

void ProcessorModel::branchTo(uint16_t instructionAddress)
{
    elapsedCycles++;
    if ((instructionAddress &0xff00) != (_programCounter & 0xff00))
        elapsedCycles++;
    jumpTo(instructionAddress);
}

void ProcessorModel::jumpTo(uint16_t instructionAddress)
{
    bool internal = true;
    switch (instructionAddress)
    {
    case InternalJSRs::__JSR_terminate:
        stop();
        return;
    case InternalJSRs::__JSR_brk_handler:
        jsr_brk_handler();
        stop();
        return;
    case InternalJSRs::__JSR_outch:
        jsr_outch(); break;
    case InternalJSRs::__JSR_get_time:
        jsr_get_time(); break;
    case InternalJSRs::__JSR_get_elapsed_time:
        jsr_get_elapsed_time(); break;
    case InternalJSRs::__JSR_get_elapsed_stime:
        jsr_get_elapsed_stime(); break;
    case InternalJSRs::__JSR_clear_elapsed_time:
        jsr_clear_elapsed_time(); break;
    case InternalJSRs::__JSR_process_events:
        jsr_process_events(); break;
    case InternalJSRs::__JSR_inch:
        jsr_inch(); break;
    case InternalJSRs::__JSR_inkey:
        jsr_inkey(); break;
    case InternalJSRs::__JSR_wait:
        jsr_wait(); break;
    case InternalJSRs::__JSR_open_file:
        jsr_open_file(); break;
    case InternalJSRs::__JSR_close_file:
        jsr_close_file(); break;
    case InternalJSRs::__JSR_rewind_file:
        jsr_rewind_file(); break;
    case InternalJSRs::__JSR_read_file:
        jsr_read_file(); break;
    case InternalJSRs::__JSR_outstr_fast:
        jsr_outstr_fast(); break;
    case InternalJSRs::__JSR_get_elapsed_cycles:
        jsr_get_elapsed_cycles(); break;
    case InternalJSRs::__JSR_get_elapsed_kcycles:
        jsr_get_elapsed_kcycles(); break;
    case InternalJSRs::__JSR_get_elapsed_mcycles:
        jsr_get_elapsed_mcycles(); break;
    case InternalJSRs::__JSR_clear_elapsed_cycles:
        jsr_clear_elapsed_cycles(); break;
    default:
        internal = false; break;
    }
    if (internal)
    {
        const InstructionInfo *instructionInfo(Assembly::findInstructionInfo(Operation::RTS, AddressingMode::Implied));
        elapsedCycles += instructionInfo->cycles;
        instructionAddress = (pullFromStack() << 8) | pullFromStack();
        if (!suppressSignalsForSpeed())
            emit statusFlagsChanged();
    }
    setProgramCounter(instructionAddress);
}

void ProcessorModel::jsr_brk_handler()
{
    uint16_t address = (pullFromStack() << 8) | pullFromStack();
    const char *memoryAddress = reinterpret_cast<const char *>(_memory + address);
    int len = std::strlen(memoryAddress);
    if (len > 255)
        len = 255;
    QString message(QString::fromLatin1(memoryAddress, len));
    sendMessageToConsole(message);
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
    setNZStatusFlags(_xregister);
}

void ProcessorModel::jsr_get_elapsed_time()
{
    uint16_t milliseconds = static_cast<uint16_t>(elapsedTimer.elapsed());
    setAccumulator(static_cast<uint8_t>(milliseconds));
    setXregister(static_cast<uint8_t>(milliseconds >> 8));
    setNZStatusFlags(_xregister);
}

void ProcessorModel::jsr_get_elapsed_stime()
{
    int seconds = (elapsedTimer.elapsed() + 500) / 1000;
    setAccumulator(static_cast<uint8_t>(seconds));
    setXregister(static_cast<uint8_t>(seconds >> 8));
    setNZStatusFlags(_xregister);
}

void ProcessorModel::jsr_clear_elapsed_time()
{
    elapsedTimer.restart();
}

void ProcessorModel::jsr_process_events()
{
    QCoreApplication::processEvents();
}

void ProcessorModel::jsr_inch(int timeout /*= -1*/, bool justWait /*= false*/)
{
    char result = '\0';
    QEventLoop loop;
    if (!justWait)
        QObject::connect(this, &ProcessorModel::receivedCharFromConsole,
                         &loop, [&](char ch) { result = ch; loop.quit(); });
    QObject::connect(this, &ProcessorModel::stopRunChanged,
                     &loop, [&]() { result = '\0'; loop.quit(); });
    QTimer timer;
    if (timeout >= 0)
    {
        QObject::connect(&timer, &QTimer::timeout,
                         &loop, [&]() { result = '\0'; loop.quit(); });
        timer.setSingleShot(true);
        timer.start(timeout * 10);
    }
    emit requestCharFromConsole();
    loop.exec();
    timer.stop();
    emit endRequestCharFromConsole();
    if (result == '\003' || result == '\033')  // Ctrl-C or Escape
        stop();
    setAccumulator(result);
    setNZStatusFlags(_accumulator);
}

void ProcessorModel::jsr_inkey()
{
    uint16_t timeout = _accumulator | (_xregister << 8);
    jsr_inch(timeout);
}

void ProcessorModel::jsr_wait()
{
    uint16_t timeout = _accumulator | (_xregister << 8);
    jsr_inch(timeout, true);
}

void ProcessorModel::jsr_open_file()
{
    uint16_t address = _accumulator | (_xregister << 8);
    bool goodName = false;
    char filename[256];
    for (int i = 0; i < sizeof(filename) - 1; i++)
    {
        char ch = memoryByteAt(address + i);
        filename[i] = ch;
        if (ch == '\0')
            goodName = true;
        if (!std::isprint(ch))
            break;
    }
    if (!goodName)
        throw ExecutionError("Bad filename");
    if (userFile.isOpen())
        throw ExecutionError("File already open");
    userFile.setFileName(filename);
    if (!userFile.open(QIODeviceBase::ReadOnly | QIODeviceBase::Text))
        throw ExecutionError(userFile.errorString());
}

void ProcessorModel::jsr_close_file()
{
    if (userFile.isOpen())
        userFile.close();
}

void ProcessorModel::jsr_rewind_file()
{
    if (userFile.isOpen())
        userFile.seek(0);
}

void ProcessorModel::jsr_read_file()
{
    bool success = false;
    if (userFile.isOpen())
    {
        char ch;
        if (userFile.getChar(&ch))
        {
            setAccumulator(ch);
            setNZStatusFlags(_accumulator);
            success = true;
        }
    }
    setStatusFlag(StatusFlags::Carry, !success);
}

void ProcessorModel::jsr_outstr_fast()
{
    uint16_t address = _accumulator | (_xregister << 8);
    const char *memoryAddress = reinterpret_cast<const char *>(_memory + address);
    QString str(QString::fromLatin1(memoryAddress));
    emit sendStringToConsole(str);
}

void ProcessorModel::jsr_get_elapsed_cycles()
{
    setAccumulator(static_cast<uint8_t>(elapsedCycles));
    setXregister(static_cast<uint8_t>(elapsedCycles >> 8));
    setNZStatusFlags(_xregister);
}

void ProcessorModel::jsr_get_elapsed_kcycles()
{
    int kcycles = (elapsedCycles + 512) / 1024;
    setAccumulator(static_cast<uint8_t>(kcycles));
    setXregister(static_cast<uint8_t>(kcycles >> 8));
    setNZStatusFlags(_xregister);
}

void ProcessorModel::jsr_get_elapsed_mcycles()
{
    int mcycles = (elapsedCycles + 512 * 1024) / 1024 / 1024;
    setAccumulator(static_cast<uint8_t>(mcycles));
    setXregister(static_cast<uint8_t>(mcycles >> 8));
    setNZStatusFlags(_xregister);
}

void ProcessorModel::jsr_clear_elapsed_cycles()
{
    elapsedCycles = 0;
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

/*virtual*/ QVariant MemoryModel::data(const QModelIndex &index, int role /*= Qt::DisplayRole*/) const /*override*/
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

/*virtual*/ QVariant MemoryModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const /*override*/
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

/*slot*/ void MemoryModel::notifyAllDataChanged()
{
    int lastRow = rowCount() - 1, lastCol = columnCount() - 1;
    if (lastRow >= 0 && lastCol >= 0)
        emit dataChanged(index(0, 0), index(lastRow, lastCol));
}

/*slot*/ void MemoryModel::clearLastMemoryChanged()
{
    if (lastMemoryChangedAddress >= 0)
    {
        QModelIndex index = addressToIndex(lastMemoryChangedAddress);
        if (!processorModel->suppressSignalsForSpeed())
            emit dataChanged(index, index, { Qt::ForegroundRole });
        lastMemoryChangedAddress = -1;
    }
}

/*slot*/ void MemoryModel::memoryChanged(uint16_t address)
{
    if (lastMemoryChangedAddress != address)
        clearLastMemoryChanged();
    lastMemoryChangedAddress = address;
    QModelIndex index = addressToIndex(address);
    if (!processorModel->suppressSignalsForSpeed())
        emit dataChanged(index, index, { Qt::DisplayRole, Qt::EditRole, Qt::ForegroundRole });
}


//
// ExecutionError Class
//

ExecutionError::ExecutionError(const QString &msg)
    : std::runtime_error(msg.toStdString())
{

}
