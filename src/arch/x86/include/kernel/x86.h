/*
 * file:		arch/x86/include/kernel/x86.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _ARCH_X86_H
#define _ARCH_X86_H

#include <share/stdint.h>
#include <share/types.h>

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

void CpuNop();

char Xchg8(char *ptr, char value);
short Xchg16(short *ptr, short value);
int Xchg32(int *ptr, int value);

#define XCHG(ptr,v) ((__typeof__(*(ptr)))__Xchg((unsigned int) \
        (v),(ptr),sizeof(*(ptr))))

/**
 * __Xchg: 交换一个内存地址和一个数值的值
 * @x: 数值
 * @ptr: 内存指针
 * @size: 地址值的字节大小
 * 
 * 返回交换前地址中的值
 */
STATIC INLINE unsigned int __Xchg(unsigned int x, 
        volatile void * ptr, int size)
{
    int old;
    switch (size) {
        case 1:
            old = Xchg8((char *)ptr, x);
            break;
        case 2:
            old = Xchg16((short *)ptr, x);
            break;
        case 4:
            old = Xchg32((int *)ptr, x);
            break;
    }
    return old;
}

#endif	/*_ARCH_X86_H*/