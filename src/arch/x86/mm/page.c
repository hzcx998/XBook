/*
 * file:		arch/x86/mm/page.c
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <config.h>
#include <page.h>
#include <ards.h>
#include <x86.h>
#include <phymem.h>
#include <share/stdint.h>
#include <book/bitmap.h>
#include <share/string.h>
#include <book/debug.h>
#include <share/math.h>
#include <book/vmspace.h>
#include <book/task.h>
#include <book/signal.h>

EXTERN struct MemNode *memNodeTable;
/* 节点数量 */
EXTERN unsigned int memNodeCount;
EXTERN unsigned int memNodeBase;


PUBLIC unsigned long Vir2Phy(void *address)
{
    return __PA(address);
}

PUBLIC void *Phy2Vir(unsigned long address)
{
    return __VA(address);
}

PUBLIC unsigned int AllocPages(unsigned int count)
{
    struct MemNode *node = GetFreeMemNode();
    if (node == NULL)
        return 0;
    
    int index = node - memNodeTable;

    /* 如果分配的页数超过范围 */
    if (index + count > memNodeCount)
        return 0; 

    /* 第一次分配的时候设置引用为1 */
    node->reference = 1;
    node->count = count;
    node->flags = 0;
    node->memCache = NULL;
    node->group = NULL;

    return MemNode2Page(node);
}

int FreePages(unsigned int page)
{
    struct MemNode *node = Page2MemNode(page);

    if (node == NULL)
        return -1;
    
	if (node->reference) {
		node->reference = 0;
		node->count = 0;
		node->flags = 0;
		node->memCache = NULL;
		node->group = NULL;
		//printk("free page at %x\n", page);
	}
    return 0;
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
PUBLIC int PageTableAdd(unsigned int virtualAddr,
		unsigned int physicAddr,
        unsigned int protect)
{
	pde_t *pde = PageGetPde(virtualAddr);
	
	if (!(*pde & PAGE_P_1)) {
		
		unsigned int pageTableAddr = AllocPages(1);
		if (!pageTableAddr) {
			printk("alloc page table failed!\n");
			return -1;
		}
		/* 填写页表 */
		*pde = pageTableAddr | protect | PAGE_P_1;
	}
	pte_t *pte = PageGetPte(virtualAddr);

	/* 填写物理页 */
	*pte = physicAddr | protect | PAGE_P_1;

	//printk(PART_TIP "add page %x->%x\n", virtualAddr, physicAddr);
	return 0;
}

PUBLIC int MapPages(unsigned int start,
    unsigned int len, 
    unsigned int protect)
{
    // 长度和页对齐
    len = PAGE_ALIGN(len);

	/* 判断长度是否超过剩余内存大小 */
    
    //printk("len %d pages %d\n", len, len / PAGE_SIZE);

	/* 分配物理页 */
	unsigned int paddr = AllocPages(len / PAGE_SIZE);

	if (!paddr) {
        printk("map pages get bad bpages!\n");
        return -1;
    }
	
	//printk("map pages:%x->%x len %x\n", start, paddr, len);
	
    unsigned int end = start + len;

	while (start < end)
	{
        //printk("map pages:%x->%x\n", start, paddr);
		// 对单个页进行链接
		PageTableAdd(start, paddr, protect);
		//fill_vir_page_talbe(start, paddr, protect);
		
		start += PAGE_SIZE;
        paddr += PAGE_SIZE;
	}

	return 0;
}

/**
 * MapPagesMaybeMapped - 映射虚拟地址到物理页
 * @start: 开始地址
 * @npages: 页的数量
 * @protect: 页保护属性
 * 
 * 虚拟地址可能已经映射过了，那就不映射，如果没有映射过，才会进行映射
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int MapPagesMaybeMapped(unsigned int start,
    unsigned int npages, 
    unsigned int protect)
{
    /* 为进程分配内存 */
    uint32_t pageIdx = 0;
    uint32_t vaddr = start;
    
    pde_t *pde;
    pte_t *pte;
    /*
    这里运行的时候，选择根据页映射的方式来进行重新映射。
    而不是重新从0开始映射，既可以提高映射效率，又可以
    节约资源。不失为一个好方法。
    */
    while (pageIdx < npages) {
        // 获取虚拟地址的页目录项和页表项
        pde = PageGetPde(vaddr);
        pte = PageGetPte(vaddr);
        
        /* 如果pde不存在，或者pte不存在，那么就需要把这个地址进行映射
        pde的判断要放在pte前面，不然会导致pde不存在而去查看pte的值，产生页故障*/
        if (!(*pde & PAGE_P_1) || !(*pte & PAGE_P_1)) {
            //printk(PART_WARRING "addr %x not maped!\n");
            // 分配一个物理页
            uint32_t page = AllocPage();
            if (!page) {
                printk(PART_ERROR "MapPagesMaybeMapped: GetFreePage for link failed!\n");
                return -1;
            }
            // 对地址进行链接，之后才能访问虚拟地址 
            if(PageTableAdd(vaddr, page, protect)) {
                printk(PART_ERROR "MapPagesMaybeMapped: PageTableAdd failed!\n");
                return -1;
            }
            // printk(PART_TIP "addr %x paddr %x now maped!\n", vaddr, PageAddrV2P(vaddrPage));
        } else {
            //printk(PART_TIP "addr %x don't need map!\n", vaddr);
        }
        vaddr += PAGE_SIZE;
        pageIdx++;
    }
    return 0;
}

