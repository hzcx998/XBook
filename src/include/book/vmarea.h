/*
 * file:		include/book/vmalloc.h
 * auther:		Jason Hu
 * time:		2019/8/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_VMALLOC_H
#define _BOOK_VMALLOC_H

#include <lib/types.h>
#include <lib/stddef.h>

PUBLIC void *vmalloc(size_t size);
PUBLIC int vfree(void *ptr);
PUBLIC int memmap(unsigned int paddr, size_t size);
PUBLIC unsigned int AllocVaddress(unsigned int size);
PUBLIC unsigned int FreeVaddress(unsigned int vaddr, size_t size);

PUBLIC void *IoRemap(unsigned long phyAddr, size_t size);


PUBLIC INIT void InitVMArea();

#endif   /* _BOOK_VMALLOC_H */
