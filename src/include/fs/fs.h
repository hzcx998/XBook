/*
 * file:		include/fs/interface.h
 * auther:		Jason Hu
 * time:		2019/9/20
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

/*
symphyses: 交接处
其实就是借口的意思。
文件系统借口和某个文件系统的对接。
这样的目的是，如果想要更换成其它文件系统的话，就直接
修改交接函数就好了。
*/

#ifndef _FS_SYMPHYSES_H
#define _FS_SYMPHYSES_H

#include <share/const.h>
#include <share/file.h>
#include <share/dir.h>
#include <share/stddef.h>

/* file operation 文件操作 */
PUBLIC int SysOpen(const char *path, unsigned int flags);
PUBLIC int SysRead(int fd, void *buffer, unsigned int size);
PUBLIC int SysWrite(int fd, void *buffer, unsigned int size);
PUBLIC int SysLseek(int fd, unsigned int offset, char flags);
PUBLIC int SysClose(int fd);
PUBLIC int SysStat(const char *pathname, struct stat *buf);
PUBLIC int SysFcntl(int fd, int cmd, int arg);
PUBLIC int SysFsync(int fd);

PUBLIC int SysRemove(const char *pathname);
PUBLIC int SysIoctl(int fd, int cmd, int arg);
PUBLIC int SysGetMode(const char* pathname);
PUBLIC int SysChangeMode(const char* pathname, int mode);

/* dir operation目录操作 */
PUBLIC int SysMakeDir(const char *pathname);
PUBLIC int SysRemoveDir(const char *pathname);
PUBLIC int SysGetCWD(char* buf, unsigned int size);
PUBLIC int SysChangeCWD(const char *pathname);
PUBLIC int SysChangeName(const char *pathname, char *name);
PUBLIC int SysAccess(const char *pathname, mode_t mode);

/* 目录打开读取关闭操作 */
PUBLIC DIR *SysOpenDir(const char *pathname);
PUBLIC void SysCloseDir(DIR *dir);
PUBLIC int SysReadDir(DIR *dir, DIRENT *buf);
PUBLIC void SysRewindDir(DIR *dir);

/* 初始化文件系统 */
PUBLIC void InitFileSystem();

#endif  /* _FS_SYMPHYSES_H */
