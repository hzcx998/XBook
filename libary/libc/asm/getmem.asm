;----
;file:		lib/getmem.asm
;auther:	Jason Hu
;time:		2020/2/28
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "sys/syscall.inc"

global getmem

; void getmem(meminfo_t *mi);
getmem:
	push ebx
    mov eax, SYS_GETMEM
	mov ebx, [esp + 4 + 4]
	int INT_VECTOR_SYS_CALL
	pop ebx
	ret
