/*
 * file:		include/lib/conio.h
 * auther:		Jason Hu
 * time:		2019/7/29
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _USER_SYS_MMAN_H
#define _USER_SYS_MMAN_H

#include <share/stdint.h>

void *mmap(uint32_t addr, uint32_t len, uint32_t prot, uint32_t flags);
int munmap(uint32_t addr, uint32_t len);

#endif  /* _USER_LIB_STDLIB_H */
