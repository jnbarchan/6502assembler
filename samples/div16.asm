dividend = $70
divisor = $72
quotient = $74
remainder = $76

lda #0
sta dividend
lda #1
sta dividend+1

lda #100
sta divisor
lda #0
sta divisor+1

jsr _div16
brk

_div16:
    lda #0
    sta quotient
    sta quotient+1
    sta remainder
    sta remainder+1
    ldx #16

_div16_loop:
    asl dividend
    rol dividend+1
    rol remainder
    rol remainder+1

    sec
    lda remainder
    sbc divisor
    sta remainder 
    lda remainder+1
    sbc divisor+1
    sta remainder+1
    bcs _div16_next

    clc
    lda remainder
    adc divisor
    sta remainder 
    lda remainder+1
    adc divisor+1
    sta remainder+1
    clc

_div16_next:
    rol quotient
    rol quotient+1
    dex
    bne _div16_loop
    rts  ; _div16
