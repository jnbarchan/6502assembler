;;; div8
;   Given a 16-bit number in dividend, divides it by 8-bit divisor and
;   stores the 8-bit result in dividend.
;   out: A: 8-bit remainder; X: 0; Y: unchanged

dividend = $70
divisor = $72
lda #1
sta dividend
lda #0
sta dividend+1
lda #1
sta divisor
jsr div16
brk


div16:
  ldx #16
  lda #0
.divloop:
  asl dividend
  rol dividend+1
  rol a
  cmp divisor
  bcc .no_sub
  sbc divisor
  inc dividend
.no_sub:
  dex
  bne .divloop
  rts
  
