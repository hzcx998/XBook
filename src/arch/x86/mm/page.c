/*
 * file:		arch/x86/mm/page.c
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <config.h>
#include <page.h>
#include <ards.h>
#include <zone.h>
#include <x86.h>
#include <area.h>
#include <share/stdint.h>
#include <book/bitmap.h>
#include <share/string.h>
#include <book/debug.h>
#include <share/math.h>
#include <book/vmspace.h>
#include <book/task.h>

/*
 * InitPageEnvironment - 初始化分页机制相关的环境
 * 
 * 把物理页和内核虚拟地址进行映射
 */
PUBLIC int InitPageEnvironment(unsigned int phyAddrStart, unsigned int physicAddrEnd)
{
	PART_START("Page");

	/* ----映射静态内存---- */
	//把页目录表设置
	uint32_t *pdt = (uint32_t *)PAGE_DIR_VIR_ADDR;
	#ifdef CONFIG_PAGE_DEBUG
	printk(" |- physic addr range %x ~ %x\n", phyAddrStart, physicAddrEnd);
	#endif
	// 求出目录项数量
	unsigned int pageDirEntryNumber = (physicAddrEnd-phyAddrStart)/(1024*PAGE_SIZE);

	// 求出剩余的页表项数量

	// 先获取的页数
	unsigned int pageTableEntryNumber = (physicAddrEnd-phyAddrStart)/PAGE_SIZE;

	// 获取页数中余下的数量，就是页表项剩余数
	pageTableEntryNumber = pageTableEntryNumber%1024;
	
	//有多少个页目录项，这个根据静态空间来进行设置
	//unsigned int pageDirEntryNumber = DIV_ROUND_UP(physicAddrEnd-phyAddrStart, 1024 * PAGE_SIZE);
	#ifdef CONFIG_PAGE_DEBUG
	printk(" |- page dir entry num:%d page table entry num:%d\n", 
			pageDirEntryNumber, pageTableEntryNumber);
	#endif
	//跳过页表区域中的第一个页表
	unsigned int *pageTablePhyAddress = (unsigned int *) (PAGE_TABLE_PHY_ADDR + 
			PAGE_SIZE*PAGE_TABLE_PHY_NR);
	#ifdef CONFIG_PAGE_DEBUG
	printk(" |- page table addr:%x phy addr:%x\n", pageTablePhyAddress, phyAddrStart);
	#endif
	int i, j;
	// 根据页目录项来填写
	for (i = 0; i < pageDirEntryNumber; i++) {
		//填写页目录表项
		//把页表地址和属性放进去
		pdt[512 + 2 + i] = (address_t)pageTablePhyAddress | PAGE_P_1 | PAGE_RW_W | PAGE_US_S;
		
		for (j = 0; j < PAGE_ENTRY_NR; j++) {
			//填写页页表项

			//把物理地址和属性放进去
			pageTablePhyAddress[j] = phyAddrStart | PAGE_P_1 | PAGE_RW_W | PAGE_US_S;
			//指向下一个物理页
			phyAddrStart += PAGE_SIZE;
		}
		//指向下一个页表
		pageTablePhyAddress += PAGE_SIZE;
	}
	// 根据剩余的页表项填写

	//有剩余的我们才填写
	if (pageTableEntryNumber != 0) {
		// i 和 pageTablePhyAddress 的值在上面的循环中已经设置
		pdt[512 + 2 + i] = (address_t)pageTablePhyAddress | PAGE_P_1 | PAGE_RW_W | PAGE_US_S;
		
		// 填写剩余的页表项数量
		for (j = 0; j < pageTableEntryNumber; j++) {
			//填写页页表项
			//把物理地址和属性放进去
			pageTablePhyAddress[j] = phyAddrStart | PAGE_P_1 | PAGE_RW_W | PAGE_US_S;
			
			//指向下一个物理页
			phyAddrStart += PAGE_SIZE;
		}
	}
	#ifdef CONFIG_PAGE_DEBUG
	printk(" |- maped to phyAddrEnd:%x to pageTablePhyAddress:%x\n", phyAddrStart, pageTablePhyAddress);
	#endif

	/* 在开启分页模式之后，我们的内核就运行在高端内存，
	那么，现在我们不能通过低端内存访问内核，所以，我们在loader
	中初始化的0~8MB低端内存的映射要取消掉才行。
	我们把用户内存空间的页目录项都清空 */ 
	
	for (i = 0; i < 512; i++) {
		pdt[i] = 0;
	}

	/* ----映射完毕---- */
	PART_END();
	return 0;
}

