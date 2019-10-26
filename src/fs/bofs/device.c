/*
 * file:		fs/bofs/device.c
 * auther:		Jason Hu
 * time:		2019/9/18
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <driver/ide.h>
#include <book/device.h>
#include <share/vsprintf.h>

#include <fs/bofs/device.h>
#include <fs/bofs/inode.h>
#include <fs/bofs/dir_entry.h>
#include <fs/bofs/super_block.h>
#include <fs/bofs/bitmap.h>

#include <book/blk-buffer.h>

/**
 * BOFS_MakeNewDevice - 创建一个设备文件
 * @sb: 超级块
 * @parentDir: 父母了
 * @name: 名字
 * @type: 类型
 * @deviceID: 设备号
 * 
 * 把设备纳入文件系统管理的范围
 */
PRIVATE int BOFS_MakeNewDevice(struct BOFS_SuperBlock *sb,
    struct BOFS_DirEntry *parentDir,
    char *name,
    char type,
    int deviceID)
{
    /**
	 * 1.分配节点ID
	 * 2.创建节点
	 * 3.创建目录项
	 */
	struct BOFS_Inode inode;
	
	int inodeID = BOFS_AllocBitmap(sb, BOFS_BMT_INODE, 1); 
	if (inodeID == -1) {
		printk(PART_ERROR "BOFS alloc inode bitmap failed!\n");
		return -1;
	}
	//printk("alloc inode id %d\n", inodeID);
	
	/* 把节点id同步到磁盘 */
	BOFS_SyncBitmap(sb, BOFS_BMT_INODE, inodeID);
    
	BOFS_CreateInode(&inode, inodeID, (BOFS_IMODE_R | BOFS_IMODE_W),
		0, sb->deviceID);

    /* 指向要描述的设备的设备id */
    inode.otherDeviceID = deviceID;

    //BOFS_DumpInode(&inode);

    /* 如果不是一个设备目录就返回 */
    if (type != BOFS_FILE_TYPE_BLOCK && type != BOFS_FILE_TYPE_CHAR && \
        type != BOFS_FILE_TYPE_NET) {
        printk("make new device: not a device type!\n");
        return -1;
    }

    struct BOFS_DirEntry childDir;

	// 创建目录项
	BOFS_CreateDirEntry(&childDir, inodeID, type, 0, name);
	
	memset(sb->iobuf, 0, SECTOR_SIZE);
	BOFS_SyncInode(&inode, sb);

	int result = BOFS_SyncDirEntry(parentDir, &childDir, sb);
	
	if(result > 0){	//successed
		if(result == 1){
			/* 当创建的目录不是使用的无效的目录项时才修改父目录的大小 */
			BOFS_LoadInodeByID(&inode, parentDir->inode, sb);
			
			inode.size += sizeof(struct BOFS_DirEntry);
			BOFS_SyncInode(&inode, sb);
		}
		return 0;
	}
    return -1;
}

/**
 * BOFS_MakeDevice - 在该目录下面创建一个
 * @path: 设备创建的路径
 * @type: 设备的类型
 * @major: 设备的主设备号
 * @minor：设备的次设备号
 */
PUBLIC int BOFS_MakeDevice(const char *path, char type, int deviceID)
{
    
/*
    if (DeviceCheckID_OK(deviceID)) {
        printk("device id error!\n");
        return -1;
    }*/

    /* 获取文件名 */
	char name[BOFS_NAME_LEN];
	memset(name,0,BOFS_NAME_LEN);
	if(!BOFS_PathToName(path, name)){
		//printk("path to name sucess!\n");
	}else{
		printk("path to name failed!\n");
		return -1;
	}

    /* 选择一个超级块 */
    struct BOFS_SuperBlock *sb = masterSuperBlock;

    struct BOFS_DirSearchRecord record;
	
    int found = BOFS_SearchDir((char *)path, &record, sb);
	
    /* 如果找到了设备文件 */
	if(!found){
        //printk("device has exist!\n");
		/* 把目录项信息重新写入磁盘 */
		
		BOFS_CloseDirEntry(record.childDir);
		
    } else {
        /* 创建设备 */	
        if (BOFS_MakeNewDevice(record.superBlock, record.parentDir, name, type, deviceID)) {
            BOFS_CloseDirEntry(record.parentDir);
			return -1;
        }
		
    }
	BOFS_CloseDirEntry(record.parentDir);
	return 0;
}

