;----
;file:		lib/sleep.asm
;auther:	Jason Hu
;time:		2019/8/8
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "sys/syscall.inc"

global sleep

; unsigned int sleep(unsigned int second);
sleep:
	push ebx

	mov eax, SYS_SLEEP
	mov ebx, [esp + 4 + 4]
	int INT_VECTOR_SYS_CALL
	
	pop ebx
	ret