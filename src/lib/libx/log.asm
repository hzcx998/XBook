;----
;file:		lib/log.asm
;auther:	Jason Hu
;time:		2019/7/29
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "book/syscall.inc"

global log

; void log(char *buf);
log:
	push ebx

	mov eax, SYS_LOG
	mov ebx, [esp + 4 + 4]	; buf
	int INT_VECTOR_SYS_CALL
	
	pop ebx
	ret