/*
 * file:		   include/share/unistd.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */
#ifndef _SHARE_UNISTD_H_
#define _SHARE_UNISTD_H_

#include <share/stdint.h>

#define R_OK 4 /* Test for read permission. */
#define W_OK 2 /* Test for write permission. */
#define X_OK 1 /* Test for execute permission. */
#define F_OK 0 /* Test for existence. */

int __brk(void * addr);
void *__sbrk(int increment);

#define brk     __brk
#define sbrk    __sbrk

#define MAX    1024

/* protect flags */
#define PROT_NONE           0x0       /* page can not be accessed */
#define PROT_READ           0x1       /* page can be read */
#define PROT_WRITE          0x2       /* page can be written */
#define PROT_EXEC           0x4       /* page can be executed */

/* map flags */
#define MAP_FIXED           0x10      /* Interpret addr exactly */

#endif  /*_SHARE_UNISTD_H_*/