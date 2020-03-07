;----
;file:		lib/execv.asm
;auther:	Jason Hu
;time:		2019/7/29
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "sys/syscall.inc"

global execv

; int execv(const char *path, const char *argv[]);
execv:
	push ebx
	push ecx
	
	mov eax, SYS_EXECV
	mov ebx, [esp + 8 + 4]
	mov ecx, [esp + 8 + 8]
	int INT_VECTOR_SYS_CALL
	
	pop ecx
	pop ebx
	
	ret