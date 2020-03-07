;----
;file:		lib/signal.asm
;auther:	Jason Hu
;time:		2019/12/23
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "sys/syscall.inc"

global signal

; int signal(int signal, sighandler_t handler);
signal:
	push ebx
    push ecx
    
	mov eax, SYS_SIGNAL
	mov ebx, [esp + 8 + 4]
    mov ecx, [esp + 8 + 4 * 2]
	int INT_VECTOR_SYS_CALL
	
	pop ecx
    pop ebx
	ret

global sigprocmask
; int sigprocmask(int how, sigset_t *set, sigset_t *oldset);
sigprocmask:
	push ebx
    push ecx
    push esi
    
	mov eax, SYS_SIGPROCMASK
	mov ebx, [esp + 12 + 4]
    mov ecx, [esp + 12 + 4 * 2]
    mov esi, [esp + 12 + 4 * 3]
	int INT_VECTOR_SYS_CALL
	
	pop esi
	pop ecx
    pop ebx
	ret

global sigpending
; int sigpending(sigset_t *set);
sigpending:
	push ebx
    
	mov eax, SYS_SIGPENDING
	mov ebx, [esp + 4 + 4]
    int INT_VECTOR_SYS_CALL
	
    pop ebx
	ret

global sigaction
; int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
sigaction:
	push ebx
    push ecx
    push esi
    
	mov eax, SYS_SIGACTION
	mov ebx, [esp + 12 + 4]
    mov ecx, [esp + 12 + 4 * 2]
    mov esi, [esp + 12 + 4 * 3]
	int INT_VECTOR_SYS_CALL
	
	pop esi
	pop ecx
    pop ebx
	ret

global kill
; int kill(pid_t pid, int signal);
kill:
	push ebx
    push ecx
    
	mov eax, SYS_KILL
	mov ebx, [esp + 8 + 4]
    mov ecx, [esp + 8 + 4 * 2]
	int INT_VECTOR_SYS_CALL
	
	pop ecx
    pop ebx
	ret

global sigpause
; int sigpause();
sigpause:
	mov eax, SYS_SIGPAUSE
	int INT_VECTOR_SYS_CALL
	ret

global sigsuspendhalf
; int sigsuspendhalf(sigset_t *set, sigset_t *oldSet);
sigsuspendhalf:
	push ebx
    push ecx
    
	mov eax, SYS_SIGSUSPEND
	mov ebx, [esp + 8 + 4]
    mov ecx, [esp + 8 + 4 * 2]
	int INT_VECTOR_SYS_CALL
	
	pop ecx
    pop ebx
	ret