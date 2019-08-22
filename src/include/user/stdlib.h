/*
 * file:		include/lib/conio.h
 * auther:		Jason Hu
 * time:		2019/7/29
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _USER_LIB_STDLIB_H
#define _USER_LIB_STDLIB_H

#include <share/stdint.h>
#include <share/stddef.h>

int fork();
int32_t getpid();
int execv(const char *path, const char *argv[]);

void msleep(int msecond);
void sleep(int second);

void exit(int status);
int _wait(int *status);

void free(void *ptr);
void *malloc(size_t size);
void memory_state();
int malloc_usable_size(void *ptr);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t size);

#endif  /* _USER_LIB_STDLIB_H */
