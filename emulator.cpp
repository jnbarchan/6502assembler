#include "emulator.h"

Emulator::Emulator(QObject *parent)
    : QObject{parent}
{
    _processorModel = new ProcessorModel(this);
    _processorModel->setInstructions(&_instructions);
    _processorModel->setBreakpoints(&_breakpoints);

    _assembler = new Assembler(this);
    _assembler->setInstructions(&_instructions);
    _assembler->setBreakpoints(&_breakpoints);

    _queueChangedSignals = false;
    connect(&pendingSignalsTimer, &QTimer::timeout, this, &Emulator::processQueuedChangedSignals);
}

void Emulator::mapInstructionNumberToFileLineNumber(int instructionNumber, QString &filename, int &lineNumber) const
{
    filename.clear();
    lineNumber = -1;
    if (_processorModel->instructions() == nullptr)
        return;
    const QList<Instruction> &instructions(*_processorModel->instructions());
    const QList<Assembler::CodeFileLineNumber> &codeFileLineNumbers(_assembler->instructionsCodeFileLineNumbers());
    if (instructionNumber < 0)
        return;
    if (instructionNumber >= instructions.size() || instructionNumber >= codeFileLineNumbers.size())
    {
        lineNumber = 1000;/*TEMPORARY*/
        return;
    }
    filename = codeFileLineNumbers.at(instructionNumber)._codeFilename;
    lineNumber = codeFileLineNumbers.at(instructionNumber)._currentCodeLineNumber;
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
        case QueuedChangeSignal::CurrentInstructionNumberChanged:
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
