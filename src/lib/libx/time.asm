;----
;file:		lib/time.asm
;auther:	Jason Hu
;time:		2019/12/27
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

; 所有与时间相关的库函数都应该放在这里

[bits 32]
[section .text]

%include "book/syscall.inc"

global alarm
; unsigned int  alarm(unsigned int second);
alarm:
	push ebx

	mov eax, SYS_ALARM
	mov ebx, [esp + 4 + 4]
	int INT_VECTOR_SYS_CALL
	
	pop ebx
	ret

global graph
; void graph(int offset, int size, void *buffer);
graph:
	push ebx
    push ecx
    push esi
    
	mov eax, SYS_GRAPHW
	mov ebx, [esp + 12 + 4]
	mov ecx, [esp + 12 + 8]
	mov esi, [esp + 12 + 12]
    int INT_VECTOR_SYS_CALL
	
	pop esi
	pop ecx
	pop ebx
	ret
