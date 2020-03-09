/*
 * file:		kernel/signal.c
 * auther:	    Jason Hu
 * time:		2019/12/22
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/signal.h>
#include <book/debug.h>
#include <book/task.h>
#include <book/syscall.h>
#include <book/schedule.h>
#include <lib/string.h>

//#define _DEBUG_SIGNAL

PRIVATE void SetSignalAction(Signal_t *signal, int signo, SignalAction_t *sa)
{
    /* 信号从1开始，所以要-1 */
    signal->action[signo - 1].flags = sa->flags;
    signal->action[signo - 1].handler = sa->handler;
}

PRIVATE void GetSignalAction(Signal_t *signal, int signo, SignalAction_t *sa)
{
    /* 信号从1开始，所以要-1 */
    sa->flags = signal->action[signo - 1].flags;
    sa->handler = signal->action[signo - 1].handler;
}

/**
 * CalcSignalLeft - 计算是否还有信号需要处理
 */
PRIVATE void CalcSignalLeft(Task_t *task)
{
    if (task->signalPending == 1 || task->signalPending == 0)
        task->signalLeft = 0;
    else 
        task->signalLeft = 1;
}

PRIVATE int DelSignalFromTask(int signo, sigset_t *set)
{
    if (!sigismember(set, signo)) {
        return 0;
    }

    sigdelset(set, signo);

    /* 删除信号消息 */
    
    return 1;
}

/**
 * HandleStopSignal - 处理停止的信号
 * @signo: 信号
 * @task: 信号对应的任务
 * 
 * 在发送信号的时候，对一些信号的后续信号处理。
 * 某些信号的后续信号是不可以屏蔽的，在这里删除那些屏蔽
 * 为后续信号扫除障碍
 * 
 */
PRIVATE void HandleStopSignal(int signo, struct Task *task)
{
    switch (signo)
    {
    /* 如果是SIGKILL和SIGCONT，那么后续可能是SIGSTOP，SIGTSTP，SIGTTOU和SIGTTIN */
    case SIGKILL:
    /* 信号是CONT的时候，就会唤醒已经停止的进程，其实CONT的功能就在这里实现了，所以后面会忽略掉 */
    case SIGCONT:  
        /* 当发送的信号是必须要进程运行的条件时，需要唤醒它 */
        if (task->status == TASK_STOPPED) {
            //printk(">>>send kill or cont to a stopped task\n");
            TaskWakeUp(task);
        }
        
        /* 退出状态置0 */
        task->exitStatus = 0;

        /* 清除一些信号信号屏蔽 */
        DelSignalFromTask(SIGSTOP, &task->signalBlocked);
        DelSignalFromTask(SIGTSTP, &task->signalBlocked);
        DelSignalFromTask(SIGTTOU, &task->signalBlocked);
        DelSignalFromTask(SIGTTIN, &task->signalBlocked);
        
        break;
    /* 如果是SIGSTOP，SIGTSTP，SIGTTOU和SIGTTIN，那么后续可能是SIGCONT */
    case SIGSTOP:
    case SIGTSTP:
    case SIGTTOU:
    case SIGTTIN:
        /* 如果又重新停止一次，就需要把SIGCONT屏蔽清除 */
        DelSignalFromTask(SIGCONT, &task->signalBlocked);
        break;
    default:
        break;
    }
}


/**
 * IgnoreSignal - 忽略信号
 * @signo: 信号
 * @task: 信号对应的任务
 * 
 * 当信号是忽略处理的时候，并且也没有加以屏蔽，那就忽略它。
 * 发送时尝试忽略一些信号，成功就返回1，不是就返回0
 */
