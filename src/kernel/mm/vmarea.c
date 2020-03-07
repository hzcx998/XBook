/*
 * file:		kernel/mm/vmarea.c
 * auther:		Jason Hu
 * time:		2019/8/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/vmarea.h>
#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <book/list.h>
#include <lib/string.h>
#include <lib/math.h>

/* 虚拟区域结构体 */
typedef struct VMArea {
	unsigned int addr;
	unsigned int size;
	struct List list;
} VMArea_t;

/* 管理虚拟地址 */

PRIVATE struct Bitmap vmBitmap;

PRIVATE unsigned int vmBaseAddress; 

/* 正在使用中的vmarea */
struct List usingVMAreaList;

/* 处于空闲状态的vmarea，成员根据大小进行排序，越小的就靠在前面 */
PRIVATE struct List freeVMAreaList;

/**
 * AllocVaddress - 分配一块空闲的虚拟地址
 * @size: 请求的大小
 */
PUBLIC unsigned int AllocVaddress(unsigned int size)
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
PUBLIC unsigned int FreeVaddress(unsigned int vaddr, size_t size)
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

PRIVATE void *__vmalloc(size_t size)
{
	/* 创建一个新的区域 */
	unsigned int start = AllocVaddress(size);
	if (!start) 
		return NULL;
	
	struct VMArea *area;

	/* 创建一个虚拟区域 */
	area = kmalloc(sizeof(struct VMArea), GFP_KERNEL);
	if (area == NULL) {
		FreeVaddress(start, size);
		return NULL;
	}

	area->addr = start;
	area->size = size;

	unsigned long flags = InterruptSave();
	
	/* 添加到虚拟区域的链表上 */
	ListAddTail(&area->list, &usingVMAreaList);

	if (MapPages(start, size, PAGE_US_S | PAGE_RW_W)) {
		FreeVaddress(start, size);
		kfree(area);
		InterruptRestore(flags);
		return NULL;
	}
	
	InterruptRestore(flags);
	//printk("vmalloc: create a area %x/%x\n", area->addr, area->size);
	return (void *)area->addr;
}


/**
 * vmalloc - 分配空间
 * @size: 空间的大小
 */
PUBLIC void *vmalloc(size_t size)
{
	size = PAGE_ALIGN(size);

	if (!size)
		return NULL;

	/* 中间用一个页隔离 */
	size += PAGE_SIZE;

	//printk("size %d\n", size);

	struct VMArea *target = NULL, *area;

	/* 先从空闲链表中查找 */
	ListForEachOwner(area, &freeVMAreaList, list) {
		/* 如果找到了大小合适的区域 */
		if (size >= area->size) {
			target = area;
			break;
		}
	}

	/* 找到一个合适大小的area，就使用它 */
	if (target != NULL) {
		//printk("vmalloc: find a free area %x/%x\n", target->addr, target->size);
		unsigned long flags = InterruptSave();

		/* 先脱离原来的空闲链表，并添加到使用链表中去 */
		ListDel(&target->list);

		ListAddTail(&target->list, &usingVMAreaList);
		
		InterruptRestore(flags);
		return (void *)target->addr;
	}

	return (void *)__vmalloc(size);
}

PRIVATE int __vfree(struct VMArea *target)
{
	struct VMArea *area;

	//printk("vfree: find a using area %x/%x\n", target->addr, target->size);
	
	/* 先脱离原来的使用链表，并添加到空闲链表中去 */
	ListDel(&target->list);

	/* 成功插入标志 */
	char insert = 0;	
	/* 这里要根据area大小进行排序，把最小的排在最前面 */
	if (ListEmpty(&freeVMAreaList)) {
		//printk("vfree: free area is empty %x/%x\n", target->addr, target->size);
		/* 链表是空，直接添加到最前面 */
		ListAdd(&target->list, &freeVMAreaList);
	} else {
		//printk("vfree: free area is not empty %x/%x\n", target->addr, target->size);
		/* 获取第一个宿主 */
		area = ListFirstOwner(&freeVMAreaList, struct VMArea, list);

		do {
			/* 根据大小来判断插入位置，小的在前面，大的在后面 */
			if (target->size > area->size) {
				/*printk("target %x/%x area %x/%x\n",
					target->addr, target->size, area->addr, area->size);*/

				/* 不满足条件，需要继续往后查找 */
				if (area->list.next == &freeVMAreaList) {
					/* 如果到达了最后面，也就是说下一个就是链表头了
						直接把target添加到队列的最后面
						*/
					ListAddTail(&target->list, &freeVMAreaList);
					insert = 1;
					//printk("vfree: insert tail %x/%x\n");
					break;
				}

				/* 获取下一个宿主 */
				area = ListOwner(area->list.next, struct VMArea, list);
				
			} else {
				/* 插入到中间的情况 */

				/* 把新节点添加到旧节点前面 */
				ListAddBefore(&target->list, &area->list);
				insert = 1;
				//printk("vfree: insert before area %x/%x\n", area->addr, area->size);

				break;
			}
		} while (&area->list != &freeVMAreaList);
	}

	return insert;
}


