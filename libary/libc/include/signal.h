/*
 * file:		include/lib/signal.h
 * auther:		Jason Hu
 * time:		2019/12/23
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_SIGNAL_H
#define _LIB_SIGNAL_H

#include "stdint.h"
#include "stddef.h"

/* 只支持32个信号 */
/* 信号值 */
#define SIGHUP      1   
#define SIGINT      2  
#define SIGQUIT     3   
#define SIGILL      4   
#define SIGTRAP     5 
#define SIGABRT     6   
#define SIGIOT      SIGABRT 
#define SIGBUS      7   
#define SIGFPE      8   
#define SIGKILL     9   
#define SIGUSR1     10  
#define SIGSEGV     11   
#define SIGUSR2     12   
#define SIGPIPE     13   
#define SIGALRM     14   
#define SIGTERM     15   
#define SIGSTKFLT   16   
#define SIGCHLD     17   
#define SIGCONT     18   
#define SIGSTOP     19  
#define SIGTSTP     20   
#define SIGTTIN     21   
#define SIGTTOU     22   
#define SIGURG      23   
#define SIGXCPU     24   
#define SIGXFSZ     25   
#define SIGVTALRM   26   
#define SIGPROF     27  
#define SIGWINCH    28 
#define SIGIO       29   
#define SIGPOLL     SIGIO  
#define SIGPWR      30  
#define SIGUNUSED   31  
#define SIGSYS      SIGUNUSED   
#define MAX_SIGNAL_NR       32 

/* 信号处理函数类型 */
typedef void (*sighandler_t) (int);

/* 信号集 */
typedef unsigned int sigset_t;

/* 处理信号时，希望是原子操作 */
typedef int sig_atomic_t;

/* sigprocmask的how参数值 */
#define SIG_BLOCK   1 //在阻塞信号集中加上给定的信号集
#define SIG_UNBLOCK 2 //从阻塞信号集中删除指定的信号集
#define SIG_SETMASK 3 //设置阻塞信号集(信号屏蔽码)

/* 信号处理函数 */
#define SIG_DFL     ((sighandler_t)0)        /* 默认信号处理方式 */
#define SIG_IGN     ((sighandler_t)1)        /* 忽略信号 */
#define SIG_ERR     ((sighandler_t)-1)       /* 错误信号返回 */

/*  */
#define SA_ONESHOT          0x00000001  /* 用户处理函数只执行一次 */

#define IS_BAD_SIGNAL(signo) \
    (signo < 1 || signo >= MAX_SIGNAL_NR)
    
/* 信号行为 */
struct sigaction {
    void (*sa_handler)(int);        // 默认信号处理函数 
    int sa_flags;                   // 信号处理标志
};

int sigaddset(sigset_t *mask,int signo);
int sigdelset(sigset_t *mask,int signo);

int sigemptyset(sigset_t *mask);
int sigfillset(sigset_t *mask);

int sigismember(sigset_t *mask,int signo);
int sigisfull(sigset_t *mask);
int sigisempty(sigset_t *mask);

/* 发出一个信号 */
int kill(pid_t pid, int signal);

/* 设置信号处理函数，设置的函数只捕捉一次 */
int signal(int signal, sighandler_t handler);

/* 设置信号屏蔽 */
int sigprocmask(int how, sigset_t *set, sigset_t *oldset);

/* 获取未决信号 */
int sigpending(sigset_t *set);

/* 设定信号集 */
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);

void abort(void);

int raise(int signum);

/* 进程暂停 */
int pause(void);

int sigsuspend(const sigset_t *mask);

/* 设置的函数一只捕捉 */
sighandler_t sigset(int sig, sighandler_t handler);

#endif  /* _LIB_SIGNAL_H */
