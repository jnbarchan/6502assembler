; mem.asm

.include "common.inc"

testmem = MEM_USER_0+$1000
testmem_size = $100
testmem16 = testmem + testmem_size
testmem16_pages = 20 * 4
testmem16_size = testmem16_pages * $100 + 5

test_mem:
    lda #>testmem
    sta dest+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_mempageset: ", 0
    lda #$99
    jsr _mempageset
    jsr _elapsed_cycles

    lda #>testmem
    sta dest+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_mempageset_count: ", 0
    ldx #testmem16_pages
    lda #$88
    jsr _mempageset_count
    jsr _elapsed_cycles

    lda #<testmem
    sta dest
    lda #>testmem
    sta dest+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_memset: ", 0
    ldy #testmem_size-1
    lda #'A'
    jsr _memset
    jsr _elapsed_cycles

    lda #<testmem16
    sta dest
    lda #>testmem16
    sta dest+1
    lda #<testmem16_size
    sta count
    lda #>testmem16_size
    sta count+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_memset16: ", 0
    lda #'C'
    jsr _memset16
    jsr _elapsed_cycles
    
    jsr .fill_testmem
    lda #<testmem
    sta src
    lda #>testmem
    sta src+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_memchr: ", 0
    ldy #testmem_size-1
    lda #'z'
    jsr _memchr
    jsr _elapsed_cycles

    jsr .fill_testmem
    lda #<testmem
    sta src
    lda #>testmem
    sta src+1
    lda #<testmem16_size
    sta count
    lda #>testmem16_size
    sta count+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_memchr16: ", 0
    lda #'z'
    jsr _memchr16
    jsr _elapsed_cycles

    jsr .fill_testmem
    lda #<testmem
    sta dest
    lda #>testmem
    sta dest+1
    lda #<testmem16
    sta src
    lda #>testmem16
    sta src+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_memcpy: ", 0
    ldy #testmem_size-1
    jsr _memcpy    
    jsr _elapsed_cycles

    jsr .fill_testmem
    lda #<testmem
    sta dest
    lda #>testmem
    sta dest+1
    lda #<(testmem - 16)
    sta src
    lda #>(testmem - 16)
    sta src+1
    lda #<testmem16_size
    sta count
    lda #>testmem16_size
    sta count+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_memcpy16: ", 0
    jsr _memcpy16    
    jsr _elapsed_cycles

    jsr .fill_testmem
    lda #<testmem
    sta dest
    lda #>testmem
    sta dest+1
    lda #<(testmem+1)
    sta src
    lda #>(testmem+1)
    sta src+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_memmove (up): ", 0
    ldy #testmem_size-1
    jsr _memmove    
    jsr _elapsed_cycles

    jsr .fill_testmem
    lda #<(testmem+1)
    sta dest
    lda #>(testmem+1)
    sta dest+1
    lda #<testmem
    sta src
    lda #>testmem
    sta src+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_memmove (down): ", 0
    ldy #testmem_size-1
    jsr _memmove    
    jsr _elapsed_cycles

    jsr .fill_testmem
    lda #<testmem
    sta dest
    lda #>testmem
    sta dest+1
    lda #<(testmem+1)
    sta src
    lda #>(testmem+1)
    sta src+1
    lda #<testmem16_size
    sta count
    lda #>testmem16_size
    sta count+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_memmove16 (up): ", 0
    jsr _memmove16    
    jsr _elapsed_cycles

    jsr .fill_testmem
    lda #<(testmem+1)
    sta dest
    lda #>(testmem+1)
    sta dest+1
    lda #<testmem
    sta src
    lda #>testmem
    sta src+1
    lda #<testmem16_size
    sta count
    lda #>testmem16_size
    sta count+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_memmove16 (down): ", 0
    jsr _memmove16    
    jsr _elapsed_cycles

    jsr .fill_testmem
    lda #<testmem
    sta dest
    lda #>testmem
    sta dest+1
    lda #<testmem
    sta src
    lda #>testmem
    sta src+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_memcmp: ", 0
    ldy #testmem_size-1
    jsr _memcmp    
    jsr _elapsed_cycles

    jsr .fill_testmem
    lda #<testmem
    sta dest
    lda #>testmem
    sta dest+1
    lda #<testmem
    sta src
    lda #>testmem
    sta src+1
    lda #<testmem16_size
    sta count
    lda #>testmem16_size
    sta count+1
    jsr __clear_elapsed_cycles
    jsr __outstr_inline
    .byte "_memcmp16: ", 0
    jsr _memcmp16    
    jsr _elapsed_cycles

    rts  ; test_mem
    brk
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.fill_testmem:
    ; fill testmem[0..255] with [0..255]
    ldx #0
.fill_loop:
    txa
    sta testmem,X
    inx
    bne .fill_loop
    rts  ; .fill_testmem
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_mempageset_count:
    ; X times (dest+1 page)[0..255] = A
.next_X:
    dex
    cpx #$ff
    beq .done
    jsr _mempageset
    jmp .next_X
.done:
    rts  ; _mempageset_count

_mempageset:
    ; (dest+1 page)[0..255] = A
    ldy #0
    sty dest
.next:
    ; (dest),Y = A
    sta (dest),Y
    iny
    bne .next
    rts  ; _mempageset

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_memset16:
    ; (dest)[0..count-1] = A
    sta ZP_A_SAVE
    ldy #0
    ; count+1 == 0?
    lda count+1
    beq .done_256
    lda ZP_A_SAVE