PRIVATE int IgnoreSignal(int signo, struct Task *task)
{
    /* 信号为0，就忽略之 */
    if (!signo)
        return 1;

    /* 如果已经屏蔽了，那么，就不会被忽略掉，尝试投递 */
    if (sigismember(&task->signalBlocked, signo)) {
        //printk(">>>a blocked signal.\n");
        return 0;
    }
        
    unsigned long handler = (unsigned long)task->signals.action[signo - 1].handler;
    
    /* 处理函数是用户自定义，不会被忽略掉 */
    if (handler > 1)
        return 0;

    //printk(">> handler:%x\n", handler);
    /* 如果处理函数是SIG_IGN，也就是一个忽略信号 */
    if (handler == 1)
        return (signo != SIGCHLD);  /* 如果信号不是子进程那么就忽略，是就不能忽略 */

    /* 默认信号处理函数，handler是SIG_DFL时 */
    switch (signo)
    {
    /* 忽略掉这些信号 */
    case SIGCONT:
    case SIGWINCH:
    case SIGCHLD:
    case SIGURG:
        return 1;
    /* 不能忽略的信号 */
    case SIGTSTP:
    case SIGTTIN:
    case SIGTTOU:
        return 0;
    default:
        return 0;
    }
}

/**
 * DeliverSignal - 信号投递
 * @signo: 信号
 * @sender: 发送者
 * @task: 任务
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int DeliverSignal(int signo, pid_t sender, struct Task *task)
{
    /* 如果信号发送者出问题就失败 */
    if (sender < 0) {
        return -1;
    }
#ifdef _DEBUG_SIGNAL
    printk("deliver signal %d from %d to %d\n", signo, task->pid, sender);
#endif
    /* 填写信号信息 */
    task->signals.sender[signo - 1] = sender;

    /* 设置对应的信号 */
    sigaddset(&task->signalPending, signo);

    /* 如果投递后，该信号没有被屏蔽，那么就尝试唤醒该进程（如果没有睡眠就不管），
    保证它能够早点处理信号，
    如果说被屏蔽了，那么就不能唤醒它
    */
    if (!sigismember(&task->signalBlocked, signo)) {
        //printk("not a blocked signal.\n");

        task->signalLeft = 1;       /* 有信号剩余 */
        
        /* 如果是阻塞中，就唤醒它 */
        if (task->status == TASK_BLOCKED || task->status == TASK_WAITING || 
            task->status == TASK_STOPPED) {
            #ifdef _DEBUG_SIGNAL
            printk("[DeliverSignal] task %s is blocked, wake up it.", task->name);
            #endif
            
            /* 如果投递给一个进程，其处于休眠状态，就要将它唤醒，并且要取消它的休眠定时器 */
            if (task->sleepTimer) {
                CancelTimer(task->sleepTimer);
                task->sleepTimer = NULL;
            }
            TaskUnblock(task);
            /* 如果在投递过程中，任务处于休眠中，那么就要用休眠计时器去唤醒它 */
        }
    } else {
        //printk("a blocked signal.\n");
    }

    /* 如果是投递了一个阻塞的信号，那么就没必要标明有信号剩余，不用处理 */

    return 0;
}

PRIVATE void BuildSignalFrame(int signo, SignalAction_t *sa, struct TrapFrame *frame)
{
    /* 获取信号栈框，在用户的esp栈下面 */
    SignalFrame_t *signalFrame = (SignalFrame_t *)((frame->esp - sizeof(SignalFrame_t)) & -8UL);

    /*printk("trap frame %x, signal frame esp %x, %x\n", 
    frame->esp, frame->esp - sizeof(SignalFrame_t), (frame->esp - sizeof(SignalFrame_t)) & -8UL);
    */
    /* 传递给handler的参数 */
    signalFrame->signo = signo;

    /* 把中断栈保存到信号栈中 */
    memcpy(&signalFrame->trapFrame, frame, sizeof(struct TrapFrame));

    /* 设置返回地址 */
    signalFrame->retAddr = signalFrame->retCode;

    /* 构建返回代码，系统调用封装
    模拟系统调用来实现从用户态返回到内核态
    mov eax, SYS_SIGRET
    int 0x80
     */
    signalFrame->retCode[0] = 0xb8; /* 给eax赋值的机器码 */
    *(int *)(signalFrame->retCode + 1) = SYS_SIGRET;    /* 把系统调用号填进去 */
    *(short *)(signalFrame->retCode + 5) = 0x80cd;      /* int对应的指令是0xcd，系统调用中断号是0x80 */
    
    /* 设置中断栈的eip成为用户设定的处理函数 */
    frame->eip = (uint32_t)sa->handler;

    /* 设置运行时的栈 */
    frame->esp = (uint32_t)signalFrame;

    /* 设置成用户态的段寄存器 */
    frame->ds = frame->es = frame->fs = frame->gs = USER_DATA_SEL;
    frame->ss = USER_STACK_SEL;
    frame->cs = USER_CODE_SEL;

}

