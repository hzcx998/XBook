/*
 * file:		arch/x86/kernel/page.c
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <page.h>
#include <ards.h>
#include <x86.h>

#include <share/stdint.h>
#include <book/bitmap.h>
#include <share/string.h>
#include <book/debug.h>

//内核和用户的内存池
struct MemoryPool kernelMemoryPool, userMemoryPool;

//内核的虚拟地址，由于用户有很多个，所以用户的虚拟地址定义在进程结构体重
struct VirtualAddr  kernelVitualAddr;

static void InitMemoryPool(uint32_t totalMem);

static uint64_t phyMemoryTotal;

/*
 * 功能: 初始化分页模式相关
 * 参数: 无
 * 返回: 成功返回0，失败返回-1
 * 说明: 对分页相关信息设置
 */
int InitPage()
{
	PART_START("Page");

	//把页目录表设置
	uint32_t *pdt = (uint32_t *)PAGE_DIR_VIR_ADDR;
	//把页目录表的第1项清空，这样，我们就只能通过高地址访问内核了
	pdt[0] = 0;
	//设置物理内存管理方式，以页框为单位
	//phyMemoryTotal = InitArds();
	//InitMemoryPool(phyMemoryTotal);
	/*
	void *a = AllocKernelPage(100);
	void *b = AllocKernelPage(1000);
	void *c = AllocKernelPage(10000);
	void *d = AllocKernelPage(10);
	printk("%x %x %x %x\n", a, b, c, d);
	
	FreeKernelPage(a,100);
	FreeKernelPage(b,1000);
	FreeKernelPage(c,10000);
	FreeKernelPage(d,10);
	*/

	PART_END();
	return 0;
}

static void InitMemoryPool(uint32_t totalMem)
{
	if (totalMem > X86_PHY_MEM_MAX) {
		totalMem = X86_PHY_MEM_MAX;
	}
	printk(" |- memory size is %x bytes %d MB.\n", totalMem, totalMem/(1024*1024));

	//计算内存
	uint32_t usedMem = FREE_PHY_START;
	uint32_t freeMem = totalMem - usedMem;

	//还有多少页可以使用
	uint32_t allFreePages = freeMem/PAGE_SIZE;

	//把空闲的一半给内核，一半给用户
	uint32_t kernelFreePages = allFreePages/2;
	uint32_t userFreePages = allFreePages - kernelFreePages;
	
	//以字节为单位的大小
	uint32_t kernelBitmapLen = kernelFreePages/8;
	uint32_t userBitmapLen = userFreePages/8;
	
	uint32_t kernelMemPoopStart = usedMem;
	//用户物理内存在内核后面
	uint32_t userMemPoopStart = kernelFreePages*PAGE_SIZE;
	
	//把数据填写到数据结构
	kernelMemoryPool.phyAddrStart = kernelMemPoopStart;
	kernelMemoryPool.poolSize = kernelFreePages*PAGE_SIZE;
	kernelMemoryPool.poolBitmap.btmpBytesLen = kernelBitmapLen;
	
	userMemoryPool.phyAddrStart = userMemPoopStart;
	userMemoryPool.poolSize = userFreePages*PAGE_SIZE;
	userMemoryPool.poolBitmap.btmpBytesLen = userBitmapLen;
	
	kernelMemoryPool.poolBitmap.bits = (void *)MEM_POOL_BITMAP_VIR_ADDR_START;
	userMemoryPool.poolBitmap.bits = (void *)(MEM_POOL_BITMAP_VIR_ADDR_START + MAX_MEM_POOL_BITMAP_SIZE);
	
	//显示信息
	/* 
	printk("  |-kernel memory pool: bitmap start:%x\n    phy addr start:%x size:%x\n", \
		kernelMemoryPool.poolBitmap.bits, kernelMemoryPool.phyAddrStart, kernelMemoryPool.poolSize);
	printk("  |-user memory pool: bitmap start:%x\n    phy addr start:%x size:%x\n", \
		userMemoryPool.poolBitmap.bits, userMemoryPool.phyAddrStart, userMemoryPool.poolSize);
	*/

	//初始化位图
	BitmapInit(&kernelMemoryPool.poolBitmap);
	BitmapInit(&userMemoryPool.poolBitmap);

	//初始化内核虚拟地址，理论上内核虚拟内存的长度是小于物理内存的长度，在这里，让他们一样
	kernelVitualAddr.vaddrBitmap.btmpBytesLen = kernelBitmapLen;
	//位图放到用户内存池后面
	kernelVitualAddr.vaddrBitmap.bits = userMemoryPool.poolBitmap.bits + MAX_MEM_POOL_BITMAP_SIZE;

	//虚拟地址的开始地址
	kernelVitualAddr.virAddrStart = KERNEL_HEAP_START;	

	/*
	printk("  |-kernel virtual addr: bitmap start:%x  vir addr start:%x\n",\
		kernelVitualAddr.vaddrBitmap.bits, kernelVitualAddr.virAddrStart);
	 */

	BitmapInit(&kernelVitualAddr.vaddrBitmap);

}

