/*
 * file:		include/lib/const.h
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_CONST_H
#define _LIB_CONST_H

/* 系统信息 */
#define OS_NAME     "Book OS -X"
#define OS_VERSION  "v0.6.9"

/* 内存常量的定义 */
#define KB      1024
#define MB      (1024*KB)
#define GB      (1024*MB)

/* 扇区大小的定义 */
#define SECTOR_SIZE      512

/* 路径长度 */
#define MAX_PATH_LEN 256

/* 管道缓冲区的大小 */
#define PIPE_BUF      4096

#endif  /*_LIB_CONST_H*/
