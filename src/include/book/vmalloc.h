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

PUBLIC void *vmalloc2(size_t size);
PUBLIC int vfree2(void *ptr);
PUBLIC int iomap(unsigned int paddr, size_t size);

PUBLIC INIT void InitVM_Area();

#endif   /* _BOOK_VMALLOC_H */
