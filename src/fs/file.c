/*
 * file:		fs/file.c
 * auther:		Jason Hu
 * time:		2019/10/26
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/share.h>

#include <fs/file.h>
#include <fs/directory.h>
#include <fs/node.h>
#include <fs/device.h>

#include <book/memcache.h>
#include <book/arch.h>
#include <book/block.h>
#include <book/blk-dev.h>
#include <share/unistd.h>

struct FileDescriptor fileDescriptorTable[MAX_OPEN_FILE_NR];

PUBLIC void InitFileDescriptor()
{
	int i;
	for (i = 0; i < MAX_OPEN_FILE_NR; i++) {
		fileDescriptorTable[i].dir = NULL;
		fileDescriptorTable[i].file = NULL;
		fileDescriptorTable[i].flags = 0;
		fileDescriptorTable[i].pos = 0;
	}
}

int GetFileDescriptor()
{
	int i = 0;
	while (i < MAX_OPEN_FILE_NR) {
		if (fileDescriptorTable[i].flags == 0) {
			fileDescriptorTable[i].flags = 0x80;
			return i;
		}
		i++;
	}
	
	printk(PART_ERROR "alloc fd over max open files!\n");
	return -1;
}

void FreeFileDescriptor(int fd)
{
    if(fd < 0 || fd >= MAX_OPEN_FILE_NR) {
		printk("fd error\n");
		return;
	}
	fileDescriptorTable[fd].dir = NULL;
	fileDescriptorTable[fd].file = NULL;
	fileDescriptorTable[fd].flags = 0;
	fileDescriptorTable[fd].pos = 0;
}

void DumpFileDescriptor(int fd)
{
	if(fd < 0 || fd >= MAX_OPEN_FILE_NR) {
		printk("fd error\n");
		return;
	}

	struct FileDescriptor *pfd = &fileDescriptorTable[fd];

	printk(PART_TIP "----File Descriptor----\n");
	printk(PART_TIP "id %d file %x dir %x pos %d flags %x\n",
		fd, pfd->file, pfd->dir, pfd->pos, pfd->flags);
	
}


/**
 * CreateFile - 创建目录
 * @name: 目录名
 * @type: 目录类型（设备目录和挂载目录）
 * @attr: 属性
 * 
 * 创建成功后返回创建的目录 
 */
PUBLIC struct File *CreateFile(char *name, char type, char attr)
{
	int size;

	switch (type)
	{
	case FILE_TYPE_DEVICE:
		size = SIZEOF_DEVICE_FILE;
		break;
	case FILE_TYPE_NODE:
		size = SIZEOF_NODE_FILE;
		break;
	default:
		printk("CreateFile: Unknown file type");
		return NULL;
	}

	struct File *file = kmalloc(size, GFP_KERNEL);
	if (file == NULL)
		return NULL;
	
	memset(file->name, 0, FILE_NAME_LEN);
	strcpy(file->name, name);

	file->type = type;
	file->attr = attr;
	file->size = 0;
	/* 设置不同类型文件的数据 */

	return file;
}

/**
 * CloseFile - 关闭一个文件
 * @pfd: 文件描述符指针
 * 
 */
PRIVATE int CloseFile(struct FileDescriptor *pfd)
{
	if(pfd == NULL) {
		return -1;
	}
	
	pfd->flags = 0;
	pfd->pos = 0;
	pfd->dir = NULL;
	
	if (pfd->file != NULL) {
		if (pfd->file->type == FILE_TYPE_DEVICE) {
			struct DeviceFile *devfile = (struct DeviceFile *)pfd->file;

			if (devfile->device != NULL) {
				/* 关闭设备 */
				DeviceClose(devfile->device->devno);
			}
			pfd->file = NULL;
		} else if (pfd->file->type == FILE_TYPE_NODE) {
			/* 释放文件在内存中的信息 */
			kfree(pfd->file);

			pfd->file = NULL;
		} 
	}
	
	return 0;
}