/**
 * HandleSignal - 处理信号
 * @frame: 中断栈框
 * @signo: 信号
 * 
 * 处理信号，分为默认信号，忽略信号，以及用户自定义信号
 * 
 * 成功返回0，失败返回-1
 */
PRIVATE int HandleSignal(struct TrapFrame *frame, int signo)
{
    /* 获取信号行为 */
    SignalAction_t *sa = &CurrentTask()->signals.action[signo - 1];

    /* 处理自定义函数 */
    //printk("handle user function!\n");
    
    /* 构建信号栈框，返回时就可以处理用户自定义函数 */
    BuildSignalFrame(signo, sa, frame);
    
    /* 执行完信号后需要把信号行为设置为默认的行为 */
    if (sa->flags & SA_ONESHOT) {
        sa->handler = SIG_DFL;
    }
    
    //printk("will return into user!\n");

    return 0;
}

/**
 * SysSignalReturn - 执行完用户信号后返回
 * @frame: 栈
 * 
 */
PUBLIC int SysSignalReturn(uint32_t ebx, uint32_t ecx, uint32_t esi, uint32_t edi, struct TrapFrame *frame)
{
    //printk("in sig return.\n");

    //printk("arg is %x %x %x %x %x\n", ebx, ecx, esi, edi, frame);

    //printk("usr esp - 4:%x esp:%x\n", *(uint32_t *)(frame->esp - 4), *(uint32_t *)frame->esp);
    
    /* 原本signalFrame是在用户栈esp-SignalFrameSize这个位置，但是由于调用了用户处理程序后，
    函数会把返回地址弹出，也就是esp+4，所以，需要通过esp-4才能获取到signalFrame */
    SignalFrame_t *signalFrame = (SignalFrame_t *)(frame->esp - 4);

    /*清除阻塞 */

    /* 还原之前的中断栈 */
    memcpy(frame, &signalFrame->trapFrame, sizeof(struct TrapFrame));
    
    //printk("in sig return ret val %d.\n", frame->eax);

    /* 信号已经被捕捉处理了 */

    CurrentTask()->signalCatched = 1;

    /* 会修改eax的值，返回eax的值 */
    return frame->eax;
}

/**
 * DoSignal - 执行信号处理
 * frame: 内核栈 
 * 
 */
