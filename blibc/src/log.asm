;----
;file:		user/lib/log.asm
;auther:	Jason Hu
;time:		2019/7/29
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "syscall.inc"

global log

; void log(char *buf);
log:
	

	mov eax, SYS_LOG
	mov ebx, [esp + 4]	; buf
	int INT_VECTOR_SYS_CALL
	
	
	ret