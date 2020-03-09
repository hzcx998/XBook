/*
 * file:		arch/x86/include/mm/page.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _X86_MM_PAGE_H
#define _X86_MM_PAGE_H

#include "pflags.h"
#include "phymem.h"

#include "../kernel/interrupt.h"
#include <book/bitmap.h>
#include <book/list.h>
#include <book/memcache.h>
#include <lib/stdint.h>
#include <lib/stddef.h>
#include <lib/stddef.h>

// 页目录类型
typedef unsigned int pde_t;
// 页表类型
typedef unsigned int pte_t;

//PDT = PAGE DIR TABLE
//内核的页目录表物理地址
#define PAGE_DIR_PHY_ADDR     0X201000
//内核的页目录表虚拟地址
#define PAGE_DIR_VIR_ADDR     0X80201000

//PT = PAGE TABLE
//内核的页表物理地址
#define PAGE_TABLE_PHY_ADDR     0X202000
//内核的页表虚拟地址
#define PAGE_TABLE_VIR_ADDR     0X80202000

// 在loader中初始化了2个页表，满足需要
#define PAGE_TABLE_PHY_NR     2

#define	 PAGE_P_1	  	1	// 0001 exist in memory
#define	 PAGE_P_0	  	0	// 0000 not exist in memory
#define	 PAGE_RW_R  	0	// 0000 R/W read/execute
#define	 PAGE_RW_W  	2	// 0010 R/W read/write/execute
#define	 PAGE_US_S  	0	// 0000 U/S system level, cpl0,1,2
#define	 PAGE_US_U  	4	// 0100 U/S user level, cpl3

#define PAGE_SHIFT 12

#define PAGE_SIZE (1U<<PAGE_SHIFT)  

#define PAGE_MASK (~(PAGE_SIZE-1))  

#define PAGE_INSIDE (PAGE_SIZE-1)  

//一个页有多少项
#define PAGE_ENTRY_NR 1024  

//获取页目录项的索引(存放了页表的地址和属性)
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)

//获取页表项的索引(存放了页的地址和属性)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

/* 
这是一种计算map和页关系的算法。具体我也没明白，
但我知道这样算就可以能够得出结果。2019/7/4注
*/
#define PAGE_MARK_USED(index, order, area) \
	BitmapChange(&((area)->map), (index) >> (1 + (order)))

#define PAGE_TO_ADDR(page, zone) (zone->physicStart + (page - zone->pageArray)*PAGE_SIZE)

/* 地址或者是数字对齐操作 */
#define ALIGN_WITH(x, y) ((x + y)&(~(y-1)))

/* 一点内存中页的数量 */
#define PAGE_NR_IN_1GB      (0X40000000/PAGE_SIZE)
#define PAGE_NR_IN_16MB      (0X1000000/PAGE_SIZE)

#define PAGE_MARK_SLAB_CACHE(page, cache, slab) \
        page->slabCache = cache;\
        page->slab = slab
        
#define PAGE_CLEAR_SLAB_CACHE(page) \
        page->slabCache = NULL;\
        page->slab = NULL
        
#define PAGE_GET_SLAB(page) page->slab
#define PAGE_GET_SLAB_CACHE(page) page->slabCache

#define CHECK_PAGE(page) \
        if (page == NULL) Panic("Page error!\n") 

// 保证值和页的大小对齐
#define PAGE_ALIGN(value) ((value&(PAGE_SIZE-1) ? value + PAGE_SIZE : value) & PAGE_MASK)

// 检测pte存在
#define PAGE_PTE_EXIST(entry) (entry&PAGE_P_1)

// 通过and运算把一个页地址去掉属性部分
#define PAGE_ADDR_MASK  0xfffff000

/* 页故障导致的错误码 */
#define PAGE_ERR_NONE_PRESENT       (0<<0)
#define PAGE_ERR_PROTECT            (1<<0)
#define PAGE_ERR_READ               (0<<1)
#define PAGE_ERR_WRITE              (1<<1)
#define PAGE_ERR_SUPERVISOR         (0<<2)
#define PAGE_ERR_USER               (1<<2)


PUBLIC int DoPageFault(struct TrapFrame *frame);

PUBLIC unsigned int Vir2PhyByTable(unsigned int vaddr);

PRIVATE INLINE pde_t *GetPageDirTable()
{
    return (pde_t *)PAGE_DIR_VIR_ADDR;
}

/**
 * PageGetPde - 获取pde
 * @vaddr: 虚拟地址
 * 
 * 通过虚拟地址获取它对应的pde
 */
PRIVATE INLINE pde_t *PageGetPde(address_t vaddr)
{
	// 获取地址对应的页目录项地址
	pde_t *pde = (address_t *)(0xfffff000 + \
	PDE_IDX(vaddr)*4);
	return pde;
}

/**
 * PageGetPte - 获取pte
 * @vaddr: 虚拟地址
 * 
 * 通过虚拟地址获取它对应的pte
 */
PRIVATE INLINE pte_t *PageGetPte(address_t vaddr)
{
	// 获取页表项地址
	pte_t *pte = (address_t *)(0xffc00000 + \
	((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr)*4);
	return pte;
}

PUBLIC unsigned int AllocPages(unsigned int count);
#define AllocPage() AllocPages(1)

PUBLIC int FreePages(unsigned int page);
#define FreePage(page) FreePages(page)

PUBLIC int PageTableAdd(unsigned int virtualAddr,
		unsigned int physicAddr,
        unsigned int protect);
PUBLIC int MapPages(unsigned int start,
    unsigned int len, 
    unsigned int protect);
PUBLIC unsigned int RemoveFromPageTable(unsigned int virtualAddr);
PUBLIC int UnmapPages(unsigned int vaddr, unsigned int len);
PUBLIC int UnmapPagesFragment(unsigned int vaddr, unsigned int len);
PUBLIC void UnmapPagesRange(pde_t *pgdir, unsigned int start, unsigned int end);

PUBLIC int MapPagesMaybeMapped(unsigned int start,
    unsigned int npages, 
    unsigned int protect);


#endif  /*_X86_MM_PAGE_H */
