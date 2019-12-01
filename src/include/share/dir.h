/*
 * file:		include/share/dir.h
 * auther:		Jason Hu
 * time:		2019/9/20
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _SHARE_DIR_H
#define _SHARE_DIR_H

/* 目录名长度 */
#define NAME_MAX 255

/* 抽象的目录 */
typedef struct {
	int inode; 					/* inode number 索引节点号 */
	off_t off;                  /* 目录项在目录中的大小 */
    unsigned char type;			/* the type of name 文件类型 */
	char name[NAME_MAX+1]; 		/* file name (null-terminated) 文件名，最长256字符 */
} DIRENT;

/* 目录就是一个指针，指向文件系统的目录 */
typedef unsigned int DIR;

/* dir operation目录操作 */
int mkdir(const char *pathname);
int rmdir(const char *pathname);
int getcwd(char* buf, unsigned int size);
int chdir(const char *pathname);
int rename(const char *pathname, char *name);

/* 目录打开读取关闭操作 */
DIR *opendir(const char *pathname);
void closedir(DIR *dir);
int readdir(DIR *dir, DIRENT *buf);
void rewinddir(DIR *dir);

#endif  /* _SHARE_DIR_H */
