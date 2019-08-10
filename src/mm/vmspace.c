/*
 * file:		mm/VMSpace.c
 * auther:		Jason Hu
 * time:		2019/8/1
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/vmarea.h>
#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/math.h>
#include <book/mmu.h>
#include <book/vmspace.h>
#include <book/task.h>

/**
 * GetUnmappedVMSpace - 获取一个没有映射的空闲空间
 * @mm: 内存管理器
 * @len: 要获取的长度
 * 
 * 查找一个没有映射的空间并返回其地址
 */
PRIVATE address_t GetUnmappedVMSpace(struct MemoryManager *mm, uint32 len)
{
    /* 地址指向没有映射的空间的最开始处 */
    address_t addr = VMS_UNMAPPED_BASE;

    // 根据地址获取一个space
    struct VMSpace *space = FindVMSpace(mm, addr);
    
    // 循环查找长度符合的空间
    while (space != NULL) {
        /* 如果到达最后，就退出查找 */
        if (USER_VMS_SIZE - len < addr)
            return -1;

        /* 如果要查找的区域在链表中间就直接返回地址 */
        if (addr + len <= space->start)
            return addr;
        
        /* 获取下一个地址 */
        addr = space->end;
        /* 获取下一个空间 */
        space = space->next;
    }
    /* 地址空间在链表的最后面 */
    return addr;
}

PUBLIC int MergeVMSpace(struct VMSpace* prev, struct VMSpace* space, struct VMSpace* next)
{

    /* 如果两者之间是连着的，prev的结束和space的开始相等 */
    if (prev != NULL && prev->end == space->start) {
        /* 其他属性页一样 */
        if (prev->pageProt == space->pageProt && prev->flags == space->flags) {
            // 把space从链表删除
            prev->end = space->end;
            prev->next = next;
            // 释放这个空间
            kfree(space);
            // 空间指向prev
            space = prev;
        }
    }
    
    /* 合并space和p */
    if (next != NULL && space->end == next->start) {
        if (space->pageProt == next->pageProt && space->flags == next->flags) {
            // 把p从链表中删除
            space->end = next->end;
            space->next = next->next;
            // 释放这个空间
            kfree(next);
        }
    }
}
/**
 * InsertVMSpace - 把空间插入链表
 * @mm: 内存管理器
 * @space: 要插入的空间
 * 
 * 成功返回0
 */
PUBLIC int InsertVMSpace(struct MemoryManager *mm, struct VMSpace* space)
{
    struct VMSpace* prev = NULL;
    /* 获取第一个空间 */
    struct VMSpace* p = mm->spaceMap;
    /*  */
    while (p != NULL) {
        /* 在中间找到一个空间 */
        if (p->start >= space->end) {
            break;
        }
        /* 保存前一个空间 */
        prev = p;
        p = p->next;
    }

    /* 把空间插入到中间或者最后 */
    space->next = p;

    /* 如果前一个不是空，那么前一个的后一个就是当前插入的空间 */
    if (prev != NULL) {
        prev->next = space;
    }
    else {
        // 前一个为空，那么就把space当做第一个空间
        mm->spaceMap = space;
    }

    /* 合并prev和space */
    //MergeVMSpace(prev, space, p);
    return 0;
}

/** 
 * RemoveVMSpace - 移除一个空间
 * @mm: 内存管理器
 * @space: 要移除的空间
 * @prev: 空间的前一个空间
 * 
 * 把一个空间从内存管理器的空间链表中移除
 */
PUBLIC void RemoveVMSpace(struct MemoryManager *mm, struct VMSpace *space, struct VMSpace *prev)
{
    /* 有prev，那么就让prev的下一个指向space的下一个 */
    if (prev != NULL) {
        prev->next = space->next;
    } else {
        /* 没有prev，就让内存管理器的空间头指针指向space的下一个 */
        mm->spaceMap = space->next;
    }
    /* 现在可以正确得移除space了，因为已经把它从链表移除 */
    kfree(space);
}

/**
 * FindVMSpace - 查找虚拟内存空间
 * @mm: 内存管理器
 * @addr: 要查找的地址
 * 
 * 查找第一个满足 addr < space->end, 并且不是NULL的空间
 */