PUBLIC int FlatClose(int fd)
{
	int ret = -1;   // defaut -1,error
	if (fd >= 0 && fd < MAX_OPEN_FILE_NR) {
		ret = CloseFile(&fileDescriptorTable[fd]);

		printk("close fd:%d success!\n", fd);
	}else{
		printk("close fd:%d failed!\n", fd);
	}
	return ret;
}

/**
 * OpenFileWithFD - 打开一个文件描述符
 * @dir: 文件所在目录
 * @file: 文件类
 * @flags: 标志
 * 
 * 成功返回fd，失败返回-1
 */
PRIVATE int OpenFileWithFD(struct Directory *dir, struct File *file, int flags)
{
	int fd = GetFileDescriptor();
	if (fd < 0) {
		printk("Get a file descriptor failed!\n");
		return -1;
	}

	struct FileDescriptor *pfd = &fileDescriptorTable[fd];

	pfd->dir = dir;
	pfd->file = file;
	pfd->flags |= flags;	/* 原来已经有使用标志，这里用或运算 */
	pfd->pos = 0;

	return fd;
}

/**
 * FlatOpen - 打开文件
 * @path: 路径
 * @flags: 标志
 * 
 * 打开成功返回fd，失败返回-1
 */
PUBLIC int FlatOpen(const char *path, int flags)
{
	printk("open: %s\n", path);

	/* 解析目录和文件，并查看目录是否存在 */
	char dname[DIR_NAME_LEN];
	bzero(dname, DIR_NAME_LEN);

	/* 解析目录名 */
	if(!ParseDirectoryName((char *)path, dname)) {
		printk("open: path %s bad dir name, please check it!\n", path);
		return -1;
	}
	printk("dir name is %s\n", dname);

	/* 获取目录 */
	struct Directory *dir = GetDirectoryByName(dname);
	if (dir == NULL) {
		printk("open: dir %s not exist!\n", dname);
		return -1;
	}

	char fname[FILE_NAME_LEN];
	bzero(fname, FILE_NAME_LEN);

	/* 解析目录名 */
	if(!ParseFileName((char *)path, fname)) {
		printk("open: path %s bad file name, please check it!\n", path);
		return -1;
	}
	printk("file name is %s\n", fname);

	/* 文件存在标志 */
	char found = 0;

	/* 文件类 */
	struct File *file = NULL;
	
	/* 具体的文件 */
	struct DeviceFile *devfile = NULL;
	struct NodeFile *nodefile = NULL;
	
	/* 搜索文件 */
	if (dir->type == DIR_TYPE_DEVICE) {		/* 设备目录 */

		/* 获取设备文件 */
		devfile = GetDeviceFileByName(&dir->deviceListHead, fname);
		
		/* 设备文件不存在，可以创建 */
		if (devfile == NULL) {
			printk("open: device file %s not exist!\n", fname);
			
		} else {
			/* 设备文件存在 */
			found = 1;
			file = (struct File *)devfile;
			
			printk("open: search device file.\n");
		}
		
	} else if (dir->type == DIR_TYPE_MOUNT) { /* 挂载目录 */
		
		/* 获取节点文件 */
		nodefile = GetNodeFileByName(dir, fname);
		
		/* 节点文件不存在，可以创建 */
		if (nodefile == NULL) {
			printk("open: node file %s not exist!\n", fname);
		} else {
			/* 设备文件存在 */
			found = 1;
			file = (struct File *)nodefile;
			
			printk("open: search node file.\n");
		}
		
	}

	/* 普通没有找到文件 */
	if (!found) {
		printk("file not found!\n");

		/* 也不是创建创建 */
		if (!(flags & O_CREAT)) {
			printk("open: file not exist and without O_CREAT!\n");

			return -1;
		}
	} else {
		
		/* 找到文件 */
		if (file->type == FILE_TYPE_DEVICE) {
			printk("device file found!\n");
			/*
			if (flags & O_CREAT) {
				printk("open: device can not open with O_CREAT!\n");
				return -1;
			}*/
		} else if (file->type == FILE_TYPE_NODE) {
			printk("node file found!\n");
		}
	}

	int fd = -1;

	/* 不是重新打开，就是创建一个新文件 */
	if ((flags & O_CREAT) && !found) {
		/* 要生成的文件的属性 */
		char attr;

		if (flags & O_RDWR) {
			attr = FILE_ATTR_R | FILE_ATTR_W;
		} else if (flags & O_RDONLY) {
			attr = FILE_ATTR_R;
		} else if (flags & O_WRONLY) {
			attr = FILE_ATTR_W;
		}
		
		if (flags & O_EXEC) {
			attr |= FILE_ATTR_X;
		}

		/* 根据不同的目录类型创建不同的文件 */
		if (dir->type == DIR_TYPE_DEVICE) {		
			printk("create device with attr %x\n", attr);
			/* 创建设备文件，不指向任何设备 */
			devfile = CreateDeviceFile(fname, attr, dir, NULL);
			if (devfile == NULL) {
				printk(PART_ERROR "create device file failed!\n");
				return -1;
			}
			/* 修改文件指针 */
			file = (struct File *)devfile;
		} else if (dir->type == DIR_TYPE_MOUNT) {		
			printk("create file with attr %x\n", attr);
			/* 创建文件 */
			nodefile = CreateNodeFile(fname, attr, dir->sb);
			if (nodefile == NULL) {
				printk(PART_ERROR "create node file failed!\n");
				return -1;
			}
			/* 修改文件指针 */
			file = (struct File *)nodefile;
		}
	}

	/* 打开一个文件到系统中 */
	fd = OpenFileWithFD(dir, file, flags);
	
	/* 其它操作 */
	if (file->type == FILE_TYPE_DEVICE) {
		printk("open device file %s\n", fname);
		
		/* 有设备指针才可以打开 */
		if (devfile->device != NULL) {
			/* 执行设备打开操作 */
			if (DeviceOpen(devfile->device->devno, flags)) {
				/* 打开失败 */
				FreeFileDescriptor(fd);
				return -1;
			}
		}
		
	} else if (file->type == FILE_TYPE_NODE) {
		printk("open node file %s\n", fname);
		
		if (fd > 0) {
			/* 是否有追加标志 */
			if (flags & O_APPEDN) {
				/* seek 到文件最后 */
				FlatLseek(fd, 0, SEEK_END);
			}
		}
	}
	return fd;
}