/* 
 * PageExpand - 拆分页
 * @page: 当前area中获取的空闲的page
 * @index: page在area中的index索引
 * @low: 低的order
 * @high: 高的order
 * @area: 当前的area
 * 
 * 尝试把大的页拆分成小的页，当从low到high中没有空闲页时来做这就事
 * 注意，这里返回页指针
 */
PRIVATE struct Page *PageExpand(struct Zone *zone, struct Page *page, 
		unsigned int index, int low, int high, struct FreeArea *area)
{
	//先计算出size的大小，这里不是页的大小，而是area能够容纳的list大小
	unsigned int size = 1 << high;
	while(high > low) {
		
		if (ZONE_BAD_RANGE(zone, page))
			Panic("zone bad range-> zone %s start %x end %x page %x", 
				zone->name, zone->pageArray, zone->pageArray + zone->pageTotalCount, page);
		
		//改变area为更小的order
		area--;
		high--;

		//size /= 2，把大小分成2半
		size >>= 1;
		
		//printk(" |- PageExpand: zone %s pages:%d index:%d order:%d area:%x\n", zone->name, zone->pageTotalCount,  index, high, area);

		//把其中一半添加到area对应的链表中
		ListAdd(&(page)->list, &(area)->pageList);

		//在area中把它标记为使用
		PAGE_MARK_USED(index, high, area);

		//指向另外一半页，通过增值来改变index指向和page地址
		index += size;
		page += size;
	}
	
	if (ZONE_BAD_RANGE(zone, page))
			Panic("zone bad range-> zone %s start %x end %x page %x", 
				zone->name, zone->pageArray, zone->pageArray + zone->pageTotalCount, page);

	/* 
	返回找到的最小的那个页
	如果有order变化就会是新找到的页，
	不然的话，就是之前传递进来的页
	*/
	return page;
}

/* 
 * GetFreePages - 分配多个页
 * @flags: 分配页的标志
 * @order: 分配2^order个页
 * 
 * 在order对应的area中去分配页，如果没有空闲的就去更大的order中去查找页
 * 注意，这里返回页的物理地址
 */
PUBLIC unsigned int GetFreePages(unsigned int flags, unsigned int order)
{
	struct Page *page;

	page = PagesAlloc(flags, order);

	if (page == NULL)
		return 0;

	struct Zone *zone = NULL;

	// 根据mask选择zone
	if (flags & ZONE_STATIC) {
		
		zone = ZoneGetByName(ZONE_STATIC_NAME);
	} else if (flags & ZONE_DYNAMIC) {
		
		zone = ZoneGetByName(ZONE_DYNAMIC_NAME);
	
	}
	//printk(" |- GetFreePages - zone:%x name:%s \n",zone, zone->name);
	if (zone == NULL) {
		return 0;
	}
	
	//把页结构转换成物理地址后返回
	return (unsigned int) PageToPhysicAddress(page);
}

/* 
 * __PagesAlloc - 分配多个页
 * @flags: 分配页的标志
 * @order: 分配2^order个页
 * @zone: 页属于哪个空间
 * 
 * 在order对应的area中去分配页，如果没有空闲的就去更大的order中去查找页
 * 注意，这里返回页指针
 */
