; assert.asm

_assert:
    beq *+3
    rts  ; _assert
    brk
    .byte 0, "Assertion failed.", 0
