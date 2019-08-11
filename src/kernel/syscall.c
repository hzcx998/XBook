/*
 * file:		kernel/syscall.c
 * auther:	Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/syscall.h>
#include <driver/console.h>
#include <book/task.h>
#include <book/vmspace.h>
#include <driver/clock.h>


/* 系统调用表 */ 
syscall_t syscallTable[MAX_SYSCALL_NR] = {
    SysLog,
    SysMmap,
    SysMunmap,
    SysFork,
    SysGetPid, 
    SysExecv, 
    SysSleep,
    SysMSleep,
    SysExit,
    SysWait,
    SysBrk,
};