PRIVATE struct Page *__PagesAlloc(unsigned int flags, unsigned int order, struct Zone *zone)
{
	//获取对应的区域
	struct FreeArea *area = zone->freeArea + order;
	
	//获取order
	unsigned int new_order = order;

	// 页指针
	struct Page *page;

	unsigned int index;
	
	//循环检测是否有符合的
	do {
		//如果不是空
		if (!ListEmpty(&area->pageList)) {
			//在里面查找
			
			//获取一个后会把它从列表删除，所以第一个总是空闲的
			page = ListFirstOwner(&area->pageList, struct Page, list);
			
			if (ZONE_BAD_RANGE(zone, page))
			Panic("zone bad range-> zone %s start %x end %x page %x", 
				zone->name, zone->pageArray, zone->pageArray + zone->pageTotalCount, page);
			
			//从链表中删除
			ListDel(&page->list);

			//计算出page在内存页数组中的偏移
			index = page - zone->pageArray;
			/*
			printk(" |- __PagesAlloc: zone %s pages:%d index:%d \norder:%d area:%x page %x\n", 
					zone->name, zone->pageTotalCount,  index, new_order, area, page); 
			 */
			/* 不是最大的order才标记 */
			if (new_order != MAX_ORDER -1)
				PAGE_MARK_USED(index, order, area); //在area中把它标记为使用
			
			// 统计空间页的使用数量
			zone->pageFreeCount -= 1UL << order;
			zone->pageUsingCount += 1UL << order;
			
			// 找到大的页，把它拆分成合适的页
			return PageExpand(zone, page, index, order, new_order, area);
		}
		//是空，就去更大的area查找
		new_order++;
		area++;
	} while (new_order < MAX_ORDER);

	//没有找到
	return NULL;
}

/* 
 * PagesAlloc - 分配多个页
 * @flags: 分配页的标志
 * @order: 分配2^order个页
 * 
 * 在order对应的area中去分配页，如果没有空闲的就去更大的order中去查找页
 * 注意，这里返回页指针
 */
PUBLIC struct Page *PagesAlloc(unsigned int flags, unsigned int order)
{
	//超出请求范围
    if (order >= MAX_ORDER) {
		return NULL;
	}
	struct Zone *zone = NULL;

	// 根据mask选择zone
	if (flags & ZONE_STATIC) {
		
		zone = ZoneGetByName(ZONE_STATIC_NAME);
	} else if (flags & ZONE_DYNAMIC) {
		
		zone = ZoneGetByName(ZONE_DYNAMIC_NAME);
	}
	
	if (zone == NULL) {
		return NULL;
	}
	//printk(" |- PagesAlloc - zone name:%s \n",zone->name);
	
	//交给核心函数执行
	return __PagesAlloc(flags, order, zone);
}

/* 
 * FreePages - 释放多个页
 * @addr: 物理页地址
 * @order: 释放2^order个页
 * 
 * 在order对应的area中去释放页，释放后查看更高的order是否可以合并
 */
PUBLIC void FreePages(unsigned int addr, unsigned int order)
{
	//printk(" |- free addr:%x\n", addr);
	if (addr != 0)
		PagesFree(PhysicAddressToPage(addr), order);
}

/* 
 * __PagesFree - 释放页多个页
 * @page: 第一个页的指针
 * @order: 释放2^order个页
 * @zone: 在哪个空间操作
 * 
 * 在order对应的area中去释放页，释放后查看更高的order是否可以合并
 */
