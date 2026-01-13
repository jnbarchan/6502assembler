#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include <QMetaEnum>
#include <QObject>

class Assembly : public QObject
{
    Q_OBJECT
public:
    enum Opcodes : uint8_t
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
        return key ? QString(key) : QString::number(value, 16);
    }

    enum AddressingMode : uint8_t
    {
        Implicit = 0,           // CLC | RTS
        Accumulator = 1,        // LSR A | ROR A
        Immediate = 2,          // LDA #10 | LDX #<LABEL | LDY #>LABEL
        ZeroPage = 3,           // LDA $00 | ASL ANSWER
        ZeroPageX = 4,          // STY $10,X | AND TEMP,X
        ZeroPageY = 5,          // LDX $10,Y | STX TEMP,Y
        Relative = 6,           // BEQ LABEL | BNE *+4
        Absolute = 7,           // JMP $1234 | JSR WIBBLE
        AbsoluteX = 8,          // STA $3000,X | ROR CRC,X
        AbsoluteY = 9,          // AND $4000,Y | STA MEM,Y
        Indirect = 10,          // JMP ($FFFC) | JMP (TARGET)
        IndexedIndirectX = 11,  // LDA ($40,X) | STA (MEM,X)
        IndirectIndexedY = 12,  // LDA ($40),Y | STA (DST),Y
    };
    Q_ENUM(AddressingMode)

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
        return key ? QString(key) : QString::number(value, 16);
    }

    enum AddressingModeFlag : uint16_t
    {
        ImplicitFlag = 0x0001,
        AccumulatorFlag = 0x0002,
        ImmediateFlag = 0x0004,
        ZeroPageFlag = 0x0008,
        ZeroPageXFlag = 0x0010,
        ZeroPageYFlag = 0x0020,
        RelativeFlag = 0x0040,
        AbsoluteFlag = 0x0080,
        AbsoluteXFlag = 0x0100,
        AbsoluteYFlag = 0x0200,
        IndirectFlag = 0x0400,
        IndexedIndirectXFlag = 0x0800,
        IndirectIndexedYFlag = 0x1000,

        AbsIndFlags = (AbsoluteFlag|IndirectFlag),
        AccZPXAbsXFlags = (AccumulatorFlag|ZeroPageFlag|ZeroPageXFlag|AbsoluteFlag|AbsoluteXFlag),
        ImmZPXAbsXYIndXYFlags = (ImmediateFlag|ZeroPageFlag|ZeroPageXFlag|AbsoluteFlag|AbsoluteXFlag|AbsoluteYFlag|IndexedIndirectXFlag|IndirectIndexedYFlag),
        ImmZPYAbsYFlags = (ImmediateFlag|ZeroPageFlag|ZeroPageYFlag|AbsoluteFlag|AbsoluteYFlag),
        ImmZPYAbsXFlags = (ImmediateFlag|ZeroPageFlag|ZeroPageXFlag|AbsoluteFlag|AbsoluteXFlag),
        ImmZPAbsFlags = (ImmediateFlag|ZeroPageFlag|AbsoluteFlag),
        ZPXAbsXYIndXYFlags = (ZeroPageFlag|ZeroPageXFlag|AbsoluteFlag|AbsoluteXFlag|AbsoluteYFlag|IndexedIndirectXFlag|IndirectIndexedYFlag),
        ZPYAbsFlags = (ZeroPageFlag|ZeroPageYFlag|AbsoluteFlag),
        ZPXAbsFlags = (ZeroPageFlag|ZeroPageXFlag|AbsoluteFlag),
        ZPXAbsXFlags = (ZeroPageFlag|ZeroPageXFlag|AbsoluteFlag|AbsoluteXFlag),
        ZPAbsFlags = (ZeroPageFlag|AbsoluteFlag),
    };
    Q_FLAG(AddressingModeFlag)
    // Q_DECLARE_FLAGS(AddressingModeFlags, AddressingModeFlag)
    // Q_FLAG(AddressingModeFlags)

    struct OpcodesInfo
    {
        Opcodes opcode;
        AddressingModeFlag modes;
    };

    static const OpcodesInfo &getOpcodeInfo(const Opcodes opcode);
    static bool opcodeSupportsAddressingMode(const Opcodes opcode, AddressingMode mode);

    static const QStringList& directives()
    {
        static const QStringList list =
        {
            ".byte", ".include", ".break", ".org",
        };
        return list;
    }

    struct __attribute__((packed)) OpcodeOperand
    {
        AddressingMode mode;
        uint16_t arg;
    };

    struct __attribute__((packed)) Instruction
    {
        Opcodes opcode; OpcodeOperand operand;
        Instruction(const Opcodes &_opcode, const OpcodeOperand &_operand)
        {
            opcode = _opcode;
            operand = _operand;
        }
    };

    enum InternalJSRs { __JSR_outch = 0xeffe, __JSR_get_time = 0xeffc, __JSR_get_elapsed_time = 0xeffa, };

private:
    static OpcodesInfo opcodesInfo[];
};


#endif // ASSEMBLY_H
