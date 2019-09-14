/*
 * file:		include/book/vmarea.h
 * auther:		Jason Hu
 * time:		2019/7/16
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_VMAREA_H
#define _BOOK_VMAREA_H

#include <share/types.h>
#include <share/const.h>
#include <book/list.h>
#include <book/arch.h>

/* 虚拟地址区域的标志 */

#define VMA_ALLOC       0X00000001     // vmalloc使用
#define VMA_MAP         0X00000002     // vmap使用

/**
 * VirtualMemoryArea - 虚拟内存区域
 * 用来管理虚拟地址的一个区域
 */
struct VirtualMemoryArea {
    struct VirtualMemoryArea *next; // 下一个虚拟地址区域
    void *addr;     // 指向的虚拟地址 
    size_t size;     // 虚拟地址区域的大小
    flags_t flags;    // 虚拟地址区域的标志
    unsigned int pages; // 虚拟区域占用了多少个物理页
};

PUBLIC INIT void InitVirtualMemoryArea();
PUBLIC void *__vmalloc(size_t size, unsigned int flags);

/**
 * vmalloc - 分配虚拟地址
 * @size: 分配空间的大小
 * 
 * 分配非连续地址空间里面的内存，物理内存是动态内存。
 * 写在头文件里面用INLINE 可以嵌入调用的地方，提高效率
 */
PRIVATE INLINE void *vmalloc(size_t size)
{
    return __vmalloc(size, GFP_DYNAMIC);
}
PUBLIC void vfree(void *addr);

PUBLIC void *vmap(address_t paddr, size_t size);
PUBLIC void vunmap(void *addr);

PUBLIC int ioremap(address_t addr, size_t size);

#endif   /*_BOOK_VMAREA_H*/
