/*
 * file:		signal.c
 * auther:	    Jason Hu
 * time:		2019/12/24
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <signal.h>
#include <stdlib.h>
#include <file.h>

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

/* 信号延缓的上半部分 */
int sigsuspendhalf(const sigset_t *set, sigset_t *oldSet);

/* 信号暂停 */
int sigpause();

int raise(int signum)
{
    return kill(getpid(), signum);
}

/* do nothing now. */
/*void fflush(void *__unused)
{
    
}*/

/* POSIX-style abort() function */
void abort(void)
{
    sigset_t            mask;
    struct sigaction    action;

    /* Caller can't ignore SIGABRT, if so reset to default */
    sigaction(SIGABRT, NULL, &action);
    if (action.sa_handler == SIG_IGN) {
        action.sa_handler = SIG_DFL;
        sigaction(SIGABRT, &action, NULL);
    }
    if (action.sa_handler == SIG_DFL)
        fflush(NULL);           /* flush all open stdio streams */

    /* Caller can't block SIGABRT; make sure it's unblocked */
    sigfillset(&mask);
    sigdelset(&mask, SIGABRT);  /* mask has only SIGABRT turned off */
    sigprocmask(SIG_SETMASK, &mask, NULL);
    kill(getpid(), SIGABRT);    /* send the signal */

    /* If we're here, process caught SIGABRT and returned */
    fflush(NULL);               /* flush all open stdio streams */
    action.sa_handler = SIG_DFL;
    sigaction(SIGABRT, &action, NULL);  /* reset to default */
    sigprocmask(SIG_SETMASK, &mask, NULL);  /* just in case ... */
    kill(getpid(), SIGABRT);                /* and one more time */
    exit(1);    /* this should never be executed ... */
}

int pause()
{
    while (1)
    {    
        /* 一只检测知道有信号被捕捉处理 */
        if (sigpause()) 
            break;   
    }
    return -1;
}

int sigsuspend(const sigset_t *mask)
{
    if (mask == NULL)
        return -1;

    int retval;
    sigset_t oldMask;

    retval = sigsuspendhalf(mask, &oldMask);
    /* 如果已经成功延缓，就直接返回，不然，再次进入等待 */
    if (retval) {
        return 1;
    }
    while (1)
    {    
        /* 一只检测知道有信号被捕捉处理 */
        if (sigpause()) 
            break;   
    }

    /* 现在已经被成功捕捉，设置原来的屏蔽 */
    retval = sigprocmask(SIG_SETMASK, &oldMask, NULL);

    /* 返回-1 */
    return -1;
}

sighandler_t sigset(int sig, sighandler_t handler)
{
    struct sigaction act, oldact;
    act.sa_handler = handler;
    act.sa_flags = 0;
    sigaction(sig, &act, &oldact);
    return oldact.sa_handler;
}
