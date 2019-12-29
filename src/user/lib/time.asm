;----
;file:		user/lib/time.asm
;auther:	Jason Hu
;time:		2019/12/27
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

; 所有与时间相关的库函数都应该放在这里

[bits 32]
[section .text]

%include "syscall.inc"

global alarm
; unsigned int  alarm(unsigned int second);
alarm:
	push ebx

	mov eax, SYS_ALARM
	mov ebx, [esp + 4 + 4]
	int INT_VECTOR_SYS_CALL
	
	pop ebx
	ret