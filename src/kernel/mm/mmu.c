/*
 * file:		kernel/mm/mmu.c
 * auther:		Jason Hu
 * time:		2019/7/18
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/mmu.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/memcache.h>
#include <book/vmarea.h>
#include <book/lowmem.h>
#include <lib/string.h>
#include <lib/math.h>

PUBLIC void SysGetMemory(meminfo_t *mi)
{
    if (mi == NULL)
        return;
        
    /* 获取内存信息 */
    mi->mi_total   = GetPhysicMemoryTotalSize();
    mi->mi_free    = GetPhysicMemoryFreeSize();
    mi->mi_used    = mi->mi_total - mi->mi_free;
    if (mi->mi_used < 0)
        mi->mi_used = 0;
}

PUBLIC void PrintMemoryInfo()
{
    meminfo_t mi;
    SysGetMemory(&mi);
    printk("Total: %d-%dMB, Free: %d-%dMB, Used: %d-%dMB\n",
        mi.mi_total, mi.mi_total / MB, mi.mi_free, mi.mi_free / MB, mi.mi_used, mi.mi_used / MB);
}

/**
 * MmuMemoryInfo - 获取内存信息
 */
PUBLIC void InitMMU()
{
    
	/* 初始化内存缓存 */
	InitMemCaches();
    
	/* 初始化内存区域 */
	InitVMArea();

    /* 初始化内存片段 */
    InitMemFragment();
    
    PrintMemoryInfo();
}