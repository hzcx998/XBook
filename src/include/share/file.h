/*
 * file:		include/share/file.h
 * auther:		Jason Hu
 * time:		2019/9/20
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _SHARE_FILE_H
#define _SHARE_FILE_H


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
#define F_OK 1
#define X_OK 2
#define R_OK 3
#define W_OK 4

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
