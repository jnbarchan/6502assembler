; sorting.asm

.include "common.inc"

nums_start = $1000
nums_end = nums_start+$200

p_nums = ZP_USER_0
p_nums_2 = p_nums+2
p_nums_last = p_nums+4
p_nums_new_last = p_nums+6
p_nums_min = p_nums+8
p_nums_save = p_nums+10

swapped = ZP_USER_1

swap_temp = ZP_TMP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

test_sorting:
    jsr fill_nums
    
    jsr insertion_sort
    ;jsr selection_sort
    ;jsr bubble_sort_optimized
    ;jsr bubble_sort
    
    jsr print_nums
    jsr verify_sorted
    jsr _elapsed_cycles
    jsr _elapsed_time
    rts  ; test_sorting
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

insertion_sort:
    jsr init_p_nums_2_from_start
    jsr inc_p_nums_2

.next_num_2:
    jsr cmp_p_nums_2_end
    bcs .done
    
    jsr .copy_ind_p_nums_2_to_save
    jsr .copy_p_nums_2_to_p_nums
    
.next_num:
    jsr dec_p_nums
    jsr cmp_p_nums_start
    bcc .done_next_num
    jsr .cmp_ind_p_nums_save
    bcc .done_next_num
    beq .done_next_num
    
    jsr .copy_ind_p_nums_to_p_nums_plus_1

    jmp .next_num
    
.done_next_num:
    jsr inc_p_nums
    jsr .copy_p_nums_save_to_ind_p_nums
    jsr inc_p_nums_2
    jmp .next_num_2
    
.done:
    rts  ; insertion_sort
    
.copy_ind_p_nums_2_to_save:
    ldy #0
    lda (p_nums_2),Y
    sta p_nums_save
    iny
    lda (p_nums_2),Y
    sta p_nums_save+1
    rts  ; .copy_ind_p_nums_2_to_save
    
.copy_p_nums_2_to_p_nums:
    lda p_nums_2
    sta p_nums
    lda p_nums_2+1
    sta p_nums+1
    rts  ; .copy_p_nums_2_to_p_nums

.cmp_ind_p_nums_save:
    ldy #1
    lda (p_nums),Y
    cmp p_nums_save+1
    bne *+7
    dey
    lda (p_nums),Y
    cmp p_nums_save
    rts  ; .cmp_ind_p_nums_save
    
.copy_ind_p_nums_to_p_nums_plus_1:
    ldy #0
    lda (p_nums),Y
    ldy #2
    sta (p_nums),Y
    dey
    lda (p_nums),Y
    ldy #3
    sta (p_nums),Y
    rts  ; .copy_ind_p_nums_to_p_nums_plus_1
    
.copy_p_nums_save_to_ind_p_nums:
    ldy #0
    lda p_nums_save
    sta (p_nums),Y
    iny
    lda p_nums_save+1
    sta (p_nums),Y
    rts  ; .copy_p_nums_save_to_ind_p_nums
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

selection_sort:
    jsr init_p_nums_2_from_start
.next_num_2:
    jsr .cmp_p_nums_2_end_minus_1
    bcs .done
    
    jsr .copy_p_nums_2_to_min
    
    jsr init_p_nums_from_p_nums_2_plus_1
.next_num:
    jsr cmp_p_nums_end
    bcs .done_next_num
    
    jsr .cmp_ind_p_nums_min
    bcs .not_min
    jsr .copy_p_nums_to_min

.not_min:
    jsr inc_p_nums
    jmp .next_num
    
.done_next_num:    
    jsr .cmp_p_nums_2_min
    beq .no_swap
    
    jsr .swap_ind_p_nums_2_p_nums_min

.no_swap:
    jsr inc_p_nums_2
    jmp .next_num_2

.done:
    rts  ; selection_sort
    
.copy_p_nums_2_to_min:
    lda p_nums_2
    sta p_nums_min
    lda p_nums_2+1
    sta p_nums_min+1
    rts  ; .copy_p_nums_2_to_min
    
.swap_ind_p_nums_2_p_nums_min:
    ldy #0
    lda (p_nums_2),Y
    sta swap_temp
    lda (p_nums_min),Y
    sta (p_nums_2),Y
    lda swap_temp
    sta (p_nums_min),Y
    iny
    lda (p_nums_2),Y
    sta swap_temp
    lda (p_nums_min),Y
    sta (p_nums_2),Y
    lda swap_temp
    sta (p_nums_min),Y
    rts  ; .swap_ind_p_nums_2_p_nums_min

.cmp_p_nums_2_end_minus_1:
    lda p_nums_2+1
    cmp #>(nums_end-2)
    bne *+6
    lda p_nums_2
    cmp #<(nums_end-2)
    rts  ; .cmp_p_nums_2_end_minus_1
    
.cmp_ind_p_nums_min:
    ldy #1
    lda (p_nums),Y
    cmp (p_nums_min),Y
    bne *+7
    dey
    lda (p_nums),Y
    cmp (p_nums_min),Y
    rts  ; .cmp_ind_p_nums_min
    
.copy_p_nums_to_min:
    lda p_nums
    sta p_nums_min
    lda p_nums+1
    sta p_nums_min+1
    rts  ; .copy_p_nums_to_min
    
.cmp_p_nums_2_min:
    lda p_nums_2+1
    cmp p_nums_min+1
    bne *+6
    lda p_nums_2
    cmp p_nums_min
    rts  ; .cmp_p_nums_2_min
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bubble_sort_optimized:
    jsr .init_p_nums_last
