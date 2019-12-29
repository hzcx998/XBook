/*
 * file:		include/book/signal.h
 * auther:		Jason Hu
 * time:		2019/12/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/* 今天是冬至，大家吃饺子吃得开心！ 2019.12.22 */

#ifndef _BOOK_SIGNAL_H
#define _BOOK_SIGNAL_H

#include <share/types.h>
#include <share/stddef.h>
#include <share/signal.h>
#include <book/atomic.h>
#include <book/spinlock.h>
#include <book/arch.h>
#include <book/list.h>

/*
已经支持，可以处理的信号有：
SIGKILL,SIGSTOP,SIGCONT,SIGABORT,SIGSEGV,SIGBUS,SIGFPE, SIGPIPE，SIGALRM
SIGSTKFLT,SIGXFZS,SIGWINCH, SIGCHLD(使用但不实现),SIGSYS，SIGUSR1,SIGUSR2,
SIGILL,SIGTRAP（只支持了异常的陷阱捕捉，而没有做处理）,SIGINT,SIGQUIT,SIGTSTP,
SIGTTIN,SIGTTOU

尚未支持的信号有：
SIGHUP,SIGURG,SIGXCPU,SIGVTALRM, SIGPROF,SIGIO,SIGPOLL,SIGPWR
*/

/* 信号行为 */
typedef struct SignalAction {
    sighandler_t handler;        /* 处理函数 */
    unsigned long flags;            /* 标志 */
} SignalAction_t;

/* 信号信息 */
typedef struct SignalMessage {
    int signal;         /* 信号 */
    pid_t pid;          /* 发送者进程id */
    int code;           /* 信号码 */
    int errno;          /* 错误码 */
} SignalMessage_t;

/* 信号结构 */
typedef struct Signal {
    Atomic_t count;                     
    struct SignalAction action[MAX_SIGNAL_NR];      /* 信号行为 */
    pid_t sender[MAX_SIGNAL_NR];                    /* 信号发送者 */
    Spinlock_t signalLock;                          /* 信号自旋锁 */
} Signal_t;

typedef struct SignalFrame {
	char *retAddr;                  /* 记录返回地址 */
	uint32 signo;                   /* 信号 */
    struct TrapFrame trapFrame;     /* 保存原来的栈框 */
	char retCode[8];                /* 构建返回的系统调用代码 */
} SignalFrame_t;

PUBLIC int DoSignal(struct TrapFrame *frame);

PUBLIC int ForceSignal(int signo, pid_t pid);

PUBLIC int SysKill(pid_t pid, int signal);
PUBLIC int SysSignal(int signal, sighandler_t handler);
PUBLIC int SysSignalAction(int signal, struct sigaction *act, struct sigaction *oldact);
PUBLIC int SysSignalReturn(uint32_t ebx, uint32_t ecx, uint32_t esi, uint32_t edi, struct TrapFrame *frame);
PUBLIC int SysSignalProcessMask(int how, sigset_t *set, sigset_t *oldset);
PUBLIC int SysSignalPending(sigset_t *set);
PUBLIC int SysSignalPause();
PUBLIC int SysSignalSuspend(sigset_t *set, sigset_t *oldSet);


#endif   /* _BOOK_SIGNAL_H */
