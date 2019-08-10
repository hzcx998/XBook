/*
 * file:		include/book/slab.h
 * auther:		Jason Hu
 * time:		2019/7/11
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_SLAB_H
#define _BOOK_SLAB_H

#include <share/types.h>
#include <share/const.h>
#include <book/list.h>
#include <book/arch.h>

#define SLAB_CACHE_NAME_LEN 20

/* 最大的slab的对象的大小 */
#define MAX_SLAB_CACHE_SIZE (128*KB)

// 静态定义的slab cache，不能销毁
#define SLAB_CACHE_FLAGS_STATIC 0X01

// 静态定义的slab ，不能销毁
#define SLAB_FLAGS_STATIC 0X01
// 动态定义的slab ，能销毁
#define SLAB_FLAGS_DYNAMIC 0X02

/* 
 * SlabCache - slab缓存结构
 * 
 * 结构体内容：
 * 记录具体slab的链表
 * 标记每个slab对象的大小
 * 记录每个slab对象的数量
 * 指向下一个cache的指针
 * 构建和析构函数
 * 名字标识
 */
struct SlabCache {
    struct List slabsFull;      // slab对象都被使用了，就放在这个链表
    struct List slabsPartial;   // slab对象一部分被使用了，就放在这个链表
    struct List slabsFree;      // slab对象都未被使用了，就放在这个链表
    
    unsigned int objectSize;    // slab中每个对象的大小
    flags_t flags;              // cache的标志位
    unsigned int objectNumber;  // 每个slab中有多少个对象

    struct SlabCache *next;           // 指向下一个同级cache的指针

    unsigned int slabSizeOrder;     // slab要占用的页的数量，用order标识
    unsigned int slabGetPageFlags;  // slab获取页的标志

    // 构建和析构函数指针
    void (*Constructor)(void *, struct SlabCache *, unsigned long);
    void (*Desstructor)(void *, struct SlabCache *, unsigned long);

    char name[SLAB_CACHE_NAME_LEN];     // cache的名字
};

/* 
一个slab的对象的总的大小 
每次创建一个slab，都已这个大小为标准，分割对象群。
*/
#define SLAB_OBJECTS_SIZE   1*MB

/*
 * Slab - slab结构
 * 
 * 结构体内容：
 * 表明是在哪个链表（full, partial, free）
 * 管理slab对象使用与否的位图
 * 指向页的结构
 * 指向第一个对象的指针
 * 表明slab使用的记录
 */
struct Slab {
    struct List list;       // 指向cache中的某个链表（full, partial, free）
    struct Bitmap map;      // 管理对象分配状态的位图
    struct Page *page;      // 指向页的结构
    unsigned char *objects;  // 指向对象群的指针
    unsigned int usingCount;       // 正在使用中的对象数量
    unsigned int freeCount;        // 空闲的对象数量
    unsigned int flags;     // slab的标志
};

/*
 * CacheSize - cache大小的结构
 * 
 * 结构体内容：
 *  描述cache大小
 *  指向具体的cache的指针
 */
struct CacheSizeDescription {
    unsigned int cacheSize;         // 描述cache的大小
    struct SlabCache *cachePtr;     // 指向对应cache的指针
};

/*
 * 先初始化不同的大小的cache。
 * （32,64,128...）
 * 分配：
 *  根据分配的大小找到对应的cache
 *  再去cache中的free或partial中找一个slab
 *  如果对应的slab中没有找到，就会分配一个新的slab
 *  然后再去查找
 *  找到后，从slab中获取一个对象
 *  然后返回
 * 释放：
 *  根据地址找到他的cache
 *  在cache中对对象进行释放
 *  在partial中找到该对象
 *  将其释放
 */
PUBLIC INIT void InitSlabCacheManagement();

PUBLIC void *kmalloc(size_t size);
PUBLIC void kfree(void *objcet);

PUBLIC int SlabCacheAllShrink();

#endif   /*_BOOK_SLAB_H*/
