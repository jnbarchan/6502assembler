; scratch.asm

.include "common.inc"

ldx #'Z'
letters:
txa
jsr __outch
dex
cpx #'A'
bne letters
lda #10
jsr __outch
lda #'!'
jsr __outch




labx4 = $4000
mem = $1000
labx2:
ldx #mem
sta mem,x

;bcc labx4

zp = ZP_USER_0
zp2 = ZP_USER_0+4
lda #$00
sta zp
sta zp2
lda #$10
sta zp+1
lda #$20
sta zp2+1
lda #44
ldy #1
sta (zp),Y
ldx #4
sta (zp,x)
rts

ldy #0
labx0:
ldx #0
labx: inx
bne labx
iny
bne labx0
rts


nop
ldx #$88
stx mem
ldx #-1
zlab:
zlab2: inx
iny
bne zlab2
rts
;mem = 6







































































































labx3:
