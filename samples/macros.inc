; macros.inc
; put only macro definitions/assignments here, no code

.macro _Mov16 dest, src
    lda src
    sta dest
    lda src+1
    sta dest+1
.endmacro  ; _Mov16

.macro _MovImm16 dest, val
    lda #<val
    sta dest
    lda #>val
    sta dest+1
.endmacro  ; _MovImm16

.macro _Add16 dest, src
    clc
    lda dest
    adc src
    sta dest
    lda dest+1
    adc src+1
    sta dest+1
.endmacro  ; _Add16

.macro _AddImm16 dest, val
    clc
    lda dest
    adc #<val
    sta dest
    lda dest+1
    adc #>val
    sta dest+1
.endmacro  ; _AddImm16

.macro _Sub16 dest, src
    sec
    lda dest
    sbc src
    sta dest
    lda dest+1
    sbc src+1
    sta dest+1
.endmacro  ; _Sub16

.macro _SubImm16 dest, val
    sec
    lda dest
    sbc #<val
    sta dest
    lda dest+1
    sbc #>val
    sta dest+1
.endmacro  ; _SubImm16

