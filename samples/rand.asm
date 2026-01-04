; -------------------------------------------------------
; 8-bit LFSR PRNG for 6502
; RND_STATE: holds current state (seed with any non-zero value)
; Returns: A = next random byte
; -------------------------------------------------------

;;        .org $8000        ; example start address

;; RND8_STATE: .byte $55      ; initial seed (non-zero)

_RND_TEST:
    lda #$55      ; initial seed (non-zero)
    sta _RND8_STATE
    jsr _RND8_NEXT
    jsr _RND8_NEXT
    jsr _RND8_NEXT

    lda #$34   ; seed low byte (non-zero)
    sta _RND16_STATE_LOW
    lda #$12   ; seed high byte (non-zero)
    sta _RND16_STATE_HIGH
    jsr _RND16_NEXT
    jsr _RND16_NEXT
    jsr _RND16_NEXT
    rts

_RND8_NEXT:
        LDA _RND8_STATE   ; load current state
        LSR A            ; shift right, bit 0 -> carry
        BCC .skip_xor8
        ; feedback when bit 0 was 1
        EOR #%10000011   ; feedback mask (arbitrary taps for LFSR)
.skip_xor8:
        STA _RND8_STATE   ; save updated state
        LDA _RND8_STATE   ; return result in A
        RTS
_RND8_STATE: NOP      ; initial seed (non-zero)


; -------------------------------------------------------
; 16-bit LFSR PRNG for 6502
; _RND_STATE_LOW / _RND_STATE_HIGH: current state
; Returns: A = low byte, X = high byte of next random number
; -------------------------------------------------------

;;        .org $8000

;; _RND16_STATE_LOW:  .byte $34   ; seed low byte (non-zero)
;; _RND16_STATE_HIGH: .byte $12   ; seed high byte (non-zero)

_RND16_NEXT:
        LDA _RND16_STATE_LOW
        LSR A                ; shift right, bit 0 -> carry
        BCC .skip_feedback
        ; feedback when bit 0 was 1
        LDA _RND16_STATE_HIGH
        EOR #%10101010       ; feedback mask for high byte (example)
        STA _RND16_STATE_HIGH
.skip_feedback:
        LDA _RND16_STATE_HIGH
        ROR _RND16_STATE_LOW     ; rotate right through low byte
        LDA _RND16_STATE_LOW     ; result low byte in A
        LDX _RND16_STATE_HIGH    ; result high byte in X
        RTS

_RND16_STATE_LOW: NOP   ; seed low byte (non-zero)
_RND16_STATE_HIGH: NOP   ; seed high byte (non-zero)
