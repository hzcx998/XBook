/*
 * file:		kernel/exit_wait.c
 * auther:	    Jason Hu
 * time:		2019/8/9
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/schedule.h>
#include <book/arch.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/task.h>

/**
 * AdopeChildren - 过继子进程给init进程
 */
PRIVATE void AdopeChildren(int parentPid)
{
    struct Task *child;
    //printk(PART_TIP "adope children now!\n");
    
    /* 在全局任务链表中查找父进程是自己的子进程 */
    ListForEachOwner(child, &taskGlobalList, globalList) {
        /* 如果进程的父pid和当前进程的pid一样，
        就说明这个进程就是当前进程的子进程 */
        if (child->parentPid == parentPid) {
            //printk(PART_TIP "find a child name %s, pid %d\n", child->name, child->pid);
    
            /* 把父进程的pid设置成0，而0是init进程的pid */
            child->parentPid = 0;
        }
    }
}


/**gl
 * AdopeChildren - 过继子进程给init进程
 */
PRIVATE void NotifyParent(int parentPid)
{
    /*  a.如果父进程处于休眠中，在等待子进程唤醒 
        b.如果父进程在等待我唤醒之前就远去，就会把我过继给init，
    所以这里也是没有问题的 
    */
    //printk(PART_TIP "notify parent now!\n");
    
    struct Task *parent;

    /* 在全局任务链表中查找子进程的父进程 */
    ListForEachOwner(parent, &taskGlobalList, globalList) {
        /* 如果进程的父pid和当前进程的pid一样，并且处于等待状态
        就说明这个进程就是当前进程的子进程 */
        if (parent->pid == parentPid && parent->status == TASK_WAITING) {
            /*printk(PART_TIP "my parent %s pid %d status %d is waitting for me, wake up him!\n",
                    parent->name, parent->pid, parent->status);
             */
            /* 将父进程唤醒 */
            TaskUnblock(parent);

            /* 唤醒后就退出查询，因为只有1个父亲，不能有多个父亲吧（偷笑） */
            break;
        }
    }
}

/**
 * ReleaseZombie - 释放僵尸态的进程
 */
PRIVATE void ReleaseZombie(struct Task *task)
{
    /* 回收页目录 */
    kfree(task->pgdir);
    /* 回收MM */
    FreeTaskMemory(task);

    /* 从全局队列中删除 */
    ListDel(&task->globalList);
}

/**
 * SysExit - 进程退出运行
 * 
 */
PUBLIC void SysExit(int status)
{
    struct Task *current = CurrentTask();
    //printk(PART_TIP "task name %s exit now!\n", current->name);
    /* 保存退出状态 */
    current->exitStatus = status;

    /* 1.把子进程过继给init进程 */
    AdopeChildren(current->pid);

    /* 2.释放自己占用的资源 */
    MemoryManagerRelease(current->mm, VMS_RESOURCE | VMS_STACK | VMS_HEAP);
    
    /* 3.如果有父进程，就通知父进程我已经远去，来帮我收尸吧
    在完成父进程唤醒之前不能调度 */
    
    /* 保存之前状态并关闭中断 */
    enum InterruptStatus oldStatus = InterruptDisable();

    NotifyParent(current->parentPid);

    /* 恢复之前的状态 */
    InterruptSetStatus(oldStatus);

    //printk(PART_TIP "I am gone, don't miss me!\n");
    
    /* 4.不让自己运行，调度出去 */
    TaskBlock(TASK_ZOMBIE);
}
   

/**
 * SysWait - 等待进程
 * @status: 保存子进程的退出状态的地址
 * 
 * 返回子进程的pid
 */
PUBLIC pid_t SysWait(int *status)
{
    struct Task *current = CurrentTask();
    //printk(PART_TIP "task name %s wait now!\n", current->name);

    pid_t childPid = -1;

    /* 找到一个子进程的标志 */
    bool found = false;

    struct Task *child, *safe;

    /* 中断状态 */
    enum InterruptStatus oldStatus;

/* 在标签附件好像不能声明一个变量 */
ToRepeat:

    /* 保存之前状态并关闭中断 */
    oldStatus = InterruptDisable();

    /* 如果找到指定子进程会把它删除，所以这里要用safe */
    ListForEachOwnerSafe(child, safe, &taskGlobalList, globalList) {
        /* 如果子进程的父进程不是当前进程，就不管 */
        if (child->parentPid != current->pid) {
            continue;
        }

        /* 如果子进程的父进程是当前进程，就找到一个子进程 */
        found = true;
        //printk(PART_TIP "found a child task.\n");
        /* 如果不是变成zombie，就不管 */
        if (child->status != TASK_ZOMBIE) 
            continue;

        /* 子进程是zombie */

        /* 保存子进程的退出状态, 和pid */
        *status = child->exitStatus;
        childPid = child->pid;
        /*
        printk(PART_TIP "find a zombie task %s pid %d exit status %d",
            child->name, childPid, *status
        );
         */
        /* 把子进程的task结构体占用的资源释放掉 */
        ReleaseZombie(child);

        /* 有一个僵尸子进程被释放，就跳转到最后，结束等待 */
        goto EndWait;
    }
    /* 恢复中断状态 */
    InterruptSetStatus(oldStatus);

    /* 找到了一个进程，但是进程还不是zombie状态，就不能删除进程 */
    if (found) {
        //printk(PART_TIP "my child is not zombie.\n");
        
        /* 休眠，等待子进程将自己唤醒 */
        TaskBlock(TASK_WAITING);

        /* 被唤醒后会在这里执行，就重复检测 */
        goto ToRepeat;
    }

    /* 没有找到进程，也就是说，没有子进程 */
EndWait:
    /* 返回等待到的子进程的pid，或者-1即没有子进程 */
    return childPid;
}