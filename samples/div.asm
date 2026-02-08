; div.asm

.include "common.inc"

_div_test:
    lda #0
    sta dividend
    lda #1
    sta dividend+1

    lda #100
    sta divisor
    lda #0
    sta divisor+1

    jsr _div16

    lda #0
    sta dividend_32
    sta dividend_32+1
    sta dividend_32+2
    lda #1
    sta dividend_32+3

    lda #10
    sta divisor_32
    lda #0
    sta divisor_32+1
    sta divisor_32+2
    sta divisor_32+3

    jsr _div32

    rts  ; _div_test

_div16:
    lda divisor
    ora divisor+1
    bne .non_zero
    brk
    .byte 1, "Division by zero.", 0
.non_zero:
    lda #0
    sta quotient
    sta quotient+1
    sta remainder
    sta remainder+1
    ldx #16

.loop:
    asl dividend
    rol dividend+1
    rol remainder
    rol remainder+1

    sec
    lda remainder
    sbc divisor
    sta remainder 
    lda remainder+1
    sbc divisor+1
    sta remainder+1
    bcs .next

    clc
    lda remainder
    adc divisor
    sta remainder 
    lda remainder+1
    adc divisor+1
    sta remainder+1
    clc

.next:
    rol quotient
    rol quotient+1
    dex
    bne .loop
    rts  ; _div16
    
_div32:
    lda divisor_32
    ora divisor_32+1
    ora divisor_32+2
    ora divisor_32+3
    bne .non_zero
    brk
    .byte 1, "Division by zero.", 0
.non_zero:
    lda #0
    sta quotient_32
    sta quotient_32+1
    sta quotient_32+2
    sta quotient_32+3
    sta remainder_32
    sta remainder_32+1
    sta remainder_32+2
    sta remainder_32+3
    ldx #32

.loop:
    asl dividend_32
    rol dividend_32+1
    rol dividend_32+2
    rol dividend_32+3
    rol remainder_32
    rol remainder_32+1
    rol remainder_32+2
    rol remainder_32+3

    sec
    lda remainder_32
    sbc divisor_32
    sta remainder_32
    lda remainder_32+1
    sbc divisor_32+1
    sta remainder_32+1
    lda remainder_32+2
    sbc divisor_32+2
    sta remainder_32+2
    lda remainder_32+3
    sbc divisor_32+3
    sta remainder_32+3
    bcs .next

    clc
    lda remainder_32
    adc divisor_32
    sta remainder_32
    lda remainder_32+1
    adc divisor_32+1
    sta remainder_32+1
    lda remainder_32+2
    adc divisor_32+2
    sta remainder_32+2
    lda remainder_32+3
    adc divisor_32+3
    sta remainder_32+3
    clc

.next:
    rol quotient_32
    rol quotient_32+1
    rol quotient_32+2
    rol quotient_32+3
    dex
    bne .loop
    rts  ; _div32
