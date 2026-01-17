#include "assembly.h"

//
// Assembly Class
//

Assembly::OperationMode Assembly::operationsModes[TotalOperations];

/*static*/ const Assembly::OperationMode &Assembly::getOperationMode(Operation operation)
{
    Q_ASSERT(operation >= 0 && operation < TotalOperations);
    return operationsModes[operation];
}

/*static*/ bool Assembly::operationSupportsAddressingMode(Operation operation, AddressingMode mode)
{
    const OperationMode &operationMode(getOperationMode(operation));
    return operationMode.modes.testFlag(AddressingModeFlag(1 << mode));
}

const Assembly::InstructionInfo Assembly::_instructionsInfo[]
{
    { 0x69, 2, 2, ADC, Immediate },
    { 0x65, 2, 3, ADC, ZeroPage },
    { 0x75, 2, 4, ADC, ZeroPageX },
    { 0x6d, 3, 4, ADC, Absolute },
    { 0x7d, 3, 4, ADC, AbsoluteX },
    { 0x79, 3, 4, ADC, AbsoluteY },
    { 0x61, 2, 6, ADC, IndexedIndirectX },
    { 0x71, 2, 5, ADC, IndirectIndexedY },

    { 0x29, 2, 2, AND, Immediate },
    { 0x25, 2, 3, AND, ZeroPage },
    { 0x35, 2, 4, AND, ZeroPageX },
    { 0x2d, 3, 4, AND, Absolute },
    { 0x3d, 3, 4, AND, AbsoluteX },
    { 0x39, 3, 4, AND, AbsoluteY },
    { 0x21, 2, 6, AND, IndexedIndirectX },
    { 0x31, 2, 5, AND, IndirectIndexedY },

    { 0x0a, 1, 2, ASL, Accumulator },
    { 0x06, 2, 5, ASL, ZeroPage },
    { 0x16, 2, 6, ASL, ZeroPageX },
    { 0x0e, 3, 6, ASL, Absolute },
    { 0x1e, 3, 7, ASL, AbsoluteX },

    { 0x90, 2, 2, BCC, Relative },
    { 0xb0, 2, 2, BCS, Relative },
    { 0xf0, 2, 2, BEQ, Relative },
    { 0x30, 2, 2, BMI, Relative },
    { 0xd0, 2, 2, BNE, Relative },
    { 0x10, 2, 2, BPL, Relative },
    { 0x50, 2, 2, BVC, Relative },
    { 0x70, 2, 2, BVS, Relative },

    { 0x24, 2, 3, BIT, ZeroPage },
    { 0x2c, 3, 4, BIT, Absolute },

    { 0x00, 1, 7, BRK, Implied },
    { 0x18, 1, 2, CLC, Implied },
    { 0xd8, 1, 2, CLD, Implied },
    { 0x58, 1, 2, CLI, Implied },
    { 0xb8, 1, 2, CLV, Implied },

    { 0xc9, 2, 2, CMP, Immediate },
    { 0xc5, 2, 3, CMP, ZeroPage },
    { 0xd5, 2, 4, CMP, ZeroPageX },
    { 0xcd, 3, 4, CMP, Absolute },
    { 0xdd, 3, 4, CMP, AbsoluteX },
    { 0xd9, 3, 4, CMP, AbsoluteY },
    { 0xc1, 2, 6, CMP, IndexedIndirectX },
    { 0xd1, 2, 5, CMP, IndirectIndexedY },

    { 0xe0, 2, 2, CPX, Immediate },
    { 0xe4, 2, 3, CPX, ZeroPage },
    { 0xec, 3, 4, CPX, Absolute },

    { 0xc0, 2, 2, CPY, Immediate },
    { 0xc4, 2, 3, CPY, ZeroPage },
    { 0xcc, 3, 4, CPY, Absolute },

    { 0xc6, 2, 5, DEC, ZeroPage },
    { 0xd6, 2, 6, DEC, ZeroPageX },
    { 0xce, 3, 6, DEC, Absolute },
    { 0xde, 3, 7, DEC, AbsoluteX },

    { 0xca, 1, 2, DEX, Implied },
    { 0x88, 1, 2, DEY, Implied },

    { 0x49, 2, 2, EOR, Immediate },
    { 0x45, 2, 3, EOR, ZeroPage },
    { 0x55, 2, 4, EOR, ZeroPageX },
    { 0x4d, 3, 4, EOR, Absolute },
    { 0x5d, 3, 4, EOR, AbsoluteX },
    { 0x59, 3, 4, EOR, AbsoluteY },
    { 0x41, 2, 6, EOR, IndexedIndirectX },
    { 0x51, 2, 5, EOR, IndirectIndexedY },

    { 0xe6, 2, 5, INC, ZeroPage },
    { 0xf6, 2, 6, INC, ZeroPageX },
    { 0xee, 3, 6, INC, Absolute },
    { 0xfe, 3, 7, INC, AbsoluteX },

    { 0xe8, 1, 2, INX, Implied },
    { 0xc8, 1, 2, INY, Implied },

    { 0x4c, 3, 3, JMP, Absolute },
    { 0x6c, 3, 5, JMP, Indirect },
    { 0x20, 3, 6, JSR, Absolute },

    { 0xa9, 2, 2, LDA, Immediate },
    { 0xa5, 2, 3, LDA, ZeroPage },
    { 0xb5, 2, 4, LDA, ZeroPageX },
    { 0xad, 3, 4, LDA, Absolute },
    { 0xbd, 3, 4, LDA, AbsoluteX },
    { 0xb9, 3, 4, LDA, AbsoluteY },
    { 0xa1, 2, 6, LDA, IndexedIndirectX },
    { 0xb1, 2, 5, LDA, IndirectIndexedY },

    { 0xa2, 2, 2, LDX, Immediate },
    { 0xa6, 2, 3, LDX, ZeroPage },
    { 0xb6, 2, 4, LDX, ZeroPageY },
    { 0xae, 3, 4, LDX, Absolute },
    { 0xbe, 3, 4, LDX, AbsoluteY },

    { 0xa0, 2, 2, LDY, Immediate },
    { 0xa4, 2, 3, LDY, ZeroPage },
    { 0xb4, 2, 4, LDY, ZeroPageX },
    { 0xac, 3, 4, LDY, Absolute },
    { 0xbc, 3, 4, LDY, AbsoluteX },

    { 0x4a, 1, 2, LSR, Accumulator },
    { 0x46, 2, 5, LSR, ZeroPage },
    { 0x56, 2, 6, LSR, ZeroPageX },
    { 0x4e, 3, 6, LSR, Absolute },
    { 0x5e, 3, 7, LSR, AbsoluteX },

    { 0xea, 1, 2, NOP, Implied },

    { 0x09, 2, 2, ORA, Immediate },
    { 0x05, 2, 3, ORA, ZeroPage },
    { 0x15, 2, 4, ORA, ZeroPageX },
    { 0x0d, 3, 4, ORA, Absolute },
    { 0x1d, 3, 4, ORA, AbsoluteX },
    { 0x19, 3, 4, ORA, AbsoluteY },
    { 0x01, 2, 6, ORA, IndexedIndirectX },
    { 0x11, 2, 5, ORA, IndirectIndexedY },

    { 0x48, 1, 3, PHA, Implied },
    { 0x08, 1, 3, PHP, Implied },
    { 0x68, 1, 4, PLA, Implied },
    { 0x28, 1, 4, PLP, Implied },

    { 0x2a, 1, 2, ROL, Accumulator },
    { 0x26, 2, 5, ROL, ZeroPage },
    { 0x36, 2, 6, ROL, ZeroPageX },
    { 0x2e, 3, 6, ROL, Absolute },
    { 0x3e, 3, 7, ROL, AbsoluteX },

    { 0x6a, 1, 2, ROR, Accumulator },
    { 0x66, 2, 5, ROR, ZeroPage },
    { 0x76, 2, 6, ROR, ZeroPageX },
    { 0x6e, 3, 6, ROR, Absolute },
    { 0x7e, 3, 7, ROR, AbsoluteX },

    { 0x40, 1, 6, RTI, Implied },
    { 0x60, 1, 6, RTS, Implied },

    { 0xe9, 2, 2, SBC, Immediate },
    { 0xe5, 2, 3, SBC, ZeroPage },
    { 0xf5, 2, 4, SBC, ZeroPageX },
    { 0xed, 3, 4, SBC, Absolute },
    { 0xfd, 3, 4, SBC, AbsoluteX },
    { 0xf9, 3, 4, SBC, AbsoluteY },
    { 0xe1, 2, 6, SBC, IndexedIndirectX },
    { 0xf1, 2, 5, SBC, IndirectIndexedY },

    { 0x38, 1, 2, SEC, Implied },
    { 0xf8, 1, 2, SED, Implied },
    { 0x78, 1, 2, SEI, Implied },

    { 0x85, 2, 3, STA, ZeroPage },
    { 0x95, 2, 4, STA, ZeroPageX },
    { 0x8d, 3, 4, STA, Absolute },
    { 0x9d, 3, 5, STA, AbsoluteX },
    { 0x99, 3, 5, STA, AbsoluteY },
    { 0x81, 2, 6, STA, IndexedIndirectX },
    { 0x91, 2, 6, STA, IndirectIndexedY },

    { 0x86, 2, 3, STX, ZeroPage },
    { 0x96, 2, 4, STX, ZeroPageY },
    { 0x8e, 3, 4, STX, Absolute },

    { 0x84, 2, 3, STY, ZeroPage },
    { 0x94, 2, 4, STY, ZeroPageX },
    { 0x8c, 3, 4, STY, Absolute },

    { 0xaa, 1, 2, TAX, Implied },
    { 0xa8, 1, 2, TAY, Implied },
    { 0xba, 1, 2, TSX, Implied },
    { 0x8a, 1, 2, TXA, Implied },
    { 0x9a, 1, 2, TXS, Implied },
    { 0x98, 1, 2, TYA, Implied },
};

