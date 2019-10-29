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
	printk(PART_TIP "file %x dir %x pos %d flags %x\n",
		pfd->file, pfd->dir, pfd->pos, pfd->flags);
	
}


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
 * CreateDirectory - 创建目录
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
	case FILE_TYPE_DATA:
		size = SIZEOF_DATA_FILE;
		break;
	default:
		printk("CreateFile: Unknown file type");
		return NULL;
	}

	struct File *file = kmalloc(size, GFP_KERNEL);
	if (file == NULL)
		return NULL;
	
	memset(file->name, 0, DIR_NAME_LEN);
	strcpy(file->name, name);

	file->type = type;
	file->attr = attr;
	/* 设置不同类型文件的数据 */

	return file;
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
	struct File *file = CreateFile(fname, FILE_TYPE_DEVICE,
		FILE_ATTR_R | FILE_ATTR_W);

	if (file == NULL) {
		kfree(dir);
		return -1;
	}

	/* 由于设备文件继承了文件，所以可以直接转换 */
	struct DeviceFile *devfile = (struct DeviceFile *)file;
	
	/* 填写设备文件的信息 */
	devfile->device = device;
	
	/* 添加到目录中去 */
	devfile->directory = dir;

	/* 设备文件添加到目录设备中 */
	ListAddTail(&devfile->list, &dir->deviceListHead);

	return 0;
}

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
			pfd->file = NULL;
		} else if (pfd->file->type == FILE_TYPE_DATA) {
			/* 释放文件信息 */

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

PRIVATE int OpenDeviceFile(struct Directory *dir, char *name, int flags)
{
	struct DeviceFile *devfile;

	struct DeviceFile *found = NULL;
	
	ListForEachOwner(devfile, &dir->deviceListHead, list) {
		if (!strcmp(devfile->super.name, name)) {
			found = devfile;
			break;
		}
	}

	if (found == NULL) {
		Panic("device file %s not found when open!\n", name);
	}


	int fd = GetFileDescriptor();
	if (fd < 0) {
		printk("Get a file descriptor failed!\n");
		return -1;
	}

	struct FileDescriptor *pfd = &fileDescriptorTable[fd];

	pfd->dir = dir;
	pfd->file = &devfile->super;
	pfd->flags |= flags;	/* 原来已经有使用标志，这里用或运算 */
	pfd->pos = 0;

	/* 执行设备打开操作 */
	if (DeviceOpen(devfile->device->devno, flags)) {
		/* 打开失败 */
		FreeFileDescriptor(fd);
		return -1;
	}

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
	/* 已经存在的文件的重新打开 */
	char reopen = 0;

	struct File *file = NULL;
	
	/* 搜索文件 */
	if (dir->type == DIR_TYPE_DEVICE) {

		/* 获取设备文件 */
		struct DeviceFile *devfile = GetDeviceFileByName(&dir->deviceListHead, fname);
		
		if (devfile == NULL) {
			printk("open: device file %s not exist!\n", fname);
			return -1;
		}
		/* 设备文件存在 */
		found = 1;
		file = (struct File *)devfile;
		
		printk("open: search device file.\n");
	} else {
		/* 挂载目录 */


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

			if (flags & O_CREAT) {
				printk("open: device can not open with O_CREAT!\n");
				return -1;
			}
		} else if (file->type == FILE_TYPE_DATA) {
			printk("data file found!\n");

			/* 数据文件有创建标志，并且以及找到 */
			if (flags & O_CREAT) {
				/* 要有其它标志才可以打开 */
				if ((flags & O_RDONLY) || (flags & O_WRONLY) || (flags & O_RDWR)) {
					reopen = 1;
				} else {
					printk("open: path %s has exist, can't create it alone!\n", path);
					return -1;
				}
			}
		}
	}

	/* 如果没有读写标志，就退出 */
	if (!(flags & O_RDONLY) && !(flags & O_WRONLY) && !(flags & O_RDWR)) {
		printk("open: without READ and WRITE flags!\n");
		return -1;
	}

	int fd = -1;

	/* 不是重新打开，就是创建一个新文件 */
	if ((flags & O_CREAT) && !reopen) {
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
		
		printk("create file with attr %x\n", attr);
		/* 创建文件 */


	}

	/* 打开文件 */
	if (file->type == FILE_TYPE_DEVICE) {
		printk("open device file %s\n", fname);
		fd = OpenDeviceFile(dir, fname, flags);
		
	} else if (file->type == FILE_TYPE_DATA) {
		/* 打开文件 */
		//fd = OpenDataFile(dir, fname, flags);
		
		if (fd > 0) {
			/* 是否有追加标志 */
			if (flags & O_APPEDN) {
				/* seek 到文件最后 */

			}
		}
	}
	return fd;
}

PRIVATE int DeviceFileWrite( struct FileDescriptor *fdptr,
	void* buf,
	unsigned int count)
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
		if (devfile->device) {
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
			} else if (pfd->file->type == FILE_TYPE_DATA) {
				
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

PRIVATE int DeviceFileRead( struct FileDescriptor *fdptr,
	void* buf,
	unsigned int count)
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
		if (devfile->device) {
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
			} else if (pfd->file->type == FILE_TYPE_DATA) {
				
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
	} else if (pfd->file->type == FILE_TYPE_DATA) {

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

	if (pfd->file->type == FILE_TYPE_DATA) {
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
			
			if (devfile->device) {
				/* 执行IOCTL命令 */
				if (!DeviceIoctl(devfile->device->devno, cmd, arg)) {
					return 0;
				}
			}
			
		} else if (pfd->file->type == FILE_TYPE_DATA) {
			
		}
	}
	
	return -1;
}