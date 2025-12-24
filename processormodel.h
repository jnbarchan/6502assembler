#ifndef PROCESSORMODEL_H
#define PROCESSORMODEL_H

#include "qabstractitemmodel.h"
#include <QMetaEnum>
#include <QObject>
#include <QTextStream>

class Assembler : public QObject
{
    Q_OBJECT
public:
    enum Opcodes
    {
        LDA, LDX, LDY, STA, STX, STY,
        TAX, TAY, TXA, TYA,
        TSX, TXS, PHA, PHP, PLA, PLP,
        AND, EOR, ORA, BIT,
        ADC, SBC, CMP, CPX, CPY,
        INC, INX, INY, DEC, DEX, DEY,
        ASL, LSR, ROL, ROR,
        JMP, JSR, RTS,
        BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS,
        CLC, CLD, CLI, CLV, SEC, SED, SEI,
        BRK, NOP, RTI
    };
    Q_ENUM(Opcodes)

    static Opcodes OpcodesKeyToValue(const char *key)
    {
        return static_cast<Opcodes>(QMetaEnum::fromType<Opcodes>().keyToValue(key));
    }
    static bool OpcodesValueIsValid(Opcodes value)
    {
        return value >= 0 && value < QMetaEnum::fromType<Opcodes>().keyCount();
    }
    static const char *OpcodesValueToKey(Opcodes value)
    {
        return QMetaEnum::fromType<Opcodes>().valueToKey(value);
    }
    static QString OpcodesValueToString(Opcodes value)
    {
        const char *key = OpcodesValueToKey(value);
        return key ? QString(key) : QString::number(value);
    }

    enum AddressingMode
    {
        Implicit = 0x0001,           // CLC | RTS
        Accumulator = 0x0002,        // LSR A | ROR A
        Immediate = 0x0004,          // LDA #10 | LDX #<LABEL | LDY #>LABEL
        ZeroPage = 0x0008,           // LDA $00 | ASL ANSWER
        ZeroPageX = 0x0010,          // STY $10,X | AND TEMP,X
        ZeroPageY = 0x0020,          // LDX $10,Y | STX TEMP,Y
        Relative = 0x0040,           // BEQ LABEL | BNE *+4
        Absolute = 0x0080,           // JMP $1234 | JSR WIBBLE
        AbsoluteX = 0x0100,          // STA $3000,X | ROR CRC,X
        AbsoluteY = 0x0200,          // AND $4000,Y | STA MEM,Y
        Indirect = 0x0400,           // JMP ($FFFC) | JMP (TARGET)
        IndexedIndirectX = 0x0800,   // LDA ($40,X) | STA (MEM,X)
        IndirectIndexedY = 0x1000,   // LDA ($40),Y | STA (DST),Y
    };
    Q_FLAG(AddressingMode)
    // Q_DECLARE_FLAGS(AddressingModes, AddressingMode)
    // Q_FLAG(AddressingModes)

    static AddressingMode AddressingModeKeyToValue(const char *key)
    {
        return static_cast<AddressingMode>(QMetaEnum::fromType<AddressingMode>().keyToValue(key));
    }
    static bool AddressingModeValueIsValid(AddressingMode value)
    {
        return value >= 0 && value < QMetaEnum::fromType<AddressingMode>().keyCount();
    }
    static const char *AddressingModeValueToKey(AddressingMode value)
    {
        return QMetaEnum::fromType<AddressingMode>().valueToKey(value);
    }
    static QString AddressingModeValueToString(AddressingMode value)
    {
        const char *key = AddressingModeValueToKey(value);
        return key ? QString(key) : QString::number(value);
    }

    struct OpcodesInfo
    {
        Opcodes opcode;
        unsigned modes;
    };

    static const OpcodesInfo &getOpcodeInfo(const Opcodes opcode);
    static bool opcodeSupportsAddressingMode(const Opcodes opcode, AddressingMode mode);

