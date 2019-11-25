/*
 * file:		kernel/syscall.c
 * auther:	Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/syscall.h>
#include <drivers/console.h>
#include <book/task.h>
#include <book/vmspace.h>
#include <drivers/clock.h>
#include <fs/interface.h>

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
    SysOpen,
    SysClose,
    SysRead,
    SysWrite,
    SysStat,
    SysLseek,
    SysUnlink,
    SysIoctl,
    SysGetMode,
    SysSetMode,
    SysMakeDir,
    SysRemoveDir,
    SysMountDir,
    SysUnmountDir,
    SysGetCWD,
    SysChangeCWD,
    SysChangeName,/*
    SysOpenDir,
    SysCloseDir,
    SysReadDir,
    SysRewindDir,*/
};
