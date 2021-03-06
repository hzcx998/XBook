/*
 * file:		kernel/task/syscall.c
 * auther:	    Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/syscall.h>
#include <book/task.h>
#include <book/vmspace.h>
#include <book/signal.h>
#include <book/alarm.h>
#include <book/fs.h>
#include <book/kgc.h>
#include <book/mmu.h>
#include <book/power.h>
#include <clock/clock.h>
#include <char/console/console.h>
#include <kgc/window/message.h>

/* 空系统调用，用来占位 */
PRIVATE void SysNull()
{
    
}

/* 系统调用表 */ 
syscall_t syscallTable[MAX_SYSCALL_NR] = {
    PrintTask,              /* 0 */
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
    SysPipe,                /* 33 */
    SysMakeFifo,            /* 34 */
    SysKill,                /* 35 */
    SysSignalReturn,        /* 36 */
    SysSignal,              /* 37 */ 
    SysSignalProcessMask,   /* 38 */
    SysSignalPending,       /* 39 */
    SysSignalAction,        /* 40 */
    SysAlarm,               /* 41 */
    SysSignalPause,         /* 42 */
    SysSignalSuspend,       /* 43 */
    SysGetPgid,             /* 44 */
    SysSetPgid,             /* 45 */
    SysNull,                /* 46 */
    SysTell,                /* 47 */
    SysIsFoot,              /* 48 */
    SysDup,                 /* 49 */
    SysDup2,                /* 50 */
    SysKGC_Message,         /* 51 */
    SysTime,                /* 52 */
    SysTaskScan,            /* 53 */
    SysGetMemory,           /* 54 */
    SysRedirect,            /* 55 */
    SysReboot,              /* 56 */
    SysGetVersion,          /* 57 */
};

/**
 * SyscallCheck - 检测num是否合法
 * @num: 系统调用号
 * 
 * 如果合法就返回1，不合法，就会发出SIGSYS信号，并返回0
 */
PUBLIC int SyscallCheck(int num)
{
    if (num >= 0 && num < MAX_SYSCALL_NR) {
        return 1;
    } else {
        ForceSignal(SIGSYS, SysGetPid());
        return 0;
    }
}
