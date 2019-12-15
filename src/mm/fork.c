/*
 * file:		mm/fork.c
 * auther:	    Jason Hu
 * time:		2019/8/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/schedule.h>
#include <book/arch.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/task.h>
#include <fs/fs.h>
#include <fs/bofs/file.h>

/* 中断返回 */
EXTERN void InterruptExit();

PRIVATE int CopyStructAndKstack(struct Task *childTask, struct Task *parentTask)
{
    /* 复制task所在的页，里面有结构体，0级栈，还有返回地址等 */
    memcpy(childTask, parentTask, PAGE_SIZE);

    /* 单独修改内容 */
    childTask->pid = ForkPid();
    childTask->elapsedTicks = 0;
    childTask->status = TASK_READY;
    childTask->ticks = childTask->priority;
    childTask->parentPid = parentTask->pid;
    /* 重新设置链表，在这里不使用ListDel，那样会删除父进程在队列中的情况
    所以这里就直接把队列指针设为NULL，后面会添加到链表中*/
    childTask->list.next = childTask->list.prev = NULL;
    childTask->globalList.next = childTask->globalList.prev = NULL;
    
    /* 和父进程有一样的ttydev */
    childTask->ttydev = parentTask->ttydev;

    /* 复制名字，在后面追加fork表明是一个fork的进程，用于测试 */
    //strcat(childTask->name, "_fork");
    return 0;
}

PRIVATE int CopyPageTable(struct Task *childTask, struct Task *parentTask)
{
    
    /* 开始内存的复制 */
    void *buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (!buf) {
        printk(PART_ERROR "CopyPageTable: kmalloc buf for data transform failed!\n");
        return -1;
    }
        
    /* 获取父目录的虚拟空间 */
    struct VMSpace *space = parentTask->mm->spaceMap;
    
    uint32_t progVaddr = 0;
    uint32_t paddr;
    
    /* 当空间不为空时就一直获取 */
    while (space != NULL) {
        /* 获取空间最开始地址 */
        progVaddr = space->start;
        // printk(PART_TIP "the space %x start %x end %x\n", space, space->start, space->end);
        /* 在空间中进行复制 */
        while (progVaddr < space->end) {
            /* 1.将进程空间中的数据复制到内核空间，切换页表后，
            还能访问到父进程中的数据 */
            memcpy(buf, (void *)progVaddr, PAGE_SIZE);

            /* 2.切换进程空间 */
            PageDirActive(childTask);

            /* 3.映射虚拟地址 */
            // 分配一个物理页
            paddr = AllocPage();
            if (!paddr) {
                printk(PART_ERROR "CopyPageTable: GetFreePage for vaddr failed!\n");
        
                /* 激活父进程并返回 */
                PageDirActive(parentTask);
                return -1;
            }
            // 页链接，从动态内存中分配一个页并且页保护是 用户，可写
            PageTableAdd(progVaddr, paddr, PAGE_US_U | PAGE_RW_W);

            /* 4.从内核复制数据到进程 */
            memcpy((void *)progVaddr, buf, PAGE_SIZE);

            /* 5.恢复父进程内存空间 */
            PageDirActive(parentTask);
            // printk(PART_TIP "copy at virtual address %x\n", progVaddr);
            /* 指向下一个页 */
            progVaddr += PAGE_SIZE;
        }
        /* 指向下一个空间 */
        space = space->next;
    }
    kfree(buf);
    return 0; 
}

