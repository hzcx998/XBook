/*
 * file:		fs/bofs/device.c
 * auther:		Jason Hu
 * time:		2019/11/30
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <book/device.h>
#include <share/vsprintf.h>

#include <fs/bofs/inode.h>
#include <fs/bofs/dir_entry.h>
#include <fs/bofs/super_block.h>
#include <fs/bofs/bitmap.h>
#include <fs/bofs/drive.h>
#include <fs/bofs/device.h>

/**
 * BOFS_CreateNewDevice - 创建一个新的设备文件
 * @sb: 超级块儿
 * @parentDir: 父目录
 * @childDir: 子目录
 * @name: 目录项的名字
 * @devno: 设备号
 * 
 * 在父目录中创建一个设备文件
 * @return: 成功返回-1，失败返回0
 */
PRIVATE int BOFS_CreateNewDevice(struct BOFS_SuperBlock *sb,
	struct BOFS_DirEntry *parentDir,
	struct BOFS_DirEntry *childDir,
	char *name,
    char type,
    dev_t devno,
    size_t size)
{
	/**
	 * 1.分配节点ID
	 * 2.创建节点
	 * 3.创建目录项
     * 4.修改节点数据
	 */
	struct BOFS_Inode inode;
	
	int inodeID = BOFS_AllocBitmap(sb, BOFS_BMT_INODE, 1); 
	if (inodeID == -1) {
		printk(PART_ERROR "BOFS alloc inode bitmap failed!\n");
		return -1;
	}
	///printk("alloc inode id %d\n", inodeID);
	
	/* 把节点id同步到磁盘 */
	BOFS_SyncBitmap(sb, BOFS_BMT_INODE, inodeID);

    //printk("BOFS_SyncBitmap\n");
	
	BOFS_CreateInode(&inode, inodeID, (BOFS_IMODE_R|BOFS_IMODE_W),
		0, sb->devno);    
    //printk("BOFS_CreateInode\n");
	
	// 创建子目录项
    unsigned short dirType;

    
    switch (type)
    {
    case DEV_TYPE_BLOCK:
        dirType = BOFS_FILE_TYPE_BLOCK;
        break;
    case DEV_TYPE_CHAR:
        dirType = BOFS_FILE_TYPE_CHAR;
        break;
    default:
        dirType = BOFS_FILE_TYPE_UNKNOWN;
        break;
    }
	BOFS_CreateDirEntry(childDir, inodeID, dirType, name);

	inode.size = size;
    inode.blocks[0] = devno;    /* 把设备号保存到数据区 */
    
	BOFS_SyncInode(&inode, sb);

    //BOFS_DumpDirEntry(childDir);
	int result = BOFS_SyncDirEntry(parentDir, childDir, sb);
	
    //BOFS_DumpDirEntry(parentDir);

    //printk("sync result:%d\n", result);
    //printk("BOFS_SyncDirEntry\n");
	if(result > 0){	//successed
		if(result == 1){
			/* 当创建的目录不是使用的无效的目录项时才修改父目录的大小 */
			BOFS_LoadInodeByID(&inode, parentDir->inode, sb);
			
			inode.size += sizeof(struct BOFS_DirEntry);
			BOFS_SyncInode(&inode, sb);
            //printk("change parent size!");

           // BOFS_DumpInode(&inode);
		}
		return 0;
	}
    printk("sync child dir failed!\n");
	return -1;
}

/**
 * BOFS_MakeDevice - 创建一个设备
 * @pathname: 路径名
 * @devno: 设备号
 * @sb: 超级块
 * 
 * 创建一个设备文件，根据系统中的设备信息，创建到磁盘上。
 * @return: 成功返回0，失败返回-1
 */
PUBLIC int BOFS_MakeDevice(const char *pathname, char type, 
        dev_t devno, size_t size, struct BOFS_SuperBlock *sb)
{
	int i, j;
	
	char *path = (char *)pathname;

	//printk("The path is %s\n", path);
	
	char *p = (char *)path;
	
	char name[BOFS_NAME_LEN];   
	
	struct BOFS_DirEntry parentDir, childDir;

    /* 第一个必须是根目录符号 */
	if(*p != '/'){
		printk("mkdev: bad path!\n");
		return -1;
	}

	char depth = 0;
	//计算一共有多少个路径/ 
	while(*p){
		if(*p == '/'){
			depth++;
		}
		p++;
	}
	
	p = (char *)path;

    /* 根据深度进行检测 */
	for(i = 0; i < depth; i++){
		//跳过目录分割符'/'
        p++;

		/*----读取名字----*/

        /* 准备获取目录名字 */
		memset(name, 0, BOFS_NAME_LEN);
		j = 0;

        /* 如果没有遇到'/'就一直获取字符 */
		while(*p != '/' && j < BOFS_NAME_LEN){
			name[j] = *p;
			j++;
			p++;
		}
		//printk("dir name is %s\n", name);
		/*----判断是否没有名字----*/
		
        /* 如果'/'后面没有名字，就直接返回 */
		if(name[0] == 0){
			/* 创建的时候，如果没有指定名字，就只能返回。 */
			return -1;
		}
		/* 如果是在根目录下面查找，那么就需要把根目录数据复制给父目录 */
		if(i == 0){
			memcpy(&parentDir, sb->rootDir->dirEntry, sizeof(struct BOFS_DirEntry));
		}
		/* 搜索子目录 */
		if(BOFS_SearchDirEntry(sb, &parentDir, &childDir, name)){
			/* 找到后，就把子目录转换成父目录，等待下一次进行搜索 */
			memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));
			//printk("BOFS_MakeDir:depth:%d path:%s name:%s has exsit!\n",depth, pathname, name);
			/* 如果文件已经存在，并且位于末尾，说明我们想要创建一个已经存在的目录，返回失败 */
			if(i == depth - 1){
				printk("mkdev: path %s has exist, can't create it again!\n", path);
				return -1;
			}
		}else{
			/* 没找到，并且是在路径末尾 */
			if(i == depth - 1){
				/* 创建设备 */
                //printk("new device file:%s type:%d devno:%x\n", name, type, devno);
                /* 在父目录中创建一个设备 */	
                if (BOFS_CreateNewDevice(sb, &parentDir, &childDir, name, type, devno, size)) {
                    return -1;
                }
			}else{
				printk("mkdev:can't create path:%s name:%s\n", pathname, name);
				return -1;
			}
		}
	}
	/*if dir has exist, it will arrive here.*/
	return 0;
}
