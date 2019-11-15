/*
 * file:		include/book/task.h
 * auther:		Jason Hu
 * time:		2019/7/30
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_TASK_H
#define _BOOK_TASK_H

#include <share/types.h>
#include <share/const.h>
#include <share/stddef.h>
#include <book/list.h>
#include <book/arch.h>
#include <book/vmspace.h>

/* 在线程中作为形参 */
typedef void ThreadFunc(void *);

enum TaskStatus {
    TASK_READY = 0,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_WAITING,
    TASK_ZOMBIE,
    TASK_DIED,
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

typedef int pid_t;

#define MAX_TASK_NAMELEN 32

#define MAX_PATH_LEN 256

#define TASK_DEFAULT_PRIO 5

#define TASK_STACK_MAGIC 0X19980325


/* 工作者线程特权级 */
#define TASK_WORKER_PRIO 3

#define MAX_STACK_ARGC 16
typedef struct Task {
    uint8_t *kstack;                // 内核栈
    pid_t pid;
    pid_t parentPid;
    enum TaskStatus status;
    pde_t *pgdir;                   // 页目录表指针
    uint32_t priority;  
    uint32_t ticks;  
    uint32_t elapsedTicks;
    int exitStatus;                 // 退出时的状态
    char name[MAX_TASK_NAMELEN];
    
    char cwd[MAX_PATH_LEN];		//当前工作路径,指针
	
    struct MemoryManager *mm;       // 内存管理
    struct List list;               // 处于所在队列的链表
    struct List globalList;         // 全局任务队列，用来查找所有存在的任务
    unsigned int stackMagic;         /* 任务的魔数 */
} Task_t;

EXTERN struct List taskReadyList;
EXTERN struct List taskGlobalList;

PUBLIC void InitTasks();
PUBLIC void PrintTask();

PUBLIC Task_t *CurrentTask();

PUBLIC void TaskBlock(enum TaskStatus state);
PUBLIC void TaskUnblock(struct Task *thread);

PUBLIC struct Task *ThreadStart(char *name, int priority, ThreadFunc func, void *arg);
PUBLIC void ThreadExit(struct Task *thread);
PUBLIC struct Task *InitFirstProcess(void *fileName, char *name);

PUBLIC pid_t ForkPid();

PUBLIC void TaskActivate(struct Task *task);
PUBLIC void AllocTaskMemory(struct Task *task);
PUBLIC void FreeTaskMemory(struct Task *task);
PUBLIC void PageDirActive(struct Task *task);
PUBLIC uint32_t *CreatePageDir();

PUBLIC uint32_t SysGetPid();

PUBLIC void TaskYield();

/* fork.c */
PUBLIC pid_t SysFork();

/* exec.c */
PUBLIC int SysExecv(const char *path, const char *argv[]);
PUBLIC int SysExecv2(const char *path, const char *argv[]);
/* sleep_wakeup.c */
PUBLIC void TaskSleep(uint32_t ticks);
PUBLIC int SysSleep(uint32_t second);
PUBLIC void TaskWakeUp(struct Task *task);
PUBLIC void TaskSleepOn(struct Task *task);

/* exit_wait.c */
PUBLIC void SysExit(int status);
PUBLIC pid_t SysWait(int *status);

#endif   /*_BOOK_TASK_H*/
