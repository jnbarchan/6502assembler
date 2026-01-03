#include "emulator.h"

Emulator::Emulator(QObject *parent)
    : QObject{parent}
{
    _processorModel = new ProcessorModel(this);
    _processorModel->setInstructions(&_instructions);

    _assembler = new Assembler(this);
    _assembler->setInstructions(&_instructions);
}

int Emulator::mapInstructionNumberToLineNumber(int instructionNumber) const
{
    if (_processorModel->instructions() == nullptr)
        return -1;
    const QList<Instruction> &instructions(*_processorModel->instructions());
    const QList<int> &lineNumbers(_assembler->instructionsCodeLineNumbers());
    Q_ASSERT(instructions.size() == lineNumbers.size());
    Q_ASSERT(instructionNumber >= 0 && instructionNumber <= instructions.size());
    if (instructionNumber < 0)
        return -1;
    if (instructionNumber >= instructions.size() || instructionNumber >= lineNumbers.size())
        return 1000;/*TEMPORARY*/
    return lineNumbers.at(instructionNumber);
}