PUBLIC struct VMSpace *FindVMSpace(struct MemoryManager *mm, address_t addr)
{
    struct VMSpace* space = mm->spaceMap;
    while (space != NULL) {
        /* 如果地址满足条件 */
        if (addr < space->end)
            return space;
        
        space = space->next;
    }
    return NULL;
}
/**
 * FindVMSpacePrev - 查找虚拟内存空间并且保存前一个空间
 * @mm: 内存管理器
 * @addr: 要查找的地址
 * @prev: 保存前一个空间的指针
 * 
 * 查找第一个满足 addr < space->end, 并且不是NULL的空间，
 * 并且把space的前一个空间保存到prev
 */
PUBLIC struct VMSpace *FindVMSpacePrev(struct MemoryManager *mm, address_t addr, struct VMSpace *prev)
{
    prev = NULL;
    struct VMSpace* space = mm->spaceMap;
    while (space != NULL) {
        /* 如果地址比查询的空间的结束地址小既 [addr, space->end] */
        if (addr < space->end)
            return space;
        
        /* 保存空间的前一个空间 */
        prev = space;
        space = space->next;
    }
    return NULL;
}

/** 
 * DoMmap - 映射地址
 * @addr: 地址
 * @len: 长度
 * @prot: 页保护
 * @flags: 空间的标志
 * 
 * 做地址映射，进程才可以读写空间
 */
PUBLIC int32 DoMmap(struct MemoryManager *mm, address_t addr, uint32_t len, uint32_t prot, uint32_t flags)
{
    //PART_START("DoMap");
    // printk(PART_TIP "DoMmap: %x, %x, %x, %x\n", addr, len, prot, flags);
    /* 让长度和页大小PAGE_SIZE对齐  */
    len = PAGE_ALIGN(len);

    /* 如果长度为0，什么也不做 */
    if (!len) {
        printk(PART_ERROR "DoMmap: len is zero!\n");
        return -1;
    }

    /* 越界就返回 */
    if (len > USER_VMS_SIZE || addr > USER_VMS_SIZE || addr > USER_VMS_SIZE - len) {
        printk(PART_ERROR "DoMmap: addr and len out of range!\n");
        return -1;
    }
    //printk(PART_TIP "len right\n");
    
    /* 如果是MAP_FIXED, 地址就要和页大小PAGE_SIZE对齐 
    也就是说，传入的地址是多少就是多少，不用去自动协调
    */
    if (flags & MAP_FIXED) {
        //printk(PART_TIP "is MAP_FIXED\n");

        // 地址需要是页对齐的
        if (addr & ~PAGE_MASK)
            return -1;
        //printk(PART_TIP "addr is right\n");

        /* 检测地址不在空间里面，也就是说 [addr, addr+len] */
        struct VMSpace* p = FindVMSpace(mm, addr);
        //printk(PART_TIP "Find space\n");

        // 存在这个空间，并且地址在这个空间中就返回。不能对这段空间映射
        if (p != NULL && addr + len > p->start) {
            printk(PART_ERROR "DoMmap: this FIXED space had existed!\n");
            return -1;
        }
            
    } else {
        // 获取一个没有映射的空间，等会儿用来进行映射
        addr = GetUnmappedVMSpace(mm, len);
        if (addr == -1) {
            printk(PART_ERROR "DoMmap: GetUnmappedVMSpace failed!\n");
            return -1;
        }
        
    }
    
    /* 从slab中分配一块内存来当做VMSpace结构 */
    struct VMSpace *space = (struct VMSpace *)kmalloc(sizeof(struct VMSpace));
    if (!space) {
        printk(PART_ERROR "DoMmap: kmalloc for space failed!\n");
        return -1;    
    }
        
    /* 设置空间 */
    space->start = addr;
    space->end = addr + len;
    space->flags = flags;
    space->pageProt = prot;

    /* 插入空间到链表中，并且尝试合并 */
    if (InsertVMSpace(mm, space)) {
        printk(PART_ERROR "DoMmap: InsertVMSpace failed!\n");
        return -1;
    }

    // printk(PART_TIP "DoMmap: %x, %x, %x, %x\n", addr, len, prot, flags);
    // PART_END();
    return addr;
}

/** 
 * DoMunmap - 取消一个空间的映射
 * @mm: 内存管理器
 * @addr: 空间的地址
 * @len: 空间的长度
 * 
 * 取消空间的映射，用于进程退出时使用
 */