PRIVATE void __PagesFree(struct Page *page, unsigned int order, struct Zone *zone)
{
	//计算page在pageArray中的偏移
	unsigned int pageIdx = page - zone->pageArray;
	//计算page在order中位图的索引
	unsigned int index = pageIdx >> (1 + order);

	unsigned int mask = (~0UL) << order;

	/* 页的索引出错 */
	if (pageIdx & ~mask) {
		Panic("page idx error!\n");
	}

	//获取order对应的area
	struct FreeArea *area = zone->freeArea + order;

	/*
	printk("order:%d area:%x page idx:%d index:%d mask:%x\n", \
		order, area, pageIdx, index, mask);
	*/
	struct Page *buddy1;
	struct Page *buddy2;
	
	// 统计空间页的使用数量
	zone->pageFreeCount += 1 << mask;
	zone->pageUsingCount -= 1 << mask;
	

	while (mask + (1 << (MAX_ORDER - 1))) {
		if (area >= zone->freeArea + MAX_ORDER) {
			Panic("area error!\n");
		}
		
		//检测取反前是否为0，如果是0，那么取反后为1，就不能释放了
		if (!BitmapTestAndChange(&area->map, index)) {
			break;
		}
		//根据算法获取buddy的指针
		buddy1 = zone->pageArray + (pageIdx ^ -mask);
		buddy2 = zone->pageArray + pageIdx;
		if (ZONE_BAD_RANGE(zone, buddy1))
			Panic("zone bad range-> zone %s start %x end %x page %x", 
				zone->name, zone->pageArray, zone->pageArray + zone->pageTotalCount, buddy1);
		
		if (ZONE_BAD_RANGE(zone, buddy2))
			Panic("zone bad range-> zone %s start %x end %x page %x", 
				zone->name, zone->pageArray, zone->pageArray + zone->pageTotalCount, buddy2);
		
		//NOTE:检查buddy的值是否符合范围

		//printk("->buddy:%x ", buddy1);

		//printk("->ListDel:%x ", &buddy1->list);
		//printk(" |- order %d buddy %x area %x index %d total %d\n", order, buddy1, area, index, zone->pageTotalCount);

		//printk("buddy addr %x\n", PageToPhysicAddress(buddy1));
		// 把buddy从对应的area中删除，等会儿合并到更高的order中去
		
		//printk("list %x\n", &buddy1->list);

		/* 在队列中才进行删除 */
		//if (ListFind(&buddy1->list, &area->pageList)) {
		ListDel(&buddy1->list);
			//printk("find \n");
		//}
		//向上移动order
		//order++;
		//area 也改变成更高的
		area++;
		//index也移动到更高级的order，变成更高的area中的map的索引
		index >>= 1;
		
		/* 
		这是一种计算map和页关系的算法。具体我也没明白，
		但我知道这样算就可以能够得出结果。2019/7/4注
		*/
		//mask指向更高的order
		mask <<= 1;
		//计算在页表中的位置
		pageIdx &= mask;
	}

	// 之前做了尽可能多的合并，把最终索引到的页添加到最后的以后area
	ListAdd(&(zone->pageArray + pageIdx)->list, &area->pageList);
	//printk("add to:%d addr:%x\n", pageIdx, zone->pageArray + pageIdx);

	
}

/* 
 * PagesFree - 释放页多个页
 * @page: 第一个页的指针
 * @order: 释放2^order个页
 * 
 * 在order对应的area中去释放页，释放后查看更高的order是否可以合并
 */
PUBLIC void PagesFree(struct Page *page, unsigned int order)
{
	//超出请求范围
    if (order >= MAX_ORDER || page == NULL) {
		return;
	}
	
	// 通过页找到zone
	struct Zone *zone = ZoneGetByPage(page);
	if (zone == NULL) 
		return;
	//交给核心函数执行
	__PagesFree(page, order, zone);
}

/**
 * GetPagesOrder - 获取页的order
 * @pages: 需要去获取order的页数
 * 
 * 成功返回order，失败返回-1
 */
PUBLIC int GetPagesOrder(uint32_t pages)
{
	
	/* 如果传入的页数不为正，或者页数超过最大可分配页数，就失败 */
	if (pages <= 0 || pages > MAX_ORDER_PAGE_NR)
		return -1;

	int i;
	for (i = 0; i < MAX_ORDER; i++) {
		/* 找到一个order的总页数大于我们需求的页数 */
		if (pow(2, i) >= pages) {
			return i;
		}
	}

	/* 理论上不会到这里 */
	return -1;
}

/*
 * PageLinkAddress - 物理地址和虚拟地址链接起来
 * @virtualAddr: 虚拟地址
 * @physicAddr: 物理地址
 * @flags: 分配页的标志
 * @prot: 页的保护
 * 
 * 把虚拟地址和物理地址连接起来，这样就能访问物理地址了
 */
