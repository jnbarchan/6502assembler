_addsub16_test:
    lda #$55
    sta $70
    lda #$66
    sta $70+1

    lda #$10
    sta $72
    lda #$20
    sta $72+1

    jsr _add16
    jsr _sub16
    rts  ; _addsub16_test

_add16:
    clc
    lda $70
    adc $72
    sta $74
    lda $70+1
    adc $72+1
    sta $74+1
    rts  ; _add16


_sub16:
    sec
    lda $70
    sbc $72
    sta $74
    lda $70+1
    sbc $72+1
    sta $74+1
    rts  ; _sub16
