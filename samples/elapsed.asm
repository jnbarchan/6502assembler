; elapsed.asm

.include "common.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
_clear_elapsed_time_cycles:
    jsr __clear_elapsed_time
    jsr __clear_elapsed_cycles
    rts  ; _clear_elapsed_time_cycles

_elapsed_time:
    lda #<dividend_32
    ldx #>dividend_32
    jsr __get_elapsed_time
    jsr _outnum32_commas
    jsr _outstr_inline
    .byte " ms", 0
    lda #10
    jsr __outch
    rts  ; _elapsed_time

_elapsed_cycles:
    lda #<dividend_32
    ldx #>dividend_32
    jsr __get_elapsed_cycles
    jsr _outnum32_commas
    jsr _outstr_inline
    .byte " cycles", 0
    lda #10
    jsr __outch
    rts  ; _elapsed_cycles

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.include "outnum.asm"
.include "outstr.asm"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