PUBLIC int FlatWrite(int fd, void* buf, size_t count)
{
	if (fd < 0 || fd >= MAX_OPEN_FILE_NR) {
		printk("write: fd error\n");
		return -1;
	}
	if(count == 0) {
		printk("write: count zero\n");
		return -1;
	}

    struct FileDescriptor *pfd = &fileDescriptorTable[fd];
	
	/* 要有写标志才可以 */
    if((pfd->flags & O_WRONLY) || (pfd->flags & O_RDWR)){
		/* 文件的模式也要有写属性 */
		if(pfd->file->attr & FILE_ATTR_W){
			int written  = -1;
			
			/* 根据不同的类型调用不同的写入方法 */
			if (pfd->file->type == FILE_TYPE_DEVICE) {
				written = DeviceFileWrite(pfd, buf, count);
			} else if (pfd->file->type == FILE_TYPE_NODE) {
				
			}
			return written;
		}else{
			printk("write: not allowed to write file without FILE_ATTR_W!\n");
		}
	} else {
		printk("write: not allowed to write file without flag O_RDWR or O_WRONLY\n");
	}
	printk("write: no write privilege!\n");
	return -1;
}

PUBLIC int FlatRead(int fd, void* buf, size_t count)
{
	if (fd < 0 || fd >= MAX_OPEN_FILE_NR) {
		printk("read: fd error\n");
		return -1;
	}
	if(count == 0) {
		printk("read: count zero\n");
		return -1;
	}

    struct FileDescriptor *pfd = &fileDescriptorTable[fd];
	
	/* 要有写标志才可以 */
    if((pfd->flags & O_RDONLY) || (pfd->flags & O_RDWR)){
		/* 文件的模式也要有写属性 */
		if(pfd->file->attr & FILE_ATTR_R){
			int read  = -1;
			
			/* 根据不同的类型调用不同的写入方法 */
			if (pfd->file->type == FILE_TYPE_DEVICE) {
				read = DeviceFileRead(pfd, buf, count);
			} else if (pfd->file->type == FILE_TYPE_NODE) {
				
			}
			return read;
		}else{
			printk("read: not allowed to read file without FILE_ATTR_R!\n");
		}
	} else {
		printk("read: not allowed to read file without flag O_RDWR or O_RDONLY\n");
	}
	printk("read: no read privilege!\n");
	return -1;
}