PUBLIC int DoMunmap(struct MemoryManager *mm, uint32 addr, uint32 len)
{
    /* 地址要和页大小PAGE_SIZE对齐 */
    if ((addr & ~PAGE_MASK) || addr > USER_VMS_SIZE || len > USER_VMS_SIZE-addr) {
        printk(PART_ERROR "DoMunmap: addr and len error!\n");
        return -1;
    }

    /* 让长度和页大小PAGE_SIZE对齐  */
    len = PAGE_ALIGN(len);

    /* 如果长度为0，什么也不做 */
    if (!len) {
        printk(PART_ERROR "DoMunmap: len is zero!\n");
        return -1;
    }
        
    /* 找到addr < space->end 的空间 */
    struct VMSpace* prev = NULL;
    struct VMSpace* space = FindVMSpacePrev(mm, addr, prev);
    /* 没找到空间就返回 */
    if (!space) {      
        printk(PART_ERROR "DoMunmap: not found the space!\n");
        return -1;
    }
        
    /* 保证地址是在空间范围内 */
    if (addr < space->start || addr+len > space->end) {
        printk(PART_ERROR "DoMunmap: addr out of space!\n");
        return -1;
    }
        
    /* 分配一个新的空间，有可能要unmap的空间会分成2个空间，例如：
    [start, addr, addr+len, end] => [start, addr], [addr+len, end]
     */
    struct VMSpace* spaceNew = (struct VMSpace*)kmalloc(sizeof(struct VMSpace));
    if (!spaceNew) {        
        printk(PART_ERROR "DoMunmap: kmalloc for spaceNew failed!\n");
        return -1;
    }

    /* 把新空间链接到链表 */
    spaceNew->start = addr+len;
    spaceNew->end = space->end;
    space->end = addr;
    spaceNew->next = space->next;
    space->next = spaceNew;

    /* 检查是否是第一部分需要移除 */
    if (space->start == space->end) {
        RemoveVMSpace(mm, space, prev);
        space = prev;
    }

    /* 检查是否是第二部分需要移除 */
    if (spaceNew->start == spaceNew->end) {
        RemoveVMSpace(mm, spaceNew, space);
    }

    /* 需要释放物理页 */
    //UnmapPageRange(addr, len);
   return 0;
}

/**
 * SysMmap - [系统调用]内存映射
 * @addr: 地址
 * @len: 长度
 * @prot: 页保护
 * @flags: 空间的标志
 */
PUBLIC int SysMmap(uint32_t addr, uint32_t len, uint32_t prot, uint32_t flags)
{
    struct Task *current = CurrentTask();
    return DoMmap(current->mm, addr, len, prot, flags);
}

/**
 * SysUnmmap - [系统调用]取消内存映射
 * @addr: 地址
 * @len: 长度
 */
PUBLIC int SysMunmap(uint32_t addr, uint32_t len)
{
    struct Task *current = CurrentTask();
    return DoMunmap(current->mm, addr, len);
}

/** 
 * InitMemoryManager - 初始化内存管理器
 * @mm: 内存管理器
 * 
 */
PUBLIC void InitMemoryManager(struct MemoryManager *mm)
{
    mm->spaceMap = NULL;
}

/** 
 * MemoryManagerRelease - 释放所有内存空间
 * @mm: 内存管理器
 */
PUBLIC void MemoryManagerRelease(struct MemoryManager *mm, unsigned int flags)
{
    struct VMSpace *space = mm->spaceMap;
    while(space){
        //printk("space: start %x end %x\n", space->start, space->end);
        space = space->next;
    }

    /* 1.释放内存映射 */
    space = mm->spaceMap;
    /* 循环把每一个vmspace从空间中释放 */
    while (space != NULL) {
        /*  1.有释放栈的标志，并且space是栈，才释放空间（栈）
            2.有资源标志，但space不是栈，才释放空间
         */
        if ((flags & VMS_STACK && space->flags & VMS_STACK) || 
            (flags & VMS_RESOURCE && !(space->flags & VMS_STACK))) {

            /* 对空间中的每一个页进行释放 */
            while (space->start < space->end) {
                PageUnlinkAddress(space->start);
                space->start += PAGE_SIZE;
            }
        }

        space = space->next;
    }
    /* 2.释放虚拟空间 */ 
    space = mm->spaceMap;
    /* 循环把每一个vmspace从空间中释放 */
    while (space != NULL) {
        struct VMSpace *del = space;
        space = space->next;
        kfree(del);
    }
    /* 释放完后把空间映射置空 */
    mm->spaceMap = NULL;
}

/**
 * InitVMSpace - 初始化虚拟空间
 */
PUBLIC void InitVMSpace()
{
    PART_START("VMSpace");

    // 注册页故障处理中断
    InterruptRegisterHandler(0x0e, DoPageFault);

    PART_END();
}
