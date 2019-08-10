/*
 * file:		arch/x86/include/mm/bootmem.h
 * auther:		Jason Hu
 * time:		2019/7/6
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _X86_MM_BOOTMEM_H
#define _X86_MM_BOOTMEM_H

#include <share/types.h>
#include <zone.h>

/*
静态内存分配器的起始位置为静态内存的起始位置
*/
#define BOOTMEM_START_ADDR  ZONE_VIR_STATIC_ADDR

/* 
bootmem是用于初始化内核内存空间以及其数据结构的一个简单
分配器。在这里，我们尽量做得简单。
*/

struct BootMem 
{
    address_t startAddress;     // 记录从哪个地方开始进行分配
    address_t currentAddress;   // 当前分配到的位置的地址
};

PUBLIC void InitBootMem();
PUBLIC void *BootMemAlloc(size_t size);
PUBLIC unsigned int BootMemPosition();
PUBLIC unsigned int BootMemSize();
#endif  /*_X86_MM_BOOTMEM_H*/
