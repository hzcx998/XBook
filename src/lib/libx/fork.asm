;----
;file:		lib/fork.asm
;auther:	Jason Hu
;time:		2019/7/29
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "book/syscall.inc"

global fork

; int fork();
fork:
	;

	mov eax, SYS_FORK
	int INT_VECTOR_SYS_CALL
	
	;
	ret