PUBLIC int DoSignal(struct TrapFrame *frame)
{
    struct Task *cur = CurrentTask();
    /* 如果没有信号，那么就直接返回 */
    if (!cur->signalLeft)
        return -1;

    /* 
    判断是否是在用户空间进入了信号处理，如果是内核空间就返回
    信号属于对进程的控制，而非是内核线程，所以这里需要排除内核态的线程
     */
    if (frame->cs != USER_CODE_SEL) {
        return -1;
    }
    
    /*
    获取信号并处理
    */
    int sig = 1;    // 从信号1开始检测

    SignalAction_t *sa;
    /* 当还有剩余的就一直循环 */
    for (sig = 1; cur->signalLeft && sig < MAX_SIGNAL_NR; sig++) {
        /* pending为1，blocked为0才可以处理该信号 */
        if (cur->signalPending & (1 << sig) && !(cur->signalBlocked & (1 << sig))) {
            #ifdef _DEBUG_SIGNAL
            printk("task %d received signal %d from task %d.\n",
                cur->pid, sig, cur->signals.sender[sig - 1]);
            #endif
            
            /* 已经获取这个信号了，删除它 */
            sigdelset(&cur->signalPending, sig);

            /* 删除发送者 */
            cur->signals.sender[sig - 1] = -1;
            CalcSignalLeft(cur);

            /* 指向信号对应的信号行为 */
            sa = &cur->signals.action[sig - 1];  

            /* 如果是忽略处理，那么就会把忽略掉 */
            if (sa->handler == SIG_IGN) {
                /* 如果是不是子进程信号，那么就忽略掉 */
                if (sig != SIGCHLD)
                    continue;
                /* 是子进程，那么，当前进程作为父进程，应该做对应的处理
                ！！！注意！！！
                在我们的系统中，不打算用信号来处理子进程，所以该处理也将被忽略
                 */
                /* 子进程发送信号来了，进行等待。
                多所有子进程检测，如果没有子进程，也可以返回
                 */
                //while ();
                //printk("parent %s recv SIGCHLD!\n");

                continue;
            }

            /* 如果是默认处理行为，就会以每个信号默认的方式进行处理 */
            if (sa->handler == SIG_DFL) {

                /* 如果是Init进程收到信号，那就啥也不做。 */
                if (cur->pid == 0)
                    continue;
                
                switch (sig)
                {
                /* 这些信号的默认处理方式其实就是忽略处理 */
                case SIGCONT:
                case SIGCHLD:
                case SIGWINCH:
                case SIGURG:
                case SIGUSR1:
                case SIGUSR2:
                    /* 忽略处理：默认就是不处理 */
                    continue;
                
                case SIGTSTP:
                case SIGTTIN:
                case SIGTTOU:
                case SIGSTOP:
                    /* 进程暂停：改变状态为STOPPED */
                    //printk("task %d stoped\n", cur->pid);
                    /* 停止运行处理 */
                    cur->status = TASK_STOPPED;
                    cur->exitStatus = sig;
                    /* 调度出去 */
                    Schedule();
                    continue;

                case SIGQUIT:
                case SIGILL:
                case SIGTRAP:
                case SIGABRT:
                case SIGFPE:
                case SIGSEGV:
                case SIGBUS:
                case SIGXCPU:
                case SIGXFSZ:
                    /* 进程流产：需要将进程转储，目前还不支持 */

                default:
                /* 进程终止：SIGUP, SIGINT, SIGKILL, SIGPIPE, SIGTERM, 
                SIGSTKFLT, SIGIO, SIGPOLL  */

                    /* 将当前信号添加到未决信号集，避免此时退出没有成功 */
                    sigaddset(&cur->signalPending, sig);
                    /* 计算信号剩余 */
                    CalcSignalLeft(cur);
                    /* 退出运行 */
                    SysExit(sig);
                }
            }

            /* 处理信号 */
            HandleSignal(frame, sig);
            
            /* 返回成功处理一个信号 */
            return 1;
        }
    }
    //printk("do signal exit.\n");

    return 0;
}


/**
 * DoSendSignal -发送信号
 * 
 */
PUBLIC int DoSendSignal(pid_t pid, int signal, pid_t sender)
{
    /* 发送信号 */
    #ifdef _DEBUG_SIGNAL
    printk("task %d sent signal %d to task %d.\n",
               sender, signal, pid);
    #endif
    
    /* 对参数进行检测 */
    if (IS_BAD_SIGNAL(signal)) {
        return -1;
    }
    Task_t *task = FindTaskByPid(pid);
    
    /* 没找到要发送的进程，返回失败 */
    if (task == NULL) {
        return -1;
    }
    int ret = 0;

    /* 信号上锁并关闭中断 */
    unsigned long flags = SpinLockIrqSave(&task->signalMaskLock);
    
    /* 如果是停止相关信号，就扫清后续信号屏蔽 */
    HandleStopSignal(signal, task);
    //printk("ignore.");
    /* 如果忽略成功，就退出 */
    if (IgnoreSignal(signal, task))
        goto ToOut;
    //printk("is member.");
    
    /* 如果已经处于未决，那就不投递 */
    if (sigismember(&task->signalPending, signal))
        goto ToOut;

    /* 进行信号投递 */
    //printk("deliver.");
    
    ret = DeliverSignal(signal, sender, task);

ToOut:
    SpinUnlockIrqSave(&task->signalMaskLock, flags);

    /* 发送后，如果进程处于可中断状态，并且还有信号需要处理，那么就唤醒进程 */
    if ((task->status == TASK_BLOCKED || task->status == TASK_WAITING || 
        task->status == TASK_STOPPED) && (task->signalPending != 1)) {
        //printk("wakeup a blocked task %s after send a signal.\n", task->name);
        
        /* 唤醒任务 */
        TaskWakeUp(task);
        
        CalcSignalLeft(task);
    }
    
    return ret;
}

