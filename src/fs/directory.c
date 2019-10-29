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

#include <fs/directory.h>
#include <fs/file.h>

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

	INIT_LIST_HEAD(&dir->deviceListHead);
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
