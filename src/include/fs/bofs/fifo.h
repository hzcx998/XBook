/*
 * file:		include/fs/bofs/fifo.h
 * auther:		Jason Hu
 * time:		2019/12/15
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/*
【阻塞模式下】
在写打开的时候，如果没有读的进程，当前进程就会阻塞（非阻塞模式会返回ENIO错误）
    有读的进程，成功返回
在读打开的时候，如果没有写的进程，当前进程就会阻塞（非阻塞模式返回成功）
    有写的进程，成功返回

【生产者--消费者模型】
在写入的时候，如果管道满了，当前进程会阻塞
在读取的时候，如果管道为空，当前进程会阻塞

【读取】
如果写端关闭，管道中有数据就读取管道中的数据，并返回读取的数据量。
没有数据，读端将不会继续阻塞，此时返回0
如果请求的读取数量大于缓冲区中的数量，返回现有量。

【写入】
如果写入的数据量不大于PIPE_BUF时，写入是原子的，如果大于，
写入者将会阻塞，直到有足够大小可以写入。
只有读端存在，写端才有意义。如果读端不存在，写端向FIFO写数据，
内核向发送端发送SIGPIPE信号（默认终止进程）

*/

#ifndef _BOFS_FIFO_H
#define _BOFS_FIFO_H

#include <lib/stdint.h>
#include <lib/types.h>

#include <fs/bofs/super_block.h>
#include <fs/bofs/pipe.h>

PUBLIC int BOFS_MakeFifo(const char *pathname, mode_t mode, struct BOFS_SuperBlock *sb);

PUBLIC int BOFS_FifoOpen(int fd, flags_t flags);

PUBLIC int BOFS_FifoClose(struct BOFS_Pipe *pipe);

PUBLIC int BOFS_FifoRead(struct BOFS_FileDescriptor *file, void *buffer, size_t count);
PUBLIC int BOFS_FifoWrite(struct BOFS_FileDescriptor *file, void *buffer, size_t count);

PUBLIC int BOFS_FifoUpdate(struct BOFS_Pipe *pipe, struct Task *task);

#endif  /* _BOFS_FIFO_H */

