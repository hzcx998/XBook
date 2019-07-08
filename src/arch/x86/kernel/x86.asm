;----
;file:		/x86/kernel/x86.asm
;auther: 	Jason Hu
;time: 		2019/6/2
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

%include "const.inc"

global 	In8
global 	Out8
global 	In16
global 	Out16
global 	In32
global 	Out32
global	DisableInterrupt
global 	EnableInterrupt
global 	CpuHlt
global 	LoadTR
global	ReadCR2
global	ReadCR3
global	WriteCR3
global	ReadCR0
global	WriteCR0
global	StoreGDTR
global	LoadGDTR
global	StoreIDTR
global 	LoadIDTR
global 	LoadEflags
global 	StoreEflags
global 	PortRead
global 	PortWrite
global 	x86CpuUD2
global 	X86Invlpg
global 	X86Cpuid

[section .text]
[bits 32]

In8:	;uint32_t In8(uint32_t port);
	mov		edx,[esp+4]
	xor		eax,eax
	in		al,dx
	ret

In16:	;uint32_t In16(uint32_t port);
	mov		edx,[esp+4]
	xor		eax,eax
	in		ax,dx
	ret
In32:	;uint32_t In32(uint32_t port);
	mov		edx,[esp+4]
	in		eax,dx
	ret

Out8:	; void Out8(uint32_t port, uint32_t data);
	mov		edx,[esp+4]
	mov		al,[esp+8]
	out		dx,al
	ret

Out16:	; void Out16(uint32_t port, uint32_t data);
	mov		edx,[esp+4]
	mov		ax,[esp+8]
	out		dx,ax
	ret	

Out32:	; void Out32(uint32_t port, uint32_t data);
	mov		edx,[esp+4]
	mov		eax,[esp+8]
	out		dx,eax
	ret	
DisableInterrupt:	; void DisableInterrupt(void);
	cli
	ret

EnableInterrupt:	; void EnableInterrupt(void);
	sti
	ret
	
CpuHlt: ;void CpuHlt(void);
	hlt
	ret
LoadTR:		; void LoadTR(uint32_t tr);
	ltr	[esp+4]			; tr
	ret
	
ReadCR2:
	mov eax,cr2
	ret
ReadCR3:
	mov eax,cr3
	ret

WriteCR3:
	mov eax,[esp+4]
	mov cr3,eax
	ret
ReadCR0:
	mov eax,cr0
	ret

WriteCR0:
	mov eax,[esp+4]
	mov cr0,eax
	ret	
	
StoreGDTR:
	mov eax, [esp + 4]
	sgdt [eax]
	ret

LoadGDTR:	;void LoadGDTR(uint32_t limit, uint32_t addr);
	mov ax, [esp + 4]
	mov	[esp+6],ax		
	lgdt [esp+6]
	
	jmp dword 0x08: .l
	
.l:
	mov ax, 0x10
	mov ds, ax 
	mov es, ax 
	mov fs, ax 
	mov ss, ax 
	mov gs, ax 
	ret

StoreIDTR:
	mov eax, [esp + 4]
	sidt [eax]
	ret

LoadIDTR:	;void LoadIDTR(uint32_t limit, uint32_t addr);
	mov		ax,[esp+4]
	mov		[esp+6],ax
	lidt	[esp+6]
	ret

LoadEflags:	; uint32_t LoadEflags(void);
	pushf		; PUSH EFLAGS
	pop		eax
	ret

StoreEflags:	; void StoreEflags(uint32_t eflags);
	mov		eax,[ESP+4]
	push	eax
	popfd		; POP EFLAGS
	ret

PortRead:	;void PortRead(u16 port, void* buf, uint32_t n);
	mov	edx, [esp + 4]		; port
	mov	edi, [esp + 4 + 4]	; buf
	mov	ecx, [esp + 4 + 4 + 4]	; n
	shr	ecx, 1		;ecx/2
	cld
	rep	insw
	ret

PortWrite:	;void PortWrite(u16 port, void* buf, uint32_t n);
	mov	edx, [esp + 4]		; port
	mov	esi, [esp + 4 + 4]	; buf
	mov	ecx, [esp + 4 + 4 + 4]	; n
	shr	ecx, 1	;ecx/2
	cld
	rep	outsw
	ret

x86CpuUD2:
	ud2
	ret
X86Invlpg:
	mov eax, [esp + 4]
	invlpg [eax]
	ret	

X86Cpuid: 	; void X86Cpuid(unsigned int id_eax, unsigned int *eax, 
			; 		unsigned int *ebx, unsigned int *ecx, unsigned int *edx);
	mov eax, [esp + 4] ; eax_id
	cpuid
	mov edi, [esp + 8]	;eax
	mov [edi], eax
	mov edi, [esp + 12]	;ebx
	mov [edi], ebx
	mov edi, [esp + 16]	;ecx
	mov [edi], ecx
	mov edi, [esp + 20]	;edx
	mov [edi], edx
	ret



