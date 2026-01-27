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

class AssemblerBreakpointProvider;

//
// Emulator Class
//
class Emulator : public QObject
{
    Q_OBJECT
public:
    explicit Emulator(QObject *parent = nullptr);
    ~Emulator();

    ProcessorModel *processorModel() const { return _processorModel; };

    Assembler *assembler() const { return _assembler; };

    const uint16_t runStartAddress() const;
    void mapInstructionAddressToFileLineNumber(uint16_t instructionAddress, QString &filename, int &lineNumber) const;
    int mapFileLineNumberToInstructionAddress(const QString &filename, int lineNumber, bool exact = false) const;
    int findBreakpoint(const QString &filename, int lineNumber) const;
    void toggleBreakpoint(const QString &filename, int lineNumber);
    void clearBreakpoints();
    void addAssemblerBreakpoint(uint16_t instructionAddress);
    void clearAssemblerBreakpoints();
    bool breakpointAt(uint16_t instructionAddress);

signals:
    void breakpointChanged(int instructionAddress);

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
    IProcessorBreakpointProvider *processorBreakpointProvider;

    Assembler *_assembler;
    AssemblerBreakpointProvider *assemblerBreakpointProvider;

    bool _queueChangedSignals;
    typedef QQueue<QueuedChangeSignal> ChangedSignalsQueue;
    ChangedSignalsQueue _changedSignalsQueue;
    QTimer pendingSignalsTimer;
};


//
// AssemblerBreakpointProvider Class
//
class AssemblerBreakpointProvider : public IAssemblerBreakpointProvider
{
public:
    AssemblerBreakpointProvider(Emulator *emulator) : _emulator(emulator) {}

private:
    Emulator *_emulator;
    Emulator *emulator() const { return _emulator; }
    void clearBreakpoints() override;
    void addBreakpoint(uint16_t instructionAddress) override;
};


//
// ProcessorBreakpointProvider Class
//
class ProcessorBreakpointProvider : public IProcessorBreakpointProvider
{
public:
    ProcessorBreakpointProvider(Emulator *emulator) : _emulator(emulator) {}

private:
    Emulator *_emulator;
    Emulator *emulator() const { return _emulator; }
    bool breakpointAt(uint16_t instructionAddress) const override;
};


//
// WatchModel Class
//
class WatchModel : public QAbstractTableModel
{
    Q_OBJECT

    // QAbstractItemModel interface
public:
    WatchModel(MemoryModel *memoryModel, Emulator *emulator);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    void recalculateAllSymbols();

private:
    struct WatchInfo
    {
        QString symbol;
        uint16_t memoryAddress;
        uint8_t value0, value1;

        WatchInfo() { symbol = QString(); memoryAddress = 0; value0 = value1 = 0; }
        bool inUse() const { return memoryAddress != 0; }
    };

    static constexpr int _rowCount = 4;
    MemoryModel *memoryModel;
    Emulator *emulator;
    QList<WatchInfo> watchInfos;

public slots:
    void memoryModelDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles = QList<int>());
};

#endif // EMULATOR_H
