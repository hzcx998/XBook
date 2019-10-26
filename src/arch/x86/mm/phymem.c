/*
 * file:		kernel/mm/phymem.c
 * auther:	    Jason Hu
 * time:		2019/9/29
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <config.h>
#include <book/debug.h>
#include <page.h>
#include <ards.h>
#include <bootmem.h>
#include <share/string.h>
#include <share/math.h>
#include <phymem.h>
#include <x86.h>
#include <book/hal.h>

/*
申请一个内存节点数组，通过引用来表明是否被使用
指向内存节点数组的指针
*/
struct MemNode *memNodeTable;
/* 节点数量 */
unsigned int memNodeCount;
unsigned int memNodeBase;

/*
 * MapDirectMemory - 物理地址和虚拟地址一对一映射
 * @start: 开始物理地址
 * @end: 结束物理地址
 * 
 * 把物理和内核虚拟地址进行映射
 */
PRIVATE int MapDirectMemory(unsigned int start, unsigned int end)
{
	/* ----映射静态内存---- */
	//把页目录表设置
	uint32_t *pdt = (uint32_t *)PAGE_DIR_VIR_ADDR;
	#ifdef CONFIG_PAGE_DEBUG
	printk(" |- physic addr range %x ~ %x\n", start, end);
	#endif
	// 求出目录项数量
	unsigned int pageDirEntryNumber = (end-start)/(1024*PAGE_SIZE);

	// 求出剩余的页表项数量

	// 先获取的页数
	unsigned int pageTableEntryNumber = (end-start)/PAGE_SIZE;

	// 获取页数中余下的数量，就是页表项剩余数
	pageTableEntryNumber = pageTableEntryNumber%1024;
	
	//有多少个页目录项，这个根据静态空间来进行设置
	//unsigned int pageDirEntryNumber = DIV_ROUND_UP(end-start, 1024 * PAGE_SIZE);
	#ifdef CONFIG_PAGE_DEBUG
	printk(" |- page dir entry num:%d page table entry num:%d\n", 
			pageDirEntryNumber, pageTableEntryNumber);
	#endif
	//跳过页表区域中的第一个页表
	unsigned int *pageTablePhyAddress = (unsigned int *) (PAGE_TABLE_PHY_ADDR + 
			PAGE_SIZE*PAGE_TABLE_PHY_NR);
	#ifdef CONFIG_PAGE_DEBUG
	printk(" |- page table addr:%x phy addr:%x\n", pageTablePhyAddress, start);
	#endif
	int i, j;
	// 根据页目录项来填写
	for (i = 0; i < pageDirEntryNumber; i++) {
		//填写页目录表项
		//把页表地址和属性放进去
		pdt[512 + PAGE_TABLE_PHY_NR + i] = (address_t)pageTablePhyAddress | PAGE_P_1 | PAGE_RW_W | PAGE_US_S;
		
		for (j = 0; j < PAGE_ENTRY_NR; j++) {
			//填写页页表项

			//把物理地址和属性放进去
			pageTablePhyAddress[j] = start | PAGE_P_1 | PAGE_RW_W | PAGE_US_S;
			//指向下一个物理页
			start += PAGE_SIZE;
		}
		//指向下一个页表
		pageTablePhyAddress += PAGE_SIZE;
	}
	// 根据剩余的页表项填写

	//有剩余的我们才填写
	if (pageTableEntryNumber != 0) {
		// i 和 pageTablePhyAddress 的值在上面的循环中已经设置
		pdt[512 + PAGE_TABLE_PHY_NR + i] = (address_t)pageTablePhyAddress | PAGE_P_1 | PAGE_RW_W | PAGE_US_S;
		
		// 填写剩余的页表项数量
		for (j = 0; j < pageTableEntryNumber; j++) {
			//填写页页表项
			//把物理地址和属性放进去
			pageTablePhyAddress[j] = start | PAGE_P_1 | PAGE_RW_W | PAGE_US_S;
			
			//指向下一个物理页
			start += PAGE_SIZE;
		}
	}
	#ifdef CONFIG_PAGE_DEBUG
	printk(" |- maped to phyAddrEnd:%x to pageTablePhyAddress:%x\n", start, pageTablePhyAddress);
	#endif

	/* 在开启分页模式之后，我们的内核就运行在高端内存，
	那么，现在我们不能通过低端内存访问内核，所以，我们在loader
	中初始化的0~8MB低端内存的映射要取消掉才行。
	我们把用户内存空间的页目录项都清空 */ 
	
	for (i = 0; i < 512; i++) {
		pdt[i] = 0;
	}

	/* ----映射完毕---- */
	return 0;
}

PUBLIC struct MemNode *GetFreeMemNode()
{
    struct MemNode *node = memNodeTable;
    
    while (node < memNodeTable + memNodeCount)
    {
        /* 没有被分配就是我们需要的 */
        if (!node->reference) {
            return node;
        }
        /* 指向下一个节点位置 */
        if (node->count) 
            node += node->count;
        else 
            node++;
        
    }
    return NULL;
}

