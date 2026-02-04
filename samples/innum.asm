; innum.asm

.include "common.inc"

_innum_test:
    jsr _outstr_inline
    .byte "Type a number, terminate by 'Enter': ", 0
    jsr _innum
    lda PAD,X
    beq .parsed_num
    
    jsr _outstr_inline
    .byte "Stopped parsing number at: ", 0
    jsr _outstr_X
    lda #10
    jsr __outch
    
.parsed_num:
    lda result
    sta dividend
    lda result+1
    sta dividend+1
    jsr _outstr_inline
    .byte "You typed number: ", 0
    jsr _outnum
    lda #10
    jsr __outch

    jsr _outstr_inline
    .byte "Type a big number, terminate by 'Enter': ", 0
    jsr _innum32
    lda PAD,X
    beq .parsed_num32
    
    jsr _outstr_inline
    .byte "Stopped parsing number at: ", 0
    jsr _outstr_X
    lda #10
    jsr __outch
    
.parsed_num32:
    lda result_32
    sta dividend_32
    lda result_32+1
    sta dividend_32+1
    lda result_32+2
    sta dividend_32+2
    lda result_32+3
    sta dividend_32+3
    jsr _outstr_inline
    .byte "You typed number: ", 0
    jsr _outnum32_commas
    lda #10
    jsr __outch

    rts  ; _innum_test
    brk

_innum:
    lda #0
    sta result
    sta result+1
    jsr _instr
    ldx #0
.next_digit:
    lda PAD,X
    beq .done
    cmp #'0'
    bcc .bad_digit
    cmp #'9'+1
    bcs .bad_digit
    lda result
    sta operand1
    lda result+1
    sta operand1+1
    lda #10
    sta operand2
    lda #0
    sta operand2+1
    jsr _mul16
    lda PAD,X
    sec
    sbc #'0'
    clc
    adc result
    sta result
    lda #0
    adc result+1
    sta result+1
    inx
    bne .next_digit
.bad_digit:
.done:
    rts  ; _innum
    
_innum32:
    lda #0
    sta result_32
    sta result_32+1
    sta result_32+2
    sta result_32+3
    jsr _instr
    ldx #0
.next_digit:
    lda PAD,X
    beq .done
    cmp #'0'
    bcc .bad_digit
    cmp #'9'+1
    bcs .bad_digit
    lda result_32
    sta operand1_32
    lda result_32+1
    sta operand1_32+1
    lda result_32+2
    sta operand1_32+2
    lda result_32+3
    sta operand1_32+3
    lda #10
    sta operand2_32
    lda #0
    sta operand2_32+1
    sta operand2_32+2
    sta operand2_32+3
    jsr _mul32
    lda PAD,X
    sec
    sbc #'0'
    clc
    adc result_32
    sta result_32
    lda #0
    adc result_32+1
    sta result_32+1
    lda #0
    adc result_32+2
    sta result_32+2
    lda #0
    adc result_32+3
    sta result_32+3
    inx
    bne .next_digit
.bad_digit:
.done:
    rts  ; _innum32
    
    
.include "instr.asm"

.include "outnum.asm"

.include "mul.asm"

