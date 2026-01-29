; rand.asm

; -------------------------------------------------------
; 8-bit LFSR PRNG for 6502
; RND_STATE: holds current state (seed with any non-zero value)
; Returns: A = next random byte
; -------------------------------------------------------

;; RND8_STATE: .byte $55      ; initial seed (non-zero)

.include "common.inc"


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
;; _RND8_STATE: NOP      ; initial seed (non-zero)


; -------------------------------------------------------
; 16-bit LFSR PRNG for 6502
; _RND_STATE_LOW / _RND_STATE_HIGH: current state
; Returns: A = low byte, X = high byte of next random number
; -------------------------------------------------------

;;        .org $8000

;; _RND16_STATE_LOW:  .byte $34   ; seed low byte (non-zero)
;; _RND16_STATE_HIGH: .byte $12   ; seed high byte (non-zero)

_RND16_NEXT:
        LDA _RND16_STATE_HIGH
        LSR A                ; shift right, bit 0 -> carry
        STA _RND16_STATE_HIGH
        ROR _RND16_STATE_LOW  ; shift full 16-bit value right
        BCC .no_feedback
        ; feedback when bit 0 was 1
        LDA _RND16_STATE_HIGH
        EOR #$B4          ; high byte of polynomial
        STA _RND16_STATE_HIGH
.no_feedback:
        LDA _RND16_STATE_LOW     ; result low byte in A
        LDX _RND16_STATE_HIGH    ; result high byte in X
        RTS

;; _RND16_STATE_LOW: NOP   ; seed low byte (non-zero)
;; _RND16_STATE_HIGH: NOP   ; seed high byte (non-zero)

_RND16_SEED_FROM_TIME:
    jsr __get_time
    sta _RND16_STATE_LOW
    stx _RND16_STATE_HIGH
    rts

_RND16_SEED_FROM_DEFAULT:
    lda #1
    sta _RND16_STATE_LOW
    lda #0
    sta _RND16_STATE_HIGH
    rts
