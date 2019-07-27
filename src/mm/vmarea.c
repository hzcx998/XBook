/*
 * file:		mm/vmarea.c
 * auther:		Jason Hu
 * time:		2019/7/17
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/vmarea.h>
#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/math.h>
#include <book/mmu.h>

/* 管理虚拟区域的链表头，用来寻找其他区域 */
struct VirtualMemoryArea *vmaList;

/**
 * VirtualMemoryAreaMapPages - 虚拟内存区域映射页
 * @vaddr: 虚拟地址
 * @paddr: 物理地址
 * @size: 获取空间的大小
 * @flags: 分配页的标志
 * 
 * 把虚拟地址和物理页进行映射，如果带有物理地址，那么就不需要重新分配新的物理地址
 * 如果没有携带物理地址（paddr==0），那么就说明需要分配新的物理地址
 */
PRIVATE INLINE int VirtualMemoryAreaMapPages(address_t vaddr, address_t paddr, size_t size, unsigned int flags)
{
	int ret;
	address_t end = vaddr + size;
	address_t physicAddr;

	// 有物理地址的映射
	char withPhysic = 0;
	if (paddr)
		withPhysic = 1;

	// 获取页表操作锁

	// ----循环分配页表----
	// 每次以4MB为一个单位进行分配
	do {
		ret = -1;
		// 不是自带物理页则说明需要分配页
		if (!withPhysic) {
			physicAddr = GetFreePage(flags);
			/*  
			NOTE:如果失败的话，应该把在此之前分配的所有页都释放，然后返回
			在这里，我们简单化处理，直接返回
			*/
			if (!physicAddr)
				break;
		}
		//printk(PART_TIP "LINK: %x with %x\n", addr, physicAddr);

		// 不是自带物理页则说明需要用我们分配的页，不然和虚拟地址一样
		if (!withPhysic)
			PageLinkAddress(vaddr, physicAddr, flags);
		else 
			PageLinkAddress(vaddr, paddr, flags);

		// 地址递增
		vaddr += PAGE_SIZE;
		// 自带物理页就才递增
		if (withPhysic)
			paddr += PAGE_SIZE;
		
		// 指向下一个页目录项
		ret = 0;
	} while (vaddr && vaddr < end);

	// 释放页表操作锁

	// 因为写入了写的页目录项，清空所有CPU高速缓存

	return ret;
}
/**
 * VirtualMemoryAreaUnmapPages - 取消虚拟内存区域映射页
 * @vaddr: 虚拟地址
 * @paddr: 物理地址
 * @size: 空间的大小
 * 
 * 取消虚拟地址和物理页映射，如果带有物理地址，那么就不释放分配的物理地址
 * 没有携带，那么说明之前是通过分配的物理地址，就需要释放
 */
PRIVATE INLINE int VirtualMemoryAreaUnmapPages(address_t vaddr, address_t paddr, size_t size)
{
	address_t end = vaddr + size;
	address_t physicAddr;

	// 有物理地址的映射
	char withPhysic = 0;
	if (paddr)
		withPhysic = 1;

	//printk(PART_TIP "VirtualMemoryAreaUnmapPages: start\n");
	
	//printk(PART_TIP "addr %x size %x end %x\n", addr, size, end);
	do {
		physicAddr = PageUnlinkAddress(vaddr);
		//printk(PART_TIP "get physicl addr %x\n", physicAddr);
		
		// 检测地址是否正确
		if (!physicAddr)
			return -1;

		// 不是自带物理页则说明需要释放已经分配的页
		if (!withPhysic) {
			FreePage(physicAddr);
		}
		vaddr += PAGE_SIZE;
	} while (vaddr && vaddr < end);
	//printk(PART_TIP "VirtualMemoryAreaUnmapPages: end\n");
	return 0;
}

/**
 * VirtualMemoryGetArea - 获取一个虚拟内存空间
 * @size: 要分配的大小
 * @flags: 分配虚拟空间的flags
 */
