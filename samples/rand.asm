; -------------------------------------------------------
; 8-bit LFSR PRNG for 6502
; RND_STATE: holds current state (seed with any non-zero value)
; Returns: A = next random byte
; -------------------------------------------------------

;;        .org $8000        ; example start address

;; RND8_STATE: .byte $55      ; initial seed (non-zero)

lda #$55      ; initial seed (non-zero)
sta RND8_STATE
jsr RND8_NEXT
jsr RND8_NEXT
jsr RND8_NEXT

lda #$34   ; seed low byte (non-zero)
sta RND16_STATE_LOW
lda #$12   ; seed high byte (non-zero)
sta RND16_STATE_HIGH
jsr RND16_NEXT
jsr RND16_NEXT
jsr RND16_NEXT

brk

RND8_NEXT:
        LDA RND8_STATE   ; load current state
        LSR A            ; shift right, bit 0 -> carry
        BCC .skip_xor8
        ; feedback when bit 0 was 1
        EOR #%10000011   ; feedback mask (arbitrary taps for LFSR)
.skip_xor8:
        STA RND8_STATE   ; save updated state
        LDA RND8_STATE   ; return result in A
        RTS
RND8_STATE: NOP      ; initial seed (non-zero)


; -------------------------------------------------------
; 16-bit LFSR PRNG for 6502
; RND_STATE_LOW / RND_STATE_HIGH: current state
; Returns: A = low byte, X = high byte of next random number
; -------------------------------------------------------

;;        .org $8000

;; RND16_STATE_LOW:  .byte $34   ; seed low byte (non-zero)
;; RND16_STATE_HIGH: .byte $12   ; seed high byte (non-zero)

RND16_NEXT:
        LDA RND16_STATE_LOW
        LSR A                ; shift right, bit 0 -> carry
        BCC .skip_feedback
        ; feedback when bit 0 was 1
        LDA RND16_STATE_HIGH
        EOR #%10101010       ; feedback mask for high byte (example)
        STA RND16_STATE_HIGH
.skip_feedback:
        LDA RND16_STATE_HIGH
        ROR RND16_STATE_LOW     ; rotate right through low byte
        LDA RND16_STATE_LOW     ; result low byte in A
        LDX RND16_STATE_HIGH    ; result high byte in X
        RTS

RND16_STATE_LOW: NOP   ; seed low byte (non-zero)
RND16_STATE_HIGH: NOP   ; seed high byte (non-zero)
