/*
 * file:		arch/x86/include/kernel/const.h
 * auther:		Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _X86_CONST_H
#define _X86_CONST_H

/* 在qemu中5测试的结果是10秒10次输出，10测试的结果是10秒5次输出，感觉是不是变慢了？
后面在vbox和vmware中再次进行输出测试。
qemu中运行稍微慢一些，所以配置5。
vbox和vmware中运行快一些，所以配置10。
不然会崩！！！
 */
#define CLOCK_QUICKEN     10		
/* 该参数应该由架构体系来定义，然后在对于的驱动实现过程中进行解析 */
#define HZ             (100 * CLOCK_QUICKEN)	    //1000 快速 100 普通0.001

#endif  /* _X86_CONST_H */