PRIVATE struct VirtualMemoryArea *VirtualMemoryGetArea(size_t size, unsigned int flags)
{
	address_t addr, next;
	struct VirtualMemoryArea **p, *tmp, *area; 

	// 为VirtualMemoryArea分配一块空间
	area = kmalloc(sizeof(struct VirtualMemoryArea), GFP_KERNEL);
	// 失败则返回
	if (!area)
		return NULL;

	// 每一个vma中间留一个页的空隙，隔离不同的vma
	size += PAGE_SIZE;	 

	// 如果大小为0就返回
	if (!size) {
		// 释放之前分配的area
		kfree(area);
		return NULL;
	}

	addr = ZONE_DYNAMIC_ADDR;
	// 开始查找一个合适的区域
	for (p = &vmaList; (tmp = *p); p = &tmp->next) {
		// 保证没有达到可寻址范围的末端。这两行我没看明白，直接搬过来(*^_^*)
		if ((size + addr) < addr) 
			goto ToError;

		// 找到一个中间区域，直接跳出循环
		if (size + addr <= (address_t )tmp->addr)
			break;
		
		// 计算下一个area空间地址
		next = (address_t )tmp->addr + tmp->size;
		
		// 如果下一个地址比当前地址大，那么当前地址就等于下一个地址
		if (next > addr)
			addr = next;

		// 如果超过了空间最大的地方就出错
		if (addr > ZONE_DYNAMIC_END - size) 
			goto ToError;
	}

	area->next = *p;	// 指向下一个区域
	// 把area加入到链表中，由于上一行已经让area->next指向了*p
	// 这里插进去后就让链表连续起来了
	*p = area;

	// 设定area中的值
	area->addr = (void *)addr;
	area->size = size;
	area->flags = flags;
	
	return area;
ToError:
	// 释放为area分配的内存
	kfree(area);
	return NULL;
}

/**
 * __vmalloc - 分配虚拟地址
 * @size: 分配空间的大小
 * @flags: 获取页的方式
 * 
 * 分配非连续地址空间里面的内存
 */
PUBLIC void *__vmalloc(size_t size, unsigned int flags)
{
	struct VirtualMemoryArea *area;
	void *addr;

	// 对传入的大小进行页对齐
	size = PAGE_ALIGN(size);

	// 对size进行判断，为0或者大于最大页数量就返回
	if (!size || (size >> PAGE_SHIFT) >  ZoneGetTotalPages(ZONE_DYNAMIC_NAME))
		return NULL;
	
	// 获取虚拟内存区域
	area = VirtualMemoryGetArea(size, VMA_ALLOC);
	if (!area)
		return NULL; 
	addr = area->addr;
	//printk(PART_TIP "GET area addr %x size %x\n", area->addr, area->size);
	// 把虚拟地址和页关联起来，不是直接映射
	if (VirtualMemoryAreaMapPages((address_t )addr, 0, area->size, flags)) {
		// 如果关联出错，就释放虚拟地址区域并返回
		kfree(area);
		return NULL;
	}
	
	// 返回分配的地址
	return addr;
}

/**
 * vfree - 释放虚拟地址
 * @addr: 要释放的地址
 * 
 * 释放非连续地址空间里面的内存
 */
PUBLIC void vfree(void *addr)
{
	struct VirtualMemoryArea **p, *tmp;
	if (!addr) 
		return;

	// 如果地址不是页对齐的地址就错误
	if ((PAGE_SIZE-1) & (address_t )addr) {
		printk(PART_WARRING "vfree addr is bad(%x)!\n", (address_t)addr);
		return;
	}

	// 在虚拟地址区域中寻找指定的区域
	for (p = &vmaList; (tmp = *p); p = &tmp->next) {
		// 如果找到了地址
		if (tmp->addr == addr && tmp->size) {
			// 把地址对应的area从区域链表中删除
			*p = tmp->next;

			// 取消虚拟地址和物理页的映射
			VirtualMemoryAreaUnmapPages((address_t)tmp->addr, 0, tmp->size);

			// 释放area占用的内存
			kfree(tmp);
			//printk(PART_TIP "vfree at %x size %x\n", (address_t )addr, tmp->size);
			return;
		}
	}
	printk(PART_TIP "vfree address not exist in vma(%x)!", (address_t)addr);
}

/** 
 * vmap - 虚拟地址映射
 * @paddr: 要求映射的物理地址
 * @size: 映射区域大小
 * 
 * 映射之后，可以通过虚拟地址访问物理地址
 */
PUBLIC void *vmap(address_t paddr, size_t size)
{
	struct VirtualMemoryArea *area;
	
	// 进行页对齐
	size = PAGE_ALIGN(size);

	// 对addr和size的范围进行判断
	if (!size || !paddr || (paddr + size) > ZONE_FIXED_ADDR)
		return NULL;

	// 获取虚拟内存区域
	area = VirtualMemoryGetArea(size, VMA_MAP);
	if (!area)
		return NULL;
	
	// 保存虚拟地址
	void *vaddr = area->addr;
	// 对地址进行映射	
	if (VirtualMemoryAreaMapPages((address_t )vaddr, paddr, size, GFP_DYNAMIC)) {
		kfree(area);
		return NULL;
	}
	return vaddr;
}