/**
 * DoSendSignal -发送信号
 * 
 */
PRIVATE int DoSendSignalGroup(pid_t gpid, int signal, pid_t sender)
{
    //printk("DoSendSignalGroup\n");
    int retval = 0;
    if (gpid > 0) {
        struct Task *task;
        ListForEachOwner(task, &taskGlobalList, globalList) {
            /* 发送给相同的组标的进程 */
            if (task->groupPid == gpid) {
                int err = DoSendSignal(task->pid, signal, CurrentTask()->pid);
                if (err == -1)      /* 如果有错误的，就记录成错误 */
                    retval = err;
            }
        }
    }
    return retval;
}

/**
 * ForceSignal - 强制发送一个信号
 * @signo: 信号
 * @task: 发送到的任务
 * 
 * 从内核中强制发送一个信号。
 * 通常是内核发生故障，例如浮点处理，管道处理出错之类的
 */
PUBLIC int ForceSignal(int signo, pid_t pid)
{
    Task_t *task = FindTaskByPid(pid);
    /* 没找到要发送的进程，返回失败 */
    if (task == NULL) {
        return -1;
    }

    /* 信号上锁并关闭中断 */
    unsigned long flags = SpinLockIrqSave(&task->signalMaskLock);
    
    /* 
    1.如果该信号的处理方法是忽略，那么就要变成默认处理方法，才会被处理
    2.由于要强制发送信号，那么该信号是不能被屏蔽的，于是要清除屏蔽
     */
    if (task->signals.action[signo - 1].handler == SIG_IGN)
        task->signals.action[signo - 1].handler = SIG_DFL;

    sigdelset(&task->signalBlocked, signo); /* 清除屏蔽阻塞 */ 

    /* 计算一下是否有信号需要处理 */
    CalcSignalLeft(task);
    
    SpinUnlockIrqSave(&task->signalMaskLock, flags);

    /* 正式发送信号过去 */
    return DoSendSignal(task->pid, signo, CurrentTask()->pid);
}

/**
 * SendBranch - 发出信号给分支
 * @pid: 接收信号进程
 * @signal: 信号值
 * @sender: 信号发送者
 * 
 * pid > 0: 发送给pid进程
 * pid = 0：发送给当前进程的进程组
 * pid = -1：发送给有权的所有进程
 * pid < -1：发送给-pid进程组的进程
 * 
 * 根据pid选择发送方式
 */
PRIVATE int SendBranch(pid_t pid, int signal, pid_t sender)
{
    if (pid > 0) {  /* 发送给单个进程 */
        return DoSendSignal(pid, signal, sender);
    } else if (pid == 0) { /* 发送给当前进程的进程组 */
        return DoSendSignalGroup(CurrentTask()->groupPid, signal, sender);
    } else if (pid == -1) { /* 发送给有权利发送的所有进程 */
        int retval = 0, count = 0, err;
        struct Task *task;
        struct Task *cur = CurrentTask();
        
        ListForEachOwner(task, &taskGlobalList, globalList) {
            if (task->pid > 0 && task != cur) {
                err = DoSendSignal(task->pid, signal, cur->pid);
                count++;
                if (err == -1)      /* 如果有错误的，就记录成错误 */
                    retval = err;
            }
        }
        return retval;
    } else {    /* pid < 0，发送给-pid组标识的进程 */
        //printk("pid is %d\n", pid);
        return DoSendSignalGroup(-pid, signal, sender);
    }
}

/**
 * SysKill - 通过系统调用发送信号
 * @pid: 接收信号进程
 * @signal: 信号值
 * 
 */
PUBLIC int SysKill(pid_t pid, int signal)
{

    return SendBranch(pid, signal, CurrentTask()->pid);
}

/**
 * SysSignal - 设定信号的处理函数
 * @signal: 信号
 * @handler: 处理函数
 * 
 */