uint32_t *PagePtePtr(uint32_t vaddr)
{
	uint32_t *pte = (uint32_t *)(0xffc00000 + \
	((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr)*4);
	return pte;
}

uint32_t *PagePdePtr(uint32_t vaddr)
{
	uint32_t *pde = (uint32_t *)(0xfffff000 + \
	PDE_IDX(vaddr)*4);
	return pde;
}

uint32_t PageAddrV2P(uint32_t vaddr)
{
	uint32_t* pte = PagePtePtr(vaddr);
	/* 
	(*pte)的值是页表所在的物理页框地址,
	去掉其低12位的页表项属性+虚拟地址vaddr的低12位
	*/
	return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
}

void *AllocPageVirtualAddr(pool_flags_t pf, uint32_t pages)
{
	int bit_idx_start = -1;
	uint32_t i, vaddr_start = 0;
	if (pf == PF_KERNEL) {
		//内核虚拟地址
		bit_idx_start = BitmapScan(&kernelVitualAddr.vaddrBitmap, pages);
		
		if (bit_idx_start == -1) {
			//分配失败
			return NULL;
		}
		//设置分配的那么多个位图位为1
		for (i = 0; i < pages; i++) {
			BitmapSet(&kernelVitualAddr.vaddrBitmap, bit_idx_start + i, 1);
		}
		vaddr_start = kernelVitualAddr.virAddrStart + bit_idx_start*PAGE_SIZE;
	} else {
		//用户虚拟地址

	}
	return (void *)vaddr_start;
}

void FreePageVirtualAddr(pool_flags_t pf, void *vaddr, uint32_t pages)
{
	uint32_t bit_idx_start = 0;
	uint32_t vir_addr = (uint32_t)vaddr;
	uint32_t n = 0;
	if (pf == PF_KERNEL) {
		//内核虚拟地址
		bit_idx_start = (vir_addr - kernelVitualAddr.virAddrStart)/PAGE_SIZE;
		while (n < pages) {
			BitmapSet(&kernelVitualAddr.vaddrBitmap, bit_idx_start + n, 0);
			n++;
		}
		
	} else {
		//用户虚拟地址

	}
}

void *PoolAllocPhyMem(struct MemoryPool *mem_pool)
{
	int bit_idx = BitmapScan(&mem_pool->poolBitmap, 1);
	if (bit_idx == -1) {
		return NULL;
	}
	BitmapSet(&mem_pool->poolBitmap, bit_idx, 1);
	uint32_t page_phy_addr = mem_pool->phyAddrStart +  bit_idx*PAGE_SIZE;
	return (void *) page_phy_addr;
}

void PoolFreePhyMem(uint32_t phy_addr)
{
	struct MemoryPool *mem_pool;
	uint32_t bit_idx = 0;
	//根据地址判断是内核的地址还是用户的地址
	if (phy_addr >= userMemoryPool.phyAddrStart) {
		mem_pool = &userMemoryPool;
	} else {
		mem_pool = &kernelMemoryPool;
	}

	bit_idx = (phy_addr-mem_pool->phyAddrStart)/PAGE_SIZE;
	//把对应的位置置0，表示没有被使用
	BitmapSet(&mem_pool->poolBitmap, bit_idx, 0);
}

/*
 * 功能: 页表的虚拟地址和一个物理地址映射
 * 参数: vir_addr 	虚拟地址
 * 		phy_addr	物理地址
 * 返回: 无
 * 说明: 直接就是虚拟地址对应物理地址，而中间的页表和页目录表是自动识别
 */
void PageTableAdd(void *vir_addr, void *phy_addr)
{
	uint32_t vaddr = (uint32_t)vir_addr, paddr = (uint32_t)phy_addr;
	uint32_t *pde = PagePdePtr(vaddr);
	uint32_t *pte = PagePtePtr(vaddr);
	
	if (*pde&PAGE_P_1) {
		//页目录项存在

		//理论上说页表是不存在的
		if (!(*pte&PAGE_P_1)) {
			//页表不存在
			//填入对应的地址
			*pte = (paddr|PAGE_US_U|PAGE_RW_W|PAGE_P_1);
		} else {
			//panic("pte exist");
			*pte = (paddr|PAGE_US_U|PAGE_RW_W|PAGE_P_1);
		}

	} else {
		//没有页目录项就创建一个
		uint32_t pde_phyaddr = (uint32_t)PoolAllocPhyMem(&kernelMemoryPool);
		*pde = (pde_phyaddr|PAGE_US_U|PAGE_RW_W|PAGE_P_1);

		//把新建的页表清空
		memset((void *)((uint32_t)pte&0xfffff000), 0, PAGE_SIZE);

		*pte = (paddr|PAGE_US_U|PAGE_RW_W|PAGE_P_1);
	}
}

void PageTableRemovePTE(uint32_t vaddr)
{
	//获取页表项，并把存在位置0表明页不存在
	uint32_t *pte = PagePtePtr(vaddr);
	*pte &= ~PAGE_P_1;

	//更新tlb
	X86Invlpg(vaddr);
}

void *PageAllocMemory(pool_flags_t pf, uint32_t pages)
{
	void *vaddr_start = AllocPageVirtualAddr(pf, pages);
	if (vaddr_start == NULL) {
		return NULL;
	}
	
	uint32_t vaddr = (uint32_t)vaddr_start, n = pages;
	struct MemoryPool *mem_pool = pf&PF_KERNEL ? &kernelMemoryPool:&userMemoryPool;

	while (n-- > 0) {
		void *page_phy_addr = PoolAllocPhyMem(mem_pool);
		
		if (page_phy_addr == NULL) {
			return NULL;
		}
		
		PageTableAdd((void *)vaddr, page_phy_addr);

		vaddr += PAGE_SIZE;
	}
	return vaddr_start;
}

void PageFreeMemory(pool_flags_t pf, void *vaddr, uint32_t pages)
{
	uint32_t phy_addr;
	uint32_t vir_addr = (uint32_t )vaddr;
	uint32_t n = 0;
	phy_addr = PageAddrV2P(vir_addr);
	uint8_t kflags;	//是否是内核池

	if (phy_addr >= userMemoryPool.phyAddrStart) {
		//用户内存池
		kflags = 0;
	} else {
		//内核内存池
		kflags = 1;
	}

	while (n < pages) {
		phy_addr = PageAddrV2P(vir_addr);

		if (kflags) {
			//对应的判断

		} else {
			//对应的判断
			
		}

		PoolFreePhyMem(phy_addr);
		PageTableRemovePTE(vir_addr);

		n++;
		vir_addr += PAGE_SIZE;
	}
	FreePageVirtualAddr(pf, vaddr, pages);
}


/*
 * 功能: 分配n个内核页，返回虚拟地址
 * 参数: pages 	需要多少个页
 * 返回: NULL 失败，非NULL就是我们分配的虚拟地址
 * 说明: 有了虚拟地址分配函数后，就可以通过以页框为单位的方式编写内存管理算法
 */
void *AllocKernelPage(uint32_t pages)
{
	void *vaddr = PageAllocMemory(PF_KERNEL, pages);
	if (vaddr != NULL) {
		memset(vaddr, 0, pages*PAGE_SIZE);
	}
	return vaddr;
}

/*
 * 功能: 释放n个物理页
 * 参数: vaddr 	虚拟地址
 * 		pages 	需要多少个页
 * 返回: 无
 * 说明: 为了和AllocKernelPage对应，这里写了一个释放内核的地址
 */
void FreeKernelPage(void *vaddr, uint32_t pages)
{
	PageFreeMemory(PF_KERNEL, vaddr, pages);
}
