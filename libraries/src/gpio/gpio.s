	.module gpio
	.optsdcc -mmcs51 --model-medium
	.globl _setDigitalOutput
	;.area RSEG    (ABS,DATA)
	;.org 0x0000
_P0	=	0x0080
_P0IFG	=	0x0089
_P1IFG	=	0x008a
_P2IFG	=	0x008b
_PICTL	=	0x008c
_P0INP	=	0x008f
_P1	=	0x0090
_P2	=	0x00a0
_P0SEL	=	0x00f3
_P1SEL	=	0x00f4
_P2SEL	=	0x00f5
_P1INP	=	0x00f6
_P2INP	=	0x00f7
_P0DIR	=	0x00fd
_P1DIR	=	0x00fe
_P2DIR	=	0x00ff

	.area BIT_BANK	(REL,OVR,DATA)
bits:
	.ds 1
	;b0 = bits[0]
	
	.area CSEG    (CODE)
; void setDigitalOutput(uint8 pin, BIT value) __reentrant
; Assumption: The caller has placed the pin argument in dpl,
; and the value argument in bits[0]
_ret:
	ret
_setDigitalOutput:
	mov	a,dpl
	mov dptr, #_setDigitalOutputTable
	
	add a,#0xE7
	jc _ret
	add a,#0x19
	mov	c,bits[0]
	jmp @a+dptr

_setDigitalOutputTable:
	mov P0_0, c
	orl _P0DIR, #0x01	
	ret
	nop
	nop
	mov P0_1, c
	orl _P0DIR, #0x02	
	ret
	nop
	nop
	mov P0_2, c
	orl _P0DIR, #0x04	
	ret
	nop
	nop
	mov P0_3, c
	orl _P0DIR, #0x08	
	ret
	nop
	nop
	mov P0_4, c
	orl _P0DIR, #0x10	
	ret
	nop
	nop
	mov P0_5, c
	orl _P0DIR, #0x20	
	ret
	nop
	nop
	