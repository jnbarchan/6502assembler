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
    jsr _outnum_commas
    lda #10
    jsr __outch

    lda #$ef
    sta dividend
    lda #$a9
    sta dividend+1
    jsr _outnum4hex
    lda #10
    jsr __outch

    lda #$ff
    sta dividend_32
    lda #$ff
    sta dividend_32+1
    lda #$ff
    sta dividend_32+2
    lda #$ff
    sta dividend_32+3
    jsr _outnum32_commas
    lda #10
    jsr __outch

    rts  ; _outnum_test
    brk

_outnum_commas:
    lda #','
    sta _OUTNUM_SEPARATOR
    bne *+7
_outnum:
    lda #0
    sta _OUTNUM_SEPARATOR
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

    lda #3+1
    sta _OUTNUM_SEPARATOR_COUNT
    jsr .next_digit

    ldx ZP_X_SAVE
    inx
    rts  ; _padnum

.next_digit:
    jsr _div16
    ldx ZP_X_SAVE

    lda _OUTNUM_SEPARATOR
    beq .no_separator
    dec _OUTNUM_SEPARATOR_COUNT
    bne .no_separator
    lda #3+1
    sta _OUTNUM_SEPARATOR_COUNT
    lda _OUTNUM_SEPARATOR
    sta PAD,X
    dex   
    
.no_separator:
    lda remainder
    clc
    adc #'0'
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

_outnum32_commas:
    lda #','
    sta _OUTNUM_SEPARATOR
    bne *+7
_outnum32:
    lda #0
    sta _OUTNUM_SEPARATOR
    jsr _padnum32
    jmp _outstr_X  ; _outnum32
_padnum32:
    lda #10
    sta divisor_32
    lda #0
    sta divisor_32+1
    sta divisor_32+2
    sta divisor_32+3

    lda #0
    ldx #PAD_SIZE-1
    sta PAD,X
    dex
    stx ZP_X_SAVE

    lda #3+1
    sta _OUTNUM_SEPARATOR_COUNT
    jsr .next_digit

    ldx ZP_X_SAVE
    inx
    rts  ; _padnum32

.next_digit:
    jsr _div32
    ldx ZP_X_SAVE

    lda _OUTNUM_SEPARATOR
    beq .no_separator
    dec _OUTNUM_SEPARATOR_COUNT
    bne .no_separator
    lda #3
    sta _OUTNUM_SEPARATOR_COUNT
    lda _OUTNUM_SEPARATOR
    sta PAD,X
    dex   
    
.no_separator:
    lda remainder_32
    clc
    adc #'0'
    sta PAD,X
    dex
    stx ZP_X_SAVE

    lda quotient_32
    ora quotient_32+1
    ora quotient_32+2
    ora quotient_32+3
    beq .done_digits

    lda quotient_32
    sta dividend_32
    lda quotient_32+1
    sta dividend_32+1
    lda quotient_32+2
    sta dividend_32+2
    lda quotient_32+3
    sta dividend_32+3
    jmp .next_digit
.done_digits:
    rts  ; .next_digit

.include "outstr.asm"

.include "div.asm"