PUBLIC int SysSignal(int signal, sighandler_t handler)
{
    /* 检测是否符合范围，并且不能是SIGKILL和SIGSTOP，这两个信号不允许设置响应 */
    if (signal < 1 || signal >= MAX_SIGNAL_NR ||
        (handler && (signal == SIGKILL || signal == SIGSTOP))) {
        return -1;
    }

    /* 设定信号处理函数 */
    SignalAction_t sa;
    sa.handler = handler;
    sa.flags |= SA_ONESHOT;     /* signal设定的函数都只捕捉一次 */
    
    struct Task *cur = CurrentTask();

    /* 当需要修改信号行为的时候，就需要上锁 */
    SpinLock(&cur->signals.signalLock);
    
    if (handler) {
        /* 设置信号行为 */
        SetSignalAction(&cur->signals, signal, &sa);

        /* 如果处理函数是忽略，或者是默认并且信号是SIGCONT,SIGCHLD,SIGWINCH，
        按照POSIX标准，需要把已经到达的信号丢弃 */
        if (sa.handler == SIG_IGN || (sa.handler == SIG_DFL && 
            (signal == SIGCONT || signal == SIGCHLD || signal == SIGWINCH))) {
            
            /* 如果需要修改信号未决和信号阻塞，就需要获取锁 */
            SpinLock(&cur->signalMaskLock);

            /* 将信号从未决信号集中删除 */
            if (DelSignalFromTask(signal, &cur->signalPending)) {
                CalcSignalLeft(cur);
            }

            SpinUnlock(&cur->signalMaskLock);
        }
    }

    SpinUnlock(&cur->signals.signalLock);
    return 0;
}


/**
 * SysSignalAction - 设定信号的处理行为
 * @signal: 信号
 * @act: 处理行为
 * @oldact: 旧的处理行为
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int SysSignalAction(int signal, struct sigaction *act, struct sigaction *oldact)
{
    /* 检测是否符合范围，并且不能是SIGKILL和SIGSTOP，这两个信号不允许设置响应 */
    if (signal < 1 || signal >= MAX_SIGNAL_NR ||
        (act && (signal == SIGKILL || signal == SIGSTOP))) {
        return -1;
    }

    struct Task *cur = CurrentTask();

    SignalAction_t sa;

    /* 当需要修改信号行为的时候，就需要上锁 */
    SpinLock(&cur->signals.signalLock);
    
    /* 备份旧的行为 */
    if (oldact) {
        GetSignalAction(&cur->signals, signal, &sa);
        /* 复制数据 */
        oldact->sa_flags = sa.flags;
        oldact->sa_handler = sa.handler;
        //printk("old act handler %x\n", oldact->sa_handler);

    }

    if (act) {
        sa.flags = act->sa_flags;
        sa.handler = act->sa_handler;

        //printk("new act handler %x\n", act->sa_handler);
        /* 设置信号行为 */
        SetSignalAction(&cur->signals, signal, &sa);

        /* 如果处理函数是忽略，或者是默认并且信号是SIGCONT,SIGCHLD,SIGWINCH，
        按照POSIX标准，需要把已经到达的信号丢弃 */
        if (sa.handler == SIG_IGN || (sa.handler == SIG_DFL && 
            (signal == SIGCONT || signal == SIGCHLD || signal == SIGWINCH))) {
            
            /* 如果需要修改信号未决和信号阻塞，就需要获取锁 */
            SpinLock(&cur->signalMaskLock);

            /* 将信号从未决信号集中删除 */
            if (DelSignalFromTask(signal, &cur->signalPending)) {
                CalcSignalLeft(cur);
            }

            SpinUnlock(&cur->signalMaskLock);
        }
    }

    SpinUnlock(&cur->signals.signalLock);
    return 0;
}

/**
 * SysSignalProcessMask - 设置进程的信号阻塞集
 * @how: 怎么设置阻塞
 * @set: 阻塞集
 * @oldset: 旧阻塞集
 * 
 * 如果set不为空，就要设置新集，如果oldset不为空，就要把旧集复制过去
 * 
 * return: 成功返回0，失败返回-1
 */
