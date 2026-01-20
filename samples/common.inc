; common.inc
; common definitions to be included by all .asm files
; put only label definiitons/assignments here, no code

ZP_MATH_A      = $20    ; 16-bit operand A
ZP_MATH_B      = $22    ; 16-bit operand B
ZP_MATH_RES    = $24    ; 16-bit result
ZP_MATH_RES2   = $26    ; 16-bit extra result
ZP_MATH_TMP    = $28    ; scratch (if needed)

dividend  = ZP_MATH_A
divisor   = ZP_MATH_B
quotient  = ZP_MATH_RES
remainder = ZP_MATH_RES2

operand1 = ZP_MATH_A
operand2 = ZP_MATH_B
result   = ZP_MATH_RES

ZP_A_SAVE      = $30
ZP_X_SAVE      = $31
ZP_Y_SAVE      = $32
ZP_TMP         = $34

ZP_USER_0 = $70
ZP_USER_1 = $80

PAD      = $0400
PAD_SIZE = $0100

_RND8_STATE       = $0500
_RND16_STATE_LOW  = $0502
_RND16_STATE_HIGH = $0503

MEM_USER_0 = $1000

