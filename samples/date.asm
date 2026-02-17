; date.asm

.include "common.inc"

tm_sec  = 0   ; 0--59
tm_min  = 2   ; 0--59
tm_hour = 4   ; 0--23
tm_mday = 6   ; 1--31
tm_mon  = 8   ; 0--11
tm_year = 10  ; year-1900
tm_wday = 12  ; 0--6
tm_yday = 14  ; 0--365
tm_size = 16  ; sizeof(struct_tm)
struct_tm = MEM_USER_0

padlen = ZP_INTERNAL
remaining_days = ZP_INTERNAL+2
new_remaining_days = ZP_INTERNAL+4

seconds_in_day = 60 * 60 * 24

test_time_0 = 0 * seconds_in_day

time = _TIME_32

_date_test:
    lda #test_time_0 & $ff
    sta time
    lda #(test_time_0 >> 8) & $ff
    sta time+1
    lda #(test_time_0 >> 16) & $ff
    sta time+2
    lda #(test_time_0 >> 24) & $ff
    sta time+3
    
    jsr print_time_seconds
    jsr _ctime
    jsr print_asctime
    
    lda #<time
    ldx #>time
    jsr __get_time
    
    jsr print_time_seconds
    jsr _ctime
    jsr print_asctime    

    rts  ; _date_test
    brk

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

struct_tm_time0:
    .word 0, 0, 0, 1, 0, 70, 4, 0 

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_ctime:
    jsr _gmtime
    jmp _asctime  ; _ctime

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_gmtime:
    ; fill struct_tm from time
    
    ; struct_tm = struct_tm_time0
    ldx #0
.copy_time0:
    lda struct_tm_time0,X
    sta struct_tm,X
    inx
    cpx #tm_size
    bne .copy_time0
    
    ; set struct_tm[tm_sec/min/hour] and struct_tm[tm_wday] and remaining_days
    jsr .set_time_and_wday_and_remaining_days
    ; set struct_tm[tm_year] and struct_tm[tm_yday] and remaining_days
    jsr .set_year_and_yday_and_remaining_days
    ; set struct_tm[tm_mon] and remaining_days
    jsr .set_month_and_remaining_days
    ; set struct_tm[tm_mday]
    lda remaining_days
    sta struct_tm+tm_mday
    inc struct_tm+tm_mday
    
    rts  ; _gmtime

.set_month_and_remaining_days:
    ; new_remaining_days = remaining_days - days_in_month[ struct_tm[tm_mon] ]
    ldx struct_tm+tm_mon
    sec
    lda remaining_days
    sbc .days_in_month,X
    sta new_remaining_days
    lda remaining_days+1
    sbc #0
    sta new_remaining_days+1   
    ; if Feb in leap year then new_remaining_days--
    lda struct_tm+tm_year
    and #3
    bne .month_not_leap_year
    lda struct_tm+tm_mon
    cmp #2
    bne .month_not_leap_year
    lda new_remaining_days
    bne *+4
    dec new_remaining_days+1   
    dec new_remaining_days
.month_not_leap_year:
    lda new_remaining_days+1
    bmi .done_month
    
    ; remaining_days = new_remaining_days
    lda new_remaining_days
    sta remaining_days
    lda new_remaining_days+1
    sta remaining_days+1
    ; struct_tm[tm_mon]++
    inc struct_tm+tm_mon
    bne .set_month_and_remaining_days

.done_month:
    rts  ; .set_month_and_remaining_days
    
.days_in_month:
    .byte 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31

.set_year_and_yday_and_remaining_days:
    ; new_remaining_days = remaining_days - 365
    sec
    lda remaining_days
    sbc #<365
    sta new_remaining_days
    lda remaining_days+1
    sbc #>365
    sta new_remaining_days+1   
    ; if leap year then new_remaining_days--
    lda struct_tm+tm_year
    and #3
    bne .year_not_leap_year
    dec new_remaining_days
    lda new_remaining_days
    cmp #$ff
    bne *+4
    dec new_remaining_days+1   
.year_not_leap_year:
    lda new_remaining_days+1
    bmi .done_year
    
    ; remaining_days = struct_tm[tm_yday] = new_remaining_days
    lda new_remaining_days
    sta struct_tm+tm_yday
    sta remaining_days
    lda new_remaining_days+1
    sta struct_tm+tm_yday+1
    sta remaining_days+1
    ; struct_tm[tm_year]++
    inc struct_tm+tm_year
    bne .set_year_and_yday_and_remaining_days

.done_year:
    rts  ; .set_year_and_yday_and_remaining_days

