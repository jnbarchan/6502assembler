#ifndef PROCESSORMODEL_H
#define PROCESSORMODEL_H

#include <QAbstractItemModel>
#include <QBrush>
#include <QElapsedTimer>
#include <QMetaEnum>
#include <QObject>

#include "assembly.h"

using Opcodes = Assembly::Opcodes;
using AddressingMode = Assembly::AddressingMode;
using AddressingModeFlag = Assembly::AddressingModeFlag;
using OpcodeOperand = Assembly::OpcodeOperand;
using Instruction = Assembly::Instruction;
using InternalJSRs = Assembly::InternalJSRs;


class MemoryModel;

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
    enum RunMode { Run, StepInto, StepOver, StepOut, Continue };
    Q_ENUM(RunMode)

    explicit ProcessorModel(QObject *parent = nullptr);

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

    char *memory();
    unsigned int memorySize() const;
    uint8_t memoryByteAt(uint16_t address) const;
    void setMemoryByteAt(uint16_t address, uint8_t value);
    uint16_t memoryWordAt(uint16_t address) const;
    uint16_t memoryZPWordAt(uint8_t address) const;

    Instruction *instructions() const;

    uint16_t programCounter() const;
    void setProgramCounter(uint16_t newProgramCounter);

    const QList<uint16_t> *breakpoints() const;
    void setBreakpoints(const QList<uint16_t> *newBreakpoints);

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
    void run();
    void continueRun();
    void stepInto();
    void stepOver();
    void stepOut();

signals:
    void sendMessageToConsole(const QString &message, QBrush colour = Qt::transparent) const;
    void sendCharToConsole(char ch) const;
    void modelReset();
    void stopRunChanged();
    void accumulatorChanged();
    void xregisterChanged();
    void yregisterChanged();
    void programCounterChanged();
    void stackRegisterChanged();
    void statusFlagsChanged();
    void memoryChanged(uint16_t address);
    void currentInstructionAddressChanged(uint16_t instructionAddress);

private:
    char _memoryData[64 * 1024];
    char *_memory;
    MemoryModel *_memoryModel;
    const uint16_t _stackBottom = 0x0100;
    uint8_t _stackRegister;
    uint8_t _accumulator, _xregister, _yregister;
    uint8_t _statusFlags;

    Instruction *_instructions;
    uint16_t _programCounter;
    const QList<uint16_t> *_breakpoints;

    bool _startNewRun, _stopRun, _isRunning;

    QElapsedTimer elapsedTimer;

    void resetModel();
    void debugMessage(const QString &message) const;
    void executionErrorMessage(const QString &message) const;
    void runInstructions(RunMode runMode);
    void runNextInstruction(const Opcodes &opcode, const OpcodeOperand &operand);
    void executeNextInstruction(const Opcodes &opcode, const OpcodeOperand &operand);
    void setNZStatusFlags(uint8_t value);
    void jumpTo(uint16_t instructionAddress);
    void jsr_brk_handler();
    void jsr_outch();
    void jsr_get_time();
    void jsr_get_elapsed_time();
};


class MemoryModel : public QAbstractTableModel
{
    Q_OBJECT

    // QAbstractItemModel interface
public:
    MemoryModel(QObject *parent);

    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    int indexToAddress(const QModelIndex &index) const { return index.isValid() ? index.row() * columnCount() + index.column() : -1; }
    QModelIndex addressToIndex(int address) const { return address >= 0 ? index(address / columnCount(), address % columnCount()) : QModelIndex(); }

private:
    ProcessorModel *processorModel;
    int lastMemoryChangedAddress;

public slots:
    void notifyAllDataChanged();
    void clearLastMemoryChanged();

private slots:
    void memoryChanged(uint16_t address);
};


class ExecutionError : public std::runtime_error
{
public:
    ExecutionError(const QString &msg);
};
#endif // PROCESSORMODEL_H
