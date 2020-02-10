/*
 * file:		include/book/bitmap.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_BITMAP_H
#define _BOOK_BITMAP_H

#include <lib/stdint.h>
#include <lib/types.h>

#define BITMAP_MASK 1
struct Bitmap {
   unsigned int btmpBytesLen;
   /* 在遍历位图时,整体上以字节为单位,细节上是以位为单位,所以此处位图的指针必须是单字节 */
   unsigned char* bits;
};

PUBLIC void BitmapInit(struct Bitmap* btmp);
PUBLIC bool BitmapScanTest(struct Bitmap* btmp, unsigned int bitIdx);
PUBLIC int BitmapScan(struct Bitmap* btmp, unsigned int cnt);
PUBLIC void BitmapSet(struct Bitmap* btmp, unsigned int bitIdx, char value);
PUBLIC int BitmapChange(struct Bitmap *btmp, unsigned int bitIdx);
PUBLIC int BitmapTestAndChange(struct Bitmap *btmp, unsigned int bitIdx);

#endif
