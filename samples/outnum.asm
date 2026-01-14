
pad = $0400
pad_top = $100-1

x_save = $80

dividend = $70
divisor = $72
quotient = $74
remainder = $76

jsr _outnum_test
brk
brk

_outnum_test:
    lda #$ef
    sta dividend
    lda #$a9
    sta dividend+1

    jsr _outnum
    lda #10
    jsr __outch
    jsr _outnum
    lda #10
    jsr __outch

    lda #$ef
    sta dividend
    lda #$a9
    sta dividend+1
    jsr _outnum4hex
    lda #10
    jsr __outch

    rts  ; _outnum_test

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
    
    
_outnum4hex:
    lda #0
    ldx #pad_top
    sta pad,X
    dex

    jsr .digits

    inx
    jsr _outstr_X
    rts  ; _outnumhex
    
.digits:
    lda dividend
    and #$0f
    jsr .hex_digit
    lda dividend
    lsr a
    lsr a
    lsr a
    lsr a
    jsr .hex_digit
    lda dividend+1
    and #$0f
    jsr .hex_digit
    lda dividend+1
    lsr a
    lsr a
    lsr a
    lsr a
    jsr .hex_digit
    rts  ; .digits
    
.hex_digit:
    cmp #10
    bpl .hex
    clc
    adc #'0'
    bne .done
.hex:
    clc
    adc #'a'-10
.done:
    sta pad,X
    dex
    rts  ; .hex_digit

.include "outstr.asm"

.include "div16.asm"