PUBLIC int PageLinkAddress(address_t virtualAddr, 
		address_t physicAddr, flags_t flags, unsigned int prot)
{
	pde_t *pde = PageGetPde(virtualAddr);
	
	//struct Page *page;
	
	//printk(PART_TIP "page vir %x pde %x\n", virtualAddr, pde);
	
	if (*pde & PAGE_P_1) {
		// 页目录项存在
		//printk(PART_WARRING "page dir entry exist %x.\n", *pde);
	
		pte_t *pte = PageGetPte(virtualAddr);
		
		//printk(PART_TIP "page pte %x\n", pte);

		// 理论上说页表是不存在的，但是如果出现内存覆盖就有可能
		if (!(*pte & PAGE_P_1)) {
			// 页表不存在
			// 填入对应的地址
			*pte = physicAddr | prot | PAGE_P_1;
			//printk(PART_WARRING "page not exist.\n");
		} else {
			//printk(PART_WARRING "PageLinkAddress: vir %x page %x already exists\n", virtualAddr, *pte & PAGE_MASK);
			
			//printk(PART_TIP "PageLinkAddress: page attr %x\n", *pte & PAGE_INSIDE);
			
			// 修改页属性
			//*pte |= prot;
			// 直接映射，实际上还应该把之前的释放掉
			/*
			uint32_t paddr = *pte & PAGE_MASK;
			printk(PART_TIP "PageLinkAddress: paddr %x\n", paddr);
			
			FreePage(paddr);
			*/
			*pte |= (prot | PAGE_P_1);
			return 1;
			
		}
	} else {
		//printk(PART_WARRING "page dir entry not exist.\n");
	
		/* 没有页目录项就创建一个，这里的页表用静态内存，
		这样就可以直接获取到它的虚拟地址*/ 
		address_t pageTableAddr = (address_t)GetFreePage(GFP_STATIC);
		if (!pageTableAddr)
			return -1;
		//printk(PART_TIP "page table paddr %x\n", pageTableAddr);
	
		// 写入页目录项
		*pde = pageTableAddr | prot | PAGE_P_1;
		
		pte_t *pte = PageGetPte(virtualAddr);
		// 写入页表项
		*pte = physicAddr | prot | PAGE_P_1;
		//printk(PART_TIP "page table and page not exist.\n");
	}
	
	/*
	// 把物理地址转换成页
	page = PhysicAddressToPage(physicAddr);
	// 填写虚拟地址
	page->virtual = virtualAddr;
	*/
	// printk(PART_TIP "link page %x phy %x vir %x\n", page, physicAddr, virtualAddr);
	return 0;
}


/*
 * MapPages - 把页进行映射
 * @start: 起始虚拟地址
 * @len: 要映射多大的空间
 * @flags: 分配页的标志
 * @prot: 页的保护
 * 
 * 成功返回0，失败返回-1
 * 最大只接受2MB的内存映射
 */
PUBLIC int MapPages(uint32_t start, uint32_t len, 
		flags_t flags, unsigned int prot)
{
    // 长度和页对齐
    len = PAGE_ALIGN(len);

	/* 判断长度是否大于单个order的最大值2MB */
	if (len > MAX_ORDER_MEM_SIZE) {
		len = MAX_ORDER_MEM_SIZE;
	}

	/* 获取页对应的order */
	int order = GetPagesOrder(len/PAGE_SIZE);
	if (order == -1)
		return -1;
	/* 分配物理页 */
	uint32_t paddr = GetFreePages(flags, order);

	if (!paddr)
		return -1;

    uint32_t end = start + len;

	while (start < end)
	{
		// 对单个页进行链接
		PageLinkAddress(start, paddr, flags, prot);
		start += PAGE_SIZE;
        paddr += PAGE_SIZE;
	}
	return 0;
}


/*
 * PageUnlinkAddress - 取消虚拟地址对应的物理链接
 * @virtualAddr: 虚拟地址
 * 
 * 取消虚拟地址和物理地址链接。
 * 在这里我们不删除页表项映射，只删除物理页，这样在以后映射这个地址的时候，
 * 可以加速运行。但是弊端在于，内存可能会牺牲一些。
 * 也消耗不了多少，4G内存才4MB。
 */
PUBLIC address_t PageUnlinkAddress(address_t virtualAddr)
{
	pte_t *pte = PageGetPte(virtualAddr);
	address_t physicAddr;

	//printk(PART_TIP "virtual %x pte %x\n", virtualAddr, *pte);

	// 去掉属性部分获取物理页地址
	physicAddr = *pte & PAGE_ADDR_MASK;

	// 如果页表项存在物理页
	if (*pte & PAGE_P_1) {
		// 清除页表项的存在位，相当于删除物理页
		//*pte &= ~PAGE_P_1;
		*pte = 0;
		
		//更新tlb，把这个虚拟地址从页高速缓存中移除
		X86Invlpg(virtualAddr);
		
		// 把物理地址转换成页
		struct Page *page = PhysicAddressToPage(VirtualToPhysic(virtualAddr));
		// 如果页存在，那么就把虚拟地址取消
		if (page) {
			page->virtual = 0;
		}
			
	}
	// 返回物理页地址
	return physicAddr;
}

