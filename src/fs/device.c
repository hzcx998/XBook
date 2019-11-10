/*
 * file:		fs/node.c
 * auther:		Jason Hu
 * time:		2019/11/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <fs/bitmap.h>
#include <share/math.h>
#include <fs/file.h>
#include <book/blk-buffer.h>
#include <fs/directory.h>
#include <fs/flat.h>
#include <fs/super_block.h>
#include <fs/device.h>

/**
 * GetDeviceFileByName - 通过名字获取一个设备文件
 * @dirList: 目录链表
 * @name: 文件名
 * 
 * @return: 成功返回目录，失败返回NULL
 */
PUBLIC struct DeviceFile *GetDeviceFileByName(struct List *deviceList, char *name)
{
	struct DeviceFile *devfile;
	ListForEachOwner(devfile, deviceList, list) {
		if (strcmp(devfile->super.name, name) == 0) {
			return devfile;
		}
	}
	return NULL;
}


PUBLIC void DumpDeviceFile(struct DeviceFile *devfile)
{
	printk(PART_TIP "----Device File----\n");
	printk(PART_TIP "name %s type %d attr %x\n",
		devfile->super.name, devfile->super.type, devfile->super.attr);
	printk(PART_TIP "dir %x device %x\n",
		devfile->directory, devfile->device);
}

/**
 * CreateDeviceFile - 创建一个设备文件
 * @name: 文件名
 * @attr: 文件属性
 * @dir: 文件所属目录
 * @device: 设备指针
 */
struct DeviceFile *CreateDeviceFile(char *name,
	char attr, struct Directory *dir, struct Device *device)
{

	/* 创建一个设备文件 */
	struct File *file = CreateFile(name, FILE_TYPE_DEVICE, attr);
	if (file == NULL) {
		return NULL;
	}

	struct DeviceFile *devfile = (struct DeviceFile *)file;

	devfile->directory = dir;
	devfile->device = device;

	ListAddTail(&devfile->list, &dir->deviceListHead);

	return devfile;
}


/**
 * MakeDeviceFile - 创建一个设备文件
 * @path: 路径名
 * @type: 文件的类型
 * @device: 设备指针
 * 
 * @return: 成功返回目录，失败返回NULL
 */
PUBLIC int MakeDeviceFile(char *dname,
	char *fname,
	char type,
	struct Device *device)
{
	/* 如果名字为空就返回 */
	if (dname == NULL || fname == NULL)
		return -1;

	printk("MakeDeviceFile: dir name %s file name %s\n", dname, fname);

	/* 搜索目录 */
	struct Directory *dir = GetDirectoryByName(dname);
	if (dir == NULL) {
		printk("dir %s not exist!\n", dname);
		return -1;
	}
	
	DumpDirectory(dir);

	/* 检测是否已经存在 */
	if (GetDeviceFileByName(&dir->deviceListHead, fname) != NULL) {
		printk("file %s in dir %s has exist!\n", fname, dname);
		return -1;
	}
	
	/* 创建一个设备文件 */
	if (CreateDeviceFile(fname, FILE_ATTR_R | FILE_ATTR_W, dir, device) == NULL) {
		printk("create device file %s failed!\n", fname);
		return -1;
	}

	return 0;
}

