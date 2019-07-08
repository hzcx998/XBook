/*
 * file:		include/book/debug.h
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_DEBUG_H
#define _BOOK_DEBUG_H

#include <share/stdint.h>
#include <share/types.h>
#include <driver/console.h>

#define DEBUG_KERNEL

#ifdef DEBUG_KERNEL
    #define PART_START(msg) printk("-> "msg" ")
    #define PART_END() printk("<-\n")
#else
    #define PART_START(msg) 
    #define PART_END() 
#endif

//内核打印函数的指针
int (*printk)(const char *fmt, ...);
int (*filterk)(char *buf, unsigned int count);

//断言
#define ASSERT
#ifdef ASSERT
void AssertionFailure(char *exp, char *file, char *baseFile, int line);
#define assert(exp)  if (exp) ; \
        else AssertionFailure(#exp, __FILE__, __BASE_FILE__, __LINE__)
#else
#define assert(exp)
#endif

void Spin(char * func_name);
void Panic(const char *fmt, ...);



#endif   /*_BOOK_DEBUG_H*/
