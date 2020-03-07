;----
;file:		lib/msleep.asm
;auther:	Jason Hu
;time:		2019/7/29
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "sys/syscall.inc"

global msleep

; void msleep(int msecond);
msleep:
	push ebx

	mov eax, SYS_MSLEEP
	mov ebx, [esp + 4 + 4]
	int INT_VECTOR_SYS_CALL
	
	pop ebx
	ret