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
