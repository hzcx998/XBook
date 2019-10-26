/*
 * file:		   include/book/config.h
 * auther:		Jason Hu
 * time:		2019/6/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_CONFIG_H
#define _BOOK_CONFIG_H


/*系统平台*/
#define CONFIG_ARCH_X86

/* CPU的数据宽度 */
#define CONFIG_ARCH_32
//#define CONFIG_ARCH_64


/* 调试配置 */
//#define CONFI_SLAB_DEBUG

/* 配置多元信号量（Multivariate semaphore） */
#define CONFIG_SEMAPHORE_M 

/* 配置二元信号量（Binary semaphore） */
//#define CONFIG_SEMAPHORE_B 


/* 如果想要用kmalloc分配128KB~4MB之间大小的内存，就需要配置此项 */
//#define CONFIG_LARGE_ALLOCS



#endif   /*_BOOK_CONFIG_H*/
