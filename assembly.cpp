#include "assembly.h"

//
// Assembler Class
//

Assembly::OpcodesInfo Assembly::opcodesInfo[]
{
    { LDA, ImmZPXAbsXYIndXYFlags },
    { LDX, ImmZPYAbsYFlags },
    { LDY, ImmZPYAbsXFlags },
    { STA, ZPXAbsXYIndXYFlags },
    { STX, ZPYAbsFlags },
    { STY, ZPXAbsFlags },
    { TAX, ImplicitFlag },
    { TAY, ImplicitFlag },
    { TXA, ImplicitFlag },
    { TYA, ImplicitFlag },
    { TSX, ImplicitFlag },
    { TXS, ImplicitFlag },
    { PHA, ImplicitFlag },
    { PHP, ImplicitFlag },
    { PLA, ImplicitFlag },
    { PLP, ImplicitFlag },
    { AND, ImmZPXAbsXYIndXYFlags },
    { EOR, ImmZPXAbsXYIndXYFlags },
    { ORA, ImmZPXAbsXYIndXYFlags },
    { BIT, ZPAbsFlags },
    { ADC, ImmZPXAbsXYIndXYFlags },
    { SBC, ImmZPXAbsXYIndXYFlags },
    { CMP, ImmZPXAbsXYIndXYFlags },
    { CPX, ImmZPAbsFlags },
    { CPY, ImmZPAbsFlags },
    { INC, ZPXAbsXFlags },
    { INX, ImplicitFlag },
    { INY, ImplicitFlag },
    { DEC, ZPXAbsXFlags },
    { DEX, ImplicitFlag },
    { DEY, ImplicitFlag },
    { ASL, AccZPXAbsXFlags },
    { LSR, AccZPXAbsXFlags },
    { ROL, AccZPXAbsXFlags },
    { ROR, AccZPXAbsXFlags },
    { JMP, AbsIndFlags },
    { JSR, AbsoluteFlag },
    { RTS, ImplicitFlag },
    { BCC, RelativeFlag },
    { BCS, RelativeFlag },
    { BEQ, RelativeFlag },
    { BMI, RelativeFlag },
    { BNE, RelativeFlag },
    { BPL, RelativeFlag },
    { BVC, RelativeFlag },
    { BVS, RelativeFlag },
    { CLC, ImplicitFlag },
    { CLD, ImplicitFlag },
    { CLI, ImplicitFlag },
    { CLV, ImplicitFlag },
    { SEC, ImplicitFlag },
    { SED, ImplicitFlag },
    { SEI, ImplicitFlag },
    { BRK, ImplicitFlag },
    { NOP, ImplicitFlag },
    { RTI, ImplicitFlag }
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
    return opcodeInfo.modes & (1 << mode);
}

