; instr.asm

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
    
    jsr _outstr_inline
    .byte "Now type one character, within 5 seconds: ", 0
    ldx #>500
    lda #<500
    jsr __inkey
    pha
    lda #10
    jsr __outch
    pla
    beq .timeout
    pha
    jsr _outstr_inline
    .byte "You typed: ", 0
    pla
    jsr __outch
    jmp .done
.timeout:
    jsr _outstr_inline
    .byte "Timed out", 0
.done:
    lda #10
    jsr __outch
    rts  ; _instr_test

_instr:
    ldx #0
.next_char:
    jsr __inch
    beq .done
    cmp #8
    bne .not_backspace
    cpx #0
    beq .next_char
    dex
    jsr __outch
    jmp .next_char
.not_backspace:
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
