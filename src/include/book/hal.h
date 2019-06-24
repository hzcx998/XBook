/*
 * file:		   include/book/hal.h
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_HAL_H
#define _BOOK_HAL_H

#include <share/stdint.h>
#include <share/types.h>

/* 
 * 硬件抽象层 （Hardware Abstraction Layer）
 * 对硬件进行抽象管理，抽象出一套通用的方法。
 */

#define HAL_NAME_LEN 32
struct Hal  {
   struct HalOperate *halOperate;
   char *halName;
   char halType;
   //用链表把所有的hal链接起来。
};

struct HalOperate  {
   void (*Init)();   //初始化函数
   void (*Open)();   //打开函数
   void (*Read)(unsigned char *, unsigned int ); //读取函数
   void (*Write)(unsigned char *, unsigned int ); //写入函数
   void (*Close)();    //关闭函数
   void (*Ioctl)(unsigned int , unsigned int ); //控制设置
};

PUBLIC void InitHal();

PUBLIC void HalInit(struct Hal *hal);

PUBLIC inline void HalOpen(struct Hal *hal);
PUBLIC inline void HalRead(struct Hal *hal,unsigned char *buffer, unsigned int count);
PUBLIC inline void HalWrite(struct Hal *hal,unsigned char *buffer, unsigned int count);
PUBLIC inline void HalClose(struct Hal *hal);
PUBLIC inline void HalIoctl(struct Hal *hal,unsigned int type, unsigned int value);

#endif   /*_BOOK_HAL_H*/
