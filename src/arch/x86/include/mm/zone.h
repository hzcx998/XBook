/*
 * file:		arch/x86/include/mm/zone.h
 * auther:		Jason Hu
 * time:		2019/7/1
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _X86_MM_ZONE_H
#define _X86_MM_ZONE_H

#include <area.h>
#include <share/stddef.h>
#include <book/bitmap.h>

/* 
 * 物理空间的划分
 */

// 0MB~1MB是体系结构相关的内存分布
#define ZONE_PHY_ARCH_ADDR          0X000000000
#define ZONE_ARCH__SIZE             0X000100000     // 1MB

// 1MB~2MB是内核镜像
#define ZONE_PHY_KERNEL_ADDR        (ZONE_PHY_ARCH_ADDR+ZONE_ARCH__SIZE)
#define ZONE_KERNEL_SIZE               0X000100000     // 1MB

// 2MB~8MB是系统重要信息的存放地址
#define ZONE_PHY_MATERIAL_ADDR      (ZONE_PHY_KERNEL_ADDR+ZONE_KERNEL_SIZE)
#define ZONE_MATERIAL_SIZE          0X000600000   // 6MB

// 8MB~16MB是设备DMA的地址
#define ZONE_PHY_DMA_ADDR           (ZONE_PHY_MATERIAL_ADDR+ZONE_MATERIAL_SIZE)
#define ZONE_DMA_SIZE               0X000800000   // 10MB

// ---- 以上是必须的基本内存使用情况 ----

// 16M以上是静态映射开始地址
#define ZONE_PHY_STATIC_ADDR           (ZONE_PHY_DMA_ADDR+ZONE_DMA_SIZE)

/* 
    我们会把16M以上的物理内存分为3部分，静态，动态，持久态。
    静态：在内核运行时就让虚拟地址和物理地址做一对一映射，
可以直接通过虚拟地址访问对应的物理地址。
分配和释放页，都是直接对地址进行操作，不需要再次做映射。
实现kmalloc和kfree部分

    动态：保留物理地址，当需要使用虚拟地址时，必须做映射，
才可以访问物理地址。可以对物理地址进行
分配和释放，这个过程都需要做页的映射操作。
实现vmalloc和vfree部分

    持久态：相对于动态的来说，没有那么频繁，一块区域分配后，
几乎就是用到系统结束，除非把某个设备卸载，
这样，会把设备对应的地址映射取消掉。
分配和释放，这个过程都需要做页的映射操作。
实现kmap和kunmap部分

    他们的位置会根据物理内存的大小做出一定的调整。
持久态的变化应该不大，因为大多数设备的内存地址映射都是已经设定好了的。
所以，我们给他一个大致的范围就好了。但是对于静态和动态这两部分，
最好做到公平起见，但是感觉静态的用的更多，而动态的也就是几个少数地方会使用，
所以，把物理内存分成2份，1份给静态，1份给动态。持久态不需要，因为他是和
设备内存映射一对一映射
*/

/*
 * 内核空间的划分，以下都是虚拟地址
 */

// 该地址是2GB虚拟地址的偏移
#define ZONE_VIR_ADDR_OFFSET        0X80000000

/* 默认内核静态空间地址时在DMA之后*/
#define ZONE_STATIC_ADDR            (ZONE_VIR_ADDR_OFFSET + ZONE_PHY_STATIC_ADDR)

/* 规划1GB虚拟地址给静态内存 */
#define ZONE_STATIC_SIZE            (0X40000000-ZONE_PHY_STATIC_ADDR)  // 1GB内存大小 @.@

/* 默认内核动态空间地址 */
#define ZONE_DYNAMIC_ADDR     (ZONE_STATIC_ADDR+ZONE_STATIC_SIZE)

/* 规划768MB虚拟地址给动态内存 */
#define ZONE_DYNAMIC_SIZE            0X30000000  // 768MB内存大小 @.@

/* 默认内核持久态空间地址 */
#define ZONE_DURABLE_ADDR     (ZONE_DYNAMIC_ADDR+ZONE_DYNAMIC_SIZE)

/* 规划252MB虚拟地址给持久态内存 */
#define ZONE_DURABLE_SIZE            0X0FC00000  // 252MB内存大小 @.@

/* 
因为在我们的分页机制中，最后一个页目录表项存放的是页目录表自己，
所以不能使用，因此少了4MB的内存空间
*/
#define ZONE_DYNAMIC_END     0XFFC00000

/* 一共有多少个空间 */
#define MAX_ZONE_NR     3

#define ZONE_STATIC_NAME     "static"
#define ZONE_DYNAMIC_NAME    "dynamic"
#define ZONE_DURABLE_NAME    "durable"

/* 
静态空间最大的页数 
1GB的页数-16MB的页数
然后在于512对齐
*/

#define ZONE_STATIC_MAX_PAGES      (PAGE_NR_IN_1GB - PAGE_NR_IN_16MB)

#define ZONE_BAD_RANGE(zone, page) \
        (!(zone->pageArray <= page && \
        page <= zone->pageArray + zone->pageTotalCount))

//#define CONFIG_ZONE_DEBUG

/* 用于描述内核内存地址空间的结构体 */
struct Zone 
{
    address_t virtualStart;     //虚拟空间的开始地址
    address_t virtualEnd;       //虚拟空间的结束地址
    size_t virtualLength;       //虚拟空间的大小
    
    address_t physicStart;      //物理空间的开始地址
    address_t physicEnd;        //虚拟空间的结束地址
    size_t physicLength;        //物理空间的大小

    flags_t flags;  //空间的状态标志
    char *name;     //空间的名字

    //与页相关的部分
    /* 页的数组,避免和页表混淆，这里用array*/
    struct Page *pageArray;
    /* 管理页的使用状态的位图 */
    struct Bitmap pageMap;
    /* 指向伙伴算法对应的area的地址 */
    struct FreeArea freeArea[MAX_ORDER];

    //对页的使用情况的统计
    unsigned int pageTotalCount;    //总共有多少个页
    unsigned int pageUsingCount;    //使用中的页数
    unsigned int pageFreeCount;     //空闲的页数

};

PUBLIC void InitZone();

EXTERN struct Zone zoneOfMemory;


/* 获取空间的方法 */
PUBLIC INLINE struct Zone *ZoneGetByName(char *name);
PUBLIC INLINE struct Zone *ZoneGetByPhysicAddress(unsigned int paddr);
PUBLIC INLINE struct Zone *ZoneGetByVirtualAddress(unsigned int vaddr);
PUBLIC INLINE struct Zone *ZoneGetByPage(struct Page *page);

/* 页和物理地址的转换 */
PUBLIC INLINE struct Page *PhysicAddressToPage(unsigned int addr);
PUBLIC INLINE address_t PageToPhysicAddress(struct Page *page);

/* 物理地址和虚拟地址的转换 */
PUBLIC INLINE address_t PhysicToVirtual(unsigned int paddr);
PUBLIC INLINE address_t VirtualToPhysic(unsigned int vaddr);

#endif   /*_X86_MM_ZONE_H */
