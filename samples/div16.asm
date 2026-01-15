dividend = $70
divisor = $72
quotient = $74
remainder = $76

_div16_test:
    lda #0
    sta dividend
    lda #1
    sta dividend+1

    lda #100
    sta divisor
    lda #0
    sta divisor+1

    jsr _div16
    rts  ; _div16_test

_div16:
    lda divisor
    ora divisor+1
    bne .non_zero
    brk
    .byte "Division by zero.", 0
.non_zero:
    lda #0
    sta quotient
    sta quotient+1
    sta remainder
    sta remainder+1
    ldx #16

.loop:
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
    bcs .next

    clc
    lda remainder
    adc divisor
    sta remainder 
    lda remainder+1
    adc divisor+1
    sta remainder+1
    clc

.next:
    rol quotient
    rol quotient+1
    dex
    bne .loop
    rts  ; _div16
