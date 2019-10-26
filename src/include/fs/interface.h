/*
 * file:		include/fs/interface.h
 * auther:		Jason Hu
 * time:		2019/9/20
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_INTERFACE
#define _FS_INTERFACE

#include <share/const.h>
#include <share/file.h>
#include <share/dir.h>

/* file operation 文件操作 */
int SysOpen(const char *path, unsigned int flags);
int SysRead(int fd, void *buffer, unsigned int size);
int SysWrite(int fd, void *buffer, unsigned int size);
int SysLseek(int fd, unsigned int offset, char flags);
int SysStat(const char *path, void *buf);
int SysClose(int fd);

int SysUnlink(const char *pathname);
int SysIoctl(int fd, int cmd, int arg);
int SysGetMode(const char* pathname);
int SysSetMode(const char* pathname, int mode);

/* dir operation目录操作 */
int SysMakeDir(const char *pathname);
int SysRemoveDir(const char *pathname);
int SysMountDir(const char *devpath, const char *dirpath);
int SysUnmountDir(const char *dirpath);
int SysGetCWD(char* buf, unsigned int size);
int SysChangeCWD(const char *pathname);
int SysChangeName(const char *pathname, char *name);

/* 目录打开读取关闭操作 */
DIR *SysOpenDir(const char *pathname);
void SysCloseDir(DIR *dir);
int SysReadDir(DIR *dir, DIRENT *buf);
void SysRewindDir(DIR *dir);

/* 初始化文件系统 */
void InitFileSystem();

#endif  /* _FS_INTERFACE */