PUBLIC int SysSignalProcessMask(int how, sigset_t *set, sigset_t *oldset)
{
    struct Task *cur = CurrentTask();

    /* 如果需要修改信号未决和信号阻塞，就需要获取锁 */
    SpinLock(&cur->signalMaskLock);

    /* 有旧集就输出 */
    if (oldset != NULL) {
        *oldset = cur->signalBlocked;
    }

    if (how == SIG_BLOCK) {     /* 把新集中为1的添加到阻塞中 */
        /* 有新集就输入 */
        if (set != NULL) {
            cur->signalBlocked |= *set;

            /* 不能阻塞SIGKILL和SIGSTOP */
            sigdelset(&cur->signalBlocked, SIGKILL);
            sigdelset(&cur->signalBlocked, SIGSTOP);
        }
    } else if (how == SIG_UNBLOCK) {    /* 把新集中为1的从阻塞中删除 */
        /* 有新集就输入 */
        if (set != NULL) {
            cur->signalBlocked &= ~*set;
        }
    } else if (how == SIG_SETMASK) {    /* 直接设置阻塞集 */
        /* 有新集就输入 */
        if (set != NULL) {
            cur->signalBlocked = *set;

            /* 不能阻塞SIGKILL和SIGSTOP */
            sigdelset(&cur->signalBlocked, SIGKILL);
            sigdelset(&cur->signalBlocked, SIGSTOP);
        }
    } else {

        SpinUnlock(&cur->signalMaskLock);
        return -1;
    }

    SpinUnlock(&cur->signalMaskLock);
    return 0;
}

/**
 * SysSignalProcessMask - 设置进程的信号阻塞集
 * @how: 怎么设置阻塞
 * @set: 阻塞集
 * @oldset: 旧阻塞集
 * 
 * 如果set不为空，就要设置新集，如果oldset不为空，就要把旧集复制过去
 * 
 * return: 成功返回0，失败返回-1
 */
PUBLIC int SysSignalPending(sigset_t *set)
{
    if (set == NULL)
        return -1;

    struct Task *cur = CurrentTask();
    
    /* 如果需要修改信号未决和信号阻塞，就需要获取锁 */
    SpinLock(&cur->signalMaskLock);

    /* 保存未决信号 */
    *set = cur->signalPending;

    SpinUnlock(&cur->signalMaskLock);

    return 0;
}

/**
 * SysSignalPause - 信号暂停
 * 
 * Pause比较特别，不只是收到信号，而且还要信号捕捉处理完成才可以
 * 已经捕捉信号返回1，否则返回0
 */
PUBLIC int SysSignalPause()
{
    struct Task *cur = CurrentTask();
    
    /* 阻塞前进行检测，如果已经捕捉了，就返回 */
    if (cur->signalCatched) {
        return 1;
    }
    
    /* 设置成未捕捉，并休眠 */
    cur->signalCatched = 0;
    
    //printk("will blocked.");
    /* 阻塞，直到有信号唤醒 */
    TaskBlock(TASK_BLOCKED);

    //printk("blocked %x wake up. %x", cur->signalBlocked, cur->signals.action[SIGINT-1].handler);
    
    /* 唤醒后还要查看是否信号已经被捕捉，已经被捕捉就是1，没有就是0 */
    return cur->signalCatched;
}

/**
 * SysSignalSuspend - 信号延缓的上半部分
 * 
 * Suspend和pause类似，不过需要在pause之前设置新的mask屏蔽。
 * 并且，还要在mask和进入pause之间保持原子（不能被其它信号中断）
 * 但是，我们仅在这里实现上半部分。下半部分到用户态实现
 * 
 * 已经捕捉信号返回1，否则返回0
 */
PUBLIC int SysSignalSuspend(sigset_t *set, sigset_t *oldSet)
{
    if (set != NULL && oldSet != NULL) {
        /* 设置进程屏蔽 */
        SysSignalProcessMask(SIG_SETMASK, set, oldSet);
    }
    //printk("ready to suspend.\n");
    /* 进入等待 */
    return SysSignalPause();
}
