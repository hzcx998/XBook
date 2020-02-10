/*
 * file:		kernel/mm/lowmem.c
 * auther:	    Jason Hu
 * time:		2020/1/26
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/arch.h>
#include <book/lowmem.h>

#include <lib/string.h>

/* 低端内存表 */
MemFragment_t memFragmentTable[MAX_MEM_FRAGMENT_NR];

/**
 * lmalloc - 分配低端内存
 * @size: 大小
 * 
 * 分配一个低端内存，如果size满足就分配，不满足就不分配
 * @return: 成功返回内存片段，失败返回NULL
 */
PUBLIC void *lmalloc(size_t size)
{
    /* 如果需求超出片段大小就直接返回 */
    if (size > MEM_FRAGMENT_SIZE) 
        return NULL;
    
    /* 查找一个空闲的并返回 */
    int i;
    for (i = 0; i < MAX_MEM_FRAGMENT_NR; i++) {
        if (memFragmentTable[i].state == MEM_FRAGMEMT_UNUSED) {
            memFragmentTable[i].state = MEM_FRAGMEMT_USING;
            /* 分配时清空 */
            memset((void *)memFragmentTable[i].address, 0, MEM_FRAGMENT_SIZE);
            return (void *)memFragmentTable[i].address;
        }
    }
    return NULL;
}

/**
 * lmfree - 低端内存释放
 * @addr: 地址
 * 
 * 释放一个内存片段
 */
PUBLIC void lmfree(void *addr)
{
    /* 查找对应的地址并释放 */
    int i;
    for (i = 0; i < MAX_MEM_FRAGMENT_NR; i++) {
        if (memFragmentTable[i].address == (uint64_t)addr) {
            memFragmentTable[i].state = MEM_FRAGMEMT_UNUSED;
            break;
        }
    }
}

/**
 * InitMemFragment - 初始化内存片段
 */
PUBLIC void InitMemFragment()
{
    uint64_t start = (uint64_t )__VA(MEM_FRAGMENT_ADDR); 
    int i;
    for (i = 0; i < MAX_MEM_FRAGMENT_NR; i++) {
        memFragmentTable[i].state = MEM_FRAGMEMT_UNUSED;
        memFragmentTable[i].address = start;
        //printk("fragment:%x\n", start);
        start += MEM_FRAGMENT_SIZE;
    }
}