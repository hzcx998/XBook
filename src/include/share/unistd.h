/*
 * file:		   include/share/unistd.h
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */
#ifndef _SHARE_UNISTD_H_
#define _SHARE_UNISTD_H_

#define R_OK 4 /* Test for read permission. */
#define W_OK 2 /* Test for write permission. */
#define X_OK 1 /* Test for execute permission. */
#define F_OK 0 /* Test for existence. */


int brk(void * addr);
void *sbrk(int increment);

#endif  /*_SHARE_UNISTD_H_*/