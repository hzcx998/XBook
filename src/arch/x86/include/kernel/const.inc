;----
;file:      arch/x86/include/kernel/const.inc
;auther:    Jason Hu
;time:      2019/6/2
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

KERNEL_STACK_TOP EQU 0x8009fc00

EOI EQU 0X20
INT_M_CTL	equ	0x20	; I/O port for interrupt controller         <Master>
INT_M_CTLMASK	equ	0x21	; setting bits in this port disables ints   <Master>
INT_S_CTL	equ	0xA0	; I/O port for second interrupt controller  <Slave>
INT_S_CTLMASK	equ	0xA1	; setting bits in this port disables ints   <Slave>
