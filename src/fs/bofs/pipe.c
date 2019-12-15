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

/**
 * BOFS_Pipe - 创建管道文件
 * @fd: 管道文件描述符，2个
 * 
 * 把创建好的管道安装到fd[]中
 */
PUBLIC int BOFS_Pipe(int fd[2])
{

    /* 分配一个全局文件描述符 */
    int globalFd = BOFS_AllocFdGlobal();
    if (globalFd == -1) {
        return -1;
    }

    /* 把file的inode当作数据区域，分配4KB给它 */
    struct BOFS_FileDescriptor *file = BOFS_GetFileByFD(globalFd);

    file->inode = kmalloc(PAGE_SIZE, GFP_KERNEL);
    if (file->inode == NULL) {
        BOFS_FreeFdGlobal(globalFd);
        return -1;
    }

    /* 初始化数据缓冲区 */
    struct IoQueue *ioqueue = (struct IoQueue *)file->inode;
    /* 初始化io队列，缓冲区位于ioqueue结构后面 */
    IoQueueInit(ioqueue, (unsigned int *)(ioqueue + 1), PAGE_SIZE - sizeof(struct IoQueue));

    /* 管道标志 */
    file->flags |= BOFS_FLAGS_PIPE;

    /* pos表明有管道打开次数 */
    file->pos = 2;

    /* 安装到描述符中 */
    fd[0] = TaskInstallFD(globalFd);
    fd[1] = TaskInstallFD(globalFd);
    
    return 0;
}

PUBLIC unsigned int BOFS_PipeRead(int fd, void *buffer, size_t count)
{
    unsigned char *buf = (unsigned char *)buffer;
    size_t bytesRead = 0;
    /* 获取全局描文件述符 */
    unsigned int globalFd = FdLocal2Global(fd);

    struct BOFS_FileDescriptor *file = BOFS_GetFileByFD(globalFd);

    /* 获取输入输出队列 */
    struct IoQueue *ioqueue = (struct IoQueue *)file->inode;

    unsigned int iolen = IO_QUEUE_LENGTH(ioqueue);

    /* 读取较小的缓冲，避免阻塞 */
    size_t size = MIN(iolen, count);
    while (bytesRead < size) {
        *buf = IoQueueGet(ioqueue);
        buf++;
        bytesRead++;
    }
    return bytesRead;
}

PUBLIC unsigned int BOFS_PipeWrite(int fd, void *buffer, size_t count)
{
    unsigned char *buf = (unsigned char *)buffer;
    size_t bytesWrite = 0;
    /* 获取全局描文件述符 */
    unsigned int globalFd = FdLocal2Global(fd);

    struct BOFS_FileDescriptor *file = BOFS_GetFileByFD(globalFd);

    /* 获取输入输出队列 */
    struct IoQueue *ioqueue = (struct IoQueue *)file->inode;

    /* 计算剩余数据量 */
    unsigned int ioleft = ioqueue->buflen / 4 - IO_QUEUE_LENGTH(ioqueue);

    /* 写入较小的缓冲，避免阻塞 */
    size_t size = MIN(ioleft, count);

    while (bytesWrite < size) {
        IoQueuePut(ioqueue, *buf);
        buf++;
        bytesWrite++;
    }
    return bytesWrite;
}
