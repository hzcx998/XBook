/*
 * file:		include/lib/conio.h
 * auther:		Jason Hu
 * time:		2019/7/29
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _USER_LIB_STDLIB_H
#define _USER_LIB_STDLIB_H

int fork();
uint32_t getpid();
int execv(const char *path, const char *argv[]);

void msleep(int msecond);
void sleep(int second);

void exit(int status);
int _wait(int *status);

#endif  /* _USER_LIB_STDLIB_H */
