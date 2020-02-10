/*
 * file:        arch/x86/kernel/mm/bootmem.c
 * auther:	    Jason Hu
 * time:		2019/7/6
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <kernel/ards.h>
#include <mm/bootmem.h>
#include <mm/page.h>
#include <lib/string.h>
#include <lib/math.h>

PRIVATE struct BootMem bootMemAlloctor;

/* 
 * InitBootMem - 初始化引导内存分配器
 * 
 * 初始化后就可以进行简单粗暴的内存分配了。
 */
PUBLIC void InitBootMem(unsigned int start, unsigned int end)
{
    bootMemAlloctor.startAddress = start;
    bootMemAlloctor.currentAddress = bootMemAlloctor.startAddress;
    bootMemAlloctor.topAddress = end;
}

/*
 * BootMemAlloc - 分配内存
 * @size: 分配多大的空间
 * 
 * 返回的是一个地址指针
 */
PUBLIC void *BootMemAlloc(size_t size)
{
    // 获取要分配的地址
    address_t addr = bootMemAlloctor.currentAddress;

    //修改分配地址
    bootMemAlloctor.currentAddress += size;
    //对地址进行对齐
    bootMemAlloctor.currentAddress = ALIGN_WITH(bootMemAlloctor.currentAddress, 32);
    if (bootMemAlloctor.currentAddress >= bootMemAlloctor.topAddress) {
        return NULL;
    }
    return (void *)addr;
}

/* 
 * BootMemPosition - 返回当前分配的地址
 */
PUBLIC unsigned int BootMemPosition()
{
    return bootMemAlloctor.currentAddress;
}

/*
 * BootMemSize - 获取已经分配了多大的空间
 */
PUBLIC unsigned int BootMemSize()
{
    return bootMemAlloctor.currentAddress - bootMemAlloctor.startAddress;
}
