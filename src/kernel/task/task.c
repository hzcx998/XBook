/*
 * file:		kernel/task/task.c
 * auther:	    Jason Hu
 * time:		2019/7/30
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/task.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/schedule.h>
#include <book/synclock.h>
#include <book/vmspace.h>
#include <book/semaphore.h>
#include <book/mutex.h>
#include <book/spinlock.h>
#include <lib/string.h>

/**
 * SwitchToUser - 跳转到用户态执行的开关 
 * @frame: 中断栈
 * 
 * 通过准备好的中断栈，进入到用户态执行
 */
EXTERN void UpdateTssInfo(struct Task *task);

PRIVATE pid_t nextPid;

/* 初始化链表头 */

// 就绪队列链表
// 全局队列链表，用来查找所有存在的任务
PUBLIC LIST_HEAD(taskGlobalList);

/* 优先级队列链表 */
PROTECT struct List taskPriorityQueue[MAX_PRIORITY_NR];

/* idle任务 */
PUBLIC Task_t *taskIdle;
/**
 * KernelThread - 执行内核线程
 * @function: 要执行的线程
 * @arg: 参数
 * 
 * 改变当前的执行流，去执行我们选择的内核线程
 */
PRIVATE void KernelThread(ThreadFunc *function, void *arg)
{
    EnableInterrupt();
    function(arg);
}

/**  
 * AllocatePid - 分配一个pid
 */
PRIVATE pid_t AllocatePid()
{
    return nextPid++;
}

/**  
 * FreePid - 释放一个pid
 */
PRIVATE void FreePid()
{
    --nextPid;
}

/**  
 * ForkPid - 分配一个pid
 * 
 * 调用私有的AllocatePid获取一个pid
 */
PUBLIC pid_t ForkPid()
{
    return AllocatePid();
}

/**
 * CreatePageDir - 创建页目录   
 */
PUBLIC uint32_t *CreatePageDir()
{
    // 分配一个页来当作页目录
    uint32_t *pageDirAddr = kmalloc(PAGE_SIZE, GFP_KERNEL);

    if (!pageDirAddr) {
        printk(PART_WARRING "kmalloc for CreatePageDir failed!\n");
        return NULL;
    }
    memset(pageDirAddr, 0, PAGE_SIZE);
    
    /* 复制页表内容,只复制内核部分 */
    memcpy((void *)((unsigned char *)pageDirAddr + 2048), 
            (void *)(PAGE_DIR_VIR_ADDR + 2048), 2048);

    /* 更新页目录表的页目录的物理地址，因为页目录表的最后一项时页目录表的物理地址 */
    uint32_t paddr = Vir2PhyByTable((uint32_t )pageDirAddr);
    /* 属性是 存在，系统，可写 */
    pageDirAddr[1023] = paddr | PAGE_P_1 | PAGE_US_S | PAGE_RW_W;
    return pageDirAddr;
}

/**
 * MakeTaskStack - 创建一个线程
 * @thread: 线程结构体
 * @function: 要去执行的函数
 * @arg: 参数
 */
PRIVATE void MakeTaskStack(struct Task *thread, ThreadFunc function, void *arg)
{
    /* 预留中断栈 */
    thread->kstack -= sizeof(struct TrapFrame);

    /* 预留线程栈 */
    thread->kstack -= sizeof(struct ThreadStack);
    /* 填写线程栈信息 */
    struct ThreadStack *threadStack = (struct ThreadStack *)thread->kstack;

    // 在KernelThread中去改变执行流，从而可以传递一个参数
    threadStack->eip = KernelThread;
    threadStack->function = function;
    threadStack->arg = arg;
    threadStack->ebp = threadStack->ebx = \
    threadStack->esi = threadStack->edi = 0;
}

PUBLIC void InitSignalInTask(struct Task *task)
{
    task->signalLeft = 0;
    task->signalCatched = 0;

    SpinLockInit(&task->signals.signalLock);
    int i;
    for (i = 0; i < MAX_SIGNAL_NR; i++) {
        /* SIGCHLD信号设置成忽略信号 */
        task->signals.action[i].handler = SIG_DFL;
        /*if (i == SIGCHLD) {
            task->signals.action[i - 1].handler = SIG_IGN;
        }*/
        task->signals.action[i].flags = 0;
        task->signals.sender[i] = -1;
    }
    AtomicSet(&task->signals.count, 0);
    
    /* 清空信号集 */
    sigemptyset(&task->signalBlocked);
    sigemptyset(&task->signalPending);

    SpinLockInit(&task->signalMaskLock);
}

