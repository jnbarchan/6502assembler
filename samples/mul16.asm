
operand1 = $70
operand2 = $72
result = $74

lda #$ff
sta operand1
lda #$02
sta operand1+1

lda #$03
sta operand2
lda #$00
sta operand2+1

jsr _mul16
brk

_mul16:
    lda #0
    sta result
    sta result+1
_mul16_next:
    lda operand2
    ora operand2+1
    beq _mul16_done
    lda operand2
    and #1
    beq _mul16_done_add
    clc
    lda operand1
    adc result
    sta result
    lda operand1+1
    adc result+1
    sta result+1
_mul16_done_add:
    asl operand1
    rol operand1+1
    lsr operand2+1
    ror operand2
    jmp _mul16_next
_mul16_done:
    rts  ; _mul16