/**
 * vfree - 释放空间
 * @ptr: 空间所在的地址
 */
PUBLIC int vfree(void *ptr)
{
	if (ptr == NULL)
		return -1;

	unsigned int addr = (unsigned int)ptr;

	if (addr < vmBaseAddress || addr >= NULL_MEM_ADDR)
		return -1;
	
	struct VMArea *target = NULL, *area;
	unsigned long flags = InterruptSave();
	
	ListForEachOwner(area, &usingVMAreaList, list) {
		/* 如果找到了对应的区域 */
		if (area->addr == addr) {
			target = area;
			break;
		}
	}

	/* 找到一个合适要释放的area，就释放它 */
	if (target != NULL) {
		if (__vfree(target)) {
			InterruptRestore(flags);
			return 0;
		}
	}
	
	/* 没找到，释放失败 */
	InterruptRestore(flags);
	return -1;
}

/** 
 * memmap - 把地址重新映射
 * @paddr: 要求映射的物理地址
 * @size: 映射区域大小
 * 
 * 用于让虚拟地址和物理地址一致，可以进行I/O地址访问
 * 对于某些特殊设备，是映射到内存中的，需要访问内存来
 * 进行设备访问。例如显存，通过这样映射之后，就可以通过
 * 虚拟地址来访问那些物理地址了。这里是1对1的关系
 */
PUBLIC int memmap(unsigned int paddr, size_t size)
{
	size = PAGE_ALIGN(size);

	// 对addr和size的范围进行判断
	if (!size || !paddr || (paddr + size) > NULL_MEM_ADDR)
		return -1;

	unsigned int vaddr = paddr;
	
	unsigned int end = vaddr + size;
	
	/* 创建一个虚拟区域 */
	struct VMArea *area = kmalloc(sizeof(struct VMArea), GFP_KERNEL);
	if (area == NULL)
		return -1;

	area->addr = vaddr;
	area->size = size;

	/* 添加到虚拟区域的链表上 */
	ListAddTail(&area->list, &usingVMAreaList);
	
	/* 地址映射 */
	while(vaddr < end) {
		/* 链接地址 */
		PageTableAdd(vaddr, paddr, PAGE_US_S | PAGE_RW_W);

		vaddr += PAGE_SIZE;
		paddr += PAGE_SIZE;
	}

	return 0;
}


/**
 * IoRemap - io内存映射
 * @phyAddr: 物理地址
 * @size: 映射的大小
 * 
 * @return: 成功返回映射后的地址，失败返回NULL
 */
PUBLIC void *IoRemap(unsigned long phyAddr, size_t size)
{
    /* 对参数进行检测,地址不能是0，大小也不能是0 */
    if (!phyAddr || !size) {
        return NULL;
    }

    /* 分配虚拟地址 */
    unsigned int vaddr = AllocVaddress(size);
    if (vaddr == -1) {
        printk("alloc virtual addr for IO remap failed!\n");
        return NULL;
    }
    //printk("alloc a virtual addr at %x\n", vaddr);

    /* 创建一个虚拟区域 */
	VMArea_t *area;

	/* 创建一个虚拟区域 */
	area = kmalloc(sizeof(VMArea_t), GFP_KERNEL);
	if (area == NULL) {
		FreeVaddress(vaddr, size);
		return NULL;
	}
    /* 设置虚拟区域参数 */
	area->addr = vaddr;
	area->size = size;
    unsigned long flags = InterruptSave();
	
	/* 添加到虚拟区域的链表上 */
	ListAddTail(&area->list, &usingVMAreaList);
    
    /* 进行io内存映射，如果失败就释放资源 */
    if (ArchIoRemap(phyAddr, vaddr, size)) {
        /* 释放分配的资源 */
        ListDel(&area->list);
        kfree(area);
        FreeVaddress(vaddr, size);
        /* 指向0，表示空 */
        vaddr = 0;
    }
    
	InterruptRestore(flags);
	
    return (void *)vaddr;    
}

