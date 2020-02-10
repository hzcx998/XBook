/*
 * file:		arch/x86/include/kernel/x86.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _X86_H
#define _X86_H

#include <lib/stdint.h>
#include <lib/types.h>

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

/* x86特性 */
#define X86_FEATURE_XMM2 (0*32+26) /* Streaming SIMD Extensions-2 */

/*
* Alternative instructions for different CPU types or capabilities.
*
* This allows to use optimized instructions even on generic binary
* kernels.
*
* length of oldinstr must be longer or equal the length of newinstr
* It can be padded with nops as needed.
*
* For non barrier like inlines please define new variants
* without volatile and memory clobber.
*/
#define Alternative(oldinstr, newinstr, feature) \
asm volatile ("661:\n\t" oldinstr "\n662:\n" \
      ".section .altinstructions,\"a\"\n" \
      "   .align 4\n" \
      "   .long 661b\n"          /* label */ \
      "   .long 663f\n"    /* new instruction */ \
      "   .byte %c0\n"          /* feature bit */ \
      "   .byte 662b-661b\n"    /* sourcelen */ \
      "   .byte 664f-663f\n"    /* replacementlen */ \
      ".previous\n" \
      ".section .altinstr_replacement,\"ax\"\n" \
      "663:\n\t" newinstr "\n664:\n" /* replacement */\
      ".previous" :: "i" (feature) : "memory")


#define MemoryBarrier() Alternative("lock; addl $0,0(%%esp)", "mfence", X86_FEATURE_XMM2)
#define ReadMemoryBarrier() Alternative("lock; addl $0,0(%%esp)", "lfence", X86_FEATURE_XMM2)

#ifdef CONFIG_X86_OOSTORE
/* Actually there are no OOO store capable CPUs for now that do SSE, 
but make it already an possibility. */
#define WriteMemoryBarrier() Alternative("lock; addl $0,0(%%esp)", "sfence", X86_FEATURE_XMM)
#else
#define WriteMemoryBarrier() __asm__ __volatile__ ("": : :"memory")
#endif

/* The "volatile" is due to gcc bugs */
#define Barrier() __asm__ __volatile__("": : :"memory")

#endif	/*_X86_H*/