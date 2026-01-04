
pad = $0400
pad_top = $100-1

x_save = $80

dividend = $70
divisor = $72
quotient = $74
remainder = $76

_outnum_test:
    lda #$ff
    sta dividend
    lda #$ff
    sta dividend+1

    jsr _outnum
    rts  ; _outnum_test

_outnum:
    lda #10
    sta divisor
    lda #0
    sta divisor+1

    lda #0
    ldx #pad_top
    sta pad,X
    dex
    stx x_save

    jsr .next_digit

    ldx x_save
    inx
    jsr _outstr_X
    rts  ; _outnum

.next_digit:
    jsr _div16

    lda remainder
    clc
    adc #'0'
    ldx x_save
    sta pad,X
    dex
    stx x_save

    lda quotient
    ora quotient+1
    beq .done_digits

    lda quotient
    sta dividend
    lda quotient+1
    sta dividend+1
    jmp .next_digit
.done_digits:
    rts  ; .next_digit

_outdigit:
    clc
    adc #'0'
    jmp __outch  ; _outdigit


.include "outstr.asm"

.include "div16.asm"

