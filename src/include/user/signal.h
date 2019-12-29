/*
 * file:		include/user/signal.h
 * auther:		Jason Hu
 * time:		2019/12/23
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _USER_LIB_SIGNAL_H
#define _USER_LIB_SIGNAL_H

#include <share/stdint.h>
#include <share/stddef.h>
#include <share/signal.h>

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
int pause();

int sigsuspend(const sigset_t *mask);

/* 设置的函数一只捕捉 */
sighandler_t sigset(int sig, sighandler_t handler);

#endif  /* _USER_LIB_SIGNAL_H */
