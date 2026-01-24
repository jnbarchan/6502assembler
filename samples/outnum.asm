; outnum.asm

.include "common.inc"

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
    jsr _padnum
    jmp _outstr_X  ; _outnum
_padnum:
    lda #10
    sta divisor
    lda #0
    sta divisor+1

    lda #0
    ldx #PAD_SIZE-1
    sta PAD,X
    dex
    stx ZP_X_SAVE

    jsr .next_digit

    ldx ZP_X_SAVE
    inx
    rts  ; _padnum

.next_digit:
    jsr _div16

    lda remainder
    clc
    adc #'0'
    ldx ZP_X_SAVE
    sta PAD,X
    dex
    stx ZP_X_SAVE

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
    jsr _padnum4hex
    jmp _outstr_X  ; _outnum4hex   
_padnum4hex:
    lda #0
    ldx #PAD_SIZE-1
    sta PAD,X
    dex

    jsr .digits

    inx
    rts  ; padnum4hex
    
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
    sta PAD,X
    dex
    rts  ; .hex_digit

.include "outstr.asm"

.include "div16.asm"

