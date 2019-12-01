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
#include <fs/fs.h>

/* 系统调用表 */ 
syscall_t syscallTable[MAX_SYSCALL_NR] = {
    SysLog,                 /* 0 */
    SysMmap,                /* 1 */
    SysMunmap,              /* 2 */
    SysFork,                /* 3 */
    SysGetPid,              /* 4 */
    SysExecv,               /* 5 */
    SysSleep,               /* 6 */
    SysMSleep,              /* 7 */
    SysExit,                /* 8 */
    SysWait,                /* 9 */
    SysBrk,                 /* 10 */
    SysOpen,                /* 11 */
    SysClose,               /* 12 */
    SysRead,                /* 13 */
    SysWrite,               /* 14 */
    SysLseek,               /* 15 */
    SysStat,                /* 16 */
    SysRemove,              /* 17 */
    SysIoctl,               /* 18 */
    SysGetMode,             /* 19 */
    SysChangeMode,          /* 20 */
    SysMakeDir,             /* 21 */
    SysRemoveDir,           /* 22 */
    SysGetCWD,              /* 23 */
    SysChangeCWD,           /* 24 */
    SysChangeName,          /* 25 */
    SysOpenDir,             /* 26 */
    SysCloseDir,            /* 27 */
    SysReadDir,             /* 28 */
    SysRewindDir,           /* 29 */
    SysAccess,              /* 30 */
    SysFcntl,               /* 31 */
    SysFsync,               /* 32 */
};
