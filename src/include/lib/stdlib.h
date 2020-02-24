/*
 * file:		include/lib/stdlib.h
 * auther:		Jason Hu
 * time:		2019/7/29
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_STDLIB_H
#define _LIB_STDLIB_H

#include "stdint.h"
#include "stddef.h"
#include "types.h"
#include "unistd.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

int fork(void);
int32_t getpid(void);

int setpgid(pid_t pid, pid_t pgid);
pid_t getpgid(pid_t pid);

int execv(const char *path, const char *argv[]);

void msleep(int msecond);
unsigned int sleep(unsigned int second);

void exit(int status);
int _wait(int *status);

void free(void *ptr);
void *malloc(size_t size);
void memory_state(void);
int malloc_usable_size(void *ptr);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t size);

int pipe(int fd[2]);
int mkfifo(const char *path, mode_t mode);

void graph(int offset, int size, void *buffer);

int system(const char *command);

#endif  /* _LIB_STDLIB_H */
