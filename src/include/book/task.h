/*
 * file:		include/book/task.h
 * auther:		Jason Hu
 * time:		2019/7/30
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_TASK_H
#define _BOOK_TASK_H

#include <lib/types.h>
#include <lib/const.h>
#include <lib/stddef.h>
#include <book/list.h>
#include <book/arch.h>
#include <book/vmspace.h>
#include <book/signal.h>
#include <book/timer.h>

/* 在线程中作为形参 */
typedef void ThreadFunc(void *);

enum TaskStatus {
    TASK_READY = 0,         /* 进程处于就绪状态 */
    TASK_RUNNING,           /* 进程正在运行中 */
    TASK_BLOCKED,           /* 进程由于某种原因被阻塞 */
    TASK_WAITING,           /* 进程处于等待子进程状态 */
    TASK_STOPPED,           /* 进程处于停止运行状态 */
    TASK_ZOMBIE,            /* 进程处于僵尸状态，退出运行 */
    TASK_DIED,              /* 进程处于死亡状态，资源已经被回收 */
};

struct ThreadStack {
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edi;
    uint32_t esi;

    /* 首次运行指向KernelThread，其它时候指向SwitchTo的返回地址 */
    void (*eip) (ThreadFunc *func, void *arg);

    uint32_t unusedRetaddr;
    ThreadFunc *function;   // 线程要调用的函数
    void *arg;  // 线程携带的参数
};

#define MAX_TASK_NAMELEN 32

#define TASK_DEFAULT_PRIO 5

#define TASK_STACK_MAGIC 0X19980325

/* 单个进程可以打开的最大文件数量 */
#define MAX_OPEN_FILES_IN_PROC 32

/* 工作者线程特权级 */
#define TASK_WORKER_PRIO 3

#define MAX_STACK_ARGC 16

typedef struct Task {
    uint8_t *kstack;                // 内核栈
    pid_t pid;                      // 自己的进程id
    pid_t parentPid;                // 父进程id
    pid_t groupPid;                 // 组id
    enum TaskStatus status;
    pde_t *pgdir;                   // 页目录表指针
    uint32_t priority;  
    uint32_t ticks;  
    uint32_t elapsedTicks;
    int exitStatus;                 // 退出时的状态
    char name[MAX_TASK_NAMELEN];
    
    char cwd[MAX_PATH_LEN];		//当前工作路径,指针
	
    int fdTable[MAX_OPEN_FILES_IN_PROC];    // 文件描述符表

    struct MemoryManager *mm;       // 内存管理
    struct List list;               // 处于所在队列的链表
    struct List globalList;         // 全局任务队列，用来查找所有存在的任务

    /* 信号相关 */
    uint8_t signalLeft;     /* 有信号未处理 */
    uint8_t signalCatched;     /* 有一个信号被捕捉，并处理了 */
    
    Signal_t signals;        /* 进程对应的信号 */
    sigset_t signalBlocked;  /* 信号阻塞 */
    sigset_t signalPending;     /* 信号未决 */
    Spinlock_t signalMaskLock;  /* 信号屏蔽锁 */

    /* alarm闹钟 */
    char alarm;                     /* 闹钟是否有效 */
    uint32_t alarmTicks;            /* 闹钟剩余的ticks计数 */
    uint32_t alarmSeconds;          /* 闹钟剩余的秒数 */

    struct Timer *sleepTimer;       /* 休眠的时候的定时器 */

    unsigned int stackMagic;         /* 任务的魔数 */
} Task_t;

#define SET_TASK_STATUS(task, stat) \
        (task)->status = stat

EXTERN struct List taskReadyList;
EXTERN struct List taskGlobalList;

PUBLIC void InitTasks();
PUBLIC void PrintTask();
PUBLIC void DumpTask(struct Task *task);

PUBLIC Task_t *CurrentTask();

PUBLIC void TaskBlock(enum TaskStatus state);
PUBLIC void TaskUnblock(struct Task *thread);

PUBLIC struct Task *ThreadStart(char *name, int priority, ThreadFunc func, void *arg);
PUBLIC void ThreadExit(struct Task *thread);
PUBLIC struct Task *InitFirstProcess(void *fileName, char *name);
PUBLIC struct Task *FindTaskByPid(pid_t pid);
PUBLIC void InitSignalInTask(struct Task *task);


PUBLIC pid_t ForkPid();

PUBLIC void TaskActivate(struct Task *task);
PUBLIC void AllocTaskMemory(struct Task *task);
PUBLIC void FreeTaskMemory(struct Task *task);
PUBLIC void PageDirActive(struct Task *task);
PUBLIC uint32_t *CreatePageDir();

PUBLIC uint32_t SysGetPid();
PUBLIC int SysSetPgid(pid_t pid, pid_t pgid);
PUBLIC pid_t SysGetPgid(pid_t pid);


PUBLIC void TaskYield();

/* fork.c */
PUBLIC pid_t SysFork();

/* exec.c */
PUBLIC int SysExecv(const char *path, const char *argv[]);
PUBLIC int SysExecv2(const char *path, const char *argv[]);
/* sleep_wakeup.c */
PUBLIC uint32_t TaskSleep(uint32_t ticks);
PUBLIC uint32_t SysSleep(uint32_t second);
PUBLIC void TaskWakeUp(struct Task *task);
PUBLIC void TaskSleepOn(struct Task *task);

/* exit_wait.c */
PUBLIC void SysExit(int status);
PUBLIC pid_t SysWait(int *status);

#endif   /*_BOOK_TASK_H*/
