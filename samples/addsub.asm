; addsub.asm

.include "common.inc"

_addsub_test:
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

    lda #$55
    sta ZP_MATH_32_A
    lda #$66
    sta ZP_MATH_32_A+1
    lda #$77
    sta ZP_MATH_32_A+2
    lda #$88
    sta ZP_MATH_32_A+3
    lda #$10
    sta ZP_MATH_32_B
    lda #$20
    sta ZP_MATH_32_B+1
    lda #$30
    sta ZP_MATH_32_B+2
    lda #$40
    sta ZP_MATH_32_B+3

    jsr _add32
    jsr _sub32

    rts  ; _addsub_test

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


_add32:
    clc
    lda ZP_MATH_32_A
    adc ZP_MATH_32_B
    sta ZP_MATH_32_RES
    lda ZP_MATH_32_A+1
    adc ZP_MATH_32_B+1
    sta ZP_MATH_32_RES+1
    lda ZP_MATH_32_A+2
    adc ZP_MATH_32_B+2
    sta ZP_MATH_32_RES+2
    lda ZP_MATH_32_A+3
    adc ZP_MATH_32_B+3
    sta ZP_MATH_32_RES+3
    rts  ; _add32

_sub32:
    sec
    lda ZP_MATH_32_A
    sbc ZP_MATH_32_B
    sta ZP_MATH_32_RES
    lda ZP_MATH_32_A+1
    sbc ZP_MATH_32_B+1
    sta ZP_MATH_32_RES+1
    lda ZP_MATH_32_A+2
    sbc ZP_MATH_32_B+2
    sta ZP_MATH_32_RES+2
    lda ZP_MATH_32_A+3
    sbc ZP_MATH_32_B+3
    sta ZP_MATH_32_RES+3
    rts  ; _sub32
