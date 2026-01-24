; addsub16.asm

.include "common.inc"

_addsub16_test:
    lda #$55
    sta ZP_MATH_A
    lda #$66
    sta ZP_MATH_A+1

    lda #$10
    sta ZP_MATH_B
    lda #$20
    sta ZP_MATH_B+1

    jsr _add16
    jsr _sub16
    rts  ; _addsub16_test

_add16:
    clc
    lda ZP_MATH_A
    adc ZP_MATH_B
    sta ZP_MATH_RES
    lda ZP_MATH_A+1
    adc ZP_MATH_B+1
    sta ZP_MATH_RES+1
    rts  ; _add16


_sub16:
    sec
    lda ZP_MATH_A
    sbc ZP_MATH_B
    sta ZP_MATH_RES
    lda ZP_MATH_A+1
    sbc ZP_MATH_B+1
    sta ZP_MATH_RES+1
    rts  ; _sub16
