#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include <QMetaEnum>
#include <QObject>

class Assembly : public QObject
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

    static const QList<Opcodes>& branchJumpOpcodes()
    {
        static const QList<Opcodes> list =
        {
            BEQ, BNE, BMI, BPL, BCS, BCC, BVS, BVC,
            JMP, JSR
        };
        return list;
    }
    static QMetaEnum OpcodesMetaEnum()
    {
        return QMetaEnum::fromType<Opcodes>();
    }
    static Opcodes OpcodesKeyToValue(const char *key)
    {
        return static_cast<Opcodes>(OpcodesMetaEnum().keyToValue(key));
    }
    static bool OpcodesValueIsValid(Opcodes value)
    {
        return value >= 0 && value < OpcodesMetaEnum().keyCount();
    }
    static const char *OpcodesValueToKey(Opcodes value)
    {
        return OpcodesMetaEnum().valueToKey(value);
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

        AbsInd = (Absolute|Indirect),
        AccZPXAbsX = (Accumulator|ZeroPage|ZeroPageX|Absolute|AbsoluteX),
        ImmZPXAbsXYIndXY = (Immediate|ZeroPage|ZeroPageX|Absolute|AbsoluteX|AbsoluteY|IndexedIndirectX|IndirectIndexedY),
        ImmZPYAbsY = (Immediate|ZeroPage|ZeroPageY|Absolute|AbsoluteY),
        ImmZPYAbsX = (Immediate|ZeroPage|ZeroPageX|Absolute|AbsoluteX),
        ImmZPAbs = (Immediate|ZeroPage|Absolute),
        ZPXAbsXYIndXY = (ZeroPage|ZeroPageX|Absolute|AbsoluteX|AbsoluteY|IndexedIndirectX|IndirectIndexedY),
        ZPYAbs = (ZeroPage|ZeroPageY|Absolute),
        ZPXAbs = (ZeroPage|ZeroPageX|Absolute),
        ZPXAbsX = (ZeroPage|ZeroPageX|Absolute|AbsoluteX),
        ZPAbs = (ZeroPage|Absolute),
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

    static const QStringList& directives()
    {
        static const QStringList list =
        {
            ".byte", ".include",
        };
        return list;
    }

    struct OpcodeOperand
    {
        AddressingMode mode;
        int arg;
    };

    struct Instruction
    {
        Opcodes opcode; OpcodeOperand operand;
        Instruction(const Opcodes &_opcode, const OpcodeOperand &_operand)
        {
            opcode = _opcode;
            operand = _operand;
        }
    };

private:
    static OpcodesInfo opcodesInfo[];
};


#endif // ASSEMBLY_H