/**
 * TaskInit - 初始化线程
 * @thread: 线程结构地址
 * @name: 线程的名字
 * @priority: 线程优先级
 */
PRIVATE void TaskInit(struct Task *thread, char *name, int priority)
{
    memset(thread, 0, sizeof(struct Task));
    // 复制名字
    strcpy(thread->name, name);

    thread->status = TASK_READY;
        
    // 修复优先级
    if (priority < 0)
        priority = 0;
    if (priority > MAX_PRIORITY_NR - 1)
        priority = MAX_PRIORITY_NR - 1;

    thread->priority = priority;
    thread->timeslice = 3;  /* 时间片大小默认值 */
    thread->ticks = thread->timeslice;
    
    thread->elapsedTicks = 0;
    
    /* 线程没有页目录表和虚拟内存管理 */
    thread->pgdir = NULL;
    thread->mm = NULL;

    thread->pid = AllocatePid();
    thread->groupPid = 0;       // 没有进程组id
    thread->parentPid = -1;
    thread->exitStatus = -1;

    memset(thread->cwd, 0, MAX_PATH_LEN);
    /* 默认的工作目录是根目录 */
    strcpy(thread->cwd, TASK_PWD_DEFAULT);
    
    // 设置中断栈为当前线程的顶端
    thread->kstack = (uint8_t *)(((uint32_t )thread) + TASK_KSTACK_SIZE);

    thread->stackMagic = TASK_STACK_MAGIC;

    /* 其余的文件描述符全部设置为-1 */
    unsigned char idx = 0;
    while (idx < MAX_OPEN_FILES_IN_PROC) {
        thread->fdTable[idx] = -1;      /* 设置为-1表示未使用 */
        idx++;
    }
    
    /* 信号相关 */
    InitSignalInTask(thread);

    /* 设置闹钟 */
    thread->alarm = 0;  /* 不设置闹钟 */
    thread->alarmTicks = 0;     /* 闹钟ticks为0 */
    thread->alarmSeconds = 0;   /* 闹钟时间为0 */
    
    /* 休眠定时器 */
    thread->sleepTimer = NULL;

    /* 窗口为空 */
    thread->window = NULL;
}

/**
 * AllocTaskMemory - 初始化任务的内存管理
 * @task: 任务
 */
PUBLIC struct Task *FindTaskByPid(pid_t pid)
{
    struct Task *task;
    /* 关闭中断 */
    unsigned long old = InterruptSave();

    ListForEachOwner(task, &taskGlobalList, globalList) {
        if (task->pid == pid) {
            InterruptRestore(old);
            return task;
        }
    }
    InterruptRestore(old);
    return NULL;
}

/**
 * AllocTaskMemory - 初始化任务的内存管理
 * @task: 任务
 */
PUBLIC void AllocTaskMemory(struct Task *task)
{
    // 初始化内存管理器
    task->mm = (struct MemoryManager *)kmalloc(sizeof(struct MemoryManager), GFP_KERNEL);
    if (!task->mm)
        Panic(PART_ERROR "kmalloc for task mm failed!\n"); 
    InitMemoryManager(task->mm);
}

/**
 * TaskMemoryInit - 初始化任务的内存管理
 * @task: 任务
 */
PUBLIC void FreeTaskMemory(struct Task *task)
{
    if (task->mm)
        kfree(task->mm);
}


/**
 * TaskPriorityQueueAddTail - 把任务添加到特权级队列末尾
 * @task: 任务
 * 
 */
PUBLIC void TaskPriorityQueueAddTail(struct Task *task)
{
    /* 添加到相应的优先级队列 */
    ASSERT(!ListFind(&task->list, &taskPriorityQueue[task->priority]));
    // 添加到就绪队列
    ListAddTail(&task->list, &taskPriorityQueue[task->priority]);
}

/**
 * TaskPriorityQueueAddHead - 把任务添加到特权级队列头部
 * @task: 任务
 * 
 */
PUBLIC void TaskPriorityQueueAddHead(struct Task *task)
{
    /* 添加到相应的优先级队列 */
    ASSERT(!ListFind(&task->list, &taskPriorityQueue[task->priority]));
    // 添加到就绪队列
    ListAdd(&task->list, &taskPriorityQueue[task->priority]);
}

