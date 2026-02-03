; mul.asm

.include "common.inc"

_mul_test:
    lda #$ff
    sta operand1
    lda #$02
    sta operand1+1

    lda #$03
    sta operand2
    lda #$00
    sta operand2+1

    jsr _mul16

    lda #$ff
    sta operand1_32
    lda #$02
    sta operand1_32+1
    sta operand1_32+2
    lda #0
    sta operand1_32+3

    lda #$ff
    sta operand2_32
    lda #$02
    sta operand2_32+1
    lda #0
    sta operand2_32+2
    sta operand2_32+3

    jsr _mul32

    rts  ; _mul_test

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

_mul32:
    lda #0
    sta result_32
    sta result_32+1
    sta result_32+2
    sta result_32+3
.next:
    lda operand2_32
    ora operand2_32+1
    ora operand2_32+2
    ora operand2_32+3
    beq .done
    lda operand2_32
    and #1
    beq .done_add
    clc
    lda operand1_32
    adc result_32
    sta result_32
    lda operand1_32+1
    adc result_32+1
    sta result_32+1
    lda operand1_32+2
    adc result_32+2
    sta result_32+2
    lda operand1_32+3
    adc result_32+3
    sta result_32+3
.done_add:
    asl operand1_32
    rol operand1_32+1
    rol operand1_32+2
    rol operand1_32+3
    lsr operand2_32+3
    ror operand2_32+2
    ror operand2_32+1
    ror operand2_32
    jmp .next
.done:
    rts  ; _mul32
