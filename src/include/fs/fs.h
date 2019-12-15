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

enum {
    D_UNKNOWN = 0,
    D_BLOCK,
    D_CHAR,
    D_NET,
};

/* sys基于ramdisk，运行在内存中 */
#define SYS_PATH_TASK   "sys:/tsk"
#define SYS_PATH_DEV    "sys:/dev"
#define SYS_PATH_DRIVE  "sys:/drv"
#define SYS_PATH_PIPE  "sys:/pip"


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
PUBLIC int SysReadDir(DIR *dir, dirent *buf);
PUBLIC void SysRewindDir(DIR *dir);
PUBLIC void SysListDir(const char *pathname);

/* 设备操作 */
PUBLIC int SysSync();

/* 设备操作 */
PUBLIC int SysMakeDev(const char *pathname, char type,
        int major, int minor, size_t size);
PUBLIC int SysMakeTsk(const char *pathname, pid_t pid);

/* 初始化文件系统 */
PUBLIC void InitFileSystem();

PUBLIC int AddTaskToFS(char *name, pid_t pid);
PUBLIC int DelTaskFromFS(char *name, pid_t pid);

/* 管道文件 */
PUBLIC int SysPipe(int fd[2]);
PUBLIC int SysMakeFifo(const char *pathname, mode_t mode);

#endif  /* _FS_SYMPHYSES_H */
