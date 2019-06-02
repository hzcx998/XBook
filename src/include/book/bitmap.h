/*
 * file:		   include/book/bitmap.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_BITMAP_H
#define _BOOK_BITMAP_H

#include <share/stdint.h>
#include <share/types.h>

#define BITMAP_MASK 1
typedef struct bitmap_s 
{
   uint32_t btmp_bytes_len;
   /* 在遍历位图时,整体上以字节为单位,细节上是以位为单位,所以此处位图的指针必须是单字节 */
   uint8_t* bits;
}bitmap_t;

void bitmap_init(struct bitmap_s* btmp);
bool bitmap_scanTest(struct bitmap_s* btmp, uint32_t bit_idx);
int bitmap_scan(struct bitmap_s* btmp, uint32_t cnt);
void bitmap_set(struct bitmap_s* btmp, uint32_t bit_idx, int8_t value);
#endif
