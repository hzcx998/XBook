;----
;file:		user/lib/sleep.asm
;auther:	Jason Hu
;time:		2019/8/8
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "syscall.inc"

global sleep

; void sleep(int second);
sleep:
	

	mov eax, SYS_SLEEP
	mov ebx, [esp + 4]
	int INT_VECTOR_SYS_CALL
	
	
	ret