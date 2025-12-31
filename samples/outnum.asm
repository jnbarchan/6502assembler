
pad = $0400
pad_top = $100-1

x_save = $80

dividend = $70
divisor = $72
quotient = $74
remainder = $76

lda #$ff
sta dividend
lda #$ff
sta dividend+1

jsr _outnum
brk

_outnum:
    lda #10
    sta divisor
    lda #0
    sta divisor+1

    lda #0
    ldx #pad_top
    sta pad,X
    dex
    stx x_save

    jsr .next_digit

    ldx x_save
    inx
    jsr _outstr_X
    rts  ; _outnum

.next_digit:
    jsr _div16

    lda remainder
    clc
    adc #'0'
    ldx x_save
    sta pad,X
    dex
    stx x_save

    lda quotient
    ora quotient+1
    beq .done_digits

    lda quotient
    sta dividend
    lda quotient+1
    sta dividend+1
    jmp .next_digit
.done_digits:
    rts  ; .next_digit

_outdigit:
    clc
    adc #'0'
    jmp __outch  ; _outdigit


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


_div16:
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
