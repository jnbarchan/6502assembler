; sortfile.asm

.include "common.inc"

last_line = PAD
next_line = PAD+$100
this_line = PAD+$200

test_sortfile:
    lda #<.filename
    ldx #>.filename
    jsr __open_file
    
    lda #0
    sta last_line
    
.rewind_file:
    jsr __rewind_file
    lda #$ff
    sta next_line
    lda #0
    sta next_line+1
    
.read_this_line:
    jsr read_into_this_line
    cpx #0
    beq .done_file
  
    jsr cmp_this_line_to_last_line
    bcc .read_this_line
    beq .read_this_line
    
    jsr cmp_this_line_to_next_line
    bcs .read_this_line

    jsr copy_this_line_to_next_line
    jmp .read_this_line

.done_file:
    lda next_line
    cmp #$ff
    beq .done_sort
    jsr copy_next_line_to_last_line
    ;jsr _outstr
    lda #<last_line
    ldx #>last_line
    jsr __outstr_fast
    ;jsr __process_events
    jmp .rewind_file

.done_sort:
    jsr __close_file
    jsr _outstr_inline
    .byte ">>Finished<<", 0
    rts  ; test_sortfile 

.filename:
    .byte "../../samples/scratchfile.txt", 0
    

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
    bcc .lt
    bne .gt
    lda this_line,X
    bne .cmp_char
.eq:
.lt:
.gt:
.return:
    rts  ; cmp_this_line_to_last_line
    
cmp_this_line_to_next_line:
    ldx #$ff
.cmp_char:
    inx
    lda this_line,X
    cmp next_line,X
    bcc .lt
    bne .gt
    lda next_line,X
    bne .cmp_char
.eq:
.lt:
.gt:
.return:
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
    

.include "outstr.asm"
