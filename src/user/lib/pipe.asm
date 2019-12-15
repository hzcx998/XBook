;----
;file:		user/lib/pipe.asm
;auther:	Jason Hu
;time:		2019/12/13
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "syscall.inc"

global pipe
; int pipe(int fd[2]);
pipe:
	push ebx

	mov eax, SYS_PIPE
	mov ebx, [esp + 4 + 4]
	int INT_VECTOR_SYS_CALL
	
    pop ebx
    ret