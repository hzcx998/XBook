/*
 * file:		kernel/mm/zone.c
 * auther:	    Jason Hu
 * time:		2019/7/1
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <config.h>
#include <book/debug.h>
#include <zone.h>
#include <page.h>
#include <ards.h>
#include <bootmem.h>
#include <share/string.h>
#include <book/hal.h>
#include <share/math.h>

struct Zone zoneOfMemory;

/* 空间数组 */
struct Zone zoneTable[MAX_ZONE_NR];

/* 
 * ZoneSetInfo - 设置空间的基础信息
 */
PRIVATE void ZoneSetInfo(unsigned int idx, address_t vstart, size_t vsize,
        address_t pstart, size_t psize, unsigned int totalPages)
{
    zoneTable[idx].virtualStart = vstart;
    zoneTable[idx].virtualLength = vsize;
    zoneTable[idx].virtualEnd = vstart + vsize;

    zoneTable[idx].physicStart = pstart;
    zoneTable[idx].physicLength = psize;
    zoneTable[idx].physicEnd = pstart + psize;

    zoneTable[idx].pageTotalCount = totalPages;
    zoneTable[idx].pageFreeCount = totalPages;
    zoneTable[idx].pageUsingCount = 0;
}
/* 
 *打印空间的信息 
 */
PRIVATE void ZonePrint()
{
    int i;
    for (i = 0; i < MAX_ZONE_NR; i++) {
        #ifdef CONFIG_ZONE_DEBUG
        printk(" |- zone range type:%d vstart:%x vlength:%x vend:%x\n -- pstart:%x plength:%x pend:%x \n",
            i,
            zoneTable[i].virtualStart,
            zoneTable[i].virtualLength,
            zoneTable[i].virtualEnd,
            zoneTable[i].physicStart,
            zoneTable[i].physicLength,
            zoneTable[i].physicEnd);
        #endif

    }
}

/*
 * ZoneGetByType - 通过获取一个空间
 * @name: 空间的名字
 */
PUBLIC struct Zone *ZoneGetByType(int type)
{
    if (type < 0 || type >= MAX_ZONE_NR) {
        return NULL;
    }
    return &zoneTable[type];    
}

/*
 * ZoneGetTotalPages - 通过获取一个空间的页的数量
 * @name: 空间的名字
 */
PUBLIC INLINE unsigned int ZoneGetTotalPages(int type)
{
    if (type < 0 || type >= MAX_ZONE_NR) {
        return 0;
    }
    return zoneTable[type].pageTotalCount;
}


/*
 * ZoneGetAllTotalPages - 获取所有空间的总页数量
 */
PUBLIC INLINE unsigned int ZoneGetAllTotalPages()
{
    int i;
    unsigned int pages = 0;
    for (i = 0; i < MAX_ZONE_NR; i++) {
        pages += zoneTable[i].pageTotalCount;
    }
    return pages;
}

/*
 * ZoneGetAllUsingPages - 获取所有空间的使用页数量
 */
PUBLIC INLINE unsigned int ZoneGetAllUsingPages()
{
    int i;
    unsigned int pages = 0;
    for (i = 0; i < MAX_ZONE_NR; i++) {
        pages += zoneTable[i].pageUsingCount;
    }
    return pages;
}

/* 
 * ZoneGetInitMemorySize - 获取空间初始化的时候的大小
 */
PUBLIC INLINE unsigned int ZoneGetInitMemorySize()
{
    return ZONE_PHY_STATIC_ADDR + BootMemSize();
}

/*
 * ZonePageInfoInit - 对空间对应的页结构进行初始化
 * @type: 空间的类型
 * 
 * 初始化页数组和页位图
 */
PRIVATE void ZonePageInfoInit(int type)
{
    // 获取空间
    struct Zone *zone = ZoneGetByType(type);
    
    /* ----物理页数组---- */
    zone->pageArray = (struct Page *)BootMemAlloc(zone->pageTotalCount*sizeof(struct Page));
    memset(zone->pageArray, 0, zone->pageTotalCount*sizeof(struct Page));
    #ifdef CONFIG_ZONE_DEBUG
    printk(" |- page array:%x size:%x\n", zone->pageArray, zone->pageTotalCount*sizeof(struct Page));
    #endif
    /* ----物理页位图---- */
    zone->pageMap.btmpBytesLen = zone->pageTotalCount / 8;
    zone->pageMap.bits = (unsigned char *)BootMemAlloc(zoneOfMemory.pageMap.btmpBytesLen);
    // 初始化位图
    BitmapInit(&zoneOfMemory.pageMap);
    
    #ifdef CONFIG_ZONE_DEBUG
    printk(" |- zone page map addr:%x len:%d\n",
        zone->pageMap.bits, zone->pageMap.btmpBytesLen);
    #endif
    /* 物理页数组初始化 */
}

