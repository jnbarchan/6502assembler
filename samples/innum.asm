
.include "common.inc"

_innum_test:
    jsr _outstr_inline
    .byte "Type a number, terminate by 'Enter': ", 0
    jsr _innum
    lda result
    sta dividend
    lda result+1
    sta dividend+1
    jsr _outstr_inline
    .byte "You typed number: ", 0
    jsr _outnum
    lda #10
    jsr __outch
    rts  ; _innum_test

_innum:
    lda #0
    sta result
    sta result+1
    jsr _instr
    ldx #0
.next_digit:
    lda PAD,X
    beq .done
    cmp #'0'
    bcc .bad_digit
    cmp #'9'+1
    bcs .bad_digit
    lda result
    sta operand1
    lda result+1
    sta operand1+1
    lda #10
    sta operand2
    lda #0
    sta operand2+1
    jsr _mul16
    lda PAD,X
    sec
    sbc #'0'
    clc
    adc result
    sta result
    lda #0
    adc result+1
    sta result+1
    inx
    bne .next_digit
    beq .done
.bad_digit:
.done:
    rts  ; _innum
    
    
.include "instr.asm"

.include "outnum.asm"

.include "mul16.asm"

