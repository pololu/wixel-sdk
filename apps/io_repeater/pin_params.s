; This defines the params and maps each port grouping into an array for easier
; access.

    .module pin_params
    .globl _P0Links
	.globl _P1Links
	.globl _P2Links
    .area CONST   (CODE)

_P0Links:
G$param_P0_0_link$0$0 == .
    .byte 0xFF,0xFF,0xFF,0xFF ; -1
G$param_P0_1_link$0$0 == .
    .byte 0,0,0,0
G$param_P0_2_link$0$0 == .
    .byte 0,0,0,0
G$param_P0_3_link$0$0 == .
    .byte 0,0,0,0
G$param_P0_4_link$0$0 == .
    .byte 0,0,0,0
G$param_P0_5_link$0$0 == .
    .byte 0,0,0,0

_P1Links:
G$param_P1_0_link$0$0 == .
    .byte 0,0,0,0
G$param_P1_1_link$0$0 == .
    .byte 0,0,0,0
G$param_P1_2_link$0$0 == .
    .byte 0,0,0,0
G$param_P1_3_link$0$0 == .
    .byte 0,0,0,0
G$param_P1_4_link$0$0 == .
    .byte 0,0,0,0
G$param_P1_5_link$0$0 == .
    .byte 0,0,0,0
G$param_P1_6_link$0$0 == .
    .byte 0,0,0,0
G$param_P1_7_link$0$0 == .
    .byte 0,0,0,0

_P2Links:
G$_P2_0_link_RESERVED$0$0 == .
    .byte 0,0,0,0
G$param_P2_1_link$0$0 == .
    .byte 1,0,0,0
