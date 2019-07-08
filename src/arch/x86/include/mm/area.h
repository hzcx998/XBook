/*
 * file:		arch/x86/include/mm/area.h
 * auther:		Jason Hu
 * time:		2019/7/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _X86_MM_AREA_H
#define _X86_MM_AREA_H

#include <book/list.h>
#include <book/bitmap.h>
#include <share/stdint.h>

/* 一共有10个order，0~9 */
#define MAX_ORDER 10

/*
在buddy内存管理系统中，我们需要保证最大的页数是512的倍数，
为什么呢？因为order 9的area的页数量是512，我们在对它进行
拆分和合并的时候，所有的页都需要从它里面分割出去。回收的
时候也是回收到它里面，所以，整个页数对齐后，就可以完美进行
分配和回收了。当所有页都回收之后，不会出现存在位于其它order
的页。这样也可以保证获取index的时候不会出错。不然就会越界。
*/
/* 最大的order的页的数量 */
#define MAX_ORDER_PAGE_NR 512

/* 空闲区域结构 */
struct FreeArea {
    struct List pageList;
    struct Bitmap map;  /* 记录当前area中的页的使用状态 */
};




#endif  /*_X86_MM_AREA_H*/
