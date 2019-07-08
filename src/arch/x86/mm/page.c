/*
 * file:		arch/x86/mm/page.c
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

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
	//需要注意，我们要保留0~4MB的虚拟地址，因为当我们发生0地址访问时需要用到这个
	
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
PRIVATE INLINE struct Page *PageExpand(struct Zone *zone, struct Page *page, 
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
 * @gfpMask: 分配页的标志
 * @order: 分配2^order个页
 * 
 * 在order对应的area中去分配页，如果没有空闲的就去更大的order中去查找页
 * 注意，这里返回页的物理地址
 */
PUBLIC unsigned int GetFreePages(unsigned int gfpMask, unsigned int order)
{
	struct Page *page;

	page = PagesAlloc(gfpMask, order);

	if (page == NULL)
		return 0;

	struct Zone *zone = NULL;

	// 根据mask选择zone
	if (gfpMask & GFP_STATIC) {
		
		zone = ZoneGetByName(ZONE_STATIC_NAME);
	} else if (gfpMask & GFP_DYNAMIC) {
		
		zone = ZoneGetByName(ZONE_DYNAMIC_NAME);
	} else if (gfpMask & GFP_DURABLE) {
		
		zone = ZoneGetByName(ZONE_DURABLE_NAME);
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
 * @gfpMask: 分配页的标志
 * @order: 分配2^order个页
 * @zone: 页属于哪个空间
 * 
 * 在order对应的area中去分配页，如果没有空闲的就去更大的order中去查找页
 * 注意，这里返回页指针
 */
PRIVATE struct Page *__PagesAlloc(unsigned int gfpMask, unsigned int order, struct Zone *zone)
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
			//在area中把它标记为使用
			PAGE_MARK_USED(index, order, area);

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
 * @gfpMask: 分配页的标志
 * @order: 分配2^order个页
 * 
 * 在order对应的area中去分配页，如果没有空闲的就去更大的order中去查找页
 * 注意，这里返回页指针
 */
PUBLIC struct Page *PagesAlloc(unsigned int gfpMask, unsigned int order)
{
	//超出请求范围
    if (order >= MAX_ORDER) {
		return NULL;
	}
	struct Zone *zone = NULL;

	// 根据mask选择zone
	if (gfpMask & GFP_STATIC) {
		
		zone = ZoneGetByName(ZONE_STATIC_NAME);
	} else if (gfpMask & GFP_DYNAMIC) {
		
		zone = ZoneGetByName(ZONE_DYNAMIC_NAME);
	} else if (gfpMask & GFP_DURABLE) {
		
		zone = ZoneGetByName(ZONE_DURABLE_NAME);
	}
	
	if (zone == NULL) {
		return NULL;
	}
	//printk(" |- PagesAlloc - zone name:%s \n",zone->name);
	
	//交给核心函数执行
	return __PagesAlloc(gfpMask, order, zone);
}

/* 
 * FreePages - 释放多个页
 * @addr: 页地址
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

	unsigned int mask = (~0) << order;

	//获取order对应的area
	struct FreeArea *area = zone->freeArea + order;

	/* 
	printk("order:%d area:%x page idx:%d index:%d mask:%x\n", \
		order, area, pageIdx, index, mask);
	*/

	struct Page *buddy1;
	//struct Page *buddy2;
	
	while (mask + (1 << (MAX_ORDER - 1))) {
		//检测取反前是否为0，如果是0，那么取反后为1，就不能释放了
		if (!BitmapTestAndChange(&area->map, index)) {
			break;
		}
		//根据算法获取buddy的指针
		buddy1 = zone->pageArray + (pageIdx ^ -mask);
		//buddy2 = zone->pageArray + pageIdx;
		if (ZONE_BAD_RANGE(zone, buddy1))
			Panic("zone bad range-> zone %s start %x end %x page %x", 
				zone->name, zone->pageArray, zone->pageArray + zone->pageTotalCount, buddy1);
		
		//NOTE:检查buddy的值是否符合范围

		//printk("->buddy:%x ", buddy1);

		//printk("->ListDel:%x ", &buddy1->list);
		//printk(" |- order %d buddy %x area %x index %d total %d\n", order, buddy1, area, index, zone->pageTotalCount);

		// 把buddy从对应的area中删除，等会儿合并到更高的order中去
		ListDel(&buddy1->list);
		//printk("->ListDel \n");
		//向上移动order
		order++;
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
    if (order >= MAX_ORDER) {
		return;
	}
	
	// 通过页找到zone
	struct Zone *zone = ZoneGetByPage(page);
	if (zone == NULL) 
		return;
	//交给核心函数执行
	__PagesFree(page, order, zone);
}
