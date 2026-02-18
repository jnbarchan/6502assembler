; sortfile.asm

.include "common.inc"

last_line = PAD
next_line = PAD+$100
this_line = PAD+$200

next_word = PAD
sorted_words_start = MEM_USER_0+$1000
sorted_words_end = sorted_words_start+$1000

this_line_reached = $70
next_word_len = $72
sorted_words_reached = $74
new_sorted_words_reached = $76
insert_word_at = $78

test_sortfile:

;    jsr _clear_elapsed_time_cycles
;    jsr test_sortfile_1
;    jsr _elapsed_cycles
;    jsr _elapsed_time
    
    jsr _clear_elapsed_time_cycles
    jsr test_sortfile_2
    jsr _elapsed_cycles
    jsr _elapsed_time
    
    rts  ; test_sortfile
    brk

.filename:
;    .byte "../../samples/scratchfile.txt", 0
    .byte "../../samples/scratchfile2.txt", 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

test_sortfile_1:
    lda #<test_sortfile.filename
    ldx #>test_sortfile.filename
    jsr __open_file
    
    lda #0
    sta last_line
    
.rewind_file:
    jsr __rewind_file
    lda #$ff
    sta next_line
    lda #0
    sta next_line+1
    
    jsr .read_lines

.done_file:
    lda next_line
    cmp #$ff
    beq .done_sort
    jsr copy_next_line_to_last_line
    lda #<last_line
    ldx #>last_line
    jsr __outstr_fast
    jmp .rewind_file

.done_sort:
    jsr __close_file
    jsr _outstr_inline
    .byte ">>Finished<<", 10, 0
    rts  ; test_sortfile_1 
    
.read_lines:
    jsr read_into_this_line
    cpx #0
    beq .done_read_lines
  
    jsr cmp_this_line_to_last_line
    bcc .read_lines
    beq .read_lines
    
    jsr cmp_this_line_to_next_line
    bcs .read_lines

    jsr copy_this_line_to_next_line
    jmp .read_lines
.done_read_lines:
    rts  ; .read_lines

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

read_into_this_line:
    ldx #0
.read_char:
    jsr __read_file
    bcs .done_this_line
    sta this_line,X
    inx
    cmp #10
    beq .done_this_line
    cpx #255
    bne .read_char
.done_this_line:
    lda #0
    sta this_line,X
.return:
    rts  ; read_into_this_line

copy_this_line_to_last_line:
    ldx #$ff
.copy_char:
    inx
    lda this_line,X
    sta last_line,X
    bne .copy_char
    rts  ; copy_this_line_to_last_line
    
cmp_this_line_to_last_line:
    ldx #$ff
.cmp_char:
    inx
    lda this_line,X
    cmp last_line,X
    bne *+7
    lda this_line,X
    bne .cmp_char
    rts  ; cmp_this_line_to_last_line
    
cmp_this_line_to_next_line:
    ldx #$ff
.cmp_char:
    inx
    lda this_line,X
    cmp next_line,X
    bne *+7
    lda next_line,X
    bne .cmp_char
    rts  ; cmp_this_line_to_next_line

copy_this_line_to_next_line:
    ldx #$ff
.copy_char:
    inx
    lda this_line,X
    sta next_line,X
    bne .copy_char
    rts  ; copy_this_line_to_next_line
    
copy_next_line_to_last_line:
    ldx #$ff
.copy_char:
    inx
    lda next_line,X
    sta last_line,X
    bne .copy_char
    rts  ; copy_next_line_to_last_line

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

test_sortfile_2:
    lda #<test_sortfile.filename
    ldx #>test_sortfile.filename
    jsr __open_file
    
    lda #<sorted_words_start
    sta sorted_words_reached
    lda #>sorted_words_start
    sta sorted_words_reached+1
    
    jsr .read_lines
    
    jsr __close_file
    jsr _outstr_inline
    .byte ">>Finished<<", 10, 0
    rts  ; test_sortfile_2
    
.read_lines:
    jsr read_into_this_line
    cpx #0
    beq .done_read_lines
    ldx #0
    stx this_line_reached 
    
.get_words:
    jsr get_next_word
    lda next_word_len
    beq .read_lines

    jsr find_word_in_sorted_words
    beq .done_insert_word
    
    lda new_sorted_words_reached
    sta insert_word_at
    lda new_sorted_words_reached+1
    sta insert_word_at+1
    jsr insert_word_into_sorted_words
.done_insert_word:
    jmp .get_words
    
.done_read_lines:
    jsr show_sorted_words
    rts  ; .read_lines

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

get_next_word:
    ldx this_line_reached
    dex
    ldy #0
.next_char:
    inx
    lda this_line,X
    beq .done_word
    jsr .is_word_char
    beq .non_word_char
    ; next_word,Y = this_line,X
    lda this_line,X
    sta next_word,Y
    iny
    bne .next_char
.non_word_char:
    ; skip leading non word char
    cpy #0
    beq .next_char
