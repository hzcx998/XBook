;----
;file:		liba/getver.asm
;auther:	Jason Hu
;time:		2020/3/2
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "sys/syscall.inc"

; void getver(char *buf, int buflen);
global getver
getver:
	push ebx
    push ecx
    
	mov eax, SYS_GETVER
	mov ebx, [esp + 8 + 4]
	mov ecx, [esp + 8 + 8]
	int INT_VECTOR_SYS_CALL

	pop ecx
	pop ebx
	ret