PUBLIC int DeviceFileWrite( struct FileDescriptor *fdptr,
	void* buf,
	size_t count)
{
	if (count <= 0) {
		printk("device file write count zero!\n");
		return -1;
	}

	if (fdptr->file) {
		/* 设备的写 */
		struct DeviceFile *devfile = (struct DeviceFile *)fdptr->file;
		
		//DumpDeviceFile(devfile);

		/* 调用设备的写操作 */
		if (devfile->device != NULL) {
			/* 块设备 */
			if (devfile->device->type == DEV_TYPE_BLOCK) {
				
				sector_t todo, done = 0;

				char *_buf = buf;

				/* 需要对数量进行判断，如果是块设备，一次最多运行256个扇区操作 */
				while (done < count) {
					/* 获取要去操作的扇区数这里用10作为分界 */
					if ((done + 256) <= count) {
						todo = 256;
					} else {
						todo = count - done;
					}

					/* 创建一个临时缓冲，因为用户态的缓冲区不能直接用来使用，需要转换成内核的缓冲才行 */
					char *buffer = kmalloc(SECTOR_SIZE *todo, GFP_KERNEL);
					if (!buffer) {
						printk("kmalloc for device write buffer failed!\n");
						return -1;
					}

					/* 复制缓冲区数据 */
					memcpy(buffer, _buf, todo * SECTOR_SIZE);

					/* 写入失败就准备返回 */
					if(DeviceWrite(devfile->device->devno, fdptr->pos, buffer, todo)) {		
						printk("device write failed!\n");				
						kfree(buffer);
						return -1;
					}

					fdptr->pos += todo;

					_buf += todo * SECTOR_SIZE;

					done += todo;
					kfree(buffer);
				}
				/* 返回完成的数据量 */
				return done;
				
			} else if (devfile->device->type == DEV_TYPE_CHAR) {
				/* 字符设备 */
				
				/* 创建一个临时缓冲，因为用户态的缓冲区不能直接用来使用，需要转换成内核的缓冲才行 */
				char *buffer = kmalloc(count, GFP_KERNEL);
				if (!buffer) {
					printk("kmalloc for device write buffer failed!\n");
					return -1;
				}

				/* 复制缓冲区数据 */
				memcpy(buffer, buf, count);

				/* 写入失败就准备返回 */
				if(DeviceWrite(devfile->device->devno, 0, buffer, count)) {		
					//printk("device write failed!\n");
					kfree(buffer);
					return -1;
				}
				kfree(buffer);
				return count;
			}
		}
	}
	printk("device write error!\n");
	return -1;
}

PUBLIC int DeviceFileRead( struct FileDescriptor *fdptr,
	void* buf,
	size_t count)
{
	if (count <= 0) {
		printk("device file read count zero!\n");
		return -1;
	}

	if (fdptr->file) {
		/* 设备的读取 */
		struct DeviceFile *devfile = (struct DeviceFile *)fdptr->file;
		
		//DumpDeviceFile(devfile);

		/* 调用设备的写操作 */
		if (devfile->device != NULL) {
			/* 块设备 */
			if (devfile->device->type == DEV_TYPE_BLOCK) {
				
				sector_t todo, done = 0;

				/* 需要对数量进行判断，如果是块设备，一次最多运行256个扇区操作 */
				while (done < count) {
					/* 获取要去操作的扇区数这里用10作为分界 */
					if ((done + 256) <= count) {
						todo = 256;
					} else {
						todo = count - done;
					}

					/* 创建一个临时缓冲，因为用户态的缓冲区不能直接用来使用，需要转换成内核的缓冲才行 */
					char *buffer = kmalloc(SECTOR_SIZE *todo, GFP_KERNEL);
					if (!buffer) {
						printk("kmalloc for device read buffer failed!\n");
						return -1;
					}

					memset(buffer, 0, todo * SECTOR_SIZE);
					
					/* 读取失败就准备返回 */
					if(DeviceRead(devfile->device->devno, fdptr->pos, buffer, todo)) {		
						printk("device read failed!\n");				
						kfree(buffer);
						return -1;
					}

					/* 复制缓冲区数据 */
					memcpy(buf, buffer, todo * SECTOR_SIZE);

					fdptr->pos += todo;

					buf += todo * SECTOR_SIZE;

					done += todo;
					kfree(buffer);
				}
				/* 返回完成的数据量 */
				return done;
				
			} else if (devfile->device->type == DEV_TYPE_CHAR) {
				/* 创建一个临时缓冲，因为用户态的缓冲区不能直接用来使用，需要转换成内核的缓冲才行 */
				char *buffer = kmalloc(count, GFP_KERNEL);
				if (!buffer) {
					printk("kmalloc for device read buffer failed!\n");
					return -1;
				}
				memset(buffer, 0, count);
				/* 字符设备 */
				/* 读取失败就准备返回 */
				if(DeviceRead(devfile->device->devno, 0, buffer, count)) {		
					//printk("device read failed!\n");				
					kfree(buffer);
					return -1;
				}
				
				/* 复制缓冲区数据 */
				memcpy(buf, buffer, count);

				kfree(buffer);

				return count;
			}
		}
	}
	printk("device read error!\n");
	return -1;
}
