;----
;file:		lib/getpid.asm
;auther:	Jason Hu
;time:		2019/7/29
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "sys/syscall.inc"


; uint32_t getpid();
global getpid
getpid:
	

	mov eax, SYS_GETPID
	int INT_VECTOR_SYS_CALL
	
	
	ret