/*
 * file:		fs/flat.c
 * auther:		Jason Hu
 * time:		2019/10/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/share.h>
#include <book/device.h>
#include <fs/flat.h>
#include <fs/file.h>
#include <fs/directory.h>

#include <driver/keyboard.h>

/**
 * ParseDirectoryName - 解析目录名
 * @path: 路径
 * @buf: 缓冲区
 * 
 * 解析格式是 目录名:文件名，中间必须要有:
 */
PUBLIC int ParseDirectoryName(char *path, char *buf)
{
	if (path == NULL)
		return 0;
	char *pos = strchr(path, ':');
	//printk("strchr:%x\n", pos);
	if (pos == NULL) {
		//printk("path no :\n");
		return 0;
	}
	//printk("not null\n");
	
	int len = pos - path;

	/* 只有:符号 */
	if (len <= 1) {
		//printk("dir is empty!\n");
		return 0;
	}

	strncpy(buf, path, len - 1);
	return 1;
}

/**
 * ParseFileName - 解析文件名
 * @path: 路径
 * @buf: 缓冲区
 * 
 * 解析格式是 目录名:文件名，中间必须要有:
 */
PUBLIC int ParseFileName(char *path, char *buf)
{
	if (path == NULL)
		return 0;
	char *pos = strrchr(path, ':');
	//printk("strrchr:%x\n", pos);
	if (pos == NULL) {
		//printk("path no :\n");
		return 0;
	}
	//printk("not null\n");
	
	/* 只有一个:，就说明没有文件名 */
	if (strlen(pos) <= 1) {
		//printk("path no file name\n");
		return 0;
	}
	strcpy(buf, pos + 1);
	return 1;
}

/**
 * InitFlatFileSystem - 初始化平坦文件系统
 */
PUBLIC int InitFlatFileSystem()
{
    PART_START("Flat File System");

	
	InitFileDescriptor();

    struct Directory *dir =  CreateDirectory("dev", DIR_TYPE_DEVICE);
    if (dir == NULL)
        return -1;
    DumpDirectory(dir);
	/* 获取设备文件 */
	struct Device *device;
	ListForEachOwner (device, &allDeviceListHead, list) {
		/* 根据注册到系统中的设备，创建对应的设备文件 */
		MakeDeviceFile("dev", device->name, FILE_TYPE_DEVICE, device);
	}
	
	ListDirectory();

	
	int fd = FlatOpen("dev:hda", O_RDWR);
	if (fd < 0) {
		printk("file open failed!\n");
	}

	DumpFileDescriptor(fd);

	char buf[1024];
	memset(buf, 0x12, 1024);
	int ret = FlatWrite(fd, buf, 2);
	
	FlatLseek(fd, 0, SEEK_SET);
	memset(buf, 0, 1024);
	ret = FlatRead(fd, buf, 2);

	printk("read count:%d", ret);

	printk("buf :%x %x %x %x", buf[0], buf[511], buf[512], buf[1023]);

	FlatClose(fd);

	
	fd = FlatOpen("dev:keyboard", O_RDWR);
	if (fd < 0) {
		printk("file open failed!\n");
	}

	DumpFileDescriptor(fd);
	
	int key;

	FlatIoctl(fd, KBD_IO_MODE, KBD_MODE_ASYNC);

	while (1) {
		if (1 == FlatRead(fd, &key, 1)) {
			printk("%c", key); 
		}
	};

	FlatClose(fd);
	

	


	/*
    MakeDeviceFile("dev:hda", FILE_TYPE_DEVICE, NULL);
	MakeDeviceFile("dev:hdb", FILE_TYPE_DEVICE, NULL);
	*/

	/*
	dir =  CreateDirectory("root", DIR_TYPE_DEVICE);
    if (dir == NULL)
        return -1;
    DumpDirectory(dir);
	
    MakeDeviceFile("root:hda", FILE_TYPE_DEVICE, NULL);
	MakeDeviceFile("root:hdb", FILE_TYPE_DEVICE, NULL);
	*/
	/*dir =  CreateDirectory("root", DIR_TYPE_DEVICE);
    if (dir == NULL)
        return -1;
    DumpDirectory(dir);
	MakeDeviceFile("root:hda", FILE_TYPE_DEVICE, NULL);
	MakeDeviceFile("root:hdb", FILE_TYPE_DEVICE, NULL);
	*/

    PART_END();
    Spin("test");
}