/**
 * vunmap - 释放虚拟地址
 * @addr: 要释放的地址
 * 
 * 释放非连续地址空间里面的内存
 */
PUBLIC void vunmap(void *addr)
{
	struct VirtualMemoryArea **p, *tmp;
	if (!addr) 
		return;

	// 如果地址不是页对齐的地址就错误
	if ((PAGE_SIZE-1) & (address_t )addr) {
		printk(PART_WARRING "vunmap addr is bad(%x)!\n", (address_t)addr);
		return;
	}

	// 在虚拟地址区域中寻找指定的区域
	for (p = &vmaList; (tmp = *p); p = &tmp->next) {
		// 如果找到了地址
		if (tmp->addr == addr && tmp->size) {
			// 把地址对应的area从区域链表中删除
			*p = tmp->next;

			// 取消虚拟地址和物理页的映射
			VirtualMemoryAreaUnmapPages((address_t)tmp->addr,(address_t)tmp->addr, tmp->size);

			// 释放area占用的内存
			kfree(tmp);
			//printk(PART_TIP "vfree at %x size %x\n", (address_t )addr, tmp->size);
			return;
		}
	}
	printk(PART_TIP "umap address not exist in vma(%x)!", (address_t)addr);
}


/** 
 * ioremap - 把地址重新映射
 * @addr: 要求映射的虚拟地址
 * @size: 映射区域大小
 * 
 * 用于让虚拟地址和物理地址一致，可以进行I/O地址访问
 * 对于某些特殊设备，是映射到内存中的，需要访问内存来
 * 进行设备访问。例如显存，通过这样映射之后，就可以通过
 * 虚拟地址来访问那些物理地址了。这里是1对1的关系
 */
PUBLIC int ioremap(address_t addr, size_t size)
{
	// 对addr和size的范围进行判断
	if (!size || !addr || (addr + size) > ZONE_FIXED_ADDR)
		return -1;
	// 对地址进行映射	
	if (VirtualMemoryAreaMapPages(addr, addr, size, GFP_DYNAMIC)) {
		return -1;
	}
	return 0;
}

/**
 * VirtualMemoryAreaTest - 虚拟地址区域测试
 */
PRIVATE void VirtualMemoryAreaTest()
{
	// ----
	printk(PART_TIP "----virtual memory area test----\n");

	void *table[5];
	int i;
	for (i = 0; i < 5; i++) {
		table[i] = vmalloc(i*50*PAGE_SIZE);
		memset(table[i], 0x5a, i*50*PAGE_SIZE);
		printk(PART_TIP "vmalloc: %x size %x B\n", table[i], i*50*PAGE_SIZE);
	}
	char *a = vmalloc(8*KB);
	printk(PART_TIP "vmalloc: %x size %x\n", a, 8*KB);
	memset(a, 0, 8*KB);
	// vfree(a);
	printk(PART_TIP "vfree: %x \n", a);
	for (i = 0; i < 5; i++) {
		vfree(table[i]);
		printk(PART_TIP "vfree: %x\n", table[i]);
	}
	
	void *mapAddr = vmap(0xe0000000, 4*MB); 
	if (!mapAddr)
		printk(PART_WARRING "vmap: failed at %x!\n", 0xe0000000);
	printk(PART_TIP "vmap: at %x\n", mapAddr);
	memset(mapAddr, 0, 4*MB);
	vunmap(mapAddr);
	printk("unmap\n");
	if (ioremap(0xe0000000, 4*MB))
		printk(PART_WARRING "ioremap failed\n");

	memset(0xe0000000, 0, 4*MB);
}
/**
 * VirtualMemoryAreaListInit - 初始化虚拟空间管理的单向链表
 */
PRIVATE void VirtualMemoryAreaListInit()
{
	// ----初始化vmaList----
	// 为vmaList分配内存
	vmaList = kmalloc(sizeof(struct VirtualMemoryArea), GFP_KERNEL);
	if (!vmaList)
		Panic("alloc memory for vmaList failed!\n"); 

	// 初始化信息
	vmaList->next = NULL;
	vmaList->flags = 0;
	vmaList->addr = (void *)ZONE_DYNAMIC_ADDR;
	vmaList->size = 0;
	vmaList->pages = 0;
} 

/**
 * InitVirtualMemoryArea - 初始化虚拟内存区域
 */
PUBLIC INIT void InitVirtualMemoryArea()
{
	PART_START("Virtual memory area");

	VirtualMemoryAreaListInit();
	//VirtualMemoryAreaTest();
	
	MmuMemoryInfo();
    //Spin("InitZone");

	PART_END();
}