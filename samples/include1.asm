; include1.asm

;lda #6 / 0

;nonsense = 7 / 0

lda #0

lda #1

.include "include2.asm"  ; include

lda #5

lda #6

rts

