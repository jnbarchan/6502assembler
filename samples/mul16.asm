
.include "common.inc"

_mul16_test:
    lda #$ff
    sta operand1
    lda #$02
    sta operand1+1

    lda #$03
    sta operand2
    lda #$00
    sta operand2+1

    jsr _mul16
    rts  ; _mul16_test

_mul16:
    lda #0
    sta result
    sta result+1
.next:
    lda operand2
    ora operand2+1
    beq .done
    lda operand2
    and #1
    beq .done_add
    clc
    lda operand1
    adc result
    sta result
    lda operand1+1
    adc result+1
    sta result+1
.done_add:
    asl operand1
    rol operand1+1
    lsr operand2+1
    ror operand2
    jmp .next
.done:
    rts  ; _mul16
