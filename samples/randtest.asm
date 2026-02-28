; mem.asm

.include "common.inc"

testmemrnd = MEM_USER_0+$1000
testmemrnd_size = $10000 >> 3

counter_32 = $70
duplicates_32 = $74
previous_number = $78
consecutives = $7a
not_picked = $7c

total_rands_32 = 32768 * 20

test_rand:
    jsr _RND32_SEED_FROM_TIME
    
    jsr pick_randoms
    jsr statistics
    jsr _elapsed_cycles
    jsr _elapsed_time

    rts  ; test_rand
    brk
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

pick_randoms:
    ; testmemrnd[0..testmemrnd_size-1] = 0
    ; and set statistics counters to 0
    jsr .init_testmemrnd

    ; counter_32 = 0
    lda #0
    sta counter_32
    sta counter_32+1
    sta counter_32+2
    sta counter_32+3
.next:
    ; counter_32 == total_rands_32 ?
    lda counter_32+3
    cmp #(total_rands_32 >> 24) & $ff
    bne .compare_result
    lda counter_32+2
    cmp #(total_rands_32 >> 16) & $ff
    bne .compare_result
    lda counter_32+1
    cmp #(total_rands_32 >> 8) & $ff
    bne .compare_result
    lda counter_32
    cmp #total_rands_32 & $ff
.compare_result:
    bcs .done
    
    ; dividend = _RND32_NEXT
    jsr _RND32_NEXT
    lda _RND16_STATE_LOW
    sta dividend
    lda _RND16_STATE_HIGH
    sta dividend+1

    ; remainder = dividend MOD 8, quotient = dividend DIV 8
    jsr .set_index_into_testmemrnd
    
    ; src = testmemrnd + quotient
    clc
    lda #<testmemrnd
    adc quotient
    sta src
    lda #>testmemrnd
    adc quotient+1
    sta src+1
    
    ; test bit at remainder of src
    ldx remainder
    ldy #0
    lda (src),Y
    and .compact_bit_masks,X
    beq .set_bit
    
    ; duplicates_32++
    inc duplicates_32
    bne *+12
    inc duplicates_32+1
    bne *+8
    inc duplicates_32+2
    bne *+4
    inc duplicates_32+3
    jmp .done_set_bit
    
.set_bit:
    ; set bit at remainder of src to 1
    ldx remainder
    ldy #0
    lda (src),Y
    ora .compact_bit_masks,X
    sta (src),Y
    ; not_picked--
    lda not_picked
    bne *+4
    dec not_picked+1
    dec not_picked
    
.done_set_bit:
    ; random number == previous_number ?
    lda _RND16_STATE_HIGH
    cmp previous_number+1
    bne .done_consecutive
    lda _RND16_STATE_LOW
    cmp previous_number
    bne .done_consecutive
    
.consecutive:
    ; consecutives++
    inc consecutives
    bne *+4
    inc consecutives+1
    
.done_consecutive:
    ; previous_number = random number
    lda _RND16_STATE_LOW
    sta previous_number    
    lda _RND16_STATE_HIGH
    sta previous_number+1

    ; counter_32++
    inc counter_32
    bne *+12
    inc counter_32+1
    bne *+8
    inc counter_32+2
    bne *+4
    inc counter_32+3
    jmp .next
    
.done:
    rts  ; pick_randoms
    
.compact_bit_masks:
    .byte $01, $02, $04, $08, $10, $20, $40, $80

.set_index_into_testmemrnd:
    ; remainder = dividend MOD 8
    lda dividend
    and #%111
    sta remainder
    ; quotient = dividend DIV 8
    lda dividend+1
    sta quotient+1
    lda dividend
    sta quotient
    lsr quotient+1
    ror quotient
    lsr quotient+1
    ror quotient
    lsr quotient+1
    ror quotient
    rts ; .set_index_into_testmemrnd

.init_testmemrnd:
    ; testmemrnd[0..testmemrnd_size-1] = 0
    lda #<testmemrnd
    sta dest
    lda #>testmemrnd
    sta dest+1
    lda #<testmemrnd_size
    sta count
    lda #>testmemrnd_size
    sta count+1
    lda #0
    jsr _memset16
    ; not_picked = duplicates_32 = consecutives = 0
    lda #0
    sta duplicates_32
    sta duplicates_32+1
    sta duplicates_32+2
    sta duplicates_32+3
    sta consecutives
    sta consecutives+1
    sta not_picked
    sta not_picked+1
    rts ; .init_testmemrnd
    
statistics:
    ; output duplicate occurrence
    jsr __outstr_inline
    .byte "Duplicates: ", 0
    lda duplicates_32
    sta dividend_32
    lda duplicates_32+1
    sta dividend_32+1
    lda duplicates_32+2
    sta dividend_32+2
    lda duplicates_32+3
    sta dividend_32+3
    jsr _outnum32_commas
    lda #10
    jsr __outch
    jsr __outstr_inline
    .byte "Consecutives: ", 0
    lda consecutives
    sta dividend
    lda consecutives+1
    sta dividend+1
    jsr _outnum
    lda #10
    jsr __outch
    jsr __outstr_inline
    .byte "Not picked: ", 0
    lda not_picked
    sta dividend
    lda not_picked+1
    sta dividend+1
    jsr _outnum
    lda #10
    jsr __outch
    rts  ; statistics
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    

.include "assert.asm"
.include "elapsed.asm"
.include "mem.asm"
.include "rand.asm"
.include "outnum.asm"
