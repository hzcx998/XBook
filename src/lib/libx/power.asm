;----
;file:		liba/reboot.asm
;auther:	Jason Hu
;time:		2020/3/2
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "book/syscall.inc"

global reboot
; int reboot();
reboot:
	mov eax, SYS_REBOOT
	int INT_VECTOR_SYS_CALL
	ret
