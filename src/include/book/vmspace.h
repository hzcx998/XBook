/*
 * file:		include/book/VMSpace.h
 * auther:		Jason Hu
 * time:		2019/8/1
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_VMSPACE_H
#define _BOOK_VMSPACE_H

#include <share/types.h>
#include <share/const.h>
#include <book/list.h>
#include <book/arch.h>
#include <user/unistd.h>

#define USER_VMS_SIZE        ZONE_VIR_ADDR_OFFSET
#define USER_STACK_TOP      USER_VMS_SIZE

/* 放到2~3G之间 */
#define VMS_UNMAPPED_BASE   ((USER_VMS_SIZE/3)*2)

#define VMS_STACK           0x01      /* 空间是栈类型 */
#define VMS_HEAP            0x02      /* 空间是堆类型 */

/* 在释放内存管理器时，如果需要释放内存资源，就要打上这个标志 */
#define VMS_RESOURCE        0x80   

/* 最大可扩展的栈的大小 */
#define MAX_VMS_STACK_SIZE    0x00800000

/* 最大可扩展的堆的大小,默认1GB */
#define MAX_VMS_HEAP_SIZE    0x40000000

/**
 * VMSpace - 虚拟空间结构
 */
struct VMSpace {
    struct MemoryManager    *mm;    // 所在的内存管理者
    address_t               start;  // 空间开始地址
    address_t               end;    // 空间结束地址
    unsigned int            pageProt;   //页保护
    flags_t                 flags;  // 空间的标志
    struct VMSpace          *next;  // 指向下一个空间
};

struct MemoryManager {
    struct VMSpace *spaceMap;   // 管理所有的空间 
    unsigned int totalPages;    // 总共占用了多少个页

    // 空间中的各种地址
    address_t      codeStart, codeEnd;
    address_t      dataStart, dataEnd;
    address_t      brkStart, brk;
    address_t      stackStart;
    address_t      argStart, argEnd;
    address_t      envStart, envEnd;

};


PUBLIC void InitVMSpace();
PUBLIC void InitMemoryManager(struct MemoryManager *mm);
PUBLIC void MemoryManagerRelease(struct MemoryManager *mm, unsigned int flags);
PUBLIC struct VMSpace *FindVMSpacePrev(struct MemoryManager *mm, 
        address_t addr, struct VMSpace *prev);
PUBLIC struct VMSpace *FindVMSpace(struct MemoryManager *mm, address_t addr);
PUBLIC int InsertVMSpace(struct MemoryManager *mm, struct VMSpace* space);
PUBLIC void RemoveVMSpace(struct MemoryManager *mm, struct VMSpace *space, 
        struct VMSpace *prev);

PUBLIC void *SysMmap(uint32_t addr, uint32_t len, uint32_t prot, uint32_t flags);
PUBLIC int SysMunmap(uint32_t addr, uint32_t len);
PUBLIC unsigned int SysBrk(unsigned int brk);

#endif   /* _BOOK_VMSPACE_H */
