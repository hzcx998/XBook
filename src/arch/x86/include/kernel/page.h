/*
 * file:		arch/x86/include/kernel/page.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _X86_PAGE_H
#define _X86_PAGE_H


#include <book/bitmap.h>
#include <share/stdint.h>

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

int init_page();

/*
虚拟地址结构体
*/
struct virtual_addr
{
    struct bitmap vaddr_bitmap;   //位图结构
    uint32_t vir_addr_start;           //虚拟地址起始地址
};


#define	 PAGE_P_1	  	1	// 0001 exist in memory
#define	 PAGE_P_0	  	0	// 0000 not exist in memory
#define	 PAGE_RW_R  	0	// 0000 R/W read/execute
#define	 PAGE_RW_W  	2	// 0010 R/W read/write/execute
#define	 PAGE_US_S  	0	// 0000 U/S system level, cpl0,1,2
#define	 PAGE_US_U  	4	// 0100 U/S user level, cpl3

#define PAGE_SIZE 4096  //一个页的大小

//获取页目录项的索引(存放了页表的地址和属性)
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)

//获取页表项的索引(存放了页的地址和属性)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

//内核可分配内存的起始地址，从4M开始
#define KERNEL_HEAP_START 0X80400000   

//空闲的物理内存的开始
#define FREE_PHY_START 0X00400000   

//内存池位图的虚拟地址
#define MEM_POOL_BITMAP_VIR_ADDR_START  0x80210000

//内核或者用户内存池位图最大为2G
#define MAX_MEM_POOL_BITMAP_SIZE 0x10000


#define X86_PHY_MEM_MAX 0xfffff000


/*
内存池结构体，用于管理内核内存和用户内存
和virtual_addr结构类似，只是多了个大小
*/
struct memory_pool
{
    struct bitmap pool_bitmap;   //位图结构
    uint32_t phy_addr_start;        //物理内存的开始地址
    uint32_t pool_size;             //内存池大小
};

/*
mem pool flags
*/
typedef enum {
    PF_KERNEL = 1,
    PF_USER,
}pool_flags_t;

uint32_t *page_ptePtr(uint32_t vaddr);
uint32_t *page_pdePtr(uint32_t vaddr);
uint32_t page_addrV2P(uint32_t vaddr);

void *alloc_pageVirtualAddr(pool_flags_t pf, uint32_t pages);
void free_pageVirtualAddr(pool_flags_t pf, void *vaddr, uint32_t pages);

void *pool_allocPhyMem(struct memory_pool *mem_pool);
void pool_freePhyMem(uint32_t phy_addr);

void *page_allocMemory(pool_flags_t pf, uint32_t pages);
void page_freeMemory(pool_flags_t pf, void *vaddr, uint32_t pages);

void page_tableAdd(void *vir_addr, void *phy_addr);
void page_tableRemovePTE(uint32_t vaddr);

void *alloc_kernelPage(uint32_t pages);
void free_kernelPage(void *vaddr, uint32_t pages);

#endif  /*_X86_PAGE_H*/