/**
 * IoRemap - io内存映射
 * @phyAddr: 物理地址
 * @size: 映射的大小
 * 
 * @return: 成功返回映射后的地址，失败返回NULL
 */
PUBLIC int IoUnmap(void *addr)
{
    if (addr == NULL) {
        return -1;
    }
    unsigned long vaddr = (unsigned long )addr;
    
	if (vaddr < vmBaseAddress || vaddr >= NULL_MEM_ADDR)
		return -1;
	
	struct VMArea *target = NULL, *area;
	unsigned long flags = InterruptSave();
	
	ListForEachOwner(area, &usingVMAreaList, list) {
		/* 如果找到了对应的区域 */
		if (area->addr == vaddr) {
			target = area;
			break;
		}
	}

	/* 找到一个合适要释放的area，就释放它 */
	if (target != NULL) {
        if (ArchIoUnmap(target->addr, target->size)) {
		    /* 取消IO映射并释放area */
            
            ListDel(&target->list);

            FreeVaddress(vaddr, target->size);

            kfree(target);

            InterruptRestore(flags);
			return 0;
		}
	}
	
	/* 没找到，释放失败 */
	InterruptRestore(flags);
	return -1;
}
#if 0
PRIVATE void VMAreaTest()
{
	char *a = vmalloc(PAGE_SIZE);
	if (a == NULL)
		printk("vmalloc failed!\n");

	memset(a, 0, PAGE_SIZE);

	
	char *b = vmalloc(PAGE_SIZE* 10);
	if (b == NULL)
		printk("vmalloc failed!\n");

	memset(b, 0, PAGE_SIZE *10);

	
	char *c = vmalloc(PAGE_SIZE *100);
	if (c == NULL)
		printk("vmalloc failed!\n");

	memset(c, 0, PAGE_SIZE *100);

	char *d = vmalloc(PAGE_SIZE *1000);
	if (d == NULL)
		printk("vmalloc failed!\n");

	memset(d, 0, PAGE_SIZE *1000);

	printk("%x %x %x %x\n", a, b, c, d);

	vfree(a);
	vfree(b);
	vfree(c);
	vfree(d);
	
	a = vmalloc(PAGE_SIZE);
	if (a == NULL)
		printk("vmalloc failed!\n");

	memset(a, 0, PAGE_SIZE);
	
	b = vmalloc(PAGE_SIZE* 10);
	if (b == NULL)
		printk("vmalloc failed!\n");

	memset(b, 0, PAGE_SIZE *10);

	c = vmalloc(PAGE_SIZE *100);
	if (c == NULL)
		printk("vmalloc failed!\n");

	memset(c, 0, PAGE_SIZE *100);

	d = vmalloc(PAGE_SIZE *1000);
	if (d == NULL)
		printk("vmalloc failed!\n");

	memset(d, 0, PAGE_SIZE *1000);

	printk("%x %x %x %x\n", a, b, c, d);

	vfree(c);
	vfree(b);
	vfree(d);
	vfree(a);

	/*memmap(0xe0000000, PAGE_SIZE*10);
	char *v = (char *)0xe0000000;
	memset(v, 0, PAGE_SIZE*10);*/
	/*
	MapPages(0xe0000000, PAGE_SIZE*10, PAGE_US_S | PAGE_RW_W);

	char *v = (char *)0xe0000000;
	memset(v, 0, PAGE_SIZE*10);

	UnmapPages(0xe0000000, PAGE_SIZE*10);
	*/
	//Spin("1");
}
#endif 
/**
 * InitVMArea - 初始化虚拟区域
 */
PUBLIC INIT void InitVMArea()
{
	/* 每一位代表1个页的分配状态 */
	vmBitmap.btmpBytesLen = HIGH_MEM_SIZE / (PAGE_SIZE * 8);
	
	/* 为位图分配空间 */
	vmBitmap.bits = kmalloc(vmBitmap.btmpBytesLen, GFP_KERNEL);

	BitmapInit(&vmBitmap);

	vmBaseAddress = HIGH_MEM_ADDR;

	/* 初始化使用中的区域链表 */
	INIT_LIST_HEAD(&usingVMAreaList);

	/* 初始化空闲的区域链表 */
	INIT_LIST_HEAD(&freeVMAreaList);

	//printk("bitmap len %d bits %x vm base %x\n", vmBitmap.btmpBytesLen, vmBitmap.bits, vmBaseAddress);
	/* 测试 */
	//VMAreaTest();
}
