;----
;file:		lib/time.asm
;auther:	Jason Hu
;time:		2019/12/27
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

; 所有与时间相关的库函数都应该放在这里

[bits 32]
[section .text]

%include "sys/syscall.inc"

global kgcmsg
; int kgcmsg(int opereate, KGC_Message_t *msg);
kgcmsg:
	push ebx
    push ecx
    
	mov eax, SYS_KGCMSG
	mov ebx, [esp + 8 + 4]
	mov ecx, [esp + 8 + 8]
	int INT_VECTOR_SYS_CALL

	pop ecx
	pop ebx
	ret
