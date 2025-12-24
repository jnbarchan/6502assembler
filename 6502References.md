# 6502 References


https://en.wikipedia.org/wiki/MOS_Technology_6502


http://www.6502.org/users/obelisk/6502/instructions.html


https://retroscience.net/writing-6502-assembler.html


https://www.masswerk.at/6502/6502_instruction_set.html


## bnf grammar for 6502

```
<line> ::= [ <label> ] [ <instruction> ] [ <comment> ] <end-of-line>
  
<label> ::= <identifier> ":" | <identifier>
<instruction> ::= <opcode> [ <operand> ]
<comment> ::= ";" { <any-character> }
<end-of-line> ::= <new-line> | <end-of-file>
  
<opcode> ::= "LDA" | "STA" | "ADC" | ... (all 6502 mnemonics)
<operand> ::= <immediate> | <absolute> | <zeropage> | <indirect> | ... (various addressing modes)
  
<immediate> ::= "#" <value>
<absolute> ::= "$" <hex-value> | <decimal-value> | <identifier>
<zeropage> ::= "<" <absolute> | ">" <absolute> | ... (assembler specific syntax for high/low byte)
<indirect> ::= "(" <addressing-mode-specific-syntax> ")"
<value> ::= <hex-value> | <decimal-value> | <binary-value>
```
