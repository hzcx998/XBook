/*
 * file:        kernel/mm/bootmem.c
 * auther:	    Jason Hu
 * time:		2019/7/6
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/debug.h>
#include <zone.h>
#include <page.h>
#include <ards.h>
#include <bootmem.h>
#include <share/string.h>
#include <share/math.h>

PRIVATE struct BootMem bootMemAlloctor;

/* 
 * InitBootMem - 初始化引导内存分配器
 * 
 * 初始化后就可以进行简单粗暴的内存分配了。
 */
PUBLIC void InitBootMem()
{
    bootMemAlloctor.startAddress = BOOTMEM_START_ADDR;
    bootMemAlloctor.currentAddress = bootMemAlloctor.startAddress;
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
