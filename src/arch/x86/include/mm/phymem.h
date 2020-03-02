/*
 * file:		arch/x86/include/mm/phymem.h
 * auther:		Jason Hu
 * time:		2019/9/29
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _X86_MM_PHYMEN_H
#define _X86_MM_PHYMEN_H

#include <lib/stddef.h>
#include <book/bitmap.h>
#include <book/list.h>
#include <book/memcache.h>

/* 
 * 物理空间的划分
 */

// 0MB~1MB是体系结构相关的内存分布
#define ARCH_MEM_ADDR               0X000000000
#define ARCH_MEM_SIZE               0X000100000     // 1MB

// 1MB~2MB是内核镜像
#define KERNEL_MEM_ADDR             (ARCH_MEM_ADDR+ARCH_MEM_SIZE)
#define KERNEL_MEM_SIZE             0X000100000     // 1MB

// 2MB~8MB是系统重要信息的存放地址
#define MATERIAL_MEM_ADDR           (KERNEL_MEM_ADDR+KERNEL_MEM_SIZE)
#define MATERIAL_MEM_SIZE           0X000600000   // 6MB

// 8MB~16MB是设备DMA的地址
#define DMA_MEM_ADDR                (MATERIAL_MEM_ADDR+MATERIAL_MEM_SIZE)
#define DMA_MEM_SIZE                0X000800000   // 10MB

// ---- 以上是必须的基本内存使用情况 ----

// 16M以上是普通内存开始地址
#define NORMAL_MEM_ADDR             (DMA_MEM_ADDR+DMA_MEM_SIZE) 

#define TOP_MEM_ADDR                0xFFFFFFFF // 最高内存地址

/* 空内存，当前页目录表物理地址的映射（不可访问） */
#define NULL_MEM_SIZE                0X400000
#define NULL_MEM_ADDR                (TOP_MEM_ADDR - NULL_MEM_SIZE + 1)

#define HIGH_MEM_SIZE               0x8000000 // 非连续性内存128MB

/* 非连续内存起始地址, 4MB+128MB*/
#define HIGH_MEM_ADDR               (TOP_MEM_ADDR - (HIGH_MEM_SIZE + NULL_MEM_SIZE) + 1)

// 该地址是2GB虚拟地址的偏移
#define PAGE_OFFSET             0X80000000

/* MemNode 内存节点，用于管理每一段物理内存（以页为单位） */
struct MemNode {
    unsigned int count;         /* 内存节点占用的页数 */
    unsigned int flags;         /* 节点标志 */
    unsigned int reference;     /* 引用次数 */
    struct MemCache *memCache;  /* 内存缓冲 */
    struct MemGroup *group;     /* 内存组 */
};
#define SIZEOF_MEM_NODE sizeof(struct MemNode) 

/* 转换成物理地址 */
#define __PA(x) ((unsigned long)(x) - PAGE_OFFSET)
/* 转换成虚拟地址 */ 
#define __VA(x) ((void *)((unsigned long)(x) + PAGE_OFFSET)) 

#define MEM_NODE_MARK_CHACHE_GROUP(node, cache, group) \
        node->memCache = cache;\
        node->group = group
        
#define MEM_NODE_CLEAR_GROUP_CACHE(node) \
        node->memCache = NULL;\
        node->group = NULL
        
#define MEM_NODE_GET_GROUP(node) node->group
#define MEM_NODE_GET_CACHE(node) node->memCache

#define CHECK_MEM_NODE(node) \
        if (node == NULL) Panic("Mem node error!\n") 

PUBLIC int InitPhysicMemory();

PUBLIC void DumpMemNode(struct MemNode *node);
PUBLIC unsigned long Vir2Phy(void *address);
PUBLIC void *Phy2Vir(unsigned long address);

PUBLIC struct MemNode *Page2MemNode(unsigned int page);
PUBLIC unsigned int MemNode2Page(struct MemNode *node);

PUBLIC struct MemNode *GetFreeMemNode();

PUBLIC unsigned int GetPhysicMemoryFreeSize();
PUBLIC unsigned int GetPhysicMemoryTotalSize();

#endif   /*_X86_MM_PHYMEN_H */
