/*
 * file:		mm/mmu.c
 * auther:		Jason Hu
 * time:		2019/7/18
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/mmu.h>
#include <book/vmarea.h>
#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/math.h>

/**
 * MmuMemoryInfo - 获取内存信息
 */
PUBLIC void MmuMemoryInfo()
{
	PART_START("Memory Information");
	// 获取页的使用情况
	unsigned int totalPages = ZoneGetAllTotalPages();
	unsigned int unsingPages = ZoneGetAllUsingPages();
	unsigned int freePages = totalPages - unsingPages;

	// 从ram获取内存信息
	unsigned int totalMemory;
	HalIoctl("ram", RAM_HAL_IO_MEMSIZE, (unsigned int)&totalMemory);

	// 空闲页大小 - 空间初始化占用的大小 = 剩余大小
	unsigned int freeMemory = freePages * PAGE_SIZE - ZoneGetInitMemorySize();

	// 使用中的大小 = 总大小 - 空闲大小
	unsigned int unsingMemory = totalMemory - freeMemory;
	
	printk("\n");
	// 显示内存使用情况
	printk(PART_TIP "Total memory %d MB, pages total memory %d MB.\n", 
		totalMemory / MB, (totalPages * PAGE_SIZE) / MB);

	printk(PART_TIP "Unsing memory %d MB, free memory %d MB.\n", 
		unsingMemory / MB, freeMemory / MB);
	
	printk(PART_TIP "Total pages %d, using pages %d free pages %d.\n", 
		totalPages, unsingPages, freePages);

	PART_END();
}