/**
 * TaskGloablListAdd - 把任务添加到全局队列
 * @task: 任务
 */
PUBLIC void TaskGloablListAdd(struct Task *task)
{
    // 保证不存在于链表中
    ASSERT(!ListFind(&task->globalList, &taskGlobalList));
    // 添加到全局队列
    ListAddTail(&task->globalList, &taskGlobalList);
    
}

/**
 * IsTaskInPriorityQueue - 把任务添加到全局队列
 * @task: 任务
 */
PUBLIC int IsTaskInPriorityQueue(struct Task *task)
{
    int i;
    for (i = 0; i < MAX_PRIORITY_NR; i++) {
        if (ListFind(&task->list, &taskPriorityQueue[i])) {
            return 1;
        }
    }
    return 0;
}


/**
 * IsAllPriorityQueueEmpty - 判断优先级队列是否为空
 */
PUBLIC int IsAllPriorityQueueEmpty()
{
    int i;
    for (i = 0; i < MAX_PRIORITY_NR; i++) {
        if (!ListEmpty(&taskPriorityQueue[i])) {
            return 0;
        }
    }
    return 1;
}

/**
 * ThreadStart - 开始一个线程
 * @name: 线程的名字
 * @func: 线程入口
 * @arg: 线程参数
 */
PUBLIC struct Task *ThreadStart(char *name, int priority, ThreadFunc func, void *arg)
{
    
    // 创建一个新的线程结构体
    struct Task *thread = (struct Task *) kmalloc(TASK_KSTACK_SIZE, GFP_KERNEL);
    
    if (!thread)
        return NULL;
    
    // 初始化线程
    TaskInit(thread, name, priority);
    
    // TaskMemoryInit(thread);

    //printk("alloc a thread at %x\n", thread);
    // 创建一个线程
    MakeTaskStack(thread, func, arg);

    /* 操作链表时关闭中断，结束后恢复之前状态 */
    unsigned long flags = InterruptSave();

    TaskGloablListAdd(thread);
    TaskPriorityQueueAddTail(thread);
    
    InterruptRestore(flags);
    return thread;
}

/**
 * CurrentTask - 获取当前运行的任务
 * 
 * 通过esp来计算出任务结构体
 */
PUBLIC Task_t *CurrentTask()
{
    uint32_t esp;
    // 获取esp的值
    asm ("mov %%esp, %0" : "=g" (esp));
    
    /* 
    由于是在内核态，所以esp是内核态的值
    取esp整数部分并且减去一个PAGE即pcb起始地址
    内核栈，我们约定2个页的大小
    */
    /* 4k对齐，直接 esp & ~(4096UL-1)
        8k对齐却不能直接这么做，需要减4k后，再这么做。
        由于，任务的地址都是位于高端地址，所以减4k不会为负。
     */

    return (Task_t *)((esp - PAGE_SIZE) & ~(PAGE_SIZE-1));
}

/**
 * MakeMainThread - 为内核主线程设定身份
 * 
 * 内核主线程就是从boot到现在的执行流。到最后会演变成idle
 * 在这里，我们需要给与它一个身份，他才可以进行多线程调度
 */
PRIVATE void MakeMainThread()
{
    // 当前运行的就是主线程
    taskIdle = CurrentTask();
    
    /* 最开始设置为最佳优先级，这样才可以往后面运行。直到运行到最后，就变成IDLE优先级 */
    TaskInit(taskIdle, "idle", TASK_PRIORITY_BEST);

    /* 设置为运行中 */
    taskIdle->status = TASK_RUNNING;

    TaskGloablListAdd(taskIdle);
}

/**
 * TaskBlock - 把任务阻塞
 */
PUBLIC void TaskBlock(enum TaskStatus state)
{
    /*
    state有4种状态，分别是TASK_BLOCKED, TASK_WAITING, TASK_STOPPED, TASK_ZOMBIE
    它们不能被调度
    */
    ASSERT ((state == TASK_BLOCKED) || 
            (state == TASK_WAITING) || 
            (state == TASK_STOPPED) ||
            (state == TASK_ZOMBIE));
    // 先关闭中断，并且保存中断状态
    unsigned long flags = InterruptSave();
    
    // 改变状态
    struct Task *current = CurrentTask();
    //printk(PART_TIP "task %s blocked with status %d\n", current->name, state);
    current->status = state;
    
    // 调度到其它任务
    Schedule();

    // 恢复之前的状态
    InterruptRestore(flags);
}   

