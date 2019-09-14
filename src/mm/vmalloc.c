/*
 * file:		mm/vmalloc.c
 * auther:		Jason Hu
 * time:		2019/8/31
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/vmalloc.h>
#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/math.h>
#include <book/list.h>

/* 管理虚拟地址 */

PRIVATE struct Bitmap vmBitmap;

PRIVATE unsigned int vmBaseAddress; 

PRIVATE struct List vmAreaList;

/* 虚拟区域结构体 */
struct VM_Area {
	unsigned int addr;
	unsigned int size;
	struct List list;
};

/**
 * AllocVaddress - 分配一块空闲的虚拟地址
 * @size: 请求的大小
 */
PRIVATE unsigned int AllocVaddress(unsigned int size)
{

	size = PAGE_ALIGN(size);
	if (!size)
		return 0;
	
	int pages = size / PAGE_SIZE;

	/* 扫描获取请求的页数 */
	int idx = BitmapScan(&vmBitmap, pages);
	if (idx == -1)
		return 0;

	int i;
	/* 把已经扫描到的位置1，表明已经分配了 */
	for (i = 0; i < pages; i++) {
		BitmapSet(&vmBitmap, idx + i, 1);
	}

	/* 返还转换好的虚拟地址 */
	return vmBaseAddress + idx * PAGE_SIZE; 
}

/**
 * AllocVaddress - 分配一块空闲的虚拟地址
 * @size: 请求的大小
 */
PRIVATE unsigned int FreeVaddress(unsigned int vaddr, size_t size)
{

	int pages = size / PAGE_SIZE;

	/* 扫描获取请求的页数 */
	int idx = (vaddr - vmBaseAddress) / PAGE_SIZE;
	if (idx == -1)
		return -1;

	int i;
	/* 把地址对应的位图到的位置0，表明已经释放了 */
	for (i = 0; i < pages; i++) {
		BitmapSet(&vmBitmap, idx + i, 0);
	}

	return 0; 
}

/**
 * vmalloc2 - 分配空间
 * @size: 空间的大小
 */
PUBLIC void *vmalloc2(size_t size)
{
	size = PAGE_ALIGN(size);

	if (!size)
		return NULL;

	unsigned int start = AllocVaddress(size);
	if (!start) 
		return NULL;

	unsigned int vaddr = start;

	unsigned int end = vaddr + size;
	
	/* 创建一个虚拟区域 */
	struct VM_Area *area = kmalloc(sizeof(struct VM_Area), GFP_KERNEL);
	if (area == NULL)
		return NULL;

	area->addr = vaddr;
	area->size = size;

	/* 添加到虚拟区域的链表上 */
	ListAddTail(&area->list, &vmAreaList);

	int order = GetPagesOrder(size / PAGE_SIZE);
	if (order == -1)
		return NULL;

	unsigned int paddr = GetFreePages(GFP_DYNAMIC, order);

	/* 地址映射 */
	while(vaddr < end) {
		/* 链接地址 */
		PageLinkAddress(vaddr, paddr, GFP_DYNAMIC, PAGE_US_S | PAGE_RW_W);

		vaddr += PAGE_SIZE;
		paddr += PAGE_SIZE;
	} 

	return (void *)start;
}


/**
 * vfree2 - 释放空间
 * @ptr: 空间所在的地址
 */
PUBLIC int vfree2(void *ptr)
{
	if (ptr == NULL)
		return -1;

	unsigned int addr = (unsigned int)ptr;

	if (addr < vmBaseAddress || addr >= ZONE_DYNAMIC_END)
		return -1;
	
	
	struct VM_Area *area, *next;

	unsigned int vaddr, paddr, firstPaddr = 0;
	char flags = 0;
	ListForEachOwnerSafe(area, next, &vmAreaList, list) {
		/* 如果找到了对应得区域 */
		if (area->addr == addr) {	
			
			int order = GetPagesOrder(area->size / PAGE_SIZE);
			if (order == -1) 
				return -1;
			
			/* 取消地址映射 */
			vaddr = area->addr;
			while(vaddr < area->addr + area->size) {
				/* 获取第一个物理地址 */
				firstPaddr = PageUnlinkAddress(vaddr);
				if (firstPaddr && !flags) {
					paddr = firstPaddr;
				}
				flags = 1;
				
				vaddr += PAGE_SIZE;
			}
			
			/* 释放虚拟地址 */
			FreeVaddress(area->addr, area->size);
			
			/* 释放物理地址 */
			FreePages(paddr, order);

			/* 从链表中脱落 */
			ListDel(&area->list);
			/* 释放结构体 */
			kfree(area);
			return 0;
		}
	}
	
	return -1;
}

