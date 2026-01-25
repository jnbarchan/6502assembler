; sortfile.asm

.include "common.inc"

test_file:
    lda #<.filename
    ldx #>.filename
    jsr __open_file
.read_line:
    ldx #0
.read_char:
    jsr __read_file
    bcs .done_file
    sta PAD,X
    inx
    cmp #10
    beq .done_line
    cpx #255
    bne .read_char
.done_line:
    lda #0
    sta PAD,X
    jsr _outstr
    jmp .read_line
.done_file:
    jsr __close_file
    rts  ; test_file; 
.filename:
    .byte "../../samples/scratchfile.txt", 0
    
.include "outstr.asm"
