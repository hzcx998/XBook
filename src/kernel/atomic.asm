;----
;file:		kernel/atomic.asm
;auther:	Jason Hu
;time:		2019/8/8
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

;!!!
;lock 锁定的时内存地址，所以操作对象值必须时内存才行
;!!!
[section .text]
[bits 32]

; void __AtomicAdd(int *a, int b);
global __AtomicAdd
__AtomicAdd:
	mov eax, [esp + 4]	; a
	mov ebx, [esp + 8]	; b
	; 通过lock前缀，让运算时原子运算
	lock add [eax], ebx	; *a += b
	ret
; void __AtomicSub(int *a, int b);
global __AtomicSub
__AtomicSub:
	mov eax, [esp + 4]	; a
	mov ebx, [esp + 8]	; b
	; 通过lock前缀，让运算时原子运算
	lock sub [eax], ebx	; *a -= b
	ret

; void __AtomicInc(int *a);
global __AtomicInc
__AtomicInc:
	mov eax, [esp + 4]	; a
	; 通过lock前缀，让运算时原子运算
	lock inc dword [eax]	; ++*a
	ret
	

; void __AtomicDec(int *a);
global __AtomicDec
__AtomicDec:
	mov eax, [esp + 4]	; a
	; 通过lock前缀，让运算时原子运算
	lock dec dword [eax]	; --*a
	ret

; void __AtomicOr(int *a, int b);
global __AtomicOr
__AtomicOr:
	mov eax, [esp + 4]	; a
	mov ebx, [esp + 8]	; b
	; 通过lock前缀，让运算时原子运算
	lock or [eax], ebx	; *a |= b
	ret

; void __AtomicAnd(int a, int b);
global __AtomicAnd
__AtomicAnd:
	mov eax, [esp + 4]	; a
	mov ebx, [esp + 8]	; b
	; 通过lock前缀，让运算时原子运算
	lock and [eax], ebx	; *a &= b
	ret
