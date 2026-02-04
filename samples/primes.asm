; primes.asm
; calculate prime numbers

.include "common.inc"

nextnum = ZP_USER_0
primes_counter = ZP_USER_0+2
primes_ptr = ZP_USER_0+4
primes_count = ZP_USER_0+6
last_prime = ZP_USER_0+8

nextnum_compact = ZP_USER_0+10
nextnum_compact_byte = ZP_USER_0+12
nextnum_compact_bit = ZP_USER_0+14
nextnum_compact_bit_mask = ZP_USER_0+15

primes = MEM_USER_0+$1000

primes_sieve_start = primes
primes_sieve_end = primes_sieve_start+$2000
primes_sieve_compact_end = primes_sieve_start+$400

max_primes = 1028

show_primes = 1  ; 0 => none, 1 => last, 2 => all

prime_test_32 = ZP_USER_1
prime_test_max_32 = ZP_USER_1+4
nextnum_32 = ZP_USER_1+8

;test_prime_number_32 = 370362977
test_prime_number_32 = 370362989

.org MEM_USER_0

main:
;    jsr _clear_elapsed_time_cycles
;    jsr run_primes_trial_division
;    jsr _clear_elapsed_time_cycles
;    jsr run_primes_eratosthenes_sieve
;    jsr _clear_elapsed_time_cycles
;    jsr run_primes_eratosthenes_sieve_compact
    jsr run_is_prime

    rts  ; main 
    brk

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

run_is_prime:
    ; prime_test_32 = test_prime_number_32
    lda #<test_prime_number_32
    sta prime_test_32
    lda #<(test_prime_number_32 >> 8)
    sta prime_test_32+1
    lda #<(test_prime_number_32 >> 16)
    sta prime_test_32+2
    lda #<(test_prime_number_32 >> 24)
    sta prime_test_32+3

    ; output prime_test_32
    jsr copy_prime_test_32_to_dividend_32
    jsr _outnum32_commas
    jsr _outstr_inline
    .byte ": ", 0

    jsr _clear_elapsed_time_cycles
    jsr is_prime
    
    ; 0 => not prime, non-0 => prime
    cmp #0
    beq .no
.yes:
    jsr _outstr_inline
    .byte "Yes", 10, 0
    jmp .return
.no:
    jsr _outstr_inline
    .byte "No", 10, 0
.return:
    jsr _elapsed_cycles
    jsr _elapsed_time
    jsr __process_events
    rts  ; run_is_prime

is_prime:
    ; if prime_test_32 <= 2 give answer
    lda prime_test_32+3
    ora prime_test_32+2
    ora prime_test_32+1
    bne .not_small_number
    lda prime_test_32
    cmp #2
    beq .yes
    bcc .no
    and #1
    beq .no
.not_small_number:
    ; divide by all numbers
    jsr is_prime_loop
    ; 0 => not prime, non-0 => prime
    cmp #0
    beq .no
.yes:
    lda #1
    bne *+4
.no:
    lda #0
    rts  ; is_prime
    
is_prime_loop:
    ; prime_test_max_32 = largest number to test as factor
    jsr calc_prime_max
    
    ; divisor_32 = 2
    lda #2
    sta divisor_32
    lda #0
    sta divisor_32+1
    sta divisor_32+2
    sta divisor_32+3
    
    ; test whether divisor_32 reached prime_test_max_32
.try_next:
    ldx #3
.cmp_next:
    lda divisor_32,X
    cmp prime_test_max_32,X
    bcc .keep_going
    bne .yes
    dex
    bpl .cmp_next
    
.keep_going:
    jsr copy_prime_test_32_to_dividend_32
    
    jsr _div32
    
    lda remainder_32
    ora remainder_32+1
    ora remainder_32+2
    ora remainder_32+3
    beq .no
    
.inc_next:
    ; divisor_32 += 1 if even (i.e. 2) else 2
    lda divisor_32
    lsr a
    lda divisor_32
    adc #1
    sta divisor_32
    lda divisor_32+1
    adc #0
    sta divisor_32+1
    lda divisor_32+2
    adc #0
    sta divisor_32+2
    lda divisor_32+3
    adc #0
    sta divisor_32+3
    
    jmp .try_next
    
.yes:
    lda #1
    bne *+4
.no:
    lda #0
    rts  ; is_prime_loop
    
calc_prime_max:
    ; prime_test_max_32 = largest number to test as factor
    ldx #3
.next_prime_test_32:
    lda prime_test_32,X
    sta sqrt_operand_32,X
    dex
    bpl .next_prime_test_32
    
    jsr _sqrt32
    
    ldx #3
