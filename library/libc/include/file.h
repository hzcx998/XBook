/*
 * file:		include/lib/file.h
 * auther:		Jason Hu
 * time:		2019/9/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_FILE_H
#define _LIB_FILE_H

#include "stdint.h"
#include "stddef.h"
#include "types.h"
#include "time.h"


/* 高4位是属性位 */
#define S_IFSOCK 0x90    //scoket
#define S_IFLNK 0x50     //符号连接
#define S_IFIFO 0x30     //先进先出
#define S_IFBLK 0x80     //区块装置
#define S_IFCHR 0x40     //字符装置
#define S_IFDIR 0x20     //目录
#define S_IFREG 0x10     //一般文件

#define S_IREAD 0x04     //文件所有者具可读取权限
#define S_IWRITE 0x02    //文件所有者具可写入权限
#define S_IEXEC 0x01     //文件所有者具可执行权限

//上述的文件类型在POSIX中定义了检查这些类型的宏定义：

#define S_ISREG (st_mode & S_IFREG)    //是否为一般文件
#define S_ISDIR (st_mode & S_IFDIR)    //是否为目录
#define S_ISCHR (st_mode & S_IFCHR)    //是否为字符设备
#define S_ISBLK (st_mode & S_IFBLK)        //是否为块设备
#define S_ISFIFO (st_mode & S_IFIFO)     //先进先出
#define S_ISLNK (st_mode & S_IFLNK)    //判断是否为符号连接
#define S_ISSOCK (st_mode & S_IFSOCK)   //是否为socket

/* 文件模式 */
#define M_IREAD  (1 << 2)     //文件可读取权限
#define M_IWRITE (1 << 1)    //文件可写入权限
#define M_IEXEC  (1 << 0)     //文件可执行权限


/* 获取文件的日期 */
#define FILE_TIME_HOU(data) ((unsigned int)((data >> 11) & 0x1f))
#define FILE_TIME_MIN(data) ((unsigned int)((data >> 5) & 0x3f))
#define FILE_TIME_SEC(data) ((unsigned int)((data & 0x1f) *2))

#define FILE_TIME_YEA(data) ((unsigned int)(((data >> 9) & 0x7f)+1980))
#define FILE_TIME_MON(data) ((unsigned int)((data >> 5) & 0xf))
#define FILE_TIME_DAY(data) ((unsigned int)(data & 0x1f))

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
long tell(int fd);
int isfoot(int fd);
void redirect(unsigned int oldfd, unsigned int newfd);
int fsync(int fd);

#define MAX_STREAM_FILE_NR  32
#define __CONFIG_STREAM_FILE_BUF

typedef struct file_struct {
    int fd;     /* 文件描述符 */
    char flags;  /* 文件标志 */
    char *buf;  /* 数据缓冲区 */
    int pos;    /* 缓冲区的位置 */
} FILE;

FILE *fopen(const char *path, const char *mode);
int fclose(FILE *stream);

int fputc(int c, FILE *fp);
int fputs(const char *s, FILE *fp);
int fprintf(FILE *fp, const char *fmt, ...);

int fgetc(FILE *fp);
char *fgets(char *str, int num, FILE *fp);

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fread(const void *ptr, size_t size, size_t nmemb, FILE *stream);
int feof(FILE *fp);
int fflush(FILE *fp);

char *getenv(const char *name);

int fileno(FILE *fp);
FILE *freopen(const char *fileName, const char *type, FILE *stream);


/* 获取字符宏 */
#define getc(fp)    fgetc(fp)

int ferror(FILE *stream);

FILE *tmpfile(void);

int ungetc(int c, FILE *stream);

void clearerr(FILE *stream);


long ftell(FILE *stream);
int fseek(FILE *stream, long offset, int fromwhere);


#define _IOFBF      0X01
#define _IOLBF      0X02
#define _IONBF      0X04

int setvbuf(FILE *stream, char *buffer, int mode, size_t size);

char *tmpnam(char *str);


#endif  /* _LIB_FILE_H */
