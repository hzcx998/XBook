;----
;file:		arch/x86/kernel/core/entry.asm
;auther:	Jason Hu
;time:		2019/6/2
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

%include "kernel/const.inc"

[bits 32]
[section .text]

extern InitArch
extern main

global _start
;这个标签是整个内核的入口，从loader跳转到这儿
_start:
	mov byte [0x800b8000+160*4+0], 'K'
	mov byte [0x800b8000+160*4+1], 0X07
	mov byte [0x800b8000+160*4+2], 'E'
	mov byte [0x800b8000+160*4+3], 0X07
	mov byte [0x800b8000+160*4+4], 'R'
	mov byte [0x800b8000+160*4+5], 0X07
	mov byte [0x800b8000+160*4+6], 'N'
	mov byte [0x800b8000+160*4+7], 0X07
	mov byte [0x800b8000+160*4+8], 'E'
	mov byte [0x800b8000+160*4+9], 0X07
	mov byte [0x800b8000+160*4+10], 'L'
	mov byte [0x800b8000+160*4+11], 0X07
	
	;init all segment registeres
	;设置一下段
	mov ax, 0x10	;the data 
	mov ds, ax 
	mov es, ax 
	mov fs, ax 
	mov gs, ax 
	mov ss, ax 
	mov esp, KERNEL_STACK_TOP

	;初始化平台架构
	call InitArch					;into arch
	
	;jmp $
	;初始化平台成功，接下来跳入到内核中去执行
	call main

Stop:
	hlt
	jmp Stop
jmp $	
