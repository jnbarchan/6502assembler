#ifndef EMULATOR_H
#define EMULATOR_H

#include <QObject>

#include "processormodel.h"
#include "assembler.h"

class Emulator;
extern Emulator *g_emulator;

class Emulator : public QObject
{
    Q_OBJECT
public:
    explicit Emulator(QObject *parent = nullptr);

    ProcessorModel *processorModel() const { return _processorModel; };

    Assembler *assembler() const { return _assembler; };

    void mapInstructionNumberToFileLineNumber(int instructionNumber, QString &filename, int &lineNumber) const;

signals:

private:
    ProcessorModel *_processorModel;
    QList<Instruction> _instructions;
    QList<uint16_t> _breakpoints;

    Assembler *_assembler;
};

#endif // EMULATOR_H
