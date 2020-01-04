;----
;file:		user/lib/exit.asm
;auther:	Jason Hu
;time:		2019/8/9
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "syscall.inc"

; int _wait(int *status);
global _wait
_wait:
	push ebx

	mov eax, SYS_WAIT
	mov ebx, [esp + 4 + 4]
	int INT_VECTOR_SYS_CALL
	
	pop ebx
	ret