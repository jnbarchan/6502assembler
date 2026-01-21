#include "emulator.h"

Emulator::Emulator(QObject *parent)
    : QObject{parent}
{
    Assembly::initInstructionInfo();

    _processorModel = new ProcessorModel(this);
    _memory = _processorModel->memory();
    _instructions = _processorModel->instructions();
    _processorModel->setBreakpoints(&_breakpoints);

    _assembler = new Assembler(this);
    _assembler->setMemory(_memory);
    _assembler->setInstructions(_instructions);
    _assembler->setBreakpoints(&_breakpoints);

    _queueChangedSignals = false;
    connect(&pendingSignalsTimer, &QTimer::timeout, this, &Emulator::processQueuedChangedSignals);
}

const uint16_t Emulator::runStartAddress() const
{
    int value;
    for (const QString &label : { "reset", "start", "START", "main", "MAIN", })
        if ((value = assembler()->codeLabelValue(label)) >= 0)
            return value;
    return assembler()->defaultLocationCounter();
}

void Emulator::mapInstructionAddressToFileLineNumber(uint16_t instructionAddress, QString &filename, int &lineNumber) const
{
    filename.clear();
    lineNumber = -1;
    const QList<Assembler::CodeFileLineNumber> &codeFileLineNumbers(_assembler->instructionsCodeFileLineNumbers());
    for (const Assembler::CodeFileLineNumber &cfln : codeFileLineNumbers)
        if (cfln._locationCounter >= instructionAddress)
        {
            filename = cfln._codeFilename;
            lineNumber = cfln._currentCodeLineNumber;
            return;
        }
    lineNumber = 1000;/*TEMPORARY*/
}

int Emulator::mapFileLineNumberToInstructionAddress(const QString &filename, int lineNumber, bool exact /*= false*/) const
{
    const QList<Assembler::CodeFileLineNumber> &codeFileLineNumbers(_assembler->instructionsCodeFileLineNumbers());
    for (const Assembler::CodeFileLineNumber &cfln : codeFileLineNumbers)
        if (cfln._codeFilename == filename)
            if (exact ? cfln._currentCodeLineNumber == lineNumber : cfln._currentCodeLineNumber >= lineNumber)
                return cfln._locationCounter;
    return -1;
}

int Emulator::findBreakpointIndex(uint16_t instructionAddress) const
{
    int i = 0;
    while (i < _breakpoints.size() && _breakpoints.at(i) < instructionAddress)
        i++;
    return i;
}

int Emulator::findBreakpoint(const QString &filename, int lineNumber) const
{
    int instructionAddress = mapFileLineNumberToInstructionAddress(filename, lineNumber, true);
    if (instructionAddress < 0)
        return -1;
    int i = findBreakpointIndex(instructionAddress);
    return i < _breakpoints.size() ? instructionAddress : -1;
}

int Emulator::toggleBreakpoint(const QString &filename, int lineNumber)
{
    int instructionAddress = mapFileLineNumberToInstructionAddress(filename, lineNumber);
    if (instructionAddress < 0)
        return -1;
    int i = findBreakpointIndex(instructionAddress);
    if (i < _breakpoints.size() && _breakpoints.at(i) == instructionAddress)
        _breakpoints.removeAt(i);
    else
        _breakpoints.insert(i, instructionAddress);
    QString newFilename;
    int newLineNumber;
    mapInstructionAddressToFileLineNumber(instructionAddress, newFilename, newLineNumber);
    Q_ASSERT(newFilename == filename);
    return newFilename == filename ? newLineNumber : -1;
}


bool Emulator::queueChangedSignals() const
{
    return _queueChangedSignals;
}

void Emulator::setQueueChangedSignals(bool newQueueChangedSignals)
{
    _queueChangedSignals = newQueueChangedSignals;
}

void Emulator::startQueuingChangedSignals()
{
    setQueueChangedSignals(true);
    pendingSignalsTimer.start(100);
}

void Emulator::endQueuingChangedSignals()
{
    pendingSignalsTimer.stop();
    setQueueChangedSignals(false);
    processQueuedChangedSignals();
}

void Emulator::enqueueQueuedChangedSignal(const QueuedChangeSignal &sig)
{
    for (int i = _changedSignalsQueue.size() - 1; i >= 0; i--)
    {
        const QueuedChangeSignal &other(_changedSignalsQueue.at(i));
        if (other.tag != sig.tag)
            continue;
        bool remove = false;
        switch (sig.tag)
        {
        case QueuedChangeSignal::CurrentCodeLineNumberChanged:
            if (other.codeLine.filename == sig.codeLine.filename)
                remove = true;
            break;
        case QueuedChangeSignal::CurrentInstructionAddressChanged:
            remove = true;
            break;
        case QueuedChangeSignal::MemoryModelDataChanged:
            if (other.memory.topLeft == sig.memory.topLeft && other.memory.bottomRight == sig.memory.bottomRight)
            {
                const QList<int> &otherRoles(other.memory.roles), &sigRoles(sig.memory.roles);
                if (otherRoles == sigRoles || sigRoles.isEmpty() || (otherRoles.size() == 1 && sigRoles.contains(otherRoles.at(0))))
                    remove = true;
                else if (otherRoles.isEmpty() || (sigRoles.size() == 1 && otherRoles.contains(sigRoles.at(0))))
                    continue;
            }
            break;
        case QueuedChangeSignal::RegisterChanged:
            if (other._register.spn == sig._register.spn)
                remove = true;
            break;
        }
        if (remove)
        {
            _changedSignalsQueue.removeAt(i);
            if (sig.tag != QueuedChangeSignal::MemoryModelDataChanged)
                break;
        }
    }
    _changedSignalsQueue.enqueue(sig);
}

/*slot*/ void Emulator::processQueuedChangedSignals()
{
    bool currentQueueChangedSignals = queueChangedSignals();
    setQueueChangedSignals(false);
    while (!_changedSignalsQueue.isEmpty())
    {
        QueuedChangeSignal sig = _changedSignalsQueue.dequeue();
        emit processQueuedChangedSignal(sig);
    }
    setQueueChangedSignals(currentQueueChangedSignals);
}
