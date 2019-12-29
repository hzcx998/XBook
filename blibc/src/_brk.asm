;----
;file:		user/lib/brk.asm
;auther:	Jason Hu
;time:		2019/8/10
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "syscall.inc"

; int _brk(void * addr);
global _brk
_brk:
	

	mov eax, SYS_BRK
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	
	
	ret