/**
 * TaskUnblock - 解除任务阻塞
 * @task: 要解除的任务
 */
PUBLIC void TaskUnblock(struct Task *task)
{
    /*
    state有2种状态，分别是TASK_BLOCKED, TASK_WAITING
    只有它们能被唤醒, TASK_ZOMBIE只能阻塞，不能被唤醒
    */
    /*ASSERT((task->status == TASK_BLOCKED) || 
        (task->status == TASK_WAITING) ||
        (task->status == TASK_STOPPED));
    */
    if (!((task->status == TASK_BLOCKED) || 
        (task->status == TASK_WAITING) ||
        (task->status == TASK_STOPPED))) {
        printk("can't unlock, %x status:%x", task, task->status);
    }

    // 先关闭中断，并且保存中断状态
    unsigned long eflags = InterruptSave();

    // 没有就绪才能够唤醒，并且就绪
    if (task->status != TASK_READY) {
        // 保证没有在就绪队列中
        ASSERT(!IsTaskInPriorityQueue(task));
        // 已经就绪是不能再次就绪的
        if (IsTaskInPriorityQueue(task)) {
            Panic("TaskUnblock: task has already in ready list!\n");
        }
        // 处于就绪状态
        task->status = TASK_READY;
        // 把任务放在最前面，让它快速得到调度
        TaskPriorityQueueAddHead(task);

            
    }
    InterruptRestore(eflags);
    
    // 恢复之前的状态
    //InterruptRestore(flags);
}

/**
 * StartProcess - 开始运行一个进程
 * @fileName: 进程的文件名
 */
PRIVATE void StartProcess(void *argv)
{
    /* 开启进程的时候，需要去执行init程序，所以这里
    调用execv来完成这个工作 */
    //SysExecv((char *)fileName, NULL);
    const char **arg = (const char **)argv;

    SysExecv((char *)arg[0], arg);
    
    printk("exec for first process failed!\n");
    /* 如果运行失败就停在这里 */
    //Panic("start init failed!\n");
}

/**
 * PageDirActive - 激活任务的页目录
 * @task: 任务
 */
PUBLIC void PageDirActive(struct Task *task)
{
    /* 进程线程都要重新设置cr3 
    但是有一个比较好一点的想法就是，
    检查前一个进程或线程，如果同属于一个页目录空间
    那么就不用重新设置cr3.这只是个猜想。^.^ 2019.8.1
    */
    uint32_t pageDirPhysicAddr = PAGE_DIR_PHY_ADDR;

    // 内核线程的页目录表是一样的，所以不会出现在pgdir里面
    if (task->pgdir != NULL) {
        // 获取转换后的地址
        pageDirPhysicAddr = Vir2PhyByTable((uint32_t )task->pgdir);
        //printk("task %s active pgdir\n", task->name);
    }
    // 修改cr3的值，切换页目录表    
    WriteCR3(pageDirPhysicAddr);
}

/**
 * TaskActivate - 激活任务
 * @task: 要激活的任务
 */
PUBLIC void TaskActivate(struct Task *task)
{
    /* 任务不能为空 */
    ASSERT(task != NULL);
    
    /* 设置为运行状态 */
    task->status = TASK_RUNNING;
    
    /* 激活任务的页目录表 */
    PageDirActive(task);

    /* 内核线程特权级为0，处理器进入中断时不会从tss中获取esp0，
    所以不需要更新tss的esp0，用户进程则需要*/
    if (task->pgdir) {
        UpdateTssInfo(task);
    }
}

/**
 * InitFirstProcess - 初始化第一个进程
 * @argv: 参数 
 * @name: 任务的名字
 */
