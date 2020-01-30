/*
 * file:		mm/lowmem.h
 * auther:		Jason Hu
 * time:		2020/1/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _MM_LOWMEN_H
#define _MM_LOWMEN_H

/*
lowmem(low memory)，低端内存管理，用于旧设备缓冲区
*/
#include <share/stddef.h>

/* 默认4个内存片段 */
#define MAX_MEM_FRAGMENT_NR 4

/* 内存片段的大小，64kb */
#define MEM_FRAGMENT_SIZE (64*1024)

/* 内存片段的起始地址 */
#define MEM_FRAGMENT_ADDR 0x10000

enum MemFragmentState {
    MEM_FRAGMEMT_UNUSED = 0,
    MEM_FRAGMEMT_USING,
};

/* 内存片段 */
typedef struct MemFragment {
    uint64_t address;     /* 开始地址 */
    uint8_t state;          /* 片段状态 */
} MemFragment_t;


PUBLIC void InitMemFragment();
PUBLIC void *lmalloc(size_t size);
PUBLIC void lmfree(void *addr);


#endif   /*_MM_LOWMEN_H */
