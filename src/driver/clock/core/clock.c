/*
 * file:		clock/core/clock.c
 * auther:		Jason Hu
 * time:		2019/6/26
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <clock/clock.h>
//#include <clock/pit/clock.h>

/* ----驱动程序初始化文件导入---- */
EXTERN void InitPitClockDriver();
/* ----驱动程序初始化文件导入完毕---- */

/**
 * InitClockSystem - 初始化时钟系统
 * 多任务的运行依赖于此
 */
PUBLIC void InitClockSystem()
{
    InitPitClockDriver();
}
