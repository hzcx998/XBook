/*
 * file:		   include/book/vmalloc.h
 * auther:		Jason Hu
 * time:		   2019/8/31
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_VMALLOC_H
#define _BOOK_VMALLOC_H

#include <share/types.h>
#include <share/stddef.h>

PUBLIC void *vmalloc(size_t size);
PUBLIC int vfree(void *ptr);
PUBLIC int memmap(unsigned int paddr, size_t size);

PUBLIC INIT void InitVMArea();

#endif   /* _BOOK_VMALLOC_H */