/*
 * ZoneAreaInit - 对空间对应的区域进行初始化
 * @type: 空间的类型
 * 
 * 初始化空间区域信息
 */
PRIVATE void ZoneAreaInit(int type)
{
    // 获取空间
    struct Zone *zone = ZoneGetByType(type);
    
    //根据物理内存区域选定位图大小
    unsigned int mask = PAGE_MASK;
    size_t bitmapSize;
    address_t phyStart = zone->physicStart;

    // 预留一个页的空间，方式分配越界
    address_t phyEnd = zone->physicEnd;
    
    //printk(" |- zone area start:%x end:%x\n", phyStart, phyEnd);

    //----初始化area信息----
    int i;
    for (i = 0; i < MAX_ORDER; i++) {
        //先初始化头链表
        INIT_LIST_HEAD(&zone->freeArea[i].pageList);

        //根据物理信息计算位图大小
        mask += mask;
        
        phyEnd = (phyEnd) & mask;   //结束地址对齐

        //计算位图位数
        bitmapSize = (phyEnd - phyStart) >> (PAGE_SHIFT + i);
        //转换成字节数
        bitmapSize = (bitmapSize + 7) >> 3;
        //字节对齐
        bitmapSize = (bitmapSize + sizeof(address_t) - 1) & ~(sizeof(address_t)-1);

        //写入位图信息
        zone->freeArea[i].map.bits = (unsigned char *) BootMemAlloc(bitmapSize);
        zone->freeArea[i].map.btmpBytesLen = bitmapSize;
        
        //初始化位图
        BitmapInit(&zone->freeArea[i].map);

        //printk(" |- area start:%x bits:%x size:%x\n", phyStart, zone->freeArea[i].map.bits, bitmapSize);

        //指针后移
        phyStart += bitmapSize;
    }
}

/*
 * ZoneFreeAreaPage - 把空间页释放到area中去
 * * @type: 空间的类型
 * 
 * 释放area，往里面写入页的信息
 */
PRIVATE void ZoneFreeAreaPage(int type)
{
    // 获取空间
    struct Zone *zone = ZoneGetByType(type);
    // 物理地址
    unsigned int phyAddr = zone->physicStart;
    // 页的开头
    struct Page *page = zone->pageArray;

    // 把所有的页都添加到最大order的area
     while (phyAddr < zone->physicEnd) {
        //添加到area，新的添加到后面
        ListAddTail(&page->list ,&zone->freeArea[MAX_ORDER - 1].pageList);
        page += MAX_ORDER_PAGE_NR;
        phyAddr += MAX_ORDER_PAGE_NR*PAGE_SIZE;
    }
    #ifdef CONFIG_ZONE_DEBUG
    printk(" |- free pages int the end: %x\n", phyAddr);
    #endif
    // 之后就可以对大块进行分配和释放了。
}

/*
 * ZoneSeparate - 对内存空间进行分割
 * @memSize: 物理内存大小
 * 
 * 把物理地址和虚拟地址进行分割
 */
