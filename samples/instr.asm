.include "common.inc"

_instr_test:
    jsr _outstr_inline
    .byte "Type some characters, terminate by 'Enter': ", 0
    jsr _instr
    jsr _outstr_inline
    .byte "You typed: ", 0
    jsr _outstr
    lda #10
    jsr __outch
    rts  ; _instr_test

_instr:
    ldx #0
.next_char:
    jsr __inch
    beq .done
    jsr __outch
    cmp #13
    beq .done
    sta PAD,X
    inx
    bne .next_char
.done:
    lda #0
    sta PAD,X
    rts  ; _instr

.include "outstr.asm"