/*
 * RemoveFromPageTable - 取消虚拟地址对应的物理链接
 * @virtualAddr: 虚拟地址
 * 
 * 取消虚拟地址和物理地址链接。
 * 在这里我们不删除页表项映射，只删除物理页，这样在以后映射这个地址的时候，
 * 可以加速运行。但是弊端在于，内存可能会牺牲一些。
 * 也消耗不了多少，4G内存才4MB。
 */
PUBLIC unsigned int RemoveFromPageTable(unsigned int virtualAddr)
{
	pde_t *pde = PageGetPde(virtualAddr);
	
	if (!(*pde & PAGE_P_1)) {
		printk("remove page without pde!\n");
		return -1;
	}

	pte_t *pte = PageGetPte(virtualAddr);
	unsigned int physicAddr;

	//printk(PART_TIP "virtual %x pte %x\n", virtualAddr, *pte);

	// 去掉属性部分获取物理页地址
	physicAddr = *pte & PAGE_ADDR_MASK;

	// 如果页表项存在物理页
	if (*pte & PAGE_P_1) {
		//printk(PART_TIP "move page %x->%x pte %x\n", virtualAddr, physicAddr, *pte);

		// 清除页表项的存在位，相当于删除物理页
		*pte &= ~PAGE_P_1;

		//更新tlb，把这个虚拟地址从页高速缓存中移除
		X86Invlpg(virtualAddr);
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
	
	/* 判断长度是否超过剩余内存大小 */
	unsigned int end = vaddr + len;
	unsigned int paddr;

	paddr = Vir2PhyByTable(vaddr);

	// 释放物理页
	FreePages(paddr);

	//printk("unmap pages:%x->%x len %x\n", vaddr, paddr, len);
	while (vaddr < end)
	{
		//printk(">>>");
		RemoveFromPageTable(vaddr);
       	//clean_vir_page_table(vaddr);
		
		//更新tlb，把这个虚拟地址从页高速缓存中移除
		X86Invlpg(vaddr);

		vaddr += PAGE_SIZE;
	}
	
	return 0;
}

/**
 * UnmapPagesFragment - 取消页映射，页之间可能不是连续的
 * @vaddr: 虚拟地址
 * @len: 内存长度
 */
PUBLIC int UnmapPagesFragment(unsigned int vaddr, unsigned int len)
{
	if (!len)
		return -1;
	
	len = PAGE_ALIGN(len);
	
	/* 判断长度是否超过剩余内存大小 */
	unsigned int end = vaddr + len;
	unsigned int paddr;
	while (vaddr < end)
	{
		paddr = RemoveFromPageTable(vaddr);

		/* 检测每一个页的时候，尝试释放页
		这样，再这个范围内的所有页都有可能会被释放掉
		*/
		if (paddr != -1)
			FreePages(paddr);

		vaddr += PAGE_SIZE;
	}

	return 0;
}

PRIVATE uint32_t ExpandStack(struct VMSpace* space, uint32_t addr)
{
	// 地址和页对其
    addr &= PAGE_MASK;

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
    /* 抛出一个段故障信号 */
    printk(PART_ERROR "# segment fault!\n");
    ForceSignal(SIGSEGV, SysGetPid());
	//Panic(PART_ERROR "# segment fault!\n");

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
	unsigned int paddr = AllocPage();
	if (!paddr) {
		return -1;
	}

	/* 因为要做页映射，所以地址必须的页对齐 */
	addr &= PAGE_MASK;
	// 页链接，从动态内存中分配一个页并且页保护是 用户，可写
	if (PageTableAdd(addr, paddr, PAGE_US_U | PAGE_RW_W)) {
		printk(PART_TIP "PageTableAdd: vaddr %x paddr %x failed!\n", addr, paddr);
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
    printk(PART_ERROR "# protection fault!\n");
    ForceSignal(SIGSEGV, SysGetPid());
	//Panic(PART_ERROR "# protection fault!\n");
    
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

		printk(PART_TIP "no space or neet expand stack\n");
		// 如果是用户空间的页故障
        if (frame->errorCode & PAGE_ERR_USER) {
			printk(PART_TIP "is user level page fault\n");
			
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
				printk(PART_TIP "no space or not VMS_STACK\n");

				// 是段故障
                printk(PART_ERROR "stack overflow, addr: %x!\n", addr);
                ForceSignal(SIGSTKFLT, SysGetPid());
                //Panic(PART_ERROR "$ segment fault, addr: %x!\n", addr);
				/*if (DoHandleNoPage(addr))
					return -1; */
            }
        }
		/*如果没有栈扩展， 也不是用户空间里面的内存，
		可能是内核中的某个地方没有做虚拟地址链接导致
		如：vmalloc
		 */
        if (!expandStack) {
			printk(PART_TIP "no space and not expand stack\n");

			// Panic(PART_ERROR "# segment fault, addr: %x!\n", addr);
			
			// 如果没有找到这个地址就不是VMSpace中引起的错误
			
			/* 内核中引起的页故障 */
			if (!(frame->errorCode & PAGE_ERR_USER)) {
				//DoVMAreaFault(addr);
                printk(PART_ERROR "$ segment fault in kernel, addr: %x!\n", addr);
                //ForceSignal(SIGSEGV, SysGetPid());
				Panic(PART_ERROR "$ segment fault, addr: %x!\n", addr);
            	return -1;
			} else {
                printk(PART_ERROR "$ segment fault in user, addr: %x!\n", addr);
                ForceSignal(SIGSEGV, SysGetPid());
                return -1;
            }
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

PUBLIC unsigned int Vir2PhyByTable(unsigned int vaddr)
{
	pte_t* pte = PageGetPte(vaddr);
	/* 
	(*pte)的值是页表所在的物理页框地址,
	去掉其低12位的页表项属性+虚拟地址vaddr的低12位
	*/
	return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}
