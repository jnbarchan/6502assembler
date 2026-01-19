pad = $0400
_outstr_inline_zp = $86

_outstr_test:
    lda #'A'
    sta pad
    clc
    adc #1
    sta pad+1
    adc #1
    sta pad+2
    adc #1
    sta pad+3

    lda #0
    sta pad+4

    jsr _outstr
    lda #10
    jsr __outch
    ldx #2
    jsr _outstr_X
    lda #10
    jsr __outch
    
    jsr _outstr_inline
    .byte "Goodbye", 10, 0
    lda #$88
    rts  ; _outstr_test

_outstr:
    ldx #0
_outstr_X:
.loop:
    lda pad,X
    beq .done
    jsr __outch
    inx
    bne .loop
.done:
    rts  ; _outstr
    
_outstr_inline:
    pla
    sta _outstr_inline_zp+1
    pla
    sta _outstr_inline_zp
    ldy #0
.loop:
    lda (_outstr_inline_zp),Y
    beq .done
    jsr __outch
    iny
    bne .loop
.done:
    iny
    tya
    clc
    adc _outstr_inline_zp
    sta _outstr_inline_zp
    lda #0
    adc _outstr_inline_zp+1
    sta _outstr_inline_zp+1
    jmp (_outstr_inline_zp)
    rts  ; _outstr_inline