.pass:
    jsr init_p_nums_from_start
    jsr .copy_p_nums_to_new_last
    jsr inc_p_nums

.next_num:
    jsr .cmp_p_nums_last
    bcs .done_pass
    
    jsr init_p_nums_2_from_p_nums_minus_1
    
    jsr cmp_ind_p_nums_p_nums_2
    bcs .no_swap
    
    jsr bubble_sort.swap_ind_p_nums_2_p_nums_min
    jsr .copy_p_nums_to_new_last
    
.no_swap:
    jsr inc_p_nums
    jmp .next_num
    
.done_pass:
    jsr .copy_p_nums_new_last_to_last

    jsr .cmp_p_nums_last_start_plus_1
    beq .done
    bcs .pass

.done:
    rts  ; bubble_sort_optimized
    
.init_p_nums_last:
    lda #<nums_end
    sta p_nums_last
    lda #>nums_end
    sta p_nums_last+1
    rts  ; .init_p_nums_last

.copy_p_nums_to_new_last:
    lda p_nums
    sta p_nums_new_last
    lda p_nums+1
    sta p_nums_new_last+1
    rts  ; .copy_p_nums_to_new_last

.copy_p_nums_new_last_to_last:
    lda p_nums_new_last
    sta p_nums_last
    lda p_nums_new_last+1
    sta p_nums_last+1
    rts  ; .copy_p_nums_new_last_to_last

.cmp_p_nums_last:
    lda p_nums+1
    cmp p_nums_last+1
    bne *+6
    lda p_nums
    cmp p_nums_last
    rts  ; .cmp_p_nums_last
    
.cmp_p_nums_last_start_plus_1:
    lda p_nums_last+1
    cmp #>(nums_start+2)
    bne *+6
    lda p_nums_last
    cmp #<(nums_start+2)
    rts  ; .cmp_p_nums_last_start_plus_1
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bubble_sort:
.pass:
    lda #0
    sta swapped
    jsr init_p_nums_from_start
    jsr inc_p_nums

.next_num:
    jsr cmp_p_nums_end
    bcs .done_pass
    
    jsr init_p_nums_2_from_p_nums_minus_1
    
    jsr cmp_ind_p_nums_p_nums_2
    bcs .no_swap
    
    jsr .swap_ind_p_nums_2_p_nums_min
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

.swap_ind_p_nums_2_p_nums_min:
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
    rts  ; .swap_ind_p_nums_p_nums_2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

fill_nums:
    ;jsr _RND16_SEED_FROM_TIME
    jsr _RND16_SEED_FROM_DEFAULT
    jsr init_p_nums_from_start
    
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

print_nums:
    jsr init_p_nums_from_start
    
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

verify_sorted:
    jsr init_p_nums_from_start
    jsr inc_p_nums

.next_num:
    jsr cmp_p_nums_end
    bcs .done
    
    jsr init_p_nums_2_from_p_nums_minus_1
    
    jsr cmp_ind_p_nums_p_nums_2
    bcc .error

    jsr inc_p_nums
    jmp .next_num
    
.error:
    brk
    .byte 0, "Not sorted!", 0

.done:
    rts  ; verify_sorted

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

init_p_nums_from_start:
    lda #<nums_start
    sta p_nums
    lda #>nums_start
    sta p_nums+1
    rts  ; init_p_nums_from_start
    
init_p_nums_from_p_nums_2_plus_1:
    clc
    lda p_nums_2
    adc #2
    sta p_nums
    lda p_nums_2+1
    adc #0
    sta p_nums+1
    rts  ; init_p_nums_from_p_nums_2_plus_1
    
cmp_p_nums_start:
    lda p_nums+1
    cmp #>nums_start
    bne *+6
    lda p_nums
    cmp #<nums_start
    rts  ; cmp_p_nums_end
    
cmp_p_nums_end:
    lda p_nums+1
    cmp #>nums_end
    bne *+6
    lda p_nums
    cmp #<nums_end
    rts  ; cmp_p_nums_end
    
inc_p_nums:
    inc p_nums
    inc p_nums
    bne *+4
    inc p_nums+1
    rts  ; inc_p_nums
    
dec_p_nums:
    lda p_nums
    bne *+4
    dec p_nums+1
    dec p_nums
    dec p_nums
    rts  ; dec_p_nums
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

init_p_nums_2_from_start:
    lda #<nums_start
    sta p_nums_2
    lda #>nums_start
    sta p_nums_2+1
    rts  ; init_p_nums_2_from_start
    
init_p_nums_2_from_p_nums_minus_1:
    sec
    lda p_nums
    sbc #2
    sta p_nums_2
    lda p_nums+1
    sbc #0
    sta p_nums_2+1
    rts  ; init_p_nums_2_from_p_nums_minus_1
    
cmp_p_nums_2_end:
    lda p_nums_2+1
    cmp #>nums_end
    bne *+6
    lda p_nums_2
    cmp #<nums_end
    rts  ; cmp_p_nums_2_end
    
cmp_ind_p_nums_p_nums_2:
    ldy #1
    lda (p_nums),Y
    cmp (p_nums_2),Y
    bne *+7
    dey
    lda (p_nums),Y
    cmp (p_nums_2),Y
    rts  ; cmp_ind_p_nums_p_nums_2
    
inc_p_nums_2:
    inc p_nums_2
    inc p_nums_2
    bne *+4
    inc p_nums_2+1
    rts  ; inc_p_nums_2
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.include "elapsed.asm"
.include "outnum.asm"
.include "outstr.asm"
.include "rand.asm"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
