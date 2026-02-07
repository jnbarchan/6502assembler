#ifndef PROCESSORMODEL_H
#define PROCESSORMODEL_H

#include <QAbstractItemModel>
#include <QBrush>
#include <QElapsedTimer>
#include <QFile>
#include <QMetaEnum>
#include <QObject>

#include "assembly.h"

using Operation = Assembly::Operation;
using AddressingMode = Assembly::AddressingMode;
using Instruction = Assembly::Instruction;
using InstructionInfo = Assembly::InstructionInfo;
using InternalJSRs = Assembly::InternalJSRs;


class MemoryModel;
class IProcessorBreakpointProvider;

//
// ProcessorModel Class
//
class ProcessorModel : public QObject
{
    Q_OBJECT
public:
    enum StatusFlags : uint8_t
    {
        Negative = 0x80,
        Overflow = 0x40,
        NotUsed = 0x20,
        Break = 0x10,
        Decimal = 0x08,
        InterruptDisable = 0x04,
        Zero = 0x02,
        Carry = 0x01
    };
    static_assert(Carry == 0x01, "StatusFlags::Carry must have a value of 0x01");
    Q_FLAG(StatusFlags)
    enum RunMode { NotRunning, TurboRun, Run, StepInto, StepOver, StepOut, Continue };
    Q_ENUM(RunMode)

    explicit ProcessorModel(QObject *parent = nullptr);
    ~ProcessorModel();

    void setProcessorBreakpointProvider(IProcessorBreakpointProvider *provider);

    MemoryModel *memoryModel() { return _memoryModel; }

    uint8_t accumulator() const;
    void setAccumulator(uint8_t newAccumulator);

    uint8_t xregister() const;
    void setXregister(uint8_t newXregister);

    uint8_t yregister() const;
    void setYregister(uint8_t newYregister);

    uint8_t stackRegister() const;
    void setStackRegister(uint8_t newStackRegister);
    uint8_t pullFromStack();
    void pushToStack(uint8_t value);
    bool isStackAddress(uint16_t address);

    uint8_t statusFlags() const;
    void setStatusFlags(uint8_t newStatusFlags);
    uint8_t statusFlag(uint8_t flagBit) const;
    void clearStatusFlag(uint8_t newFlagBit);
    void setStatusFlag(uint8_t newFlagBit);
    void setStatusFlag(uint8_t newFlagBit, bool on);

    uint8_t *memory();
    unsigned int memorySize() const;
    uint8_t memoryByteAt(uint16_t address) const;
    void setMemoryByteAt(uint16_t address, uint8_t value);
    uint16_t memoryWordAt(uint16_t address) const;
    uint16_t memoryZPWordAt(uint8_t address) const;

    Instruction *instructions() const;

    uint16_t programCounter() const;
    void setProgramCounter(uint16_t newProgramCounter);

    bool suppressSignalsForSpeed() const;
    bool trackingMemoryChanged() const;
    void memoryChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, bool foregroundOnly = false);

    RunMode currentRunMode() const;

    bool isRunning() const;
    void setIsRunning(bool newIsRunning);
    bool stopRun() const;
    void setStopRun(bool newStopRun);

    bool startNewRun() const;
    void setStartNewRun(bool newStartNewRun);

    const Instruction *nextInstructionToExecute() const;

public slots:
    void restart();
    void endRun();
    void stop();
    void turboRun();
    void run();
    void continueRun();
    void stepInto();
    void stepOver();
    void stepOut();

signals:
    void sendMessageToConsole(const QString &message, QBrush colour = Qt::transparent) const;
    void sendStringToConsole(const QString &str) const;
    void sendCharToConsole(char ch) const;
    void requestCharFromConsole();
    void endRequestCharFromConsole();
    void receivedCharFromConsole(char ch);
    void modelReset();
    void isRunningChanged();
    void stopRunChanged();
    void accumulatorChanged();
    void xregisterChanged();
    void yregisterChanged();
    void programCounterChanged();
    void stackRegisterChanged();
    void statusFlagsChanged();
    void currentInstructionAddressChanged(uint16_t instructionAddress);

private:
    uint8_t _memoryData[64 * 1024];
    uint8_t *_memory;
    MemoryModel *_memoryModel;
    const uint16_t _stackBottom = 0x0100;
    uint8_t _stackRegister;
    uint8_t _accumulator, _xregister, _yregister;
    uint8_t _statusFlags;

    Instruction *_instructions;
    uint16_t _programCounter;
    const IProcessorBreakpointProvider *processorBreakpointProvider;

    struct HaveChangedState
    {
        bool stackRegister;
        bool accumulator, xregister, yregister;
        bool statusFlags;
        bool programCounter;
        struct MemoryChanged {
            int topLeftRow, topLeftColumn, bottomRightRow, bottomRightColumn;
            void clear()
            {
                topLeftRow = topLeftColumn = INT_MAX;
                bottomRightRow = bottomRightColumn = -1;
            }
        } memoryChanged, memoryChangedForegroundOnly;
        bool trackingMemoryChanged;

        void clear()
        {
            stackRegister = accumulator = xregister = yregister = statusFlags = programCounter = false;
            memoryChanged.clear();
            memoryChangedForegroundOnly.clear();
        }
    };
    HaveChangedState haveChangedState;

    bool _startNewRun, _stopRun, _isRunning;
    RunMode _currentRunMode;

    QFile userFile;
    QElapsedTimer elapsedTimer;
    uint32_t elapsedCycles;

    void resetModel();
    void setCurrentRunMode(RunMode newCurrentRunMode);
    void catchUpSuppressedSignals();
    void debugMessage(const QString &message) const;
    void executionErrorMessage(const QString &message) const;
    void runInstructions(RunMode runMode);
    void runNextInstruction(const Instruction &instruction);
    void executeNextInstruction(const Instruction &instruction);
    void setNZStatusFlags(uint8_t value);
    void branchTo(uint16_t instructionAddress);
    void jumpTo(uint16_t instructionAddress);
    void jsr_brk_handler();
    void jsr_outch();
    void jsr_get_time();
    void jsr_get_elapsed_time();
    void jsr_clear_elapsed_time();
    void jsr_process_events();
    void jsr_inch(int timeout = -1, bool justWait = false);
    void jsr_inkey();
    void jsr_wait();
    void jsr_open_file();
    void jsr_close_file();
    void jsr_rewind_file();
    void jsr_read_file();
    void jsr_outstr_fast();
    void jsr_get_elapsed_cycles();
    void jsr_clear_elapsed_cycles();
};


//
// MemoryModel Class
//
class MemoryModel : public QAbstractTableModel
{
    Q_OBJECT

    // QAbstractItemModel interface
public:
    MemoryModel(QObject *parent);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    int indexToAddress(const QModelIndex &index) const { return index.isValid() ? index.row() * columnCount() + index.column() : -1; }
    QModelIndex addressToIndex(int address) const { return address >= 0 ? index(address / columnCount(), address % columnCount()) : QModelIndex(); }

    void notifyAllDataChanged();
    void clearLastMemoryChanged();
    void memoryChanged(uint16_t address);

private:
    ProcessorModel *processorModel;
    int lastMemoryChangedAddress;
};


//
// ExecutionError Class
//
class ExecutionError : public std::runtime_error
{
public:
    ExecutionError(const QString &msg);
};


//
// IProcessorBreakpointProvider Class
//
class IProcessorBreakpointProvider
{
public:
    virtual ~IProcessorBreakpointProvider() = default;
    virtual bool breakpointAt(uint16_t instructionAddress) const = 0;
};

#endif // PROCESSORMODEL_H