PRIVATE int CopyVMSpace(struct Task *childTask, struct Task *parentTask)
{
    /* 空间头 */
    struct VMSpace *tail = NULL;
    /* 指向父任务的空间 */
    struct VMSpace *p = parentTask->mm->spaceMap;
    while (p != NULL) {
        /* 分配一个空间 */
        struct VMSpace *space = (struct VMSpace *)kmalloc(
                sizeof(struct VMSpace), GFP_KERNEL);
        if (space == NULL) {
            printk(PART_ERROR "CopyVMSpace: kmalloc for space failed!\n");
            return -1;
        }
            
        /* 复制空间信息 */
        *space = *p;
        /* 把下一个空间置空，后面加入链表 */
        space->next = NULL;

        /* 如果空间表头是空，那么就让空间表头指向第一个space */
        if (tail == NULL)
            childTask->mm->spaceMap = space;    
        else 
            tail->next = space; /* 让空间位于子任务的空间表的最后面 */

        /* 让空间头指向新添加的空间 */
        tail = space;

        /* 获取下一个空间 */
        p = p->next;
    }

    // printk(PART_TIP "#child space#\n");

    /* 打印子进程space */
    /* p = childTask->mm->spaceMap;
    while (p != NULL) {
        printk(PART_TIP "[child] space %x start %x end %x flags %x\n",
                p, p->start, p->end, p->flags);
        p = p->next;
    }*/

    return 0;
}

PRIVATE int BuildChildStack(struct Task *childTask)
{
    /* 1.让子进程返回0 */

    /* 获取中断栈框 */
    struct TrapFrame *frame = (struct TrapFrame *)(
            (uint32_t)childTask + PAGE_SIZE - sizeof(struct TrapFrame));
    /*
	printk("edi: %x esi: %x ebp: %x esp: %x\n", 
			frame->edi, frame->esi, frame->ebp, frame->esp);
	printk("ebx: %x edx: %x ecx: %x eax: %x\n", 
			frame->ebx, frame->edx, frame->ecx, frame->eax);
	printk("gs: %x fs: %x es: %x ds: %x\n", 
			frame->gs, frame->fs, frame->es, frame->ds);
	printk("err: %x eip: %x cs: %x eflags: %x\n", 
			frame->errorCode, frame->eip, frame->cs, frame->eflags);
	printk("esp: %x ss: %x\n", 
			frame->esp, frame->ss);
	*/

    //printk(PART_TIP "task at %x fram at %x\n", childTask, frame);
    /* 设置eax为0，就相当于设置了子任务的返回值为0 */
    frame->eax = 0;

    /* 线程栈我们需要的数据只有5个，即ebp，ebx，edi，esi，eip */
    struct ThreadStack *threadStack = (struct ThreadStack *)\
        ((uint32_t *)frame - 5);

    /* 把SwitchTo的返回地址设置成InterruptExit，直接从中断返回 */
    //threadStack->eip = (uint32_t)&InterruptExit;
    threadStack->eip = InterruptExit;
    
    //printk(PART_TIP "threadStack eip %x\n", threadStack->eip);
    /* 下面的赋值只是用来使线程栈构建更清晰，下面2行的赋值其实不必要，
    因为在进入InterruptExit之后会进行一系列pop把寄存器覆盖掉 */
    threadStack->ebp = threadStack->ebx = \
    threadStack->esi = threadStack->edi = 0;
    
    /* 把构建的线程栈的栈顶最为SwitchTo恢复数据时的栈顶 */
    childTask->kstack = (uint8_t *)&threadStack->ebp;
    //printk(PART_TIP "kstack %x\n", childTask->kstack);
    
    /* 2.为SwitchTo构建线程环境，在中断栈框下面 */
    //uint32_t *retAddrInThreadStack = (uint32_t *)frame - 1;

    /* 这3行只是为了梳理线程栈中的关系，不一定要写出来 */
    /* uint32_t *esiInInThreadStack = (uint32_t *)frame - 2;
    uint32_t *ediInInThreadStack = (uint32_t *)frame - 3;
    uint32_t *ebxInInThreadStack = (uint32_t *)frame - 4; */
    /* ebp在线程栈中的地址便是当时esp（0级栈的栈顶），也就是esp
    为 "(uint32_t *)frame - 5"*/
    //uint32_t *ebpInInThreadStack = (uint32_t *)frame - 5;

    /* 把SwitchTo的返回地址设置成InterruptExit，直接从中断返回 */
    // *retAddrInThreadStack = (uint32_t)&InterruptExit;

    /* 下面的赋值只是用来使线程栈构建更清晰，下面2行的赋值其实不必要，
    因为在进入InterruptExit之后会进行一系列pop把寄存器覆盖掉 */
    /* *ebpInInThreadStack = *ebxInInThreadStack = \
    *ediInInThreadStack = *esiInInThreadStack = 0;*/

    /* 把构建的线程栈的栈顶最为SwitchTo恢复数据时的栈顶 */
    //childTask->kstack = (uint8_t *)ebpInInThreadStack;

    return 0;
} 