.done_word:
    ; next_word,Y = 0
    lda #0
    sta next_word,Y
    ; next_word_len = Y
    sty next_word_len
    ; this_line_reached = X
    stx this_line_reached
    rts  ; get_next_word
    
.is_word_char:
    cmp #'_'
    beq .is_word_char_yes
    cmp #'A'
    bcc *+6
    cmp #'Z'+1
    bcc .is_word_char_yes
    cmp #'a'
    bcc *+6
    cmp #'z'+1
    bcc .is_word_char_yes
    cpy #0
    beq .is_word_char_no
    cmp #'0'
    bcc *+6
    cmp #'9'+1
    bcc .is_word_char_yes
.is_word_char_no:
    lda #0
    beq *+4
.is_word_char_yes:
    lda #1
    rts  ; .is_word_char
    
find_word_in_sorted_words:
    ; assert next_word[next_word_len] == 0
    ldy next_word_len
    lda next_word,Y
    beq *+7
    lda #0
    jsr _assert
    ; next_word_len++ (for terminating 0]
    inc next_word_len
    
    ; new_sorted_words_reached = sorted_words_start
    lda #<sorted_words_start
    sta new_sorted_words_reached
    lda #>sorted_words_start
    sta new_sorted_words_reached+1
    
.next_word:
    ; compare new_sorted_words_reached against sorted_words_reached
    lda new_sorted_words_reached+1
    cmp sorted_words_reached+1
    bne *+6
    lda new_sorted_words_reached
    cmp sorted_words_reached
    bcs .not_found
    
    ; compare (next_word) against (new_sorted_words_reached)
    lda #<next_word
    sta src
    lda #>next_word
    sta src+1
    lda new_sorted_words_reached
    sta dest
    lda new_sorted_words_reached+1
    sta dest+1
    ldy next_word_len
    jsr _memcmp
    bcc .not_found
    beq .found
    
    ; new_sorted_words_reached += word length
    jsr move_over_sorted_word
    
    jmp .next_word

.not_found:
    lda #1
.found:
    rts  ; find_word_in_sorted_words
    
insert_word_into_sorted_words:
    ; new_sorted_words_reached = sorted_words_reached + next_word_len
    clc
    lda sorted_words_reached
    adc next_word_len
    sta new_sorted_words_reached
    lda sorted_words_reached+1
    adc #0
    sta new_sorted_words_reached+1
    
    ; compare new_sorted_words_reached against sorted_words_end for enough space
    lda new_sorted_words_reached+1
    cmp #>sorted_words_end
    bne *+6
    lda new_sorted_words_reached
    cmp #<sorted_words_end
    bcs .done
    
    ; move memory upwards from (insert_word_at) by next_word_len to make room
    clc
    lda insert_word_at
    sta src
    adc next_word_len
    sta dest
    lda insert_word_at+1
    sta src+1
    adc #0
    sta dest+1
    sec
    lda sorted_words_reached
    sbc insert_word_at
    sta count
    lda sorted_words_reached+1
    sbc insert_word_at+1
    sta count+1
    jsr _memmove16
    
    ; copy next_word to (insert_word_at)
    jsr .copy_next_word_to_insert_word_at
    
    ; sorted_words_reached = new_sorted_words_reached
    lda new_sorted_words_reached
    sta sorted_words_reached
    lda new_sorted_words_reached+1
    sta sorted_words_reached+1
    
.done:
    rts  ; insert_word_into_sorted_words
    
.copy_next_word_to_insert_word_at:
    ; copy next_word to (insert_word_at)
    ldy #$ff
.next_copy_char:
    iny
    lda next_word,Y
    sta (insert_word_at),Y
    bne .next_copy_char
    rts  ; .copy_next_word_to_insert_word_at
    
show_sorted_words:
    ; new_sorted_words_reached = sorted_words_start
    lda #<sorted_words_start
    sta new_sorted_words_reached
    lda #>sorted_words_start
    sta new_sorted_words_reached+1
    
.next_sorted_word:
    ; compare new_sorted_words_reached against sorted_words_reached
    lda new_sorted_words_reached+1
    cmp sorted_words_reached+1
    bne *+6
    lda new_sorted_words_reached
    cmp sorted_words_reached
    bcs .done_show
    
    ; show word at new_sorted_words_reached
    lda new_sorted_words_reached
    ldx new_sorted_words_reached+1
    jsr __outstr_fast
    lda #10
    jsr __outch
    
    ; new_sorted_words_reached += word length
    jsr move_over_sorted_word
    jmp .next_sorted_word
    
.done_show:
    rts  ; show_sorted_words
    
move_over_sorted_word:
    ; new_sorted_words_reached += word length
    ldy #0
.next_char:
    lda (new_sorted_words_reached),Y
    inc new_sorted_words_reached
    bne *+4
    inc new_sorted_words_reached+1
    cmp #0
    bne .next_char
    rts  ; move_over_sorted_word

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.include "assert.asm"
.include "mem.asm"
.include "elapsed.asm"
.include "outnum.asm"
.include "outstr.asm"