.next_prime_test_max_32:
    lda sqrt_result_32,X
    sta prime_test_max_32,X
    dex
    bpl .next_prime_test_max_32

    rts  ; calc_prime_max

copy_prime_test_32_to_dividend_32:
    lda prime_test_32
    sta dividend_32
    lda prime_test_32+1
    sta dividend_32+1
    lda prime_test_32+2
    sta dividend_32+2
    lda prime_test_32+3
    sta dividend_32+3
    rts  ; copy_prime_test_32_to_dividend_32
    
copy_quotient_32_to_prime_test_32:
    lda quotient_32
    sta prime_test_max_32
    lda quotient_32+1
    sta prime_test_max_32+1
    lda quotient_32+2
    sta prime_test_max_32+2
    lda quotient_32+3
    sta prime_test_max_32+3
    rts  ; copy_quotient_32_to_prime_test_32
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

run_primes_trial_division:
    ; primes_count = 0
    lda #0
    sta primes_count
    sta primes_count+1
    
    ; nextnum = 2
    lda #2
    sta nextnum
    lda #0
    sta nextnum+1
    
.test_next_num:
    ; primes_counter = 0
    lda #0
    sta primes_counter
    sta primes_counter+1
    ; primes_ptr = &primes
    lda #<primes
    sta primes_ptr
    lda #>primes
    sta primes_ptr+1
.next_primes_count:
    ; if primes_counter == primes_count goto .no_more_primes
    lda primes_counter+1
    cmp primes_count+1
    bne .more_primes
    lda primes_counter
    cmp primes_count
    beq .no_more_primes
    
.more_primes:
    ; if exceeded sqrt(nextnum) goto .no_more_primes
    jsr .prime_stop_dividing
    beq *+4
    bcs .no_more_primes
    ; if divisible goto .try_next_num
    jsr .prime_divisible
    beq .try_next_num
    
    ; primes_counter++
    inc primes_counter
    bne *+4
    inc primes_counter+1
    ; primes_ptr++
    inc primes_ptr
    bne *+4
    inc primes_ptr+1
    ; primes_ptr++
    inc primes_ptr
    bne *+4
    inc primes_ptr+1
    ; try division by next prime
    jmp .next_primes_count
    
.no_more_primes:
    ; store nextnum as prime into (primes_ptr)
    jsr .found_prime   
    ; show prime found?
    lda #show_primes
    cmp #2
    bcc *+5
    jsr found_prime_output
    
    ; primes_count++, test for finished
    lda primes_count+1
    cmp #>max_primes
    bne .try_next_num
    lda primes_count
    cmp #<max_primes
    beq .finished
    
.try_next_num:
    ; nextnum++, test for finished
    inc nextnum
    bne *+6
    inc nextnum+1
    beq .finished
    ; if nextnum & 1 == 0 (even) nextnum++
    lda nextnum
    and #1
    bne *+4
    inc nextnum
    jmp .test_next_num

.finished:
    jsr _outstr_inline
    .byte "Trial division:", 10, 0
    ; show last prime found?
    lda #show_primes
    beq *+5
    jsr found_prime_output
    jsr _elapsed_cycles
    jsr _elapsed_time
    jsr __process_events
    rts  ; run_primes_trial_division
    
.prime_stop_dividing:
    ldy #0
    lda (primes_ptr),Y
    sta operand1
    sta operand2
    iny
    lda (primes_ptr),Y
    sta operand1+1
    sta operand2+1

    jsr _mul16
    lda result+1
    cmp nextnum+1
    bne *+6
    lda result
    cmp nextnum
    rts  ; .prime_stop_dividing
    
.prime_divisible:
    ; test if nextnum % (primes_ptr) == 0
    lda nextnum
    sta dividend
    lda nextnum+1
    sta dividend+1
        
    ldy #0
    lda (primes_ptr),Y
    sta divisor
    iny
    lda (primes_ptr),Y
    sta divisor+1
    
    jsr _div16
    lda remainder
    ora remainder+1
    rts  ; .prime_divisible
    
.found_prime:
    ; last_prime = nextnum
    lda nextnum
    sta last_prime
    lda nextnum+1
    sta last_prime+1
    
    ; primes_ptr = &primes
    lda #<primes
    sta primes_ptr
    lda #>primes
    sta primes_ptr+1

    ; primes_ptr += primes_count + primes_count
    lda primes_count
    clc
    adc primes_ptr
    sta primes_ptr
    lda primes_count+1
    adc primes_ptr+1
    sta primes_ptr+1
    lda primes_count
    clc
    adc primes_ptr
    sta primes_ptr
    lda primes_count+1
    adc primes_ptr+1
    sta primes_ptr+1
    
    ; (primes_ptr) = nextnum
    ldy #0
    lda nextnum
    sta (primes_ptr),Y
    iny
    lda nextnum+1
    sta (primes_ptr),Y

    ; primes_count++
    inc primes_count
    bne *+4
    inc primes_count+1
    rts  ; .found_prime

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
run_primes_eratosthenes_sieve_compact:
    ; set primes_sieve_start .. primes_sieve_compact_end to 0s
    jsr .clear_primes_sieve

    ; primes_count = 0
    lda #0
    sta primes_count
    sta primes_count+1
    
    ; nextnum = 2
    lda #2
    sta nextnum
    lda #0
    sta nextnum+1
    