PUBLIC struct MemNode *Page2MemNode(unsigned int page)
{ 
    int index = (page - memNodeBase) >> PAGE_SHIFT;

    struct MemNode *node = memNodeTable + index;

    /* 必须是在正确的范围 */
    if (node < memNodeTable || node >= memNodeTable + memNodeCount)
        return NULL;

    return node;
}

PUBLIC unsigned int MemNode2Page(struct MemNode *node)
{ 
    int index = node - memNodeTable;

    return memNodeBase + (index << PAGE_SHIFT);
}

PUBLIC void DumpMemNode(struct MemNode *node)
{ 
    printk(PART_TIP "----Mem Node----\n");
    printk(PART_TIP "count: %d flags:%x reference:%d\n",
        node->count, node->flags, node->reference);
    if (node->memCache && node->group) {
        printk(PART_TIP "cache: %x group:%x\n",
            node->memCache, node->group);
        
    }
}

/* 
 * ZoneCutUesdMemory - 把已经使用的内存剪掉
 * 
 * 因为内存管理器本身要占用一定内存，在这里把它从可分配中去掉
 */
PRIVATE void CutUesdMemory()
{
    /* 剪切掉引导分配的空间 */
    unsigned int usedMem = BootMemSize();
    unsigned int usedPages = DIV_ROUND_UP(usedMem, PAGE_SIZE);
    AllocPages(usedPages);
}

/**
 * InitPhysicMemory - 初始化物理内存管理以及一些相关设定
 */
PUBLIC int InitPhysicMemory()
{
    PART_START("Physic Memory");
    //----获取内存大小----
    unsigned int memSize;
    //打开设备
    HalOpen("ram");
    //从设备获取信息
    HalIoctl("ram", RAM_HAL_IO_MEMSIZE, (unsigned int)&memSize);

    /* 根据内存大小划分区域
    如果内存大于1GB:
        1G预留128MB给非连续内存，其余给内核和用户程序平分，内核多余的部分分给用户
    如果内存小于1GB：
        预留128MB给非连续内存，其余平分给内核和用户程序
     */
    unsigned int normalSize;
    unsigned int userSize;
    
    normalSize = (memSize - (NORMAL_MEM_ADDR + HIGH_MEM_SIZE + NULL_MEM_SIZE)) / 2; 
    userSize = memSize - normalSize;
    if (normalSize > 1*GB) {
        unsigned int moreSize = normalSize - 1*GB;

        /* 挪出多余的给用户 */
        userSize += moreSize;
        normalSize -= moreSize;
    }
    /* 由于引导中只映射了0~8MB，所以这里从DMA开始 */
    MapDirectMemory(DMA_MEM_ADDR, NORMAL_MEM_ADDR + normalSize);
    /* 根据物理内存大小对内存分配器进行限定 */
    InitBootMem(PAGE_OFFSET + NORMAL_MEM_ADDR , PAGE_OFFSET + (NORMAL_MEM_ADDR + normalSize));
    

    /* 节点数量就是页数量 */
    memNodeCount = (normalSize + userSize)/PAGE_SIZE;
    memNodeBase = NORMAL_MEM_ADDR;

    unsigned int memNodeTableSize = memNodeCount * SIZEOF_MEM_NODE;
    
    /* 分配内存节点数组 */
    memNodeTable = BootMemAlloc(memNodeTableSize);
    if (memNodeTable == NULL) {
        Panic(PART_ERROR "boot mem alloc for mem node table failed!\n");
    }

    printk(PART_TIP "mem node table at %x size:%x %d MB\n", memNodeTable, memNodeTableSize, memNodeTableSize/MB);
    
    memset(memNodeTable, 0, memNodeTableSize);

    CutUesdMemory();
/*
    unsigned int a = AllocPages(1000);
    unsigned int b = AllocPages(2);
    unsigned int c = AllocPages(10);

    printk("a=%x,b=%x,c=%x\n", a, b, c);
    
    FreePages(c);
    FreePages(b);
    FreePages(a);

    a = AllocPages(2);
    b = AllocPages(3);
    c = AllocPages(4);

    printk("a=%x,b=%x,c=%x\n", a, b, c);
    
    void *pa = Phy2Vir(a);
    void *pb = Phy2Vir(b);
    printk("a=%x,b=%x\n", pa, pb);

    memset(pa, 1, PAGE_SIZE * 2);
    */
    /*MapPages(0xc5000000, PAGE_SIZE * 100, PAGE_RW_W | PAGE_US_S);
	
    char *v = (char *)0xc5000000;
    memset(v, 1, PAGE_SIZE * 100);

    UnmapPages(0xc5000000, PAGE_SIZE * 100);


    MapPages(0xc6000000, PAGE_SIZE * 200, PAGE_RW_W | PAGE_US_S);

    v = (char *)0xc6000000;
    memset(v, 1, PAGE_SIZE * 200);

    UnmapPages(0xc6000000, PAGE_SIZE * 200);
	*/

    //memset(v, 0, PAGE_SIZE * 2);
    //Panic("test");
    //PART_END();
    return 0;
}   
