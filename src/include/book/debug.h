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
#include <drivers/console.h>

#define CONFIG_SERIAL_DEBUG


#define DEBUG_PART


#ifdef DEBUG_PART
    #define PART_START(msg) printk("-> "msg" ")
    #define PART_END() printk("<-\n")
    #define PART_TIP " |- "
    #define PART_WARRING " ^- "
    #define PART_ERROR " !- "
#else
    #define PART_START(msg) 
    #define PART_END() 
    #define PART_TIP 
    #define PART_WARRING
    #define PART_ERROR
#endif

//内核打印函数的指针
int (*printk)(const char *fmt, ...);
int (*filterk)(char *buf, unsigned int count);

//断言
#define CONFIG_ASSERT
#ifdef CONFIG_ASSERT
void AssertionFailure(char *exp, char *file, char *baseFile, int line);
#define ASSERT(exp)  if (exp) ; \
        else AssertionFailure(#exp, __FILE__, __BASE_FILE__, __LINE__)
#else
#define ASSERT(exp)
#endif

void Spin(char * func_name);
void Panic(const char *fmt, ...);

PUBLIC void DebugColor(unsigned int color);
PUBLIC void InitDebugPrint();

#endif   /*_BOOK_DEBUG_H*/
