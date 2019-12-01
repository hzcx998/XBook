;----
;file:		user/lib/exit.asm
;auther:	Jason Hu
;time:		2019/8/9
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "syscall.inc"

; void exit(int status);
global exit
exit:
	push ebx

	mov eax, SYS_EXIT
	mov ebx, [esp + 4 + 4]
	int INT_VECTOR_SYS_CALL
	
	pop ebx
	ret