/*
 * file:		lib/signal.c
 * auther:	    Jason Hu
 * time:		2019/12/24
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <lib/signal.h>

/**
 * sigaddset - 信号集添加一个信号
 * @mask: 信号集
 * @signo: 信号
 */
int sigaddset(sigset_t *mask,int signo)
{
    if (IS_BAD_SIGNAL(signo))
        return -1;
    *mask |= (1 << signo);
    return 0;
}

/**
 * sigdelset - 信号集删除一个信号
 * @mask: 信号集
 * @signo: 信号
 */
int sigdelset(sigset_t *mask,int signo)
{
    if (IS_BAD_SIGNAL(signo))
        return -1;
    *mask &= ~(1 << signo);
    return 0;
}

/**
 * sigemptyset - 信号集置空
 * @mask: 信号集
 */
int sigemptyset(sigset_t *mask)
{
    *mask = 1;  /* 把第一位置1 */ 
    return 0;
}

/**
 * sigfillset - 信号集置满
 * @mask: 信号集
 */
int sigfillset(sigset_t *mask)
{
    *mask = 0xffffffff;  /* 全部置1 */ 
    return 0;
}

/**
 * sigismember - 判断信号是否置1
 * @mask: 信号集
 */
int sigismember(sigset_t *mask,int signo)
{
    if (IS_BAD_SIGNAL(signo))
        return 0;
    return (*mask & (1 << signo));
}

/**
 * sigisempty - 判断信号集是空集
 * @mask: 信号集
 */
int sigisempty(sigset_t *mask)
{
    if (*mask > 1) {
        return 0;
    } else {
        return 1;
    }
}

/**
 * sigisfull - 判断信号集是满集
 * @mask: 信号集
 */
int sigisfull(sigset_t *mask)
{
    if (*mask == 0xffffffff) {
        return 1;
    } else {
        return 0;
    }
}