PRIVATE void ZoneSeparate(size_t memSize)
{
    // 对总内存进行截取，最大不能超过黑洞内存的大小
    if (memSize > ZONE_VIR_BLACK_HOLE_ADDR) {
        memSize = ZONE_VIR_BLACK_HOLE_ADDR;
    }

    //先计算位于STATIC开始的地方有多少个页
    unsigned int totalPages = (memSize-ZONE_PHY_STATIC_ADDR) / PAGE_SIZE;

    // 静态页和动态页数量
    unsigned int pagesForStatic, pagesForDynamic;

    /*
    总共页数大于空间分割间隔，就进行分割，把大于1G的部分都给动态化
    如果不是，就2分处理，把物理内存对半分
    */
    if (totalPages < ZONE_SEPARATE_MAX_PAGES) {
        // 平均分成2块，这里获取每一个块的页数
        unsigned int pagesInEveryBlock = totalPages /= 2;
        // 分成2块后剩余的页数，避免浪费
        unsigned int pagesLeft = totalPages %= 2;
        // 设置静态页数量
        pagesForStatic = pagesInEveryBlock;
        // 需要是MAX_ORDER_PAGE_NR的倍数，向下对齐
        pagesForStatic -= pagesForStatic%MAX_ORDER_PAGE_NR;
        
        // 设置动态页数量
        pagesForDynamic = pagesInEveryBlock + pagesLeft;
        // 需要是MAX_ORDER_PAGE_NR的倍数，向下对齐
        pagesForDynamic -= pagesForDynamic%MAX_ORDER_PAGE_NR;
    } else {
        // 设置静态页数量
        pagesForStatic = ZONE_STATIC_MAX_PAGES;
        // 需要是MAX_ORDER_PAGE_NR的倍数，向下对齐
        pagesForStatic -= pagesForStatic%MAX_ORDER_PAGE_NR;
        
        // 设置动态页数量,动态页是静态页之外的
        pagesForDynamic = totalPages - pagesForStatic;
        // 需要是MAX_ORDER_PAGE_NR的倍数，向下对齐
        pagesForDynamic -= pagesForDynamic%MAX_ORDER_PAGE_NR;
    }

    //统计的物理地址开始于静态区域
    address_t physicAddressStart = ZONE_PHY_STATIC_ADDR;

    // 初始化zone范围
    
    //静态物理内存占1份
    ZoneSetInfo(ZONE_TYPE_STATIC, ZONE_VIR_STATIC_ADDR, ZONE_VIR_STATIC_SIZE, 
        physicAddressStart, pagesForStatic*PAGE_SIZE, pagesForStatic);

    physicAddressStart += pagesForStatic*PAGE_SIZE;
    
    //动态物理内存占1份
    ZoneSetInfo(ZONE_TYPE_DYNAMIC, ZONE_VIR_DYNAMIC_ADDR, ZONE_VIR_DYNAMIC_SIZE,
        physicAddressStart, pagesForDynamic*PAGE_SIZE, 
        pagesForDynamic);

    // ----空间分割完毕-----

}

/* 
 * ZoneCutUesdMemory - 把已经使用的内存剪掉
 * 
 * 因为内存管理器本身要占用一定内存，在这里把它从可分配中去掉
 */
PRIVATE void ZoneCutUesdMemory()
{
    unsigned int usedMem = BootMemSize();
    #ifdef CONFIG_ZONE_DEBUG
    printk(" |- used memory size:%x bytes %d MB", usedMem, usedMem/(1024*1024));
    #endif
    // 从buddy系统中移除这些内存，因为是从前往后排列的，所以只要从前往后摘取就行了

    // 要去掉多少个最大order的页，这里向上取余
    unsigned int delCount = DIV_ROUND_UP(usedMem, (MAX_ORDER_PAGE_NR << PAGE_SHIFT));
    #ifdef CONFIG_ZONE_DEBUG
    printk(" |- memory will alloc %d order size %x\n", delCount, MAX_ORDER_PAGE_NR << PAGE_SHIFT);
    #endif
    // 把已经使用的页分配掉

    struct List *list, *next;
    // 遍历链表，从前面删除指定个数
    ListForEachSafe(list, next, &zoneTable[0].freeArea[MAX_ORDER - 1].pageList) {
        // 从链表删除
        ListDel(list);

        // 减少个数，如果为0就退出
        delCount--;
        if (!delCount)
            break;
    }
}

/*
 * ZoneGetByPage - 通过页结构得到zone空间
 * @page: 页结构
 * 
 * 返回页所在的对应的zone
 */
PUBLIC struct Zone *ZoneGetByPage(struct Page *page)
{
    struct Zone *zone = NULL;
    int i;
    // 循环查询zone
    for (i = 0; i < MAX_ZONE_NR; i++) {
        zone = &zoneTable[i];
        // 找到后跳出
        if (zone->pageArray <= page && page < zone->pageArray + zone->pageTotalCount) {
            break;
        }
    }
    return zone;
}


/*
 * ZoneGetByVirtualAddress - 通过虚拟地址得到zone空间
 * @vaddr: 虚拟地址
 * 
 * 返回虚拟地址对应的zone
 */
PUBLIC struct Zone *ZoneGetByVirtualAddress(unsigned int vaddr)
{
    struct Zone *zone = NULL;

    /* 如果是在黑洞中就返回空 */
    if (vaddr >= ZONE_VIR_BLACK_HOLE_ADDR) {
        return zone;
    }

    zone = &zoneTable[ZONE_TYPE_STATIC];

    /* 如果不是static，就一定时dynamic */
    if (!(zone->virtualStart <= vaddr && vaddr < zone->virtualEnd))
        zone = &zoneTable[ZONE_TYPE_DYNAMIC];
    
    return zone;
}


/*
 * ZoneGetByPhysicAddress - 把物理地址转换成空间
 * @paddr: 需要转换的地址
 * 
 * 如果获取失败则返回NULL
 */
