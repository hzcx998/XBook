/*
 * file:		kernel/mm/phymem.c
 * auther:	    Jason Hu
 * time:		2019/9/29
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <config.h>
#include <book/debug.h>
#include <page.h>
#include <ioremap.h>
#include <book/vmarea.h>

PUBLIC int ArchIoRemap(unsigned long phyAddr, unsigned long virAddr, size_t size)
{
    unsigned int endAddr = virAddr + size;
    while (virAddr < endAddr) {
        /* 添加页面 */
        if (PageTableAdd(virAddr, phyAddr, PAGE_RW_W | PAGE_US_S)) {
            printk("io remap page add failed!\n");
            return -1;
        }
	
        virAddr += PAGE_SIZE;
        phyAddr += PAGE_SIZE;
    }
    return 0;
}

PUBLIC int ArchIoUnmap(unsigned int addr, size_t size)
{
    unsigned int endAddr = addr + size;
    
    /* 取消虚拟地址的内存映射 */
    while (addr < endAddr) {
        if (RemoveFromPageTable(addr) == -1) {
            return -1;
        }
        addr += PAGE_SIZE;
    }
    return 0;
}