PRIVATE int CopyPageTableAndVMSpace(struct Task *childTask, struct Task *parentTask)
{
    /* 复制页表内容，因为所有的东西都在里面 */
    if (CopyPageTable(childTask, parentTask))
        return -1;
    
    /* 复制VMSpace */
    if (CopyVMSpace(childTask, parentTask))
        return -1;
    
    return 0;
}

/**
 * CopyTask - 拷贝父进程的资源给子进程
 */
PRIVATE int CopyTask(struct Task *childTask, struct Task *parentTask)
{
    /* 1.复制任务结构体和内核栈道子进程 */
    if (CopyStructAndKstack(childTask, parentTask))
        return -1;
    
    /*
    printk(PART_TIP "in SysFork now. parent %s-%x child %s-%x\n", 
            parentTask->name, parentTask, childTask->name, childTask);
     */

    /* 2.初始化任务的内存管理器 */
    AllocTaskMemory(childTask);
    
    childTask->pgdir = CreatePageDir();
    if (childTask->pgdir == NULL) {
        printk(PART_ERROR "CopyTask: CreatePageDir failed!\n");
        return -1;
    }
    /* 3.复制页表和虚拟内存空间 */
    if (CopyPageTableAndVMSpace(childTask, parentTask))
        return -1;
    // printk(PART_TIP "CopyPageTableAndVMSpace\n");

    /* 4.构建线程栈和修改返回值 */
    BuildChildStack(childTask);
    //Spin("BuildChildStack");
    // printk(PART_TIP "BuildChildStack\n");

    /* 更新打开节点文件 */
    BOFS_UpdateInodeOpenCounts(childTask);

    return 0;
}

/**
 * SysFork - fork系统调用
 * 
 * 如果失败，应该回滚回收之前分配的内存
 * 创建一个和自己一样的进程
 * 返回-1则失败，返回0表示子进程自己，返回>0表示父进程
 */
PUBLIC pid_t SysFork()
{
    /* 保存之前状态并关闭中断 */
    enum InterruptStatus oldStatus = InterruptDisable();
    
    /* 把当前任务当做父进程 */
    struct Task *parentTask = CurrentTask();
    /* 为子进程分配空间 */
    struct Task *childTask = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (childTask == NULL) {
        printk(PART_ERROR "SysFork: kmalloc for child task failed!\n");
        return -1;
    }
        
    /* 当前中断处于关闭中，并且父进程有页目录表 */
    ASSERT(InterruptGetStatus() == INTERRUPT_OFF && \
    parentTask->pgdir != NULL);
    
    /* 复制进程 */
    if (CopyTask(childTask, parentTask)) {
        printk(PART_ERROR "SysFork: copy task failed!\n");
        kfree(childTask);
        return -1;
    }
    
    if (AddTaskToFS(childTask->name, childTask->pid)) {
        kfree(childTask->pgdir);
        kfree(childTask);
        return -1;
    }

    //Spin("test");
    /* 把子进程添加到就绪队列和全局链表 */
    // 保证不存在于链表中
    ASSERT(!ListFind(&childTask->list, &taskReadyList));
    // 添加到就绪队列
    ListAddTail(&childTask->list, &taskReadyList);

    // 保证不存在于链表中
    ASSERT(!ListFind(&childTask->globalList, &taskGlobalList));
    // 添加到全局队列
    ListAddTail(&childTask->globalList, &taskGlobalList);

    /* 恢复之前的状态 */
    InterruptSetStatus(oldStatus);

    /*
    printk(PART_TIP "task %s pid %d fork task %s pid %d\n", 
        parentTask->name, parentTask->pid, childTask->name, childTask->pid
    );*/
    

    //printk("fork return!\n");
    /* 返回子进程的pid */
    return childTask->pid;
}