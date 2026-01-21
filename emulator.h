#ifndef EMULATOR_H
#define EMULATOR_H

#include <QObject>
#include <QQueue>
#include <QSpinBox>
#include <QTimer>

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

    const uint16_t runStartAddress() const;
    void mapInstructionAddressToFileLineNumber(uint16_t instructionAddress, QString &filename, int &lineNumber) const;
    int mapFileLineNumberToInstructionAddress(const QString &filename, int lineNumber, bool exact = false) const;
    int findBreakpoint(const QString &filename, int lineNumber) const;
    int toggleBreakpoint(const QString &filename, int lineNumber);

private:
    int findBreakpointIndex(uint16_t instructionAddress) const;

public:
    struct QueuedChangeSignal
    {
        enum SignalType { CurrentCodeLineNumberChanged, CurrentInstructionAddressChanged, MemoryModelDataChanged, RegisterChanged, } tag;

        struct { QString filename; int lineNumber; } codeLine{};

        struct { uint16_t instructionAddress; } instruction{};

        struct { QModelIndex topLeft, bottomRight; QList<int> roles; } memory{};

        struct { QSpinBox *spn; int value; } _register{};

        QueuedChangeSignal(SignalType tag) : tag(tag) {};
        static QueuedChangeSignal currentCodeLineNumberChanged(const QString &filename, int lineNumber)
        {
            QueuedChangeSignal qcs(CurrentCodeLineNumberChanged);
            qcs.codeLine.filename = filename; qcs.codeLine.lineNumber = lineNumber;
            return qcs;
        }
        static QueuedChangeSignal currentInstructionAddressChanged(uint16_t instructionAddress)
        {
            QueuedChangeSignal qcs(CurrentInstructionAddressChanged);
            qcs.instruction.instructionAddress = instructionAddress;
            return qcs;
        }
        static QueuedChangeSignal memoryModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>())
        {
            QueuedChangeSignal qcs(MemoryModelDataChanged);
            qcs.memory.topLeft = topLeft; qcs.memory.bottomRight = bottomRight; qcs.memory.roles = roles;
            return qcs;
        }
        static QueuedChangeSignal registerChanged(QSpinBox *spn, int value)
        {
            QueuedChangeSignal qcs(RegisterChanged);
            qcs._register.spn = spn; qcs._register.value = value;
            return qcs;
        }
    };

    bool queueChangedSignals() const;
    void setQueueChangedSignals(bool newQueueChangedSignals);
    void startQueuingChangedSignals();
    void endQueuingChangedSignals();
    void enqueueQueuedChangedSignal(const QueuedChangeSignal &sig);

signals:
    void processQueuedChangedSignal(const QueuedChangeSignal &sig);

private slots:
    void processQueuedChangedSignals();

private:
    ProcessorModel *_processorModel;
    uint8_t *_memory;
    Instruction *_instructions;
    QList<uint16_t> _breakpoints;

    Assembler *_assembler;

    bool _queueChangedSignals;
    typedef QQueue<QueuedChangeSignal> ChangedSignalsQueue;
    ChangedSignalsQueue _changedSignalsQueue;
    QTimer pendingSignalsTimer;
};

#endif // EMULATOR_H
