#ifndef PROCESSORMODEL_H
#define PROCESSORMODEL_H

#include <QAbstractItemModel>
#include <QMetaEnum>
#include <QObject>

#include "assembly.h"

using Opcodes = Assembly::Opcodes;
using AddressingMode = Assembly::AddressingMode;
using OpcodeOperand = Assembly::OpcodeOperand;
using Instruction = Assembly::Instruction;


class MemoryModel;

class ProcessorModel : public QObject
{
    Q_OBJECT
public:
    enum StatusFlags
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
    enum RunMode { Run, StepInto, StepOver, StepOut };

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

    uint8_t memoryByteAt(uint16_t address) const;
    void setMemoryByteAt(uint16_t address, uint8_t value);
    uint16_t memoryWordAt(uint16_t address) const;
    uint16_t memoryZPWordAt(uint8_t address) const;
    unsigned int memorySize() const;

    const QList<Instruction> *instructions() const;
    void setInstructions(const QList<Instruction> *newInstructions);

    int currentInstructionNumber() const;
    void setCurrentInstructionNumber(int newCurrentInstructionNumber);

    bool isRunning() const;
    void setIsRunning(bool newIsRunning);
    bool stopRun() const;
    void setStopRun(bool newStopRun);

    bool startNewRun() const;
    void setStartNewRun(bool newStartNewRun);

public slots:
    void restart();
    void endRun();
    void stop();
    void run();
    void stepInto();
    void stepOver();
    void stepOut();

signals:
    void sendMessageToConsole(const QString &message) const;
    void sendCharToConsole(char ch) const;
    void modelReset();
    void stopRunChanged();
    void accumulatorChanged();
    void xregisterChanged();
    void yregisterChanged();
    void stackRegisterChanged();
    void statusFlagsChanged();
    void memoryChanged(uint16_t address);
    void currentInstructionNumberChanged(int instructionNumber);

private:
    QByteArray _memory;
    MemoryModel *_memoryModel;
    const uint16_t _stackBottom = 0x0100;
    uint8_t _stackRegister;
    uint8_t _accumulator, _xregister, _yregister;
    uint8_t _statusFlags;

    const QList<Instruction> *_instructions;
    int _currentInstructionNumber;

    bool _startNewRun, _stopRun, _isRunning;

    void resetModel();
    void debugMessage(const QString &message);
    void run(RunMode runMode);
    void runNextStatement(const Opcodes &opcode, const OpcodeOperand &operand);
    void executeNextStatement(const Opcodes &opcode, const OpcodeOperand &operand);
    void setNZStatusFlags(uint8_t value);
    void jumpTo(uint16_t instructionNumber);
    void jsr_outch();
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
    void clearLastMemoryChanged();

private slots:
    void memoryChanged(uint16_t address);
};

#endif // PROCESSORMODEL_H
