/*
 * file:		arch/x86/include/kernel/ards.h
 * auther:		Jason Hu
 * time:		2019/6/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _X86_ARDS_H
#define	_X86_ARDS_H

#include <share/types.h>
#include <share/stdint.h>

#define ARDS_ADDR 0x80001000 //ARDS结构从哪儿开始储存

#define MAX_ARDS_NR 12 //最大有12个ards结构

//#define CONFIG_ARDS_DEBUG

/*
ards结构体
*/
struct Ards
{
	unsigned int baseLow;  //基址低32位
	unsigned int base_high;
	unsigned int lengthLow;  //长度低32位
	unsigned int length_high;			
	unsigned int type;  //该结构的类型(1可以被系统使用)
};

uint64_t InitArds();

#endif