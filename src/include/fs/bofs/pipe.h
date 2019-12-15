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

PUBLIC bool BOFS_IsPipe(unsigned int localFd);
PUBLIC int BOFS_Pipe(int fd[2]);
PUBLIC unsigned int BOFS_PipeRead(int fd, void *buffer, size_t count);
PUBLIC unsigned int BOFS_PipeWrite(int fd, void *buffer, size_t count);

#endif  /* _BOFS_PIPE_H */

