; sqrt.asm

.include "common.inc"

sqrt_operand_32 = _SQRT_OPERAND_32
sqrt_result_32 = _SQRT_RESULT_32
sqrt_lower_32 = _SQRT_LOWER_32
sqrt_mid_32 = _SQRT_MID_32
sqrt_upper_32 = _SQRT_UPPER_32
sqrt_temp_32 = ZP_MATH_32_RES2

_test_sqrt_number_32 = 370362989

_sqr_sqrt_test:
    lda #$ff
    sta operand1_32
    lda #$08
    sta operand1_32+1
    lda #0
    sta operand1_32+2
    sta operand1_32+3

    jsr _sqr

    lda #_test_sqrt_number_32 & $ff
    sta sqrt_operand_32
    lda #(_test_sqrt_number_32 >> 8) & $ff
    sta sqrt_operand_32+1
    lda #(_test_sqrt_number_32 >> 16) & $ff
    sta sqrt_operand_32+2
    lda #(_test_sqrt_number_32 >> 24) & $ff
    sta sqrt_operand_32+3

    jsr _sqrt32

    rts  ; _sqr_sqrt_test
    brk
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_sqr:
    lda operand1_32
    sta operand2_32
    lda operand1_32+1
    sta operand2_32+1
    lda operand1_32+2
    sta operand2_32+2
    lda operand1_32+3
    sta operand2_32+3
    jmp _mul32  ; _sqr

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_sqrt32:
    ; if sqrt_operand_32 < 2 sqrt_result_32 = sqrt_operand_32
    lda sqrt_operand_32+3
    ora sqrt_operand_32+2
    ora sqrt_operand_32+1
    bne .not_small_number
    lda sqrt_operand_32
    cmp #2
    bcs .not_small_number
    sta sqrt_result_32
    lda #0
    sta sqrt_result_32+1
    sta sqrt_result_32+2
    sta sqrt_result_32+3
    jmp .done
.not_small_number:
    ; sqrt_lower_32 = 0
    lda #0
    sta sqrt_lower_32
    sta sqrt_lower_32+1
    sta sqrt_lower_32+2
    sta sqrt_lower_32+3

    ; sqrt_upper_32 = sqrt_operand_32 + 1
    clc
    lda sqrt_operand_32
    adc #1
    sta sqrt_upper_32
    lda sqrt_operand_32+1
    adc #0
    sta sqrt_upper_32+1
    lda sqrt_operand_32+2
    adc #0
    sta sqrt_upper_32+2
    lda sqrt_operand_32+3
    adc #0
    sta sqrt_upper_32+3

.loop:
    ; sqrt_temp_32 = sqrt_upper_32 - 1
    sec
    lda sqrt_upper_32
    sbc #1
    sta sqrt_temp_32
    lda sqrt_upper_32+1
    sbc #0
    sta sqrt_temp_32+1
    lda sqrt_upper_32+2
    sbc #0
    sta sqrt_temp_32+2
    lda sqrt_upper_32+3
    sbc #0
    sta sqrt_temp_32+3
    
    ; test for sqrt_lower_32 != sqrt_temp_32
    lda sqrt_lower_32+3
    cmp sqrt_temp_32+3
    bne .adjust_bounds
    lda sqrt_lower_32+2
    cmp sqrt_temp_32+2
    bne .adjust_bounds
    lda sqrt_lower_32+1
    cmp sqrt_temp_32+1
    bne .adjust_bounds
    lda sqrt_lower_32
    cmp sqrt_temp_32
    beq .done

.adjust_bounds:
    jsr ___sqrt_adjust_bounds
    jmp .loop
    
.done:
    rts  ; _sqrt32
    
___sqrt_adjust_bounds:
    ; sqrt_mid_32 = sqrt_lower_32 + sqrt_upper_32
    clc
    lda sqrt_lower_32
    adc sqrt_upper_32
    sta sqrt_mid_32
    lda sqrt_lower_32+1
    adc sqrt_upper_32+1
    sta sqrt_mid_32+1
    lda sqrt_lower_32+2
    adc sqrt_upper_32+2
    sta sqrt_mid_32+2
    lda sqrt_lower_32+3
    adc sqrt_upper_32+3
    sta sqrt_mid_32+3
    ; sqrt_mid_32 /= 2
    lsr sqrt_mid_32+3
    ror sqrt_mid_32+2
    ror sqrt_mid_32+1
    ror sqrt_mid_32
    
    ; operand1_32 = sqrt_mid_32
    lda sqrt_mid_32
    sta operand1_32
    lda sqrt_mid_32+1
    sta operand1_32+1
    lda sqrt_mid_32+2
    sta operand1_32+2
    lda sqrt_mid_32+3
    sta operand1_32+3
    ; result_32 = operand1_32 * operand1_32
    jsr _sqr
    
    ; test for result_32 >= sqrt_operand_32
    ldx #3
.next:
    lda result_32,X
    cmp sqrt_operand_32,X
    bcc .lt
    bne .gt
    dex
    bpl .next
.eq:
.gt:
    ; sqrt_result_32 = sqrt_upper_32 = sqrt_mid_32
    lda sqrt_mid_32
    sta sqrt_upper_32
    sta sqrt_result_32
    lda sqrt_mid_32+1
    sta sqrt_upper_32+1
    sta sqrt_result_32+1
    lda sqrt_mid_32+2
    sta sqrt_upper_32+2
    sta sqrt_result_32+2
    lda sqrt_mid_32+3
    sta sqrt_upper_32+3
    sta sqrt_result_32+3
    jmp .done
.lt:
    ; sqrt_lower_32 = sqrt_mid_32
    lda sqrt_mid_32
    sta sqrt_lower_32
    lda sqrt_mid_32+1
    sta sqrt_lower_32+1
    lda sqrt_mid_32+2
    sta sqrt_lower_32+2
    lda sqrt_mid_32+3
    sta sqrt_lower_32+3

.done:
    rts  ; ___sqrt_adjust_bounds
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.include "mul.asm"
