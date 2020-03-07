;----
;file:		lib/pipe.asm
;auther:	Jason Hu
;time:		2019/12/13
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "sys/syscall.inc"

global pipe
; int pipe(int fd[2]);
pipe:
	push ebx

	mov eax, SYS_PIPE
	mov ebx, [esp + 4 + 4]
	int INT_VECTOR_SYS_CALL
	
    pop ebx
    ret


global mkfifo
; int mkfifo(const char *path, mode_t mode);
mkfifo:
	push ebx
    push ecx

	mov eax, SYS_FIFO
	mov ebx, [esp + 8 + 4]
    mov ecx, [esp + 8 + 8]
	int INT_VECTOR_SYS_CALL
    
	pop ecx
    pop ebx
    ret