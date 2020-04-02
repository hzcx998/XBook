/*
 * file:		include/sys/stat.h
 * auther:		Jason Hu
 * time:		2020/3/11
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _LIB_STAT_H
#define _LIB_STAT_H

#include "stdint.h"


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

#endif  /* _LIB_STAT_H */