.test_next_num:
    ; set nextnum_compact_byte/bit
    lda nextnum
    sta nextnum_compact
    lda nextnum+1
    sta nextnum_compact+1
    jsr .nextnum_compact_to_byte_bit
    ; primes_ptr = primes_sieve_start + nextnum_compact_byte
    clc
    lda #<primes_sieve_start
    adc nextnum_compact_byte
    sta primes_ptr
    lda #>primes_sieve_start
    adc nextnum_compact_byte+1
    sta primes_ptr+1
    
    ; test for primes_sieve_compact_end
    lda primes_ptr+1
    cmp #>primes_sieve_compact_end
    bne *+6
    lda primes_ptr
    cmp #<primes_sieve_compact_end
    bcs .finished
    
    ; see whether primes_ptr[nextnum_compact_byte/bit] is already marked as non-prime
    ldy #0
    lda (primes_ptr),Y
    ldx nextnum_compact_bit
    and .nextnum_compact_bit_masks,X
    bne .try_next_num

    ; mark all multiples in primes_sieve_start..primes_sieve_end
    jsr .found_prime   
    ; show prime found?
    lda #show_primes
    cmp #2
    bcc *+5
    jsr found_prime_output

.try_next_num:
    ; nextnum++, test for finished
    inc nextnum
    bne *+4
    inc nextnum+1
    beq .finished
    ; if nextnum & 1 == 0 (even) nextnum++
    lda nextnum
    and #1
    bne *+4
    inc nextnum
    jmp .test_next_num
    
.finished:
    jsr _outstr_inline
    .byte "Eratosthenes sieve compact:", 10, 0
    ; show last prime found?
    lda #show_primes
    beq *+5
    jsr found_prime_output
    jsr _elapsed_cycles
    jsr _elapsed_time
    jsr __process_events
    rts ; run_primes_eratosthenes_sieve_compact
    
.found_prime:
    ; last_prime = nextnum
    lda nextnum
    sta last_prime
    lda nextnum+1
    sta last_prime+1
    
.found_prime_next:
    ; mark primes_ptr[nextnum_compact_byte/bit] as non-prime
    ldy #0
    lda (primes_ptr),Y
    ldx nextnum_compact_bit
    ora .nextnum_compact_bit_masks,X
    sta (primes_ptr),Y
    
    ; nextnum_compact += nextnum
    clc
    lda nextnum_compact
    adc nextnum
    sta nextnum_compact
    lda nextnum_compact+1
    adc nextnum+1
    sta nextnum_compact+1
    jsr .nextnum_compact_to_byte_bit
    
    ; primes_ptr = primes_sieve_start + nextnum_compact_byte
    clc
    lda #<primes_sieve_start
    adc nextnum_compact_byte
    sta primes_ptr
    lda #>primes_sieve_start
    adc nextnum_compact_byte+1
    sta primes_ptr+1
    
    ; test for primes_sieve_compact_end
    lda primes_ptr+1
    cmp #>primes_sieve_compact_end
    bne *+6
    lda primes_ptr
    cmp #<primes_sieve_compact_end
    bcc .found_prime_next
    
    ; primes_count++
    inc primes_count
    bne *+4
    inc primes_count+1
    rts  ; .found_prime

.clear_primes_sieve:
    ; primes_ptr = primes_sieve_start
    lda #<primes_sieve_start
    sta primes_ptr
    lda #>primes_sieve_start
    sta primes_ptr+1

.clear_primes_sieve_next:
    ; test for primes_sieve_compact_end
    lda primes_ptr+1
    cmp #>primes_sieve_compact_end
    bne *+6
    lda primes_ptr
    cmp #<primes_sieve_compact_end
    bcs .done_clear_primes_sieve
    
    ; (primes_ptr) = 0
    ldy #0
    lda #0
    sta (primes_ptr),Y
    ; primes_ptr++
    inc primes_ptr
    bne *+4
    inc primes_ptr+1
    
    jmp .clear_primes_sieve_next

.nextnum_compact_bit_masks:
    .byte $01, $02, $04, $08, $10, $20, $40, $80