PUBLIC struct Task *InitFirstProcess(char **argv, char *name)
{
    // 创建一个新的线程结构体
    struct Task *thread = (struct Task *) kmalloc(TASK_KSTACK_SIZE, GFP_KERNEL);
    if (!thread)
        return NULL;
    // 初始化线程
    TaskInit(thread, name, TASK_PRIORITY_USER);
    /* 由于是第一个进程，所以我们不占用其pid，归还分配的pid，
    并且把它设置成0，也就是说，他是init进程 */
    FreePid();
    /* 将pid设置为0，表明他是init进程 */
    thread->pid = 0;
    
    AllocTaskMemory(thread);
    // 创建页目录
    thread->pgdir = CreatePageDir();
    if (!thread->pgdir)
        return NULL;
    
    //printk("alloc a thread at %x\n", thread);
    // 创建一个线程
    MakeTaskStack(thread, StartProcess, (void *)argv);
    
    /* 操作链表时关闭中断，结束后恢复之前状态 */
    unsigned long flags = InterruptSave();

    /* 添加到队尾 */
    TaskGloablListAdd(thread);
    TaskPriorityQueueAddTail(thread);
    
    InterruptRestore(flags);
    return thread;
}

/**
 * TaskYield - 任务让出当前cpu占用
 * 
 * 相当于任务在这个调度轮回中不运行，下一个轮回才运行
 */
PUBLIC void TaskYield()
{
    struct Task *current = CurrentTask();
    /* 保存状态并关闭中断 */
    unsigned long flags = InterruptSave();
    
    /* 添加回特权级队列 */
    TaskPriorityQueueAddTail(current);
    
    /* 设置状态为就绪 */
    current->status = TASK_READY;

    /* 调度到其它任务，自己不运行 */
    Schedule();

    /* 当再次运行到自己的时候就恢复之前的状态 */
    InterruptRestore(flags);
}

/**
 * SysGetPid - 获取任务id
 */
PUBLIC uint32_t SysGetPid()
{
    return CurrentTask()->pid;
}

/**
 * SysSetPgid - 设置任务的进程组id
 * @pid: 要设置的进程的pid
 * @pgid: 要设置成的进程组id
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int SysSetPgid(pid_t pid, pid_t pgid)
{
    struct Task *task = FindTaskByPid(pid);
    if (task == NULL) 
        return -1;

    task->groupPid = pgid;

    return 0;
}

/**
 * SysGetPgid - 设置任务的进程组id
 * @pid: 要获取的进程的pid
 * 
 * 成功返回pgid，失败返回-1
 */
PUBLIC pid_t SysGetPgid(pid_t pid)
{
    struct Task *task = FindTaskByPid(pid);
    if (task == NULL) 
        return -1;

    return task->groupPid;
}

/**
 * PrintTask - 打印所有任务
 */
PUBLIC void PrintTask()
{
    printk(PART_TIP "\n----Task----\n");
    struct Task *task;
    ListForEachOwner(task, &taskGlobalList, globalList) {
        printk(PART_TIP "name %s pid %d ppid %d gpid %d status %d\n", 
            task->name, task->pid, task->parentPid, task->groupPid, task->status);
    }

}

/*
PRIVATE Spinlock_t consoleLock;

PRIVATE void lockPrintk(char *buf)
{
    unsigned long old = SpinLockSaveIntrrupt(&consoleLock);
    printk(buf);
    SpinUnlockRestoreInterrupt(&consoleLock, old);
}
*/

//PRIVATE Mutex_t consoleLock;

/*DEFINE_MUTEX(consoleLock); 

PRIVATE void lockPrintk(char *buf)
{
    MutexLock(&consoleLock);
    printk(buf);

    MutexUnlock(&consoleLock);

}*/

int testA = 0, testB = 0;
void ThreadA(void *arg)
{
    //char *par = arg;
    int i = 0;
    while (1) {
        i++;
        testA++;
        if (i%0x10000 == 0) {
            
            //lockPrintk("<abcdefghabcdef> ");
           
            //lockPrintk(par);
        }
/*
        if (i > 0xf000000)
            Panic("halt");*/
    }
}

void ThreadB(void *arg)
{
    //char *par = arg;
    int i = 0;
    // log("hello\n");
    while (1) {
        i++;
        testB++;
        if (i%0x10000 == 0) {
            
            //lockPrintk("[12345678123456] ");
            
            //SysMSleep(3000);
            //printk("%x ", testB);
        }
        /*if (i > 0xf000000)
            Panic("halt");*/
    }
}

/**
 * SysTaskScan - 任务扫描
 * 
 */
