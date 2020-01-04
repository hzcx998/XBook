;----
;file:		user/lib/task.asm
;auther:	Jason Hu
;time:		2019/12/29
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "syscall.inc"

; pid_t getpgid(pid_t pid);
global getpgid
getpgid:
	push ebx
	mov eax, SYS_GETPGID
	mov ebx, [esp + 4 + 4]
    int INT_VECTOR_SYS_CALL
	pop ebx
	ret

; int setpgid(pid_t pid, pid_t pgid);
global setpgid
setpgid:
	push ebx
    push ecx
    
	mov eax, SYS_SETPGID
	mov ebx, [esp + 8 + 4]
    mov ecx, [esp + 8 + 4 * 2]
    int INT_VECTOR_SYS_CALL
	
    pop ecx
    pop ebx
	ret