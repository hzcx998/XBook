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
    SYS_LOG = 0,
    SYS_MMAP,
    SYS_MUNMAP,
    SYS_FORK,
    SYS_GETPID, 
    SYS_EXECV,
    SYS_SLEEP,
    SYS_MSLEEP,
    SYS_EXIT,
    SYS_WAIT,
    SYS_BRK,
    SYS_OPEN,
    SYS_CLOSE,
    SYS_READ,
    SYS_WRITE,
    SYS_LSEEK,
    SYS_STAT,
    SYS_UNLINK,
    SYS_IOCTL,
    SYS_GETMODE,
    SYS_SETMODE,
    SYS_MKDIR,
    SYS_RMDIR,
    SYS_MOUNT,
    SYS_UNMOUNT,
    SYS_GETCWD,
    SYS_CHDIR,
    SYS_RENAME,
    SYS_OPENDIR,
    SYS_CLOSEDIR,
    SYS_READDIR,
    SYS_REWINDDIR,
    MAX_SYSCALL_NR,
};



#endif   /*_BOOK_SYSCALL_H*/
