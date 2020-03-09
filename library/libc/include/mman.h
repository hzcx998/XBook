/*
 * file:		include/lib/conio.h
 * auther:		Jason Hu
 * time:		2019/7/29
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_MMAN_H
#define _LIB_MMAN_H

#include "stdint.h"


/* 物理内存信息 */
typedef struct meminfo {
    unsigned long mi_total;    /* 物理内存总大小 */
    unsigned long mi_free;     /* 物理内存空闲大小 */
    unsigned long mi_used;     /* 物理内存已使用大小 */
} meminfo_t;

void getmem(meminfo_t *mi);

void *mmap(uint32_t addr, uint32_t len, uint32_t prot, uint32_t flags);
int munmap(uint32_t addr, uint32_t len);

#endif  /* _LIB_MMAN_H */
