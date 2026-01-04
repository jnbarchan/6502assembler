pad = $0400

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
    rts  ; _outstr_test

_outstr:
    ldx #0
_outstr_X:
.loop:
    lda pad,X
    beq .done
    jsr __outch
    inx
    jmp .loop
.done:
    rts  ; _outstr
