/*
 * file:		include/book/syscall.h
 * auther:		Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_SYSCALL_H
#define _BOOK_SYSCALL_H


typedef void* syscall_t;

enum {
    SYS_LOG = 0,            /* 0 */
    SYS_MMAP,               /* 1 */
    SYS_MUNMAP,             /* 2 */
    SYS_FORK,               /* 3 */
    SYS_GETPID,             /* 4 */
    SYS_EXECV,              /* 5 */
    SYS_SLEEP,              /* 6 */
    SYS_MSLEEP,             /* 7 */
    SYS_EXIT,               /* 8 */
    SYS_WAIT,               /* 9 */
    SYS_BRK,                /* 10 */
    SYS_OPEN,               /* 11 */
    SYS_CLOSE,              /* 12 */
    SYS_READ,               /* 13 */
    SYS_WRITE,              /* 14 */
    SYS_LSEEK,              /* 15 */
    SYS_STAT,               /* 16 */
    SYS_REMOVE,             /* 17 */
    SYS_IOCTL,              /* 18 */
    SYS_GETMODE,            /* 19 */
    SYS_SETMODE,            /* 20 */
    SYS_MKDIR,              /* 21 */
    SYS_RMDIR,              /* 22 */
    SYS_GETCWD,             /* 23 */
    SYS_CHDIR,              /* 24 */
    SYS_RENAME,             /* 25 */
    SYS_OPENDIR,            /* 26 */
    SYS_CLOSEDIR,           /* 27 */
    SYS_READDIR,            /* 28 */
    SYS_REWINDDIR,          /* 29 */
    SYS_ACCESS,             /* 30 */
    SYS_FCNTL,              /* 31 */
    SYS_FSYNC,              /* 32 */
    SYS_PIPE,               /* 33 */
    SYS_FIFO,               /* 34 */
    MAX_SYSCALL_NR,
};



#endif   /*_BOOK_SYSCALL_H*/
