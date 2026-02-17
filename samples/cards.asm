; cards.asm

.include "common.inc"

card      = $70
card_suit = $71
card_rank = $72

pack_count     = $0600
pack           = $0601
shuffled_count = $0700
shuffled       = $0701

test_cards:
    jsr _RND16_SEED_FROM_DEFAULT
    jsr _RND16_SEED_FROM_TIME
    jsr .init_pack
    jsr .shuffle
    jsr .show_shuffled
    jsr .verify_shuffled
    rts  ; test_cards
    brk
    
.init_pack:
    ; pack_count = 52
    ldx #52
    stx pack_count
    ; pack[0..pack_count-1] = 0..51
    dex
.next_init:
    txa
    sta pack,X
    dex
    bpl .next_init
    rts  ; .init_cards

.shuffle:
    ; shuffled_count = 0
    ldx #0
    stx shuffled_count
.next_shuffle:
    ; _RND16_NEXT
    jsr .rnd16_mod_pack_count
    
    ; shuffled[shuffled_count++] = pack[remainder]
    ldx remainder
    lda pack,X
    ldx shuffled_count
    sta shuffled,X
    inc shuffled_count
    
    ; move pack[remainder+1...] to pack[remainder...]
    ldx remainder
.move_loop:
    inx
    lda pack,X
    dex
    sta pack,X
    inx
    cpx pack_count
    bmi .move_loop
    ; until pack_count = 0
    dec pack_count
    bne .next_shuffle
    rts  ; .shuffle

.rnd16_mod_pack_count:
    ; set _RND16_STATE_LOW/HIGH
    jsr _RND16_NEXT
    ; random number /%= pack_count
    lda _RND16_STATE_LOW
    sta dividend
    lda _RND16_STATE_HIGH
    sta dividend+1
    lda pack_count
    sta divisor
    lda #0
    sta divisor+1
    jsr _div16
    rts  ; .rnd16_mod_pack_count
    
.show_shuffled:
    ldx #0
.next_show:
    lda shuffled,X
    sta card
    jsr .suit_and_rank
    
    ; suit
    ldy card_suit
    lda .suits,Y
    jsr __outch
    ; rank
    ldy card_rank
    lda .ranks,Y
    cmp #'0'
    bne .outch
    ; "10"
    lda #'1'
    jsr __outch
    lda #'0'
.outch:
    jsr __outch
    inx
    cpx #52
    beq .done
    ; ", "
    lda #','
    jsr __outch
    lda #' '
    jsr __outch
    jmp .next_show
.done:
    lda #10
    jsr __outch
    rts  ; .show_cards
    
.verify_shuffled:
    ; pack[0..pack_count-1] = 1
    lda #1
    ldx #51
.verify_loop_0:
    sta pack,X
    dex
    bpl .verify_loop_0
    
    ; check shuffled[0..51] are all different
    ldy #51
.verify_loop_1:
    ldx shuffled,Y
    lda pack,X
    jsr _assert
    dec pack,X
    dey
    bpl .verify_loop_1
    rts  ; .verify_shuffled

.suits:
    .byte 'C', 'D', 'H', 'S'
.ranks:
    .byte '2', '3', '4', '5', '6', '7', '8', '9', '0', 'J', 'Q', 'K', 'A'
    
.suit_and_rank:
    ; card_suit = card / 13, card_rank = card % 13
    lda #$ff
    sta card_suit
    lda card
.next_suit:
    inc card_suit
    sta card_rank
    sec
    sbc #13
    bpl .next_suit
    rts  ; .suit_and_rank
    

.include "assert.asm"
.include "rand.asm"
.include "div.asm"
