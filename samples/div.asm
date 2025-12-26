dividend = $70
divisor = $72
quotient = $74
remainder = $76

lda #$10
sta dividend

lda #$5
sta divisor

jsr _div8
brk

_div8:
jmp _div8_done
_div8_done:
rts
