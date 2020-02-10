/*
 * file:		include/lib/conio.h
 * auther:		Jason Hu
 * time:		2019/7/29
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_MMAN_H
#define _LIB_MMAN_H

#include <lib/stdint.h>

void *mmap(uint32_t addr, uint32_t len, uint32_t prot, uint32_t flags);
int munmap(uint32_t addr, uint32_t len);

#endif  /* _LIB_MMAN_H */