PUBLIC struct Zone *ZoneGetByPhysicAddress(unsigned int paddr)
{
    struct Zone *zone = NULL;
    int i;
    // 循环查询zone
    for (i = 0; i < MAX_ZONE_NR; i++) {
        zone = &zoneTable[i];
        // 找到后跳出
        if (zone->physicStart <= paddr && paddr < zone->physicEnd) {
            break;
        }
    }
    return zone;
}
/* 
 * PhysicAddressToPage - 把物理地址转换成页结构
 * @addr: 要转换的地址
 * 
 * 如果失败返回NULL
 */
PUBLIC struct Page *PhysicAddressToPage(unsigned int addr)
{
	struct Zone *zone = NULL;
    int i;
    // 循环查询zone
    for (i = 0; i < MAX_ZONE_NR; i++) {
        zone = &zoneTable[i];
        // 找到后跳出
        if (zone->physicStart <= addr && addr < zone->physicEnd) {
            break;
        }
    }
    
    //没找到就退出
    if (zone == NULL) {
        return NULL;
    }
    // 把地址转换成也表中的索引
    unsigned int pageIndex = ((addr - zone->physicStart) >> PAGE_SHIFT);
    //printk(" |- zone name:%s index:%d\n", zone->name, pageIndex);
    // 返回页
    return zone->pageArray + pageIndex;
}

/*
 * PageToPhysicAddress - 把页结构转换成页对应的物理地址
 * @page: 页结构
 * 
 * 注意：返回的是物理内存地址，不能直接访问，需要转换成虚拟地址后才可以访问
 */
PUBLIC address_t PageToPhysicAddress(struct Page *page)
{
    struct Zone *zone = NULL;
    int i;
    // 循环查询zone
    for (i = 0; i < MAX_ZONE_NR; i++) {
        zone = &zoneTable[i];

        //printk(" |- zone :%x start:%x end:%x page:%x\n", zone, zone->pageArray, zone->pageArray + zone->pageTotalCount, page);  

        // 找到后跳出
        if (zone->pageArray <= page && page < zone->pageArray + zone->pageTotalCount) {
            break;
        }
    }
    //printk(" |- zone name:%s idx:%d\n", zone->name, page - zone->pageArray);
    //printk(" |- zone name:%s idx:%d\n", zone->name, page - zone->pageArray);

    //没找到就退出
    if (zone == NULL) {
        return 0;
    }

    //unsigned int addr = (zone->physicStart + ((page - zone->pageArray ) << PAGE_SHIFT));
    //printk(" |- PageToPhysicAddress# addr:%x base:%x\n", addr, zone->physicStart);
    
    return (zone->physicStart + ((page - zone->pageArray ) << PAGE_SHIFT));
}

/*
 * VirtualToPhysic - 把虚拟地址转换成物理地址
 * vaddr: 要转换的虚拟地址
 * 
 * 注意：如果转换失败则返回0
 */
PUBLIC address_t VirtualToPhysic(unsigned int vaddr)
{
    // 通过地址找到空间
    struct Zone *zone = ZoneGetByVirtualAddress(vaddr);

    if (zone == NULL) {
        return -1;
    }

    unsigned int physicAddr = 0;

    int type = zone - zoneTable;

    // 根据不同的zone选择不同的方法
    if (type == ZONE_TYPE_STATIC) {
        // 静态是直接映射的，所以只要把虚拟地址减去内核虚拟起始地址就行了。
        physicAddr = vaddr - ZONE_VIR_ADDR_OFFSET;
    } else {
        // 动态的话就需要在当前页目录表中转换。
        physicAddr = PageAddrV2P(vaddr);
    }

    return physicAddr;
}

/*
 * PhysicToVirtual - 把物理地址转换成虚拟地址
 * paddr: 要转换的物理地址
 * 
 * 注意：如果转换失败则返回0
 */
PUBLIC address_t PhysicToVirtual(unsigned int paddr)
{
    // 通过地址找到空间
    struct Zone *zone = ZoneGetByPhysicAddress(paddr);

    if (zone == NULL) {
        return -1;
    }
    
    unsigned int virtualAddr = 0;
    //printk(" |- PhysicToVirtual# zone name:%s \n", paddr, zone->virtualStart);
    
    int type = zone - zoneTable;

    // 根据不同的zone选择不同的方法
    if (type == ZONE_TYPE_STATIC) {
        // 静态是直接映射的，所以只要把虚拟地址加上内核虚拟起始地址就行了。
        virtualAddr = paddr + ZONE_VIR_ADDR_OFFSET;
        //printk(" |- PhysicToVirtual# paddr:%x base:%x \n", paddr, zone->virtualStart);
    } else {
        // 先把地址转换成页，再获取页里面保存的虚拟地址

        // 先获取页在数组中的索引
        unsigned int pageIndex = ((paddr - zone->physicStart) >> PAGE_SHIFT);
        
        // 获取物理页结构
        struct Page *page = zone->pageArray + pageIndex;
        if (!page) 
            return -1;
        virtualAddr = page->virtual;

        // printk("page idx %d addr %x vir %x phy %x\n", pageIndex, page, virtualAddr, PageToPhysicAddress(page));
    }

    return virtualAddr;
}
/*
 * ZoneBuddySystemTest - 内存系统测试
 */