Assembly::InstructionInfo Assembly::instructionsInfo[TotalInstructions];


/*static*/ void Assembly::initInstructionInfo()
{
    for (int i = 0; i < TotalOperations; i++)
        operationsModes[i].modes = AddressingModeFlags(0);

    for (int i = 0; i < sizeof(_instructionsInfo) / sizeof(_instructionsInfo[0]); i++)
    {
        const InstructionInfo &info(_instructionsInfo[i]);
        Q_ASSERT(info.isValid());
        Q_ASSERT(info.bytes > 0 && info.bytes <= 3 && info.cycles > 0);
        switch (info.bytes)
        {
        case 1: Q_ASSERT(info.addrMode == Implied || info.addrMode == Accumulator); break;
        case 2: Q_ASSERT(info.addrMode == Immediate || info.addrMode == ZeroPage || info.addrMode == ZeroPageX || info.addrMode == ZeroPageY
                     || info.addrMode == Relative || info.addrMode == IndexedIndirectX || info.addrMode == IndirectIndexedY); break;
        case 3: Q_ASSERT(info.addrMode == Absolute || info.addrMode == AbsoluteX || info.addrMode == AbsoluteY || info.addrMode == Indirect); break;
        default: Q_ASSERT(false); break;
        }

        Q_ASSERT(!instructionsInfo[info.opcodeByte].isValid());
        instructionsInfo[info.opcodeByte] = info;

        Q_ASSERT(info.operation < TotalOperations);
        operationsModes[info.operation].modes.setFlag(AddressingModeFlag(1 << info.addrMode));
    }
    QMetaEnum me = OperationsMetaEnum();
    for (int i = 0; i < me.keyCount(); i++)
    {
        Operation value = static_cast<Operation>(me.value(i));
        bool found = false;
        for (int j = 0; j < TotalInstructions && !found; j++)
            found = instructionsInfo[j].operation == value;
        Q_ASSERT(found);
    }
    for (int i = 0; i < TotalOperations; i++)
        Q_ASSERT(operationsModes[i].modes != AddressingModeFlags(0));
}

const Assembly::InstructionInfo *Assembly::findInstructionInfo(Operation operation, AddressingMode addrMode)
{
    for (int i = 0; i < TotalInstructions; i++)
        if (instructionsInfo[i].operation == operation && instructionsInfo[i].addrMode == addrMode)
            return &instructionsInfo[i];
    return nullptr;
}