/**
 * UnmapPages - 取消页映射
 * @vaddr: 虚拟地址
 * @len: 内存长度
 */
PUBLIC int UnmapPages(unsigned int vaddr, unsigned int len)
{
	if (!len)
		return -1;
	
	len = PAGE_ALIGN(len);
	
	/* 判断长度是否大于单个order的最大值2MB */
	if (len > MAX_ORDER_MEM_SIZE) {
		len = MAX_ORDER_MEM_SIZE;
	}
	
	unsigned int end = vaddr + len;
	unsigned int paddr, firstPhyAddr = 0;
	while (vaddr < end)
	{
		paddr = PageUnlinkAddress(vaddr);
		// 保存首地址
		if (!firstPhyAddr)
			firstPhyAddr = paddr;

		vaddr += PAGE_SIZE;
	}
	/* 获取页对应的order */
	int order = GetPagesOrder(len/PAGE_SIZE);
	if (order == -1)
		return -1;
	
	// 释放物理页
	FreePages(firstPhyAddr, order);
	return 0;
}

PRIVATE uint32_t ExpandStack(struct VMSpace* space, uint32_t addr)
{
	// 地址和页对其
    addr &= PAGE_MASK;

	//addr -= PAGE_SIZE;
    
	// 修改为新的空间开始
	space->start = addr;
    return 0;
}

/**
 * MakePteWrite - 让pte有写属性
 * @addr: 要设置的虚拟地址
 */
PRIVATE int MakePteWrite(unsigned int addr)
{
	if (addr > USER_VM_SIZE)
		return -1;

	pde_t *pde = PageGetPde(addr);
	pte_t *pte = PageGetPte(addr);
	
	/* 虚拟地址的pde和pte都要存在才能去设置属性 */
	if (!(*pde & PAGE_P_1))
		return -1;

	if (!(*pte & PAGE_P_1))
		return -1;
	
	/* 标记写属性 */
	*pte |= PAGE_RW_W;
	return 0;
}

/**
 * DoVMAreaFault - 内核中的vmarea映射故障
 * @addr: 虚拟地址
 * 
 */
PRIVATE int DoVMAreaFault(uint32_t addr)
{
	Panic(PART_ERROR "# segment fault!\n");
	return 0;
}

/**
 * DoHandleNoPage - 处理没有物理页
 * @addr: 虚拟地址
 * 
 * 执行完后虚拟地址就可以访问了
 */
PRIVATE int DoHandleNoPage(uint32_t addr)
{
	// 分配一个物理页
	address_t paddr = GetFreePage(GFP_DYNAMIC);
	if (!paddr) {
		return -1;
	}

	/* 因为要做页映射，所以地址必须的页对齐 */
	addr &= PAGE_MASK;
	// 页链接，从动态内存中分配一个页并且页保护是 用户，可写
	if (PageLinkAddress(addr, paddr, GFP_DYNAMIC, PAGE_US_U | PAGE_RW_W)) {
		printk(PART_TIP "PageLinkAddress: vaddr %x paddr %x failed!\n", addr, paddr);
		return -1;	
	}
	//printk(PART_TIP "alloc and map pages\n");
	return 0;
}

/**
 * DoProtectionFault - 执行页保护异常
 * @space: 所在空间
 * @addr: 页地址
 * @write: 写标志
 */
PRIVATE int DoProtectionFault(struct VMSpace* space, address_t addr, uint32_t write)
{
    //printk(PART_TIP "handle protection fault, addr: %x\n", addr);

	/* 没有写标志，说明该段内存不支持内存写入，就直接返回吧 */
	if (write) {
		//printk(PART_TIP "have write protection\n");
		
		/* 只有设置写属性正确才能返回 */
		int ret = MakePteWrite(addr);
		if (ret)
			return -1;
		
		/* 虽然写入的写标志，但是还是会出现缺页故障，在此则处理一下缺页 */
		if (DoHandleNoPage(addr))
			return -1; 

		return 0;
	} else {
		//printk(PART_TIP "no write protection\n");
		
	}
	Panic(PART_ERROR "# protection fault!\n");
	return -1;
}

