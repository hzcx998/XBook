/*
 * file:		    include/book/memcache.h
 * auther:		    Jason Hu
 * time:		    2019/10/1
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_MEMCACHE_H
#define _BOOK_MEMCACHE_H

#include <lib/const.h>
#include <lib/types.h>
#include <lib/stddef.h>
#include <book/config.h>
#include <book/bitmap.h>
#include <book/list.h>

/*
当内存对象大小小于1024时，储存在一个页中。
+-----------+
| group     | 
| bitmap    |
| objects   |
+-----------+
当内存对象大于1024时，对象和纪律信息分开存放。记录信息存放在一个页中，
对象存放在其他页中。
+-----------+
| group     | 
| bitmap    |
+-----------+
| objects   |
+-----------+
*/

/* 最大的mem的对象的大小 */
#ifdef CONFIG_LARGE_ALLOCS 
	#define MAX_MEM_CACHE_SIZE (4*MB)
#else
	#define MAX_MEM_CACHE_SIZE (128*KB)
#endif

struct MemGroup {
    struct List list;           // 指向cache中的某个链表（full, partial, free）
    struct Bitmap map;          // 管理对象分配状态的位图
    unsigned char *objects;     // 指向对象群的指针
    unsigned int usingCount;    // 正在使用中的对象数量
    unsigned int freeCount;     // 空闲的对象数量
    unsigned int flags;         // group的标志
};

#define SIZEOF_MEM_GROUP sizeof(struct MemGroup)

#define MEM_CACHE_NAME_LEN 24

struct MemCache {
    struct List fullGroups;      // group对象都被使用了，就放在这个链表
    struct List partialGroups;   // group对象一部分被使用了，就放在这个链表
    struct List freeGroups;      // group对象都未被使用了，就放在这个链表

    unsigned int objectSize;    // group中每个对象的大小
    flags_t flags;              // cache的标志位
    unsigned int objectNumber;  // 每个group中有多少个对象
    
    char name[MEM_CACHE_NAME_LEN];     // cache的名字
};

struct CacheSize {
    unsigned int cacheSize;         // 描述cache的大小
    struct MemCache *memCache;      // 指向对应cache的指针
};

PUBLIC int InitMemCaches();

PUBLIC void *kmalloc(size_t size, unsigned int flags);
PUBLIC void kfree(void *objcet);
PUBLIC int kmshrink();

#endif   /* _BOOK_MEMCACHE_H */
