; primes.asm
; calculate prime numbers

dividend = $70
divisor = $72
quotient = $74
remainder = $76

primes_counter = $66
primes_ptr = $68

nextnum = $90
primes_count = $1000
primes = primes_count+2

main:
run_primes:
    lda #0
    sta primes_count
    sta primes_count+1
    
    lda #1
    sta nextnum
    lda #0
    sta nextnum+1
    
.try_next_num:
    inc nextnum
    bne *+9
    inc nextnum+1
    bne *+5
    jmp .finished
    
    lda #0
    sta primes_counter
    sta primes_counter+1
    lda #primes & $ff
    sta primes_ptr
    lda #primes >> 8
    sta primes_ptr+1
.next_primes_count:
    lda primes_counter+1
    cmp primes_count+1
    bne .more_primes
    lda primes_counter
    cmp primes_count
    beq .no_more_primes
    
.more_primes:
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
    beq .try_next_num
    
    inc primes_counter
    bne *+4
    inc primes_counter+1
    inc primes_ptr
    bne *+4
    inc primes_ptr+1
    inc primes_ptr
    bne *+4
    inc primes_ptr+1
    jmp .next_primes_count
    
.no_more_primes:
    lda #primes & $ff
    sta primes_ptr
    lda #primes >> 8
    sta primes_ptr+1

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
    
    ldy #0
    lda nextnum
    sta (primes_ptr),Y
    iny
    lda nextnum+1
    sta (primes_ptr),Y

    inc primes_count
    bne *+5
    inc primes_count+1
    
    lda primes_count
    sta dividend
    lda primes_count+1
    sta dividend+1
    jsr _outnum
    lda #':'
    jsr __outch
    lda #' '
    jsr __outch
    lda nextnum
    sta dividend
    lda nextnum+1
    sta dividend+1
    jsr _outnum
    lda #10
    jsr __outch
    jsr __process_events
;.break
    
    lda primes_count+1
    cmp #1000 >> 8
    bne .try_next_num2
    lda primes_count
    cmp #1000 & $ff
    beq .finished
    
.try_next_num2:
    jmp .try_next_num

.finished:
    jsr __process_events
    rts  ; run_primes

.include "outnum.asm"