.next_256:
    ; (dest),Y = A
    sta (dest),Y
    iny
    bne .next_256
    ; dest+1 ++, count+1 --
    inc dest+1
    dec count+1
    ; count+1 == 0?
    bne .next_256
.done_256:
    ldy count
    jmp _memset  ; _memset16

_memset:
    ; (dest)[Y-1..0] = A
    dey
    cpy #$ff
    beq .done
    ; (dest),Y = A
    sta (dest),Y
    jmp _memset
.done:
    rts  ; _memset

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_memchr16:
    ; src = index of A in (src)[0..count-1]
    ; src == $ffff => not found
    sta ZP_A_SAVE
    ldy #0
    ; count+1 == 0?
    lda count+1
    beq .done_256
    lda ZP_A_SAVE
.next_256:
    ; cmp (src),Y
    cmp (src),Y
    beq .found
    iny
    bne .next_256
    ; src+1 ++, count+1 --
    inc src+1
    dec count+1
    ; count+1 == 0?
    bne .next_256
.done_256:
    ldy count
    jsr _memchr
    cpy #$ff
    bne .found
    sty src
    sty src+1
    bne .done
.found:
    clc
    tya
    adc src
    sta src
    bcc *+4
    inc src+1
.done:
    rts  ; _memchr16

_memchr:
    ; Y = index of A in (src)[0..Y-1]
    ; $ff => not found
    sty ZP_Y_SAVE
    ldy #0
.next:
    cpy ZP_Y_SAVE
    beq .not_found
    ; cmp (src),Y
    cmp (src),Y
    beq .done
    iny
    bne .next
.not_found:
    ldy #$ff
.done:
    rts  ; _memchr

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_memcpy16:
    ; (dest)[0..count-1] = (src)[0..count-1]
    ldy #0
    ; count+1 == 0?
    lda count+1
    beq .done_256
.next_256:
    ; (dest),Y = (src),Y
    lda (src),Y
    sta (dest),Y
    iny
    bne .next_256
    ; src+1 ++, dest+1 ++, count+1 --
    inc src+1
    inc dest+1
    dec count+1
    ; count+1 == 0?
    bne .next_256
.done_256:
    ldy count
    jmp _memcpy  ; _memcpy16
    
_memcpy:
    ; (dest)[0..Y-1] = (src)[0..Y-1]
    sty ZP_Y_SAVE
    ldy #0
.next:
    cpy ZP_Y_SAVE
    beq .done
    ; (dest),Y = (src),Y
    lda (src),Y
    sta (dest),Y
    iny
    bne .next
.done:
    rts  ; _memcpy

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_memmove16:
    ; (dest)[0..count-1] = (src)[0..count-1]
    ; (dest)[count-1..0] = (src)[count-1..0]
    ; compare dest > src
    lda dest+1
    cmp src+1
    bne *+6
    lda dest
    cmp src
    bcs .downward
.upward:
    ; dest < src
    jmp _memcpy16
.downward:
    ; dest >= src
    ; src += count
    clc
    lda src
    adc count
    sta src
    lda src+1
    adc count+1
    sta src+1
    ; dest += count
    clc
    lda dest
    adc count
    sta dest
    lda dest+1
    adc count+1
    sta dest+1

    ldy #$ff
.next_256:
    ; count+1 == 0?
    lda count+1
    beq .done_256
    ; src+1 --, dest+1 --, count+1 --
    dec src+1
    dec dest+1
    dec count+1
.next:
    ; (dest),Y = (src),Y
    lda (src),Y
    sta (dest),Y
    dey
    cpy #$ff
    bne .next
    beq .next_256
.done_256:
    ; src -= count
    sec
    lda src
    sbc count
    sta src
    lda src+1
    sbc #0
    sta src+1
    ; dest -= count
    sec
    lda dest
    sbc count
    sta dest
    lda dest+1
    sbc #0
    sta dest+1

    ldy count
    jmp _memmove.downward  ; _memmove16
    
_memmove:
    ; (dest)[0..Y-1] = (src)[0..Y-1]
    ; (dest)[Y-1..0] = (src)[Y-1..0]
    lda dest+1
    cmp src+1
    bne *+6
    lda dest
    cmp src
    bcs .downward
.upward:
    jmp _memcpy
.downward:
    dey
    cpy #$ff
    beq .done
    ; (dest),Y = (src),Y
    lda (src),Y
    sta (dest),Y
    jmp .downward
.done:
    rts  ; _memmove

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_memcmp16:
    ; set Z,C flags from comparing (src)[0..count-1] against (dest)[0..count-1]
    ldy #0
    ; count+1 == 0?
    lda count+1
    beq .done_256
.next_256:
    ; compare (src),Y against (dest),Y
    lda (src),Y
    cmp (dest),Y
    bne .done
    iny
    bne .next_256
    ; src+1 ++, dest+1 ++, count+1 --
    inc src+1
    inc dest+1
    dec count+1
    ; count+1 == 0?
    bne .next_256
.done_256:
    ldy count
    jmp _memcmp  ; _memcmp16
.done:
    rts  ; _memcmp16

_memcmp:
    ; set Z,C flags from comparing (src)[0..Y-1] against (dest)[0..Y-1]
    sty ZP_Y_SAVE
    ldy #0
.next:
    cpy ZP_Y_SAVE
    beq .eq
    ; compare (src),Y against (dest),Y
    lda (src),Y
    cmp (dest),Y
    bne .done
    iny
    bne .next
.eq:
    sec
.done:
    rts  ; _memcmp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    

.include "assert.asm"
.include "elapsed.asm"
