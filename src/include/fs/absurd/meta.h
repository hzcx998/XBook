/*
 * file:		include/fs/absurd/meta.h
 * auther:		Jason Hu
 * time:		2019/9/4
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _FS_ABSURD_META
#define _FS_ABSURD_META

#include <share/stdint.h>
#include <share/types.h>
#include <share/string.h>

typedef unsigned int metaid_t;

/* 在结构体后面加上 __attribute__ ((packed)) 就能保持原有大小 */

/*
最基础的数据是元信息和数据区域
*/
#define META_NAMELEN     254

#define MAX_META_NAMELEN     (META_NAMELEN+1)

/* 默认meta id数量 */
#define DEFAULT_META_ID_NR            8192

/* 元信息的名字（256字节） */
struct MetaName {
    char length;        /* 实际名字长度 */
    char string[];      /* 字符串 */
}__attribute__ ((packed));

/* 元信息的数据（8字节） */
struct MetaData {
    unsigned int location;      /* 数据区位置 */
    unsigned int sectors;       /* 数据区的扇区数 */
}__attribute__ ((packed));

enum {
    META_UNKNOWN = 0,
    META_SUPERBLOCK,
    META_DIRECTORY,
    META_FILE,
    META_DEVICE,
    MAX_META_NR
};

/* 元信息，保持4字节对齐 */
struct MetaInfo {
    
    /* id信息 */
    metaid_t id;
    
    /* 实际数据大小 */
    unsigned int size;      
    
    /* 自己所在的位置 */
    unsigned int selfLocation;      

    /* 自己占用的大小 */
    unsigned int selfSize;      

    /* 时间日期 */

    /* 描述类型 */
    unsigned char type;

    /* 设备号，记录所在的设备 */
    unsigned int deviceID;

    /* 数据 */
    struct MetaData data;       
    /* 名字总是放在最后，因为要用作柔性数组 */
    struct MetaName name;
}__attribute__ ((packed));

#define SIZEOF_META_INFO   sizeof(struct MetaInfo)

PUBLIC void SetMetaData(struct MetaInfo *meta, unsigned int location,
    unsigned int sectors);

PUBLIC void MetaInfoInit(struct MetaInfo *meta,
    char *name,
    unsigned int selfLocation,
    unsigned char type,
    unsigned int deviceID);

PUBLIC void ShowMetaInfo(struct MetaInfo *meta);

#endif  //_FS_ABSURD_META