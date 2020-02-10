/*
 * file:		include/book/iostream.h
 * auther:		Jason Hu
 * time:		2019/8/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_IOSTREAM_H
#define _BOOK_IOSTREAM_H

#include <lib/types.h>
#include <lib/stddef.h>

#define IOSTREAM_NAMELEN    32

#define IOSTREAM_SEEL_SET    1

/* 输入输出流类似于文件，只是说它是在内存中的流
提出背景：由于不能再深层次中进行文件的读写，所以，通过最开始
把文件加载到iostream中，然后再对文件读写，就是相当于对内存操作了
 */
struct IoStream {
    unsigned char *start;         /* 数据开始地址 */
    unsigned int pos;       /* 当前操作的位置 */
    unsigned int size;      /* 流的大小 */
    char name[IOSTREAM_NAMELEN];    /* 流的名字 */
};

struct IoStream *CreateIoStream(char *name, size_t size);
int IoStreamSeek(struct IoStream *stream, unsigned offset, char flags);
int IoStreamRead(struct IoStream *stream, void *buffer, size_t size);
int IoStreamWrite(struct IoStream *stream, void *buffer, unsigned int size);
void DestoryIoStream(struct IoStream *stream);

#endif   /* _BOOK_IOSTREAM_H */
