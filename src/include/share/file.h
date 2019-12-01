/*
 * file:		include/share/file.h
 * auther:		Jason Hu
 * time:		2019/9/20
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _SHARE_FILE_H
#define _SHARE_FILE_H

#include <share/unistd.h>
#include <share/stdint.h>
#include <share/stddef.h>

#define S_IFMT   0170000    //文件类型的位遮罩
#define S_IFSOCK 0140000    //scoket
#define S_IFLNK 0120000     //符号连接
#define S_IFREG 0100000     //一般文件
#define S_IFBLK 0060000     //区块装置
#define S_IFDIR 0040000     //目录
#define S_IFCHR 0020000     //字符装置
#define S_IFIFO 0010000     //先进先出

#define S_ISUID 04000     //文件的(set user-id on execution)位
#define S_ISGID 02000     //文件的(set group-id on execution)位
#define S_ISVTX 01000     //文件的sticky位


#define S_IREAD 00400     //文件所有者具可读取权限
#define S_IWRITE 00200    //文件所有者具可写入权限
#define S_IEXEC 00100     //文件所有者具可执行权限

#define S_IRUSR S_IREAD     //文件所有者具可读取权限
#define S_IWUSR S_IWRITE    //文件所有者具可写入权限
#define S_IXUSR S_IEXEC     //文件所有者具可执行权限

#define S_IRGRP 00040             //用户组具可读取权限
#define S_IWGRP 00020             //用户组具可写入权限
#define S_IXGRP 00010             //用户组具可执行权限

#define S_IROTH 00004             //其他用户具可读取权限
#define S_IWOTH 00002             //其他用户具可写入权限
#define S_IXOTH 00001             //其他用户具可执行权限

//上述的文件类型在POSIX中定义了检查这些类型的宏定义：
#define S_ISLNK (st_mode)    //判断是否为符号连接
#define S_ISREG (st_mode)    //是否为一般文件
#define S_ISDIR (st_mode)    //是否为目录
#define S_ISCHR (st_mode)    //是否为字符装置文件
#define S_ISBLK (s3e)        //是否为先进先出
#define S_ISSOCK (st_mode)   //是否为socket

struct stat {
	size_t st_size;         //以字节为单位的文件容量
	mode_t st_mode;         //文件访问权限
    ino_t st_ino;           //索引节点号
    
	dev_t st_dev;           //文件使用的设备号
    dev_t st_rdev;          //设备文件的设备号
    
    time_t st_ctime;	    //最后一次改变该文件状态的时间
	time_t st_mtime;	    //最后一次修改该文件的时间
	time_t st_atime;	    //最后一次访问该文件的时间
	
    blksize_t st_blksize;   //包含该文件的磁盘块的大小
    blkcnt_t  st_blocks;    //该文件所占的磁盘块
};

int open(const char *path, unsigned int flags);
int close(int fd);
int read(int fd, void *buffer, unsigned int size);
int write(int fd, void *buffer, unsigned int size);
int lseek(int fd, unsigned int offset, char flags);
int stat(const char *path, struct stat *buf);
int remove(const char *pathname);
int ioctl(int fd, int cmd, int arg);
int getmode(const char* pathname);
int setmode(const char* pathname, int mode);
int fcntl(int fd, int cmd, long arg);

#endif  /* _SHARE_FILE_H */
