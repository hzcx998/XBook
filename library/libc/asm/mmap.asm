;----
;file:		lib/mmap.asm
;auther:	Jason Hu
;time:		2019/7/29
;copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
;----

[bits 32]
[section .text]

%include "sys/syscall.inc"

global mmap

; int mmap(uint32_t addr, uint32_t len, uint32_t prot, uint32_t flags);
mmap:
	

	mov eax, SYS_MMAP
	mov ebx, [esp + 4]
	mov ecx, [esp + 8]
	mov esi, [esp + 12]
	mov edi, [esp + 16]
	int INT_VECTOR_SYS_CALL
	
	
	ret