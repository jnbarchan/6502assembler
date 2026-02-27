; mem.asm

.include "common.inc"

testmemrnd = MEM_USER_0+$1000
testmemrnd_pages = 20 * 4
testmemrnd_size = testmemrnd_pages * $100 + 5

counter = $70
duplicates = $72

;total_rands = testmemrnd_size << 3
total_rands = 32768

test_rand:
    jsr _RND32_SEED_FROM_TIME
    
    jsr _count_rand_duplicates
    jsr _elapsed_cycles
    jsr _elapsed_time

    rts  ; test_rand
    brk
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_count_rand_duplicates:
    ; testmemrnd[0..testmemrnd_size-1] = 0
    ; duplicates = 0
    jsr .init_testmemrnd

    ; counter = 0
    lda #0
    sta counter
    sta counter+1
.next:
    ; counter == total_rands?
    lda counter+1
    cmp #>total_rands
    bne *+6
    lda counter
    cmp #<total_rands
    bcs .done
    
    ; dividend = _RND32_NEXT
    jsr _RND32_NEXT
    lda _RND16_STATE_LOW
    sta dividend
    lda _RND16_STATE_HIGH
    sta dividend+1

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
    
    ; duplicate
    jsr .duplicate_found

    jmp .done_set_bit
    
.set_bit:
    ; set bit at remainder of src to 1
    ldx remainder
    ldy #0
    lda (src),Y
    ora .compact_bit_masks,X
    sta (src),Y
    
.done_set_bit:
    ; counter++
    inc counter
    bne .next
    inc counter+1
    jmp .next
.done:
    rts  ; _memfillrand
    
.compact_bit_masks:
    .byte $01, $02, $04, $08, $10, $20, $40, $80

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
    ; duplicates = 0
    lda #0
    sta duplicates
    sta duplicates+1
    rts ; .init_testmemrnd
    
.duplicate_found:
    ; duplicate random number found
    ; duplicates++
    inc duplicates
    bne *+4
    inc duplicates+1
    ; output duplicate occurrence
    lda duplicates
    sta dividend
    lda duplicates+1
    sta dividend+1
    jsr _outnum
    jsr __outstr_inline
    .byte ". Duplicate: ", 0
    lda _RND16_STATE_LOW
    sta dividend
    lda _RND16_STATE_HIGH
    sta dividend+1
    jsr _outnum4hex
    lda #10
    jsr __outch
    rts  ; .duplicate

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    

.include "assert.asm"
.include "elapsed.asm"
.include "mem.asm"
.include "rand.asm"
.include "outnum.asm"