PUBLIC off_t FlatLseek(int fd, off_t offset, char whence)
{
	if (fd < 0 || fd >= MAX_OPEN_FILE_NR) {
		printk("lseek: fd error\n");
		return -1;
	}

	struct FileDescriptor* pfd = &fileDescriptorTable[fd];
	
	int newPos = 0;   //新位置必须 < 文件大小

	int fileSize;

	if (pfd->file == NULL) {
		printk("lseek: file not exist!\n");
		return -1;
	}

	if (pfd->file->type == FILE_TYPE_DEVICE) {
		struct DeviceFile *devfile = (struct DeviceFile *)pfd->file;
		
		if (devfile->device->type == DEV_TYPE_BLOCK) {
			/* 获取块设备 */
			struct BlockDevice *blkdev = (struct BlockDevice *)devfile->device;
			/* 获取设备文件大小 */
			fileSize = blkdev->disk->capacity;

		} else if (devfile->device->type == DEV_TYPE_CHAR) {
			/* 字符设备不需要Seek */
			return -1;
		}
	} else if (pfd->file->type == FILE_TYPE_NODE) {

	}
	//printk("size is %d\n", fileSize);

	switch (whence) {
		case SEEK_SET: 
			newPos = offset; 
			break;
		case SEEK_CUR: 
			newPos = (int)pfd->pos + offset; 
			break;
		case SEEK_END: 
			newPos = fileSize + offset;
			break;
		default :
			printk("lseek: unknown whence!\n");
			break;
	}

	if (pfd->file->type == FILE_TYPE_NODE) {
		/* 文件范围之内才可以 */
		if (newPos < 0 || newPos > fileSize) {	 
			return -1;
		}
	} else if (pfd->file->type == FILE_TYPE_DEVICE) {
		/* 设备扇区数范围之内才可以 */
		if (newPos < 0 || newPos > fileSize) {	 
			return -1;
		}
	}
	pfd->pos = newPos;

	return pfd->pos;
}

/**
 * FlatIoctl - 输入输出控制
 * @fd: 文件描述符
 * @cmd: 命令
 * @arg: 参数
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int FlatIoctl(int fd, unsigned int cmd, unsigned int arg)
{
	if (fd < 0 || fd >= MAX_OPEN_FILE_NR) {
		printk("write: fd error\n");
		return -1;
	}

    struct FileDescriptor *pfd = &fileDescriptorTable[fd];
	if (pfd->file) {
		/* 根据不同的类型调用不同的写入方法 */
		if (pfd->file->type == FILE_TYPE_DEVICE) {
			struct DeviceFile *devfile = (struct DeviceFile *)pfd->file;
			
			if (devfile->device != NULL) {
				/* 执行IOCTL命令 */
				if (!DeviceIoctl(devfile->device->devno, cmd, arg)) {
					return 0;
				}
			}
			
		} else if (pfd->file->type == FILE_TYPE_NODE) {
			
		}
	}
	
	return -1;
}
