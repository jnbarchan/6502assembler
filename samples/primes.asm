; primes.asm
; calculate prime numbers

operand1 = $70
operand2 = $72
result = $74

dividend = $70
divisor = $72
quotient = $74
remainder = $76

primes_counter = $66
primes_ptr = $68

nextnum = $90
primes_count = $1000
primes = primes_count+2

max_primes = 100

main:
run_primes:
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
    ; if reached sqrt(nextnum) goto .no_more_primes
    jsr .prime_stop_dividing
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
    ; show prime found
    jsr .found_prime_output
    
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
    jsr .elapsed_time
    jsr __process_events
    rts  ; run_primes
    
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
    bcc .not_greater
    bne .greater
    lda result
    cmp nextnum
    bcc .not_greater
    bne .greater
.not_greater:
    clc
    rts  ; .prime_stop_dividing
.greater:
    sec
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
    ; primes_ptr = &primes
    lda #primes & $ff
    sta primes_ptr
    lda #primes >> 8
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
    bne *+5
    inc primes_count+1
    rts  ; .found_prime
    
.found_prime_output:
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
    ; print nextnum
    lda nextnum
    sta dividend
    lda nextnum+1
    sta dividend+1
    ; print newline
    jsr _outnum
    lda #10
    jsr __outch
    ;jsr __process_events
    rts  ; .run_primes_output
    
.elapsed_time:
    jsr __get_elapsed_time
    sta dividend
    stx dividend+1
    jsr _outnum
    jsr _outstr_inline
    .byte " ms", 0
    lda #10
    jsr __outch
    rts  ; elapsed_time

.include "outnum.asm"
.include "mul16.asm"
