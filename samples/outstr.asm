pad = $0040

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
brk

_outstr:
    ldx #0
_outstr_X:
_outstr_loop:
    lda pad,X
    beq _outstr_done
    jsr __outch
    inx
    jmp _outstr_loop
_outstr_done:
    rts  ; _outstr
