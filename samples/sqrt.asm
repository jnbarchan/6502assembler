; sqrt.asm

.include "common.inc"

sqrt_operand_32 = _SQRT_OPERAND_32
sqrt_result_32 = _SQRT_RESULT_32
sqrt_lower = _SQRT_LOWER
sqrt_mid = _SQRT_MID
sqrt_upper = _SQRT_UPPER

_sqrt_number_too_large_32 = $fffe0002

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
    ; sqrt_result_32 = 0
    lda #0
    sta sqrt_result_32+3
    sta sqrt_result_32+2
    sta sqrt_result_32+1
    sta sqrt_result_32
    
    ; if sqrt_operand_32 < 2 then sqrt_result_32 = sqrt_operand_32
    lda sqrt_operand_32+3
    ora sqrt_operand_32+2
    ora sqrt_operand_32+1
    bne .not_small_number
    lda sqrt_operand_32
    cmp #2
    bcs .not_small_number
    sta sqrt_result_32
    jmp .done
    
.not_small_number:
    ; init sqrt_lower and sqrt_upper
    jsr ___sqrt_init_bounds

.loop:   
    ; test for sqrt_lower <= sqrt_upper
    lda sqrt_lower+1
    cmp sqrt_upper+1
    bcc .adjust_bounds
    bne .done
    lda sqrt_lower
    cmp sqrt_upper
    bcc .adjust_bounds
    beq .adjust_bounds
    bne .done

.adjust_bounds:
    jsr ___sqrt_adjust_bounds
    jmp .loop
    
.done:
    rts  ; _sqrt32
    
___sqrt_init_bounds:
    ; if sqrt_operand_32 >= _sqrt_number_too_large_32 then too large
    lda sqrt_operand_32+3
    cmp #(_sqrt_number_too_large_32 >> 24) & $ff
    bcc .not_number_too_large
    bne .number_too_large
    lda sqrt_operand_32+2
    cmp #(_sqrt_number_too_large_32 >> 16) & $ff
    bcc .not_number_too_large
    bne .number_too_large
    lda sqrt_operand_32+1
    cmp #(_sqrt_number_too_large_32 >> 8) & $ff
    bcc .not_number_too_large
    bne .number_too_large
    lda sqrt_operand_32
    cmp #_sqrt_number_too_large_32 & $ff
    bcc .not_number_too_large
.number_too_large:
    brk
    .byte "Sqrt too large.", 0

.not_number_too_large:
    ; sqrt_lower = 0
    lda #0
    sta sqrt_lower
    sta sqrt_lower+1
    ; sqrt_upper = min(sqrt_operand_32, $ffff)
    lda #$ff
    sta sqrt_upper+1
    sta sqrt_upper
    lda sqrt_operand_32+3
    ora sqrt_operand_32+2
    bne .done
.not_large_number:
    lda sqrt_operand_32+1
    sta sqrt_upper+1
    lda sqrt_operand_32
    sta sqrt_upper

.done:
    rts  ; ___sqrt_init_bounds
    
___sqrt_adjust_bounds:
    ; sqrt_mid = (sqrt_lower + sqrt_upper) / 2
    clc
    lda sqrt_lower
    adc sqrt_upper
    sta sqrt_mid
    lda sqrt_lower+1
    adc sqrt_upper+1
    sta sqrt_mid+1
    ror sqrt_mid+1
    ror sqrt_mid
    
    ; operand1_32 = sqrt_mid
    lda sqrt_mid
    sta operand1_32
    lda sqrt_mid+1
    sta operand1_32+1
    lda #0
    sta operand1_32+2
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
    ; sqrt_result_32 = sqrt_mid
    lda sqrt_mid
    sta sqrt_result_32
    lda sqrt_mid+1
    sta sqrt_result_32+1
    ; sqrt_upper = sqrt_mid - 1
    sec
    lda sqrt_mid
    sbc #1
    sta sqrt_upper
    lda sqrt_mid+1
    sbc #0
    sta sqrt_upper+1
    jmp .done
.lt:
    ; sqrt_lower = sqrt_mid + 1
    clc
    lda sqrt_mid
    adc #1
    sta sqrt_lower
    lda sqrt_mid+1
    adc #0
    sta sqrt_lower+1

.done:
    rts  ; ___sqrt_adjust_bounds
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.include "mul.asm"
