/*
 * file:		include/share/file.h
 * auther:		Jason Hu
 * time:		2019/9/20
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _SHARE_FILE_H
#define _SHARE_FILE_H

#include <share/unistd.h>

struct stat {
    unsigned int st_type;
	unsigned int st_size;
	unsigned int st_mode;
	unsigned int st_device;
};

int open(const char *path, unsigned int flags);
int close(int fd);
int read(int fd, void *buffer, unsigned int size);
int write(int fd, void *buffer, unsigned int size);
int lseek(int fd, unsigned int offset, char flags);
int stat(const char *path, struct stat *buf);
int unlink(const char *pathname);
int ioctl(int fd, int cmd, int arg);
int getmode(const char* pathname);
int setmode(const char* pathname, int mode);

#endif  /* _SHARE_FILE_H */