PUBLIC int SysTaskScan(taskscan_status_t *ts, unsigned int *idx)
{
    if (ts == NULL)
        return -1;

    /* 如果是0，就从第一个 */
    int n = 0;
    Task_t *task;
    ListForEachOwner (task, &taskGlobalList, globalList) {
        /* 到达了我们需求的数 */
        if (n == *idx) {
            /* 复制数据信息 */
            ts->ts_pid = task->pid;
            ts->ts_ppid = task->parentPid;
            ts->ts_gpid = task->groupPid;
            ts->ts_state = task->status;
            ts->ts_priority = task->priority;
            ts->ts_ticks = task->ticks;
            ts->ts_runticks = task->elapsedTicks;
            memset(ts->ts_name, 0, 32);
            strcpy(ts->ts_name, task->name);
            *idx = *idx + 1;
            return 0;
        }
        n++;
    }
    /* 已经到达末尾了 */
    return -1;
}

/**
 * SysGetVersion - 获取系统版本
 * @buf: 缓冲区
 * @buflen: 缓冲区长度
 */
PUBLIC void SysGetVersion(char *buf, int buflen)
{
    if (strlen(OS_NAME) + strlen(OS_VERSION) >= buflen) {
        memcpy(buf, "buf too short! :( ", buflen);    
    } else {
        strcpy(buf, OS_NAME);
        strcat(buf, OS_VERSION);
    }
}


PUBLIC void DumpTask(struct Task *task)
{
    printk(PART_TIP "----Task----\n");
    printk(PART_TIP "name:%s pid:%d parent pid:%d status:%d\n", task->name, task->pid, task->parentPid, task->status);
    printk(PART_TIP "pgdir:%x priority:%d ticks:%d elapsed ticks:%d\n", task->pgdir, task->priority, task->ticks, task->elapsedTicks);
    printk(PART_TIP "exit code:%d mm:%x stack mageic:%d\n", task->exitStatus, task->mm, task->stackMagic);
}

/* 配置直接运行应用程序，可用于应用开发调试 */
#define CONFIG_APP_DIRECT 1

PRIVATE char *initArgv[3];

/**
 * InitUserProcess - 初始化用户进程
 */
PUBLIC void InitUserProcess()
{
    /* 暂时的提示语言 */
#if CONFIG_APP_DIRECT == 0
	/* 加载init进程 */
    initArgv[0] =  "root:/init";
    initArgv[1] =  0;
#else
    /* 直接加载应用程序 */
    initArgv[0] =  "root:/init";
    initArgv[1] =  0;
#endif /* CONFIG_APP_DIRECT */

    InitFirstProcess(initArgv, "init");

}

/**
 * SystemPause - 系统暂停运行。
 * 
 * 在初始化的最后调用，当前任务演变成idle任务，等待随时调动
 */
PUBLIC void SystemPause()
{
    /* 设置特权级为最低，变成阻塞，当没有其它任务运行时，就会唤醒它 */
    taskIdle->status = TASK_BLOCKED;
    taskIdle->priority = TASK_PRIORITY_IDLE;
    /* 然后设置成最低特权级 */
    TaskPriorityQueueAddHead(taskIdle);
    /* 调度到其它任务，直到又重新被调度 */
    Schedule();

    /* idle线程 */
	while (1) {
		/* 进程默认处于阻塞状态，如果被唤醒就会执行后面的操作，
		知道再次被阻塞 */
		
        /* 打开中断 */
		EnableInterrupt();
		/* 执行cpu停机 */
		CpuHlt();

	};
}
/**
 * InitTasks - 初始化多任务环境
 */
PUBLIC void InitTasks()
{

    /* 初始化特权队列 */
    int i;
    for (i = 0; i < MAX_PRIORITY_NR; i++) {
        INIT_LIST_HEAD(&taskPriorityQueue[i]);
    }

    /* 跳过init进程的pid = 0，后面执行init的时候会把它的pid设置为0*/
    nextPid = 1;

    /* 最开始初始化init进程，让它的pid为0
    首先，在这里，先不执行init进程
     */
    // TaskExecute(0, "init");
    
    //SyncLockInit(&consoleLock);
    //MutexInit(&consoleLock);

    //DumpMutex(&consoleLock);
    MakeMainThread();
   
    /* 有可能做测试阻塞main线程，那么就没有线程，
    在切换任务的时候就会出错，所以这里创建一个测试线程 */
    //ThreadStart("test", 1, ThreadA, "NULL");
    
    //ThreadStart("test2", 1, ThreadB, "NULL");
    
	// 在初始化多任务之后才初始化任务的虚拟空间
	InitVMSpace();

    //while (1);
}
