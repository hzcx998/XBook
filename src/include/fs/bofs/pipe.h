/*
 * file:		include/fs/bofs/pipe.h
 * auther:		Jason Hu
 * time:		2019/12/12
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/*

【读写规则】
1.如果所有写端的文件描述符都关闭，仍然读取，那么剩余数据读取后，再次读取返回0，就像到达文件末尾一样。
2、如果有写端没有关闭，但也没有写入数据，此时读取，剩余数据读取后，再次读取会阻塞，直到管道有数据可读后才返回。
3.如果所有读端的文件描述符都关闭，仍然写入，那么会发出SIGPIPE信号，默认终止进程，但可以捕捉
4、如果有读端没有关闭，但也没有读取数据，此时写入，写满时会阻塞，直到有空位置才写入数据，并返回。
*/

#ifndef _BOFS_PIPE_H
#define _BOFS_PIPE_H

#include <share/stdint.h>
#include <share/types.h>
#include <book/atomic.h>
#include <book/ioqueue.h>

/* 一个管道最多能被多少个任务占用 */
#define MAX_PIPE_PER_TASK_NR    32

/* 管道大小为1个页的大小 */
#define PIPE_SIZE    4096



/* 进程间通信管道 */
struct BOFS_Pipe {
    struct IoQueue ioqueue;     /* 输入输出队列 */
    Atomic_t readReference;     /* 读端引用 */
    Atomic_t writeReference;    /* 写端引用 */
    struct List waitList;       /* 打开时的等待队列 */

    /* 使用通道的任务的进程id */
    
    /* 读任务 */
    pid_t readTaskTable[MAX_PIPE_PER_TASK_NR];
    /* 写任务 */
    pid_t writeTaskTable[MAX_PIPE_PER_TASK_NR];
};

PUBLIC int BOFS_PipeInit(struct BOFS_Pipe *pipe);

PUBLIC bool BOFS_IsPipe(unsigned int localFd);
PUBLIC int BOFS_Pipe(int fd[2]);
PUBLIC unsigned int BOFS_PipeRead(int fd, void *buffer, size_t count);
PUBLIC unsigned int BOFS_PipeWrite(int fd, void *buffer, size_t count);
PUBLIC int BOFS_PipeInTable(struct BOFS_Pipe *pipe, pid_t pid);

PUBLIC int BOFS_PipeRecordReadTask(struct BOFS_Pipe *pipe, pid_t pid);
PUBLIC int BOFS_PipeRecordWriteTask(struct BOFS_Pipe *pipe, pid_t pid);
PUBLIC int BOFS_PipeEraseReadTask(struct BOFS_Pipe *pipe, pid_t pid);
PUBLIC int BOFS_PipeEraseWriteTask(struct BOFS_Pipe *pipe, pid_t pid);


#endif  /* _BOFS_PIPE_H */

