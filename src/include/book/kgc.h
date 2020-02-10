/*
 * file:		include/book/kgc.h
 * auther:		Jason Hu
 * time:		2020/1/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 图形子系统 */
#ifndef _BOOK_KGC_H
#define _BOOK_KGC_H

#include <lib/types.h>
#include <lib/stdint.h>

/* KGC内核图形核心kernel graph core */
PUBLIC int InitKGC();

PUBLIC void SysGraphWrite(int offset, int size, void *buffer);

PUBLIC void KGC_CoreDraw(uint32_t position, uint32_t area, void *buffer, uint32_t color);

#endif   /* _BOOK_KGC_H */
