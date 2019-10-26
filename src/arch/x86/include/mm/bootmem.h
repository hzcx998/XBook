/*
 * file:		arch/x86/include/mm/bootmem.h
 * auther:		Jason Hu
 * time:		2019/7/6
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _X86_MM_BOOTMEM_H
#define _X86_MM_BOOTMEM_H

#include <share/types.h>

/* 
bootmem是用于初始化内核内存空间以及其数据结构的一个简单
分配器。在这里，我们尽量做得简单。
*/

struct BootMem 
{
    address_t startAddress;     // 记录从哪个地方开始进行分配
    address_t currentAddress;   // 当前分配到的位置的地址
    unsigned int topAddress;         // 引导内存的界限
};

PUBLIC void InitBootMem(unsigned int start, unsigned int limit);
PUBLIC void *BootMemAlloc(size_t size);
PUBLIC unsigned int BootMemPosition();
PUBLIC unsigned int BootMemSize();

#endif  /*_X86_MM_BOOTMEM_H*/