PRIVATE void ZoneBuddySystemTest()
{
    int i, j;
    unsigned int paddr, *vaddr, phy;
    
    for (i = MAX_ORDER - 1; i >= 0; i--) {
        paddr =  GetFreePages(ZONE_STATIC, i);
        
        vaddr = (unsigned int *)PhysicToVirtual(paddr);

        phy = VirtualToPhysic((unsigned int)vaddr);
        
        printk(" |- phy addr:%x vir addr:%x phy:%x %d\n", paddr, vaddr, phy, i);
        
        *vaddr = 0x12345678;
        vaddr[1023] = 0x545ad;

        if (paddr != 0) {
            FreePages(paddr, i);
        }
    }
    Spin("ZoneBuddySystemTest");
    unsigned int size = 0;
    for (i = 0; i < MAX_ORDER; i++) {
        paddr =  GetFreePages(ZONE_STATIC, i);

        vaddr = (unsigned int *)PhysicToVirtual(paddr);

        phy = VirtualToPhysic((address_t)vaddr);

        for (j = 0; j < i; j++) {
            if (j == 0) {
                size = 1;
            } else {
                size *= 2;
            }
        }
        printk(" |- phy addr:%x vir addr:%x phy:%x %d size:%d\n", paddr, vaddr, phy, i, size);
        memset(vaddr, 0, size*PAGE_SIZE);

        if (paddr != 0) {
            FreePages(paddr, i);
        }
    }

    for (i = 0; i < MAX_ORDER; i++) {
        paddr =  GetFreePages(ZONE_DYNAMIC, i);

        printk(" |- phy addr:%x\n", paddr);
        if (paddr != 0) {
            FreePages(paddr, i);
        }
    }
 
}

/* 
 * InitZone - 初始化内存空间
 * 
 * 这是内存管理的基础部分
 */
PUBLIC void InitZone()
{
    PART_START("Zone");
    
    //----获取内存大小----
    size_t memSize;
    //打开设备
    HalOpen("ram");
    //从设备获取信息
    HalIoctl("ram", RAM_HAL_IO_MEMSIZE, (unsigned int)&memSize);
    #ifdef CONFI_ZONE_DEBUG
    printk("\n |- memory size from ram hal:%x\n", memSize);
    #endif

    if (memSize < RAM_HAL_BASIC_SIZE) 
        Panic("Sorry! Your computer memory is %d MB,please make sure your computer memory is at least %d MB.\n", 
        memSize/MB, RAM_HAL_BASIC_SIZE/MB);

    // ----根据物理地址计算物理内存分配----
    ZoneSeparate(memSize);

    // ----显示zone 范围----
    ZonePrint();

    /*
	因为在引导的时候就已经把0~4MB映射好了，所以可以直接使用这里面的内存
    页映射,我们需要从最开始映射到静态结束
	*/
    InitPageEnvironment(ZONE_PHY_DMA_ADDR, zoneTable[0].physicEnd);

    // ----初始化static的空间页和area信息----
    ZonePageInfoInit(ZONE_TYPE_STATIC);
    ZoneAreaInit(ZONE_TYPE_STATIC);
    ZoneFreeAreaPage(ZONE_TYPE_STATIC);
    
    // ----初始化dynamic的空间页和area信息----
    ZonePageInfoInit(ZONE_TYPE_DYNAMIC);
    ZoneAreaInit(ZONE_TYPE_DYNAMIC);
    ZoneFreeAreaPage(ZONE_TYPE_DYNAMIC);
    
    /* 还需要注意一点，因为存放buddy系统信息也要占用一定空间，
    所以我们需要把占用的空间腾出来。直接最简单的方法就是在buddy
    中直接摘取相应的大小空间。
     */
    ZoneCutUesdMemory();

    #ifdef CONFIG_ZONE_DEBUG
    printk(" |- bootmem:%x \n", BootMemPosition());
    #endif
    //ZoneBuddySystemTest();
   
    PART_END();
}