/** 
 * iomap - 把地址重新映射
 * @paddr: 要求映射的物理地址
 * @size: 映射区域大小
 * 
 * 用于让虚拟地址和物理地址一致，可以进行I/O地址访问
 * 对于某些特殊设备，是映射到内存中的，需要访问内存来
 * 进行设备访问。例如显存，通过这样映射之后，就可以通过
 * 虚拟地址来访问那些物理地址了。这里是1对1的关系
 */
PUBLIC int iomap(unsigned int paddr, size_t size)
{
	size = PAGE_ALIGN(size);

	// 对addr和size的范围进行判断
	if (!size || !paddr || (paddr + size) > ZONE_FIXED_ADDR)
		return -1;

	unsigned int vaddr = paddr;
	
	unsigned int end = vaddr + size;
	
	/* 创建一个虚拟区域 */
	struct VM_Area *area = kmalloc(sizeof(struct VM_Area), GFP_KERNEL);
	if (area == NULL)
		return -1;

	area->addr = vaddr;
	area->size = size;

	/* 添加到虚拟区域的链表上 */
	ListAddTail(&area->list, &vmAreaList);
	
	/* 地址映射 */
	while(vaddr < end) {
		/* 链接地址 */
		PageLinkAddress(vaddr, paddr, GFP_DYNAMIC, PAGE_US_S | PAGE_RW_W);

		vaddr += PAGE_SIZE;
		paddr += PAGE_SIZE;
	} 

	return 0;
}

PRIVATE void VM_AreaTest()
{
	/*
	unsigned int vaddr = AllocVaddress(PAGE_SIZE);
	printk("alloc a vaddr: %x\n", vaddr);

	FreeVaddress(vaddr, PAGE_SIZE);

	vaddr = AllocVaddress(PAGE_SIZE*16);
	printk("alloc a vaddr: %x\n", vaddr);
	
	
	vaddr = vmalloc2(PAGE_SIZE*2);
	printk("alloc a vaddr: %x\n", vaddr);
	
	vfree2(vaddr);

	vaddr = AllocVaddress(PAGE_SIZE);
	printk("alloc a vaddr: %x\n", vaddr);
	
	vaddr = vmalloc2(PAGE_SIZE*10);
	printk("alloc a vaddr: %x\n", vaddr);
	
	vfree2(vaddr);
	
	vaddr = vmalloc2(PAGE_SIZE*100);
	printk("alloc a vaddr: %x\n", vaddr);
	
	vfree2(vaddr);*/
}

/**
 * InitVM_Area - 初始化虚拟区域
 */
PUBLIC INIT void InitVM_Area()
{
	PART_START("VM Area");
	/* 每一位代表1个页的分配状态 */
	vmBitmap.btmpBytesLen = ZONE_DYNAMIC_SIZE / (PAGE_SIZE * 8);
	
	/* 为位图分配空间 */
	vmBitmap.bits = kmalloc(vmBitmap.btmpBytesLen, GFP_KERNEL);

	BitmapInit(&vmBitmap);

	vmBaseAddress = ZONE_DYNAMIC_ADDR;

	/* 初始化区域链表 */
	INIT_LIST_HEAD(&vmAreaList);


	printk("bitmap len %d bits %x vm base %x\n", vmBitmap.btmpBytesLen, vmBitmap.bits, vmBaseAddress);
	/* 测试 */
	//VM_AreaTest();

	/* 做地址映射 */
	char *addr = vmalloc2(2*MB);
	printk("addr %x\n", addr);
	if (!addr) {
		Panic("addr null");
	}
	memset(addr, 0, 1024);
	
	/*
	iomap(0xe0000000, 4*MB);
	memset(0xe0000000, 0, 4*MB);
	*/

	//Panic("test");
	PART_END();
}
