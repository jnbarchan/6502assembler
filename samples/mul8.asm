_mul8_test:
    lda #$10
    sta $70

    lda #$5
    sta $72

    jsr _mul8
    rts  ; _mul8_test

_mul8:
    lda #0
    sta $74
.next:
    lda $72
    beq .done
    and #1
    beq .done_add
    clc
    lda $70
    adc $74
    sta $74
.done_add:
    asl $70
    lsr $72
    jmp .next
.done:
    rts  ; _mul8
