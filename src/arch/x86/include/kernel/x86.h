/*
 * file:		arch/x86/include/kernel/x86.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _ARCH_X86_H
#define _ARCH_X86_H

#include <share/stdint.h>

uint32_t In8(uint32_t port);
uint32_t In16(uint32_t port);
uint32_t In32(uint32_t port);
void Out8(uint32_t port, uint32_t data);
void Out16(uint32_t port, uint32_t data);
void Out32(uint32_t port, uint32_t data);
void DisableInterrupt(void);
void EnableInterrupt(void);
void CpuHlt(void);
void LoadTR(uint32_t tr);
int32_t ReadCR0(void );
int32_t ReadCR3(void );
uint32_t ReadCR2(void );

void WriteCR0(uint32_t address);
void WriteCR3(uint32_t address);
void StoreGDTR(uint32_t gdtr);
void LoadGDTR(uint32_t limit, uint32_t addr);
void StoreIDTR(uint32_t idtr);
void LoadIDTR(uint32_t limit, uint32_t addr);
uint32_t LoadEflags(void);
void StoreEflags(uint32_t eflags);
void PortRead(uint16_t port, void* buf, uint32_t n);
void PortWrite(uint16_t port, void* buf, uint32_t n);
void x86CpuUD2();
void X86Invlpg(uint32_t vaddr);
void X86Cpuid(unsigned int id_eax, unsigned int *eax, \
        unsigned int *ebx, unsigned int *ecx, unsigned int *edx);


#endif	/*_ARCH_X86_H*/