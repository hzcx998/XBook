/*
 * file:		arch/x86/include/mm/pflags.h
 * auther:		Jason Hu
 * time:		2019/7/4
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _X86_MM_PFLAGS_H
#define _X86_MM_PFLAGS_H

/* Page的标志 */

/* 标志物理页是否需要刷新到磁盘。保证该页写入磁盘之前不会被释放 */
#define PG_dirty    0x01

/* 当I/O操作错误，设置 */
#define PG_error    0x02

/* 动态页面，不是静态映射的 */
#define PG_dynamic    0x04

/* 当有磁盘I/O操作时，上锁，完成操作时释放锁 */
#define PG_locked    0x08

/* 表示页没有被使用 */
#define PG_unused    0x10

/* GetFreePage-获取页的标志 */

/* 静态空间的页 */
#define GFP_STATIC        0X01

/* 动态空间的页 */
#define GFP_DYNAMIC       0X02

/* 持久化空间的页 */
#define GFP_DURABLE       0X04

#endif  /*_X86_MM_PFLAGS_H */
