/*
 * file:		include/lib/dir.h
 * auther:		Jason Hu
 * time:		2019/9/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_DIR_H
#define _LIB_DIR_H

/* 目录名长度 */
#define NAME_MAX 255

/* 需要和文件系统中地目录类型一致 */
#define DIR_UNKNOWN         0X00
#define DIR_NORMAL          0X01
#define DIR_DIRECTORY       0X02
#define DIR_TASK            0X04
#define DIR_BLOCK           0X08
#define DIR_CHAR            0X10
#define DIR_NET             0X20
#define DIR_FIFO            0X40

#define DIR_INVALID         0X8000

/* 抽象的目录 */
typedef struct {
	int inode; 					/* inode number 索引节点号 */
	off_t off;                  /* 目录项在目录中的大小 */
    unsigned short type;			/* the type of name 文件类型 */
	char name[NAME_MAX+1]; 		/* file name (null-terminated) 文件名，最长256字符 */
} dirent;

/* 目录就是一个指针，指向文件系统的目录 */
typedef unsigned int DIR;

/* dir operation目录操作 */
int mkdir(const char *pathname);
int rmdir(const char *pathname);
int getcwd(char* buf, unsigned int size);
int chdir(const char *pathname);
int rename(const char *pathname, const char *name);

/* 目录打开读取关闭操作 */
DIR *opendir(const char *pathname);
void closedir(DIR *dir);
int readdir(DIR *dir, dirent *buf);
void rewinddir(DIR *dir);

#endif  /* _LIB_DIR_H */