.set_time_and_wday_and_remaining_days:
    ; dividend_32 = time
    jsr copy_time_to_dividend_32
    ; dividend_32 /%= 60 seconds
    lda #60
    sta divisor_32
    lda #0
    sta divisor_32+1
    sta divisor_32+2
    sta divisor_32+3
    jsr _div32
    ; struct_tm[tm_sec] = LOBYTE(remainder_32)
    lda remainder_32
    sta struct_tm+tm_sec

    ; dividend_32 = quotient_32
    jsr copy_quotient_32_to_dividend_32
    ; dividend_32 /%= 60 minutes
    jsr _div32
    ; struct_tm[tm_min] = LOBYTE(remainder_32)
    lda remainder_32
    sta struct_tm+tm_min
    
    ; dividend_32 = quotient_32
    jsr copy_quotient_32_to_dividend_32
    ; dividend_32 /%= 24 hours
    lda #24
    sta divisor_32
    jsr _div32
    ; struct_tm[tm_hour] = LOBYTE(remainder_32)
    lda remainder_32
    sta struct_tm+tm_hour
    
    ; assert quotient_32 < $10000
    lda quotient_32+2
    ora quotient_32+3
    eor #$ff
    jsr _assert
    
    ; dividend = remaining_days = LOWORD(quotient_32)
    lda quotient_32
    sta remaining_days
    sta dividend
    lda quotient_32+1
    sta remaining_days+1
    sta dividend+1
    
    ; dividend /%= 7
    lda #7
    sta divisor
    lda #0
    sta divisor+1
    jsr _div16
    
    ; struct_tm[tm_wday] = (remainder + struct_tm_time0[tm_wday]) % 7
    clc
    lda remainder
    adc struct_tm_time0+tm_wday
    cmp #7
    bcc *+4
    sbc #7
    sta struct_tm+tm_wday
    
    rts  ; .set_time_and_wday_and_remaining_days

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

_asctime:
    ; store string at PAD
    
    ; weekday string
    ldy #0
    lda struct_tm+tm_wday
    asl A
    asl A
    tax
.wday_loop:
    lda .weekday_names,X
    beq .done_wday_loop
    sta PAD,Y
    iny
    inx
    bne .wday_loop
.done_wday_loop:
    jsr .space_to_pad
    
    ; day number
    lda struct_tm+tm_mday
    sta dividend
    lda #0
    sta dividend+1
    jsr _padnum
    jsr .copy_padnum_to_pad
    jsr .space_to_pad
    
    ; month string
    lda struct_tm+tm_mon
    asl A
    asl A
    tax
.mon_loop:
    lda .month_names,X
    beq .done_mon_loop
    sta PAD,Y
    iny
    inx
    bne .mon_loop
.done_mon_loop:
    jsr .space_to_pad
    
    ; hour number
    lda #0
    sta dividend+1
    lda struct_tm+tm_hour
    sta dividend
    lda #2
    jsr _padnum_min_width
    jsr .copy_padnum_to_pad
    jsr .colon_to_pad
    ; minute number
    lda struct_tm+tm_min
    sta dividend
    lda #2
    jsr _padnum_min_width
    jsr .copy_padnum_to_pad
    jsr .colon_to_pad
    ; second number
    lda struct_tm+tm_sec
    sta dividend
    lda #2
    jsr _padnum_min_width
    jsr .copy_padnum_to_pad
    jsr .space_to_pad
    
    ; "GMT "
    ldx #0
.gmt_loop:
    lda .gmt,X
    beq .done_gmt_loop
    sta PAD,Y
    iny
    sty padlen
    inx
    bne .gmt_loop
.done_gmt_loop:
    
    ; year number
    clc
    lda struct_tm+tm_year
    adc #<1900
    sta dividend
    lda struct_tm+tm_year+1
    adc #>1900
    sta dividend+1
    jsr _padnum
    jsr .copy_padnum_to_pad
    
    lda #0
    sta PAD,Y
    rts  ; _asctime

.colon_to_pad:
    lda #':'
    bne *+4
.space_to_pad:
    lda #' '
    sta PAD,Y
    iny
    sty padlen
    rts  ; .space_to_pad

.copy_padnum_to_pad:
    ldy padlen
.copy_padnum_loop:
    lda PAD,X
    beq .done_padnum_loop
    sta PAD,Y
    iny
    inx
    bne .copy_padnum_loop
.done_padnum_loop:
    sty padlen
    rts  ; .copy_padnum_to_pad
    
.weekday_names:
    .byte "Sun", 0, "Mon", 0, "Tue", 0, "Wed", 0, "Thu", 0, "Fri", 0, "Sat", 0

.month_names:
    .byte "Jan", 0, "Feb", 0, "Mar", 0, "Apr", 0, "May", 0, "Jun", 0
    .byte "Jul", 0, "Aug", 0, "Sep", 0, "Oct", 0, "Nov", 0, "Dec", 0
    
.gmt:
    .byte "GMT ", 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

print_asctime:
    jsr _outstr
    lda #10
    jmp __outch ; print_asctime

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

print_time_seconds:
    jsr copy_time_to_dividend_32
    jsr __outstr_inline
    .byte "Time (seconds): ", 0
    jsr _outnum32_commas
    lda #10
    jsr __outch
    
    rts  ; print_time_seconds

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

copy_quotient_32_to_dividend_32:
    ; quotient_32 = dividend_32
    lda quotient_32
    sta dividend_32
    lda quotient_32+1
    sta dividend_32+1
    lda quotient_32+2
    sta dividend_32+2
    lda quotient_32+3
    sta dividend_32+3
    rts  ; copy_quotient_32_to_dividend_32

copy_time_to_dividend_32:
    ; dividend_32 = time
    lda time
    sta dividend_32
    lda time+1
    sta dividend_32+1
    lda time+2
    sta dividend_32+2
    lda time+3
    sta dividend_32+3
    rts  ; copy_time_to_dividend_32

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    

.include "assert.asm"
.include "outnum.asm"
