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

#define ARDS_ADDR 0x80000500 //ARDS结构从哪儿开始储存
#define ARDS_NR   (0x80000500+0x100-4) //记录的ards数量

/*
ards结构体
*/
struct ards_s
{
	uint32_t base_low;  //基址低32位
	uint32_t base_high;
	uint32_t length_low;  //长度低32位
	uint32_t length_high;			
	uint32_t type;  //该结构的类型(1可以被系统使用)
};

uint32_t init_ards();

#endif