.nextnum_compact_to_byte_bit:
    lda nextnum_compact+1
    sta nextnum_compact_byte+1
    lda nextnum_compact
    sta nextnum_compact_byte
    and #%111
    sta nextnum_compact_bit
    lsr nextnum_compact_byte+1
    ror nextnum_compact_byte
    lsr nextnum_compact_byte+1
    ror nextnum_compact_byte
    lsr nextnum_compact_byte+1
    ror nextnum_compact_byte
    
    rts  ; .nextnum_to_compact

.done_clear_primes_sieve:
    rts  ; .clear_primes_sieve

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    
run_primes_eratosthenes_sieve:
    ; set primes_sieve_start .. primes_sieve_end to 0s
    jsr .clear_primes_sieve

    ; primes_count = 0
    lda #0
    sta primes_count
    sta primes_count+1
    
    ; nextnum = 2
    lda #2
    sta nextnum
    lda #0
    sta nextnum+1
    
.test_next_num:
    ; primes_ptr = primes_sieve_start + nextnum
    clc
    lda #<primes_sieve_start
    adc nextnum
    sta primes_ptr
    lda #>primes_sieve_start
    adc nextnum+1
    sta primes_ptr+1
    
    ; test for primes_sieve_end
    lda primes_ptr+1
    cmp #>primes_sieve_end
    bne *+6
    lda primes_ptr
    cmp #<primes_sieve_end
    bcs .finished
    
    ; see whether primes_ptr[nextnum] is already marked as non-prime
    ldy #0
    lda (primes_ptr),Y
    bne .try_next_num

    ; mark all multiples in primes_sieve_start..primes_sieve_end
    jsr .found_prime   
    ; show prime found?
    lda #show_primes
    cmp #2
    bcc *+5
    jsr found_prime_output

.try_next_num:
    ; nextnum++, test for finished
    inc nextnum
    bne *+4
    inc nextnum+1
    beq .finished
    ; if nextnum & 1 == 0 (even) nextnum++
    lda nextnum
    and #1
    bne *+4
    inc nextnum
    jmp .test_next_num
    
.finished:
    jsr _outstr_inline
    .byte "Eratosthenes sieve:", 10, 0
    ; show last prime found?
    lda #show_primes
    beq *+5
    jsr found_prime_output
    jsr _elapsed_cycles
    jsr _elapsed_time
    jsr __process_events
    rts ; run_primes_eratosthenes_sieve
    
.found_prime:
    ; last_prime = nextnum
    lda nextnum
    sta last_prime
    lda nextnum+1
    sta last_prime+1
    
.found_prime_next:
    ; mark (primes_ptr+nextnum) as non-prime
    ldy #0
    lda #1
    sta (primes_ptr),Y
    
    ; primes_ptr += nextnum
    clc
    lda primes_ptr
    adc nextnum
    sta primes_ptr
    lda primes_ptr+1
    adc nextnum+1
    sta primes_ptr+1
    
    ; test for primes_sieve_end
    lda primes_ptr+1
    cmp #>primes_sieve_end
    bne *+6
    lda primes_ptr
    cmp #<primes_sieve_end
    bcc .found_prime_next
    
    ; primes_count++
    inc primes_count
    bne *+4
    inc primes_count+1
    rts  ; .found_prime

.clear_primes_sieve:
    ; primes_ptr = primes_sieve_start
    lda #<primes_sieve_start
    sta primes_ptr
    lda #>primes_sieve_start
    sta primes_ptr+1

.clear_primes_sieve_next:
    ; test for primes_sieve_end
    lda primes_ptr+1
    cmp #>primes_sieve_end
    bne *+6
    lda primes_ptr
    cmp #<primes_sieve_end
    bcs .done_clear_primes_sieve
    
    ; (primes_ptr) = 0
    ldy #0
    lda #0
    sta (primes_ptr),Y
    ; primes_ptr++
    inc primes_ptr
    bne *+4
    inc primes_ptr+1
    
    jmp .clear_primes_sieve_next

.done_clear_primes_sieve:
    rts  ; .clear_primes_sieve

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

found_prime_output:
    ; print primes_count
    lda primes_count
    sta dividend
    lda primes_count+1
    sta dividend+1
    jsr _outnum
    ; print ": "
    lda #':'
    jsr __outch
    lda #' '
    jsr __outch
    ; print last_prime
    lda last_prime
    sta dividend
    lda last_prime+1
    sta dividend+1
    ; print newline
    jsr _outnum
    lda #10
    jsr __outch
    ;jsr __process_events
    rts  ; .found_prime_output
    
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.include "elapsed.asm"
.include "sqrt.asm"
.include "div.asm"
.include "mul.asm"