/**
 * DoPageFault - 页故障处理
 * @frame: 中断栈框
 * 
 * 错误码的内容 
 * bit 0: 0 no page found, 1 protection fault
 * bit 1: 0 read, 1 write
 * bit 2: 0 kernel, 1 user
 */
PUBLIC int DoPageFault(struct TrapFrame *frame)
{
	struct Task *current = CurrentTask();
	// printk(PART_TIP "task %s is in page fault\n", current->name);
	
    address_t addr = 0xffffffff;
	// 获取发生故障的地址
	addr = ReadCR2();

    //printk(PART_TIP "page fault addr %x, errorCode %x\n", addr, frame->errorCode);

	//Panic("DoPageFault");

	// 在虚拟空间中查找这个地址
    struct VMSpace* space = FindVMSpace(current->mm, addr);
	
	if (addr >= USER_VM_SIZE) {
		
		/* 内核中引起的页故障 */
		if (!(frame->errorCode & PAGE_ERR_USER)) {
			DoVMAreaFault(addr);
		}

	}

	/* 没有找到空间或者是越界 */
    if (space == NULL || space->start > addr) {
		
        // 栈向下扩展的标志
		uint32 expandStack = 0;

		//printk(PART_TIP "no space or neet expand stack\n");
		// 如果是用户空间的页故障
        if (frame->errorCode & PAGE_ERR_USER) {
			//printk(PART_TIP "is user level page fault\n");
			
			// 如果是栈空间，是向下越界，并且栈的大小不能超过8MB
            if (space != NULL && 
				(space->flags & VMS_STACK) && 
				(space->end - space->start) < MAX_VMS_STACK_SIZE && 
				(addr + 32 >= frame->esp)) {
				//printk(PART_TIP "have space and space is VMS_STACK\n");
		
				/* 
				如果是esp指针小于当前栈空间的起始位置+32，就说明我们需要扩展栈
				来保证我们运行正常
				*/
				//printk(PART_TIP "expand stack\n");
				ExpandStack(space, addr);
				expandStack = 1;
			
            } else {
				//printk(PART_TIP "no space or not VMS_STACK\n");
		
				// 是段故障
                Panic(PART_ERROR "$ segment fault, addr: %x!\n", addr);
            }
        }
		/*如果没有栈扩展， 也不是用户空间里面的内存，
		可能是内核中的某个地方没有做虚拟地址链接导致
		如：vmalloc
		 */ 
        if (!expandStack) {
			//printk(PART_TIP "no space and not expand stack\n");
		
			// Panic(PART_ERROR "# segment fault, addr: %x!\n", addr);
			
			// 如果没有找到这个地址就不是VMSpace中引起的错误
			
			/* 内核中引起的页故障 */
			if (!(frame->errorCode & PAGE_ERR_USER)) {
				DoVMAreaFault(addr);
			}
            return -1;
        }
    }

	// 找到这个地址所在的空间
    if (space != NULL && space->start <= addr) {
		//printk(PART_TIP "found space and addr is right\n");
		
		/* 如果是保护故障 */
		if (frame->errorCode & PAGE_ERR_PROTECT) {
			//printk(PART_TIP "it is protection\n");

			//printk(PART_TIP "cs %x eip %x esp %x\n", frame->cs, frame->eip, frame->esp);
			//Panic("DoProtectionFault");
			/* 执行保护故障操作 */
			return DoProtectionFault(space, addr, (uint32_t)(frame->errorCode & PAGE_ERR_WRITE));
		}

		/* 没有映射物理页，
		通常是mmap之后访问内存导致
		扩展栈页之后也可能到达这儿
		 */
		//printk(PART_TIP "handle no page, addr: %x\n", addr);

		if (DoHandleNoPage(addr))
			return -1; 
        
    }
    return 0;
}


PUBLIC uint32_t PageAddrV2P(uint32_t vaddr)
{
	pte_t* pte = PageGetPte(vaddr);
	/* 
	(*pte)的值是页表所在的物理页框地址,
	去掉其低12位的页表项属性+虚拟地址vaddr的低12位
	*/
	return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}
