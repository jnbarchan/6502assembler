#ifndef ASSEMBLY_H
#define ASSEMBLY_H

#include <QMetaEnum>
#include <QObject>

class Assembly : public QObject
{
    Q_OBJECT
public:
    enum Operation : uint8_t
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
    Q_ENUM(Operation)
    static constexpr int TotalOperations = RTI + 1;
    static_assert(TotalOperations == 56);

    static const QList<Operation>& branchJumpOperations()
    {
        static const QList<Operation> list =
        {
            BEQ, BNE, BMI, BPL, BCS, BCC, BVS, BVC,
            JMP, JSR
        };
        return list;
    }
    static QMetaEnum OperationsMetaEnum()
    {
        return QMetaEnum::fromType<Operation>();
    }
    static Operation OperationKeyToValue(const char *key)
    {
        return static_cast<Operation>(OperationsMetaEnum().keyToValue(key));
    }
    static bool OperationValueIsValid(Operation value)
    {
        return value >= 0 && value < OperationsMetaEnum().keyCount();
    }
    static const char *OperationValueToKey(Operation value)
    {
        return OperationsMetaEnum().valueToKey(value);
    }
    static QString OperationValueToString(Operation value)
    {
        const char *key = OperationValueToKey(value);
        return key ? QString(key) : QString::number(value, 16);
    }

    enum AddressingMode : uint8_t
    {
        Implied = 0,            // CLC | RTS
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
        ImpliedFlag = 0x0001,
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
    };
    Q_FLAG(AddressingModeFlag)
    Q_DECLARE_FLAGS(AddressingModeFlags, AddressingModeFlag)
    Q_FLAG(AddressingModeFlags)

    struct OperationMode
    {
        Operation operation;
        AddressingModeFlags modes;
    };

    static const OperationMode &getOperationMode(Operation operation);
    static bool operationSupportsAddressingMode(Operation operation, AddressingMode mode);

    static const QStringList& directives()
    {
        static const QStringList list =
        {
            ".byte", ".include", ".break", ".org",
        };
        return list;
    }

    struct InstructionInfo
    {
        uint8_t opcodeByte;
        uint8_t bytes;
        uint8_t cycles;
        Operation operation;
        AddressingMode addrMode;

        bool isValid() const { return bytes != 0; }
    };
    static constexpr int TotalInstructions = 256;

    static void initInstructionInfo();
    static const InstructionInfo &getInstructionInfo(uint8_t opcodeByte) { return instructionsInfo[opcodeByte]; }
    static const InstructionInfo *findInstructionInfo(Operation operation, AddressingMode addrMode);

    struct __attribute__((packed)) Instruction
    {
        uint8_t opcodeByte;
        uint16_t operand;
        Instruction(uint8_t _opcodeByte, uint16_t _operand)
        {
            opcodeByte = _opcodeByte;
            operand = _operand;
        }

        const InstructionInfo &getInstructionInfo() const { return instructionsInfo[opcodeByte]; }
    };

    enum InternalJSRs { __JSR_terminate = 0x0000, __JSR_brk_handler = 0xfffe, __JSR_outch = 0xfffc, __JSR_get_time = 0xfffa,
                        __JSR_get_elapsed_time = 0xfff8, __JSR_clear_elapsed_time = 0xfff6, };

private:
    static OperationMode operationsModes[TotalOperations];
    static const InstructionInfo _instructionsInfo[];
    static InstructionInfo instructionsInfo[TotalInstructions];
};


#endif // ASSEMBLY_H
