/*
 * file:		include/lib/unistd.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */
#ifndef _LIB_UNISTD_H_
#define _LIB_UNISTD_H_

/* file open 文件打开 */
#define O_RDONLY 0x01
#define O_WRONLY 0x02
#define O_RDWR 0x04
#define O_CREAT 0x08
#define O_TRUNC 0x10
#define O_APPEDN 0x20
#define O_EXEC 0x80

/* file seek */
#define SEEK_SET 1
#define SEEK_CUR 2
#define SEEK_END 3

/* file acesss 文件检测 */
#define R_OK 4 /* Test for read permission. */
#define W_OK 2 /* Test for write permission. */
#define X_OK 1 /* Test for execute permission. */
#define F_OK 0 /* Test for existence. */

int brk(void * addr);
void *sbrk(int increment);

int access(const char *filenpath, int mode);

unsigned int alarm(unsigned int seconds);


#endif  /*_LIB_UNISTD_H_*/