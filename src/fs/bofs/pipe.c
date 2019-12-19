/*
 * file:		fs/bofs/pipe.c
 * auther:		Jason Hu
 * time:		2019/12/12
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/device.h>
#include <book/ioqueue.h>

#include <fs/bofs/inode.h>
#include <fs/bofs/file.h>
#include <fs/bofs/pipe.h>

#include <share/math.h>

/* 今天是双12，我没有网购，只买了一瓶美年达，然后创建了一个pipe.c的文件 */

/**
 * BOFS_IsPipe - 检测文件是否是管道文件
 * @localFd: 进程局部fd
 * 
 * 是则返回1，不是则返回0
 */
PUBLIC bool BOFS_IsPipe(unsigned int localFd)
{
    /*if (localFd > 2) 
        printk("check pipe local %d\n>>>", localFd);
    */
    unsigned int globalFd = FdLocal2Global(localFd);
    
    /*if (localFd > 2) 
        printk("<<<");
    */
    if (BOFS_GlobalFdHasFlags(globalFd, BOFS_FLAGS_PIPE)) {
        return 1;
    } else {
        return 0;
    }
}

PUBLIC int BOFS_PipeRecordReadTask(struct BOFS_Pipe *pipe, pid_t pid)
{
    int i;
    for (i = 0; i < MAX_PIPE_PER_TASK_NR; i++) {
        /* 如果已经存在，再次记录就会出错 */
        if (pipe->readTaskTable[i] == pid) {
            return -1;
        }
        if (pipe->readTaskTable[i] == -1) {
            break;
        }
    }
    if (i >= MAX_PIPE_PER_TASK_NR) 
        return -1;
    
    /* 记录pid */
    pipe->readTaskTable[i] = pid;
    return 0;
}

PUBLIC int BOFS_PipeRecordWriteTask(struct BOFS_Pipe *pipe, pid_t pid)
{
    int i;
    for (i = 0; i < MAX_PIPE_PER_TASK_NR; i++) {
        /* 如果已经存在，再次记录就会出错 */
        if (pipe->writeTaskTable[i] == pid) {
            return -1;
        }
        if (pipe->writeTaskTable[i] == -1) {
            break;
        }
    }
    if (i >= MAX_PIPE_PER_TASK_NR) 
        return -1;
    
    /* 记录pid */
    pipe->writeTaskTable[i] = pid;
    return 0;
}

PUBLIC int BOFS_PipeEraseReadTask(struct BOFS_Pipe *pipe, pid_t pid)
{
    int i;
    for (i = 0; i < MAX_PIPE_PER_TASK_NR; i++) {
        if (pipe->readTaskTable[i] == pid) {
            pipe->readTaskTable[i] = -1;
            return 0;
        }
    }
    return -1;
}

PUBLIC int BOFS_PipeEraseWriteTask(struct BOFS_Pipe *pipe, pid_t pid)
{
    int i;
    for (i = 0; i < MAX_PIPE_PER_TASK_NR; i++) {
        if (pipe->writeTaskTable[i] == pid) {
            pipe->writeTaskTable[i] = -1;
            return 0;
        }
    }
    return -1;
}

/**
 * BOFS_PipeInTable - 检测任务在哪个表中
 * 
 * 在读表中，返回0，
 * 写表中，返回1，
 * 不在表中，返回-1
 */
PUBLIC int BOFS_PipeInTable(struct BOFS_Pipe *pipe, pid_t pid)
{
    int i;
    for (i = 0; i < MAX_PIPE_PER_TASK_NR; i++) {
        if (pipe->readTaskTable[i] == pid) {
            return 0;
        }
        if (pipe->writeTaskTable[i] == pid) {
            return 1;
        }
    }
    return -1;
}

PUBLIC int BOFS_PipeInit(struct BOFS_Pipe *pipe)
{
    /* 分配缓冲区 */
    unsigned char *buf = kmalloc(PIPE_SIZE, GFP_KERNEL);
    if (buf == NULL) {
        return -1;
    }

    /* 初始化io队列 */
    IoQueueInit(&pipe->ioqueue, buf, PIPE_SIZE, IQ_FACTOR_8);

    /* 设置引用计数位0 */
    AtomicSet(&pipe->readReference, 0);
    AtomicSet(&pipe->writeReference, 0);
    
    INIT_LIST_HEAD(&pipe->waitList);

    int i;
    for (i = 0; i < MAX_PIPE_PER_TASK_NR; i++) {
        pipe->readTaskTable[i] = -1;    /* -1表示没有任务 */
        pipe->writeTaskTable[i] = -1;    /* -1表示没有任务 */
    }

    return 0;
}

/**
 * BOFS_Pipe - 创建管道文件
 * @fd: 管道文件描述符，2个
 * 
 * 把创建好的管道安装到fd[]中
 */