    struct OpcodeOperand
    {
        AddressingMode mode;
        int arg;
    };

private:
    static constexpr unsigned AbsInd = (Absolute|Indirect);
    static constexpr unsigned AccZPXAbsX = (Accumulator|ZeroPage|ZeroPageX|Absolute|AbsoluteX);
    static constexpr unsigned ImmZPXAbsXYIndXY = (Immediate|ZeroPage|ZeroPageX|Absolute|AbsoluteX|AbsoluteY|IndexedIndirectX|IndirectIndexedY);
    static constexpr unsigned ImmZPYAbsY = (Immediate|ZeroPage|ZeroPageY|Absolute|AbsoluteY);
    static constexpr unsigned ImmZPYAbsX = (Immediate|ZeroPage|ZeroPageX|Absolute|AbsoluteX);
    static constexpr unsigned ImmZPAbs = (Immediate|ZeroPage|Absolute);
    static constexpr unsigned ZPXAbsXYIndXY = (ZeroPage|ZeroPageX|Absolute|AbsoluteX|AbsoluteY|IndexedIndirectX|IndirectIndexedY);
    static constexpr unsigned ZPYAbs = (ZeroPage|ZeroPageY|Absolute);
    static constexpr unsigned ZPXAbs = (ZeroPage|ZeroPageX|Absolute);
    static constexpr unsigned ZPXAbsX = (ZeroPage|ZeroPageX|Absolute|AbsoluteX);
    static constexpr unsigned ZPAbs = (ZeroPage|Absolute);
    static OpcodesInfo opcodesInfo[];
};

using Opcodes = Assembler::Opcodes;
using AddressingMode = Assembler::AddressingMode;
using OpcodeOperand = Assembler::OpcodeOperand;


class MemoryModel;
class ProcessorModel;
extern ProcessorModel *g_processorModel;

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

    int currentCodeLineNumber() const;
    void setCurrentCodeLineNumber(int newCurrentCodeLineNumber);

    void setCodeLabel(const QString &key, int value);

    void setCode(QTextStream *codeStream);

    bool isRunning() const;
    void setIsRunning(bool newIsRunning);
    bool stopRun() const;
    void setStopRun(bool newStopRun);

public slots:
    void restart(bool assemblePass2 = false);
    void stop();
    void run();
    void step();

signals:
    void sendMessageToConsole(const QString &message) const;
    void sendCharToConsole(char ch) const;
    void modelReset();
    void accumulatorChanged();
    void xregisterChanged();
    void yregisterChanged();
    void stackRegisterChanged();
    void statusFlagsChanged();
    void memoryChanged(uint16_t address);
    void currentCodeLineNumberChanged(int lineNumber);

private:
    QByteArray _memory;
    MemoryModel *_memoryModel;
    const uint16_t _stackBottom = 0x0100;
    uint8_t _stackRegister;
    uint8_t _accumulator, _xregister, _yregister;
    uint8_t _statusFlags;

    QTextStream *_codeStream;
    QStringList _codeLines;
    int _currentCodeLineNumber;
    QMap<QString, int> _codeLabels;
    bool _codeLabelRequiresColon = true;

    QString _currentLine, _currentToken;
    QTextStream _currentLineStream;

    bool doneAssemblePass1;
    bool _stopRun, _isRunning;

    void resetModel();
    void debugMessage(const QString &message);
    void runNextStatement();
    void assemblePass1();
    bool assembleNextStatement(bool &hasOpcode, Opcodes &opcode, OpcodeOperand &operand, bool &eof);
    bool getNextLine();
    bool getNextToken();
    bool tokenIsLabel() const;
    bool tokenIsInt() const;
    int tokenToInt(bool *ok) const;
    int tokenValueAsInt(bool *ok) const;
    void assignLabelValue(const QString &label, int value);
    void executeNextStatement(const Opcodes &opcode, const OpcodeOperand &operand);
    void setNZStatusFlags(uint8_t value);
    void jumpTo(uint16_t lineNumber);
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

private:
    ProcessorModel *processorModel;
    int lastMemoryChangedAddress;

    int indexToAddress(const QModelIndex &index) const { return index.isValid() ? index.row() * columnCount() + index.column() : -1; }
    QModelIndex addressToIndex(int address) const { return address >= 0 ? index(address / columnCount(), address % columnCount()) : QModelIndex(); }

private slots:
    void memoryChanged(uint16_t address);
};

#endif // PROCESSORMODEL_H
