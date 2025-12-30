#include "assembly.h"

//
// Assembler Class
//

Assembly::OpcodesInfo Assembly::opcodesInfo[]
    {
        { LDA, ImmZPXAbsXYIndXY },
        { LDX, ImmZPYAbsY },
        { LDY, ImmZPYAbsX },
        { STA, ZPXAbsXYIndXY },
        { STX, ZPYAbs },
        { STY, ZPXAbs },
        { TAX, Implicit },
        { TAY, Implicit },
        { TXA, Implicit },
        { TYA, Implicit },
        { TSX, Implicit },
        { TXS, Implicit },
        { PHA, Implicit },
        { PHP, Implicit },
        { PLA, Implicit },
        { PLP, Implicit },
        { AND, ImmZPXAbsXYIndXY },
        { EOR, ImmZPXAbsXYIndXY },
        { ORA, ImmZPXAbsXYIndXY },
        { BIT, ZPAbs },
        { ADC, ImmZPXAbsXYIndXY },
        { SBC, ImmZPXAbsXYIndXY },
        { CMP, ImmZPXAbsXYIndXY },
        { CPX, ImmZPAbs },
        { CPY, ImmZPAbs },
        { INC, ZPXAbsX },
        { INX, Implicit },
        { INY, Implicit },
        { DEC, ZPXAbsX },
        { DEX, Implicit },
        { DEY, Implicit },
        { ASL, AccZPXAbsX },
        { LSR, AccZPXAbsX },
        { ROL, AccZPXAbsX },
        { ROR, AccZPXAbsX },
        { JMP, AbsInd },
        { JSR, Absolute },
        { RTS, Implicit },
        { BCC, Relative },
        { BCS, Relative },
        { BEQ, Relative },
        { BMI, Relative },
        { BNE, Relative },
        { BPL, Relative },
        { BVC, Relative },
        { BVS, Relative },
        { CLC, Implicit },
        { CLD, Implicit },
        { CLI, Implicit },
        { CLV, Implicit },
        { SEC, Implicit },
        { SED, Implicit },
        { SEI, Implicit },
        { BRK, Implicit },
        { NOP, Implicit },
        { RTI, Implicit }
    };

/*static*/ const Assembly::OpcodesInfo &Assembly::getOpcodeInfo(const Opcodes opcode)
{
    Q_ASSERT(opcode >= 0 && opcode < sizeof(opcodesInfo) / sizeof(opcodesInfo[0]));
    const OpcodesInfo &opcodeInfo(opcodesInfo[opcode]);
    return opcodesInfo[opcode];
}

/*static*/ bool Assembly::opcodeSupportsAddressingMode(const Opcodes opcode, AddressingMode mode)
{
    const OpcodesInfo &opcodeInfo(getOpcodeInfo(opcode));
    return opcodeInfo.modes & mode;
}

