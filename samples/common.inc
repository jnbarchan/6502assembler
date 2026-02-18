; common.inc
; common definitions to be included by all .asm files
; put only label definiitons/assignments here, no code

ZP_MATH_A    = $20    ; 16-bit operand A
ZP_MATH_B    = $22    ; 16-bit operand B
ZP_MATH_RES  = $24    ; 16-bit result
ZP_MATH_RES2 = $26    ; 16-bit extra result
ZP_MATH_TMP  = $28    ; scratch (if needed)

ZP_MATH_32_A    = $20    ; 32-bit operand A
ZP_MATH_32_B    = $24    ; 32-bit operand B
ZP_MATH_32_RES  = $28    ; 32-bit result
ZP_MATH_32_RES2 = $2c    ; 32-bit extra result

dividend  = ZP_MATH_A
divisor   = ZP_MATH_B
quotient  = ZP_MATH_RES
remainder = ZP_MATH_RES2

dividend_32  = ZP_MATH_32_A
divisor_32   = ZP_MATH_32_B
quotient_32  = ZP_MATH_32_RES
remainder_32 = ZP_MATH_32_RES2

operand1 = ZP_MATH_A
operand2 = ZP_MATH_B
result   = ZP_MATH_RES

operand1_32 = ZP_MATH_32_A
operand2_32 = ZP_MATH_32_B
result_32   = ZP_MATH_32_RES

dest  = operand1
src   = operand2
count = result

ZP_A_SAVE = $40
ZP_X_SAVE = $41
ZP_Y_SAVE = $42
ZP_TMP    = $44

ZP_INTERNAL = $60
ZP_USER_0   = $70
ZP_USER_1   = $80

PAD      = $0400
PAD_SIZE = $0100

_RND8_STATE        = $0500
_RND16_STATE_LOW   = $0502
_RND16_STATE_HIGH  = $0503
_RND16_STATE_HIGH2 = $0504
_RND16_STATE_HIGH3 = $0505

_PADNUM_MIN_WIDTH       = $0506
_PADNUM_SEPARATOR       = $0507
_PADNUM_SEPARATOR_COUNT = $0508

_TIME_32 = $050a

_SQRT_OPERAND_32 = $0510
_SQRT_RESULT_32  = $0514
_SQRT_LOWER      = $0518
_SQRT_MID        = $051a
_SQRT_UPPER      = $052c


MEM_USER_0 = $1000

