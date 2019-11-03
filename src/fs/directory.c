/*
 * file:		fs/directory.c
 * auther:		Jason Hu
 * time:		2019/10/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/share.h>
#include <book/memcache.h>
#include <book/arch.h>

#include <fs/flat.h>

#include <fs/file.h>
#include <fs/directory.h>
#include <fs/bitmap.h>

#include <book/blk-disk.h>
#include <book/device.h>

LIST_HEAD(directoryListHead);

/**
 * GetDirectoryByName - 通过名字获取一个目录
 * @name: 目录名
 * 
 * @return: 成功返回目录，失败返回NULL
 */
PUBLIC struct Directory *GetDirectoryByName(char *name)
{
	struct Directory *dir;
	ListForEachOwner(dir, &directoryListHead, list) {
		if (strcmp(dir->name, name) == 0) {
			return dir;
		}
	}
	return NULL;
}

/**
 * CreateDirectory - 创建目录
 * @name: 目录名
 * @type: 目录类型（设备目录和挂载目录）
 * 
 * 创建成功后返回创建的目录 
 */
PUBLIC struct Directory *CreateDirectory(char *name, char type)
{
	/* 检测是否已经存在 */
	if(GetDirectoryByName(name) != NULL) {
		printk("dir %s has exist!\n", name);
		return NULL;
	}

	struct Directory *dir = kmalloc(SIZEOF_DIRECTORY, GFP_KERNEL);
	if (dir == NULL)
		return NULL;
	
	/* 初始化链表 */
	ListAddTail(&dir->list, &directoryListHead);

	memset(dir->name, 0, DIR_NAME_LEN);
	strcpy(dir->name, name);

	dir->type = type;

	/* 初始化设备目录信息 */
	INIT_LIST_HEAD(&dir->deviceListHead);

	/* 初始化挂载目录信息 */
	dir->devno = 0;
	dir->blkdev = NULL;
	dir->sb = NULL;

	return dir;
}

/**
 * DestoryDirectory - 销毁目录
 * @name: 目录名
 * 
 * 根据目录名销毁对应的目录
 */
PUBLIC int DestoryDirectory(char *name)
{
	struct Directory *dir = GetDirectoryByName(name);
	if (dir == NULL)
		return -1;

	/* 设备目录 */
	if (dir->type == DIR_TYPE_DEVICE) {
		/* 只能销毁空目录 */
		if (ListEmpty(&dir->deviceListHead)) {
			/* 从目录链表中删除 */
			ListDel(&dir->list);

			/* 释放目录的空间 */
			kfree(dir);
			return 1;
		}	
	} else {

	}
	return 0;
}

PUBLIC void ListDirectory()
{
	printk(PART_TIP "----List Directory----\n");
	struct Directory *dir;
	struct DeviceFile *devfile;
	ListForEachOwner(dir, &directoryListHead, list) {
		printk(PART_TIP "dir name:%s type %d\n", dir->name, dir->type);
		if (dir->type == DIR_TYPE_DEVICE) {
			ListForEachOwner(devfile, &dir->deviceListHead, list) {
				printk("    " PART_TIP "file name:%s type:%d ", devfile->super.name, devfile->super.type);
				printk("dir:%x device:%x\n", devfile->directory, devfile->device);
			}
		}
	}
}

PUBLIC void DumpDirectory(struct Directory *dir)
{
	printk(PART_TIP "----Flat Directory----\n");
	printk(PART_TIP "name:%s type:%d\n", dir->name, dir->type);
}


/**
 * MountDirectory - 挂载目录
 * @devpath: 设备文件的路径
 * @mntname: 要挂载的目录的名字
 * 
 * 通过一个块设备文件，挂载一个目录，然后就可以访问该分区里面的文件了。
 * 挂载过程：先看文件是否存在，要挂载的文件是否存在。
 * 然后再创建一个目录作为挂载目录，然后在设置挂载信息
 */
PUBLIC int MountDirectory(char *devpath, char *mntname)
{
	/* 查看设备是否已经存在 */
	printk("mount: %s\n", devpath);

	/* 解析目录和文件，并查看目录是否存在 */
	char dname[DIR_NAME_LEN];
	bzero(dname, DIR_NAME_LEN);

	/* 解析目录名 */
	if(!ParseDirectoryName((char *)devpath, dname)) {
		printk("mount: path %s bad dir name, please check it!\n", devpath);
		return -1;
	}
	printk("dir name is %s\n", dname);

	/* 获取目录 */
	struct Directory *dir = GetDirectoryByName(dname);
	if (dir == NULL) {
		printk("mount: dir %s not exist!\n", dname);
		return -1;
	}

	char fname[FILE_NAME_LEN];
	bzero(fname, FILE_NAME_LEN);

	/* 解析目录名 */
	if(!ParseFileName((char *)devpath, fname)) {
		printk("mount: path %s bad file name, please check it!\n", devpath);
		return -1;
	}
	printk("file name is %s\n", fname);

	/* 获取设备文件 */
	struct DeviceFile *devfile = GetDeviceFileByName(&dir->deviceListHead, fname);
	
	if (devfile == NULL) {
		printk("mount: device file %s not exist!\n", fname);
		return -1;
	}

	DumpDeviceFile(devfile);

	/* 只有设备文件才能挂载 */
	if (devfile->super.type != FILE_TYPE_DEVICE) {
		printk("mount: file %s not a device file!\n", fname);
		return -1;
	}

	/* 设备文件必须是块设备才能挂载 */
	if (devfile->device->type != DEV_TYPE_BLOCK) {
		printk("mount: device file %s not a block device!\n", fname);
		return -1;
	}

	/* 检测挂载目录是否已经存在 */

	/* 获取挂载目录，不存在才能创建 */
	struct Directory *mntdir = GetDirectoryByName(mntname);
	if (mntdir != NULL) {
		printk("mount: mount dir %s has exist!\n", mntname);
		return -1;
	}

	/* 创建一个目录，当做挂载目录 */
	mntdir = CreateDirectory(mntname, DIR_TYPE_MOUNT);

	if (mntdir == NULL) {
		printk("mount: create mount dir %s failed!\n", mntname);
		return -1;
	}

	/* 记录挂载信息 */
	mntdir->devno = devfile->device->devno;		/* 保存设备号 */

	/* 转换成块设备 */
	mntdir->blkdev = (struct BlockDevice *)devfile->device;
	
	DumpBlockDevice(mntdir->blkdev);
	DumpDiskPartition(mntdir->blkdev->part);
	
	/* 根据分区信息从磁盘上加载超级块 */
	mntdir->sb = BuildFS(mntdir->blkdev->super.devno, mntdir->blkdev->part->startSector,
		mntdir->blkdev->part->sectorCounts, mntdir->blkdev->blockSize, DEFAULT_NODE_FILE_NR);

	if (mntdir->sb == NULL) {
		printk("mount: build file system failed!\n");
		return -1;
	}

	/* 加载超级块的位图   */
	if (LoadSuperBlockBitmap(mntdir->sb)) {
		printk("mount: load bitmap failed!\n");
		return -1;
	}

	/* 挂载成功 */
	return 0;
}
