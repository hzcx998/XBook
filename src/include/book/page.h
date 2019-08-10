/*
 * file:		include/book/page.h
 * auther:		Jason Hu
 * time:		2019/7/1
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_PAGE_H
#define _BOOK_PAGE_H

#include <share/stddef.h>
#include <book/mmzone.h>

/* 描述物理页的结构 */
struct Page {
    flags_t flags;  //页的标志
    unsigned int count;     //引用了多少次
    address_t physicAddress;    //页的物理地址
    struct Zone *zone;  //所属的空间
};

#endif   /*_BOOK_PAGE_H */
