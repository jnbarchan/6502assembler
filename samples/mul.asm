lda #$10
sta $70
;lda #$66
;sta $70+1

lda #$5
sta $72
;lda #$0
;sta $72+1

jsr _mul8
brk

_mul8:
lda #0
sta $74
_mul8_next:
lda $72
beq _mul8_done
and #1
beq _mul8_done_add
clc
lda $70
adc $74
sta $74
_mul8_done_add:
asl $70
lsr $72
jmp _mul8_next
_mul8_done:
rts

_add16:
clc
lda $70
adc $72
sta $74
lda $71
adc $73
sta $75
rts


_sub16:
sec
lda $70
sbc $72
sta $74
lda $71
sbc $73
sta $75
rts



