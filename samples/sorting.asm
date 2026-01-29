; sorting.asm

.include "common.inc"

nums_start = $1000
nums_end = nums_start+$200

p_nums = ZP_USER_0
p_nums_2 = p_nums+2
p_nums_last = p_nums+4
p_nums_new_last = p_nums+6

swapped = ZP_USER_1

swap_temp = ZP_TMP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

test_sorting:
    jsr fill_nums
    jsr bubble_sort_optimized
    jsr verify_sorted
    jsr print_nums
    jsr elapsed_cycles
    jsr elapsed_time
    rts  ; test_sorting
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bubble_sort_optimized:
    lda #<nums_end
    sta p_nums_last
    lda #>nums_end
    sta p_nums_last+1
.pass:
    jsr init_p_nums
    lda p_nums
    sta p_nums_new_last
    lda p_nums+1
    sta p_nums_new_last+1
    jsr inc_p_nums

.next_num:
    jsr cmp_p_nums_last
    bcs .done_pass
    
    jsr init_p_nums_2
    
    jsr cmp_p_nums_p_nums_2
    bcs .no_swap
    
    jsr bubble_sort.swap_p_nums_p_nums_2
    lda p_nums
    sta p_nums_new_last
    lda p_nums+1
    sta p_nums_new_last+1
    
.no_swap:
    jsr inc_p_nums
    jmp .next_num
    
.done_pass:
    lda p_nums_new_last
    sta p_nums_last
    lda p_nums_new_last+1
    sta p_nums_last+1
    
    jsr cmp_p_nums_last_start_plus_1
    beq .done
    bcs .pass

.done:
    rts  ; bubble_sort_optimized

cmp_p_nums_last:
    lda p_nums+1
    cmp p_nums_last+1
    bcc .lt
    bne .gt
    lda p_nums
    cmp p_nums_last
.eq:
.lt:
.gt:
    rts  ; cmp_p_nums_last
    
cmp_p_nums_last_start_plus_1:
    lda p_nums_last+1
    cmp #>(nums_start+2)
    bcc .lt
    bne .gt
    lda p_nums_last
    cmp #<(nums_start+2)
.eq:
.lt:
.gt:
    rts  ; cmp_p_nums_last_start_plus_1
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bubble_sort:
.pass:
    lda #0
    sta swapped
    jsr init_p_nums
    jsr inc_p_nums

.next_num:
    jsr cmp_p_nums_end
    bcs .done_pass
    
    jsr init_p_nums_2
    
    jsr cmp_p_nums_p_nums_2
    bcs .no_swap
    
    jsr .swap_p_nums_p_nums_2
    lda #1
    sta swapped
    
.no_swap:
    jsr inc_p_nums
    jmp .next_num
    
.done_pass:
    lda swapped
    bne .pass

.done:
    rts  ; bubble_sort

.swap_p_nums_p_nums_2:
    ldy #0
    lda (p_nums),Y
    sta swap_temp
    lda (p_nums_2),Y
    sta (p_nums),Y
    lda swap_temp
    sta (p_nums_2),Y
    iny
    lda (p_nums),Y
    sta swap_temp
    lda (p_nums_2),Y
    sta (p_nums),Y
    lda swap_temp
    sta (p_nums_2),Y
    rts  ; .swap_p_nums_p_nums_2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

fill_nums:
    ;jsr _RND16_SEED_FROM_TIME
    jsr _RND16_SEED_FROM_DEFAULT
    jsr init_p_nums
    
.next_num:
    jsr cmp_p_nums_end
    bcs .done
    
.fill_next_num:
    jsr _RND16_NEXT
    ldy #0
    lda _RND16_STATE_LOW
    sta (p_nums),Y
    iny
    lda _RND16_STATE_HIGH
    sta (p_nums),Y

    jsr inc_p_nums
    jmp .next_num

.done:
    rts  ; fill_nums

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

verify_sorted:
    jsr init_p_nums
    jsr inc_p_nums

.next_num:
    jsr cmp_p_nums_end
    bcs .done
    
    jsr init_p_nums_2
    
    jsr cmp_p_nums_p_nums_2
    bcc .error

    jsr inc_p_nums
    jmp .next_num
    
.error:
    brk
    .byte "Not sorted!", 0

.done:
    rts  ; verify_sorted

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

print_nums:
    jsr init_p_nums
    
.next_num:
    jsr cmp_p_nums_end
    bcs .done
    beq .done
    
    jsr .print_num

    jsr inc_p_nums
    jmp .next_num
    
.done:
    lda #10
    jsr __outch
    rts  ; print_nums

.print_num:
    ldy #0
    lda (p_nums),Y
    sta dividend
    iny
    lda (p_nums),Y
    sta dividend+1
    jsr _outnum4hex
    lda #10
    jmp __outch

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

init_p_nums:
    lda #<nums_start
    sta p_nums
    lda #>nums_start
    sta p_nums+1
    rts  ; init_p_nums
    
cmp_p_nums_end:
    lda p_nums+1
    cmp #>nums_end
    bcc .lt
    bne .gt
    lda p_nums
    cmp #<nums_end
.eq:
.lt:
.gt:
    rts  ; cmp_p_nums_end
    
inc_p_nums:
    inc p_nums
    inc p_nums
    bne *+4
    inc p_nums+1
    rts  ; inc_p_nums
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

init_p_nums_2:
    sec
    lda p_nums
    sbc #2
    sta p_nums_2
    lda p_nums+1
    sbc #0
    sta p_nums_2+1
    rts  ; .init_p_nums_2
    
cmp_p_nums_p_nums_2:
    ldy #1
    lda (p_nums),Y
    cmp (p_nums_2),Y
    bcc .lt
    bne .gt
    dey
    lda (p_nums),Y
    cmp (p_nums_2),Y
.eq:
.lt:
.gt:
    rts  ; cmp_p_nums_p_nums_2
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
elapsed_cycles:
    jsr __get_elapsed_kcycles
    sta dividend
    stx dividend+1
    jsr _outnum
    jsr _outstr_inline
    .byte "k cycles", 0
    lda #10
    jsr __outch
    rts  ; elapsed_cycles 

elapsed_time:
    jsr __get_elapsed_time
    sta dividend
    stx dividend+1
    jsr _outnum
    jsr _outstr_inline
    .byte " ms", 0
    lda #10
    jsr __outch
    rts  ; elapsed_time

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.include "outstr.asm"
.include "outnum.asm"
.include "rand.asm"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
