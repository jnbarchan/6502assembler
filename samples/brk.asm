; brk.asm

.include "common.inc"

saved_vector = ZP_USER_0

main:
    lda __BRKV
    sta catch_brk.saved_vector
;    sta saved_vector
    lda __BRKV+1
    sta catch_brk.saved_vector+1
;    sta saved_vector+1
    
    lda #<ignore_brk
    sta __BRKV
    lda #>ignore_brk
    sta __BRKV+1
    
    brk
    .byte 0
    
    lda #<catch_brk
    sta __BRKV
    lda #>catch_brk
    sta __BRKV+1
    
    brk
    .byte 0, "Done brk.", 0
    rts
    brk
    
ignore_brk:
    rti
    
catch_brk:
    jsr __outstr_inline
    .byte "In catch_brk", 0
    jmp (.saved_vector)
;    jmp (saved_vector)
    
.saved_vector:
    .byte 0, 0
    

.include "outstr.asm"