PUBLIC int BOFS_Pipe(int fd[2])
{
    /*
    1.分配读写文件
    2.创建管道
    3.初始化读写文件和管道
    4.安装文件描述符到任务中
    */
    struct BOFS_FileDescriptor *files[2];

    /* 分配文件 */
    int readFd = BOFS_AllocFdGlobal();
    if (readFd == -1) {
        goto ToFailed;
    }
    int writeFd = BOFS_AllocFdGlobal();
    if (writeFd == -1) {
        goto ToFreeReadFd;
    }

    /* 分配管道 */
    struct BOFS_Pipe *pipe = kmalloc(sizeof(struct BOFS_Pipe), GFP_KERNEL);
    if (pipe == NULL) {
        goto ToFreeWriteFd;
    }

    if (BOFS_PipeInit(pipe)) {
        goto ToFreePipe;
    }

    /* 管道的读写引用都为1 */
    AtomicSet(&pipe->readReference, 1);
    AtomicSet(&pipe->writeReference, 1);
    
    /* 获取文件 */
    files[0] = BOFS_GetFileByFD(readFd);
    files[1] = BOFS_GetFileByFD(writeFd);
    
    /* 节点都指向管道 */
    files[0]->pipe = pipe;
    files[1]->pipe = pipe;
    
    files[0]->pos = 0;  /* 读管道文件 */
    files[1]->pos = 1;  /* 写管道文件 */

    files[0]->flags |= BOFS_FLAGS_PIPE;     /* 管道文件 */
    files[1]->flags |= BOFS_FLAGS_PIPE;       /* 管道文件 */

    /* 设置文件引用 */
    AtomicSet(&files[0]->reference, 1);
    AtomicSet(&files[1]->reference, 1);
    
    /* 安装到描述符中 */
    fd[0] = TaskInstallFD(readFd);
    fd[1] = TaskInstallFD(writeFd);

    return 0;
ToFreePipe:
    kfree(pipe);
ToFreeWriteFd:
    BOFS_FreeFdGlobal(writeFd);
ToFreeReadFd:
    BOFS_FreeFdGlobal(readFd);
ToFailed:
    return -1;
}

PUBLIC unsigned int BOFS_PipeRead(int fd, void *buffer, size_t count)
{
    //printk("in BOFS_PipeRead, fd %d\n", fd);
    /* 获取全局描文件述符 */
    unsigned int globalFd = FdLocal2Global(fd);

    struct BOFS_FileDescriptor *file = BOFS_GetFileByFD(globalFd);

    unsigned char *buf = (unsigned char *)buffer;
    size_t bytesRead = 0;
    /* 获取输入输出队列 */
    struct IoQueue *ioqueue = (struct IoQueue *)&file->pipe->ioqueue;
    unsigned int iolen;
    /* 判断写端是否关闭 */
    if (AtomicGet(&file->reference) > 0) {

        /* 尝试获取一个字符，如果成功，继续往后，不然就会阻塞 */
        *buf++ = IoQueueGet(ioqueue);
        bytesRead++;
        
        /* 获取较小的数据量 */
        iolen = MIN(count - 1, IO_QUEUE_LENGTH(ioqueue));

        /* 没有关闭，正常读取 */
        while (iolen > 0) {
            *buf++ = IoQueueGet(ioqueue);
            bytesRead++;
            iolen--;
        }
    } else {
        /* 已经关闭，读取剩余量 */
        iolen = IO_QUEUE_LENGTH(ioqueue);
        while (iolen > 0) {
            *buf++ = IoQueueGet(ioqueue);
            bytesRead++;
            iolen--;
        }
    }
    return bytesRead;
}

PUBLIC unsigned int BOFS_PipeWrite(int fd, void *buffer, size_t count)
{
    //printk("in BOFS_PipeWrite, fd %d\n", fd);

    /* 获取全局描文件述符 */
    unsigned int globalFd = FdLocal2Global(fd);
    struct BOFS_FileDescriptor *file = BOFS_GetFileByFD(globalFd);

    unsigned char *buf = (unsigned char *)buffer;
    size_t bytesWrite = 0;
    
    /* 获取输入输出队列 */
    struct IoQueue *ioqueue = (struct IoQueue *)&file->pipe->ioqueue;
    /* 判断写端是否关闭 */
    if (AtomicGet(&file->reference) > 0) {
        while (count > 0) {
            IoQueuePut(ioqueue, *buf++);
            bytesWrite++;
            count--;
        }
    } else {
        /* 读端已经关闭，发出SIGPIPE信号 */
        printk(PART_ERROR "pipe write occur a SIGPIPE!\n");
    } 
    return bytesWrite;
}

PUBLIC void BOFS_PipeClose(struct BOFS_FileDescriptor *file)
{
    //printk("pipe ref %d\n", AtomicGet(&file->reference));
    
    /* 主动close的时候会减少一个，自动close的时候会再进入之前做一个减少，导致可能为负 */
    AtomicDec(&file->reference);

    if (AtomicGet(&file->reference) <= 0) {
        struct BOFS_Pipe *pipe = file->pipe;

        if (file->pos == 0) {
            /* 是读文件，把管道读引用设置为0 */
            AtomicSet(&pipe->readReference, 0);
            //printk("will free read pipe file\n");
        } else if (file->pos == 1) {
            /* 是写文件，把管道写引用设置为0 */
            AtomicSet(&pipe->writeReference, 0);
            //printk("will free write pipe file\n");
        }
        BOFS_FreeFileDescriptorByPionter(file);
        
        /* 如果管道读写引用都为0，那么就释放管道 */
        if (AtomicGet(&pipe->readReference) == 0 && 
            AtomicGet(&pipe->writeReference) == 0) {
            
            printk("will free pipe\n");
            /* 释放缓冲区 */
            kfree(pipe->ioqueue.buf);
            /* 释放管道 */
            kfree(pipe);
        }
    }
}  
