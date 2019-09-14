/*
 * file:		fs/absurd/main.c
 * auther:		Jason Hu
 * time:		2019/9/1
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/deviceio.h>
#include <share/string.h>
#include <driver/ide.h>
#include <fs/absurd/meta.h>

PUBLIC void SetMetaData(struct MetaInfo *meta, unsigned int location,
    unsigned int sectors)
{
    meta->data.location = location;
    meta->data.sectors = sectors;
}

PRIVATE void SetMetaName(struct MetaInfo *meta, char *name)
{
    meta->name.length = strlen(name);

    /* 长度和4字节对齐 */
    meta->name.length = (meta->name.length + 4) & ~(3UL);
    
    /* 修复长度 */
    if (meta->name.length > META_NAMELEN) {
        meta->name.length = META_NAMELEN;
    }

    /* 复制名字 */
    memset(meta->name.string, 0, meta->name.length + 1);
    memcpy(meta->name.string, name, meta->name.length);
}

/**
 * MetaInfoInit - 初始化元信息
 * @meta: 元信息
 * @name: 名字
 * @type: 类型
 */
PUBLIC void MetaInfoInit(struct MetaInfo *meta,
    char *name,
    unsigned int selfLocation,
    unsigned char type,
    unsigned int deviceID)
{
    memset(meta, 0, SIZEOF_META_INFO);
    
    /*设置名字 */
    SetMetaName(meta, name);
    
    /* 位置 */
    meta->selfLocation = selfLocation;
    
    /* 设置类型 */
    meta->type = type;
    
    /* 设定设备 */
    meta->deviceID = deviceID;

    /* 最开始大小为0 */
    meta->size = 0;

    /* 还未分配id */
    meta->id = 0;

    /* 设定时间 */

    /* 计算自己的大小 */
    meta->selfSize = SIZEOF_META_INFO + meta->name.length;
}

/**
 * ShowMetaInfo - 显示元信息
 * @meta: 元信息
 */
PUBLIC void ShowMetaInfo(struct MetaInfo *meta)
{
    if (!meta)
        return;
    printk("\n" PART_TIP "----MetaInfo----\n");

    /* 元信息区域 */

    printk(PART_TIP "name: %s length %d\n", meta->name.string, 
        meta->name.length);
    
    printk(PART_TIP "id: %d size %x\n", meta->id, meta->size);
    
    printk(PART_TIP "self location %d self size %d bytes\n", 
        meta->selfLocation, meta->selfSize);

    printk(PART_TIP "device id %d\n", meta->deviceID);
    
    char *type;
    switch (meta->type)
    {
    case META_SUPERBLOCK:
        type = "superblock";
        break;
    case META_DIRECTORY:
        type = "directory";
        break;
    case META_FILE:
        type = "file";
        break;
    case META_DEVICE:
        type = "device";
        break;
    default:
        type = "unknown";
        break;
    }
    printk(PART_TIP "type: %s\n", type);
    
    /* 数据区域 */
    printk(PART_TIP "data location: %d sectors: %d\n", meta->data.location,
        meta->data.sectors);

}
