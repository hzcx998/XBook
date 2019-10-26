/*
 * file:		fs/bofs/dir.c
 * auther:		Jason Hu
 * time:		2019/9/7
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <driver/ide.h>
#include <driver/clock.h>
#include <book/share.h>
#include <book/device.h>
#include <book/task.h>

#include <fs/bofs/dir.h>
#include <fs/bofs/dir_entry.h>
#include <fs/bofs/bitmap.h>
#include <fs/bofs/super_block.h>
#include <fs/bofs/file.h>

#include <book/blk-buffer.h>

/**
 * BOFS_PathToName - 解析出最终的名字
 * @pathname: 路径名
 * @namebuf: 储存名字的地方
 * 
 * @return: 成功返回0，失败返回-1
 */
PUBLIC int BOFS_PathToName(const char *pathname, char *namebuf)
{
	char *p = (char *)pathname;
	char depth = 0;
	char name[BOFS_NAME_LEN];
	int i,j;
	
	if(*p != '/'){	//First must be /
		printk("path %s first must be /\n", pathname);
		return -1;
	}
	//Count how many dir 
	while(*p){
		if(*p == '/'){
			depth++;
		}
		p++;
	}
	//printk("depth:%d \n",depth);
	p = (char *)pathname;
	for(i = 0; i < depth; i++){
		
		memset(name, 0, BOFS_NAME_LEN);
		
		p++;	//skip '/'
		j = 0;
		//get a dir name
		while(*p != '/' && j < BOFS_NAME_LEN){	//if not arrive next '/'
			name[j] = *p;
			j++;
			p++;
		}
		name[j] = 0;
		//printk("name:%s %d\n",name, i);
		if(name[0] == 0){	//no name
			printk("no name!\n");
			return -1;
		}
		
		if(i == depth-1){	//name is what we need
			//printk("path to name %s\n", name);
			j = 0;
			while(name[j]){	//if not arrive next '/'
				namebuf[j] = name[j];	// transform to A~Z
				j++;
			}
			namebuf[j] = 0;
			return 0;	//success
		}
	}
	return -1;
}

/**
 * BOFS_ListDir - 列出目录下面的内容
 * @pathname: 路径名
 * @level: 显示等级，不同的等级显示的内容不一样
 * 
 */
void BOFS_ListDir(const char *pathname, int level)
{
	//printk("List dir path: %s\n", pathname);
	//open a dir
	struct BOFS_Dir *dir = BOFS_OpenDir(pathname);

	if(dir == NULL){
		return;
	}

	BOFS_RewindDir(dir);
	
	struct BOFS_DirEntry* dirEntry;
	char type;
	
	struct BOFS_Inode inode;
	
	dirEntry = BOFS_ReadDir(dir);
	while(dirEntry != NULL){
		
		if(level == 2){
			if(dirEntry->type == BOFS_FILE_TYPE_DIRECTORY){
				type = 'd';
			}else if(dirEntry->type == BOFS_FILE_TYPE_NORMAL){
				type = 'f';
			}else if(dirEntry->type == BOFS_FILE_TYPE_INVALID){
				type = 'i';
			}else if(dirEntry->type == BOFS_FILE_TYPE_MOUNT){
				type = 'm';
			}else if(dirEntry->type == BOFS_FILE_TYPE_BLOCK){
				type = 'b';
			}else if(dirEntry->type == BOFS_FILE_TYPE_CHAR){
				type = 'c';
			}
			/* 需要加载目录所在的超级块中的节点 */
			BOFS_LoadInodeByID(&inode, dirEntry->inode, dir->superBlock);
			
			printk("%d/%d/%d ",
				DATA16_TO_DATE_YEA(inode.crttime>>16),
				DATA16_TO_DATE_MON(inode.crttime>>16),
				DATA16_TO_DATE_DAY(inode.crttime>>16));
			printk("%d:%d:%d ",
				DATA16_TO_TIME_HOU(inode.crttime&0xffff),
				DATA16_TO_TIME_MIN(inode.crttime&0xffff),
				DATA16_TO_TIME_SEC(inode.crttime&0xffff));
			printk("%c %d %s %d\n", type, inode.size, dirEntry->name, inode.deviceID);
		}else if(level == 1){
			if(dirEntry->type != BOFS_FILE_TYPE_INVALID){
				if(dirEntry->type == BOFS_FILE_TYPE_DIRECTORY){
					type = 'd';
				}else if(dirEntry->type == BOFS_FILE_TYPE_NORMAL){
					type = 'f';
				}else if(dirEntry->type == BOFS_FILE_TYPE_MOUNT){
					type = 'm';	
				}else if(dirEntry->type == BOFS_FILE_TYPE_BLOCK){
					type = 'b';
				}else if(dirEntry->type == BOFS_FILE_TYPE_CHAR){
					type = 'c';
				}
				
				/* 需要加载目录所在的超级块中的节点 */
				BOFS_LoadInodeByID(&inode, dirEntry->inode, dir->superBlock);
			
				printk("%d/%d/%d ",
					DATA16_TO_DATE_YEA(inode.crttime>>16),
					DATA16_TO_DATE_MON(inode.crttime>>16),
					DATA16_TO_DATE_DAY(inode.crttime>>16));
				printk("%d:%d:%d ",
					DATA16_TO_TIME_HOU(inode.crttime&0xffff),
					DATA16_TO_TIME_MIN(inode.crttime&0xffff),
					DATA16_TO_TIME_SEC(inode.crttime&0xffff));
				printk("%c %d %s %d\n", type, inode.size, dirEntry->name, inode.deviceID);
			}
		}else if(level == 0){
			if(dirEntry->type != BOFS_FILE_TYPE_INVALID){
				printk("%s ", dirEntry->name);
			}
		}
		dirEntry = BOFS_ReadDir(dir);
	}
	BOFS_CloseDir(dir);
	printk("\n");
	
}

/**
 * BOFS_RewindDir - 重置目录游标位置
 * @dir: 目录
 */
void BOFS_RewindDir(struct BOFS_Dir *dir)
{
	dir->pos = 0;
}

/**
 * BOFS_ReadDir - 读取目录
 * @dir: 目录
 * 
 * @return: 成功返回一个目录项，失败返回NULL
 */
PUBLIC struct BOFS_DirEntry *BOFS_ReadDir(struct BOFS_Dir *dir)
{
	if(dir == NULL){	
		return NULL;
	}

	/* 没有数据 */
	if (dir->pos >= dir->size) {
		return NULL;
	}

	/* 目录项指向缓冲中的一个 */
	struct BOFS_DirEntry *dirEntry = (struct BOFS_DirEntry *)&dir->buf[dir->pos];
	
	/* 改变位置 */
	dir->pos += BOFS_SIZEOF_DIR_ENTRY;
	
	return dirEntry;
}

/**
 * BOFS_DumpDir - 调试目录
 * @dir: 目录
 */
PUBLIC void BOFS_DumpDir(struct BOFS_Dir* dir)
{
	printk(PART_TIP "---- Dir ----\n");
    printk(PART_TIP "dir entry:%x inode:%d sizeid:%d pos:%x \n",
        dir->dirEntry, dir->dirEntry->inode, dir->size, dir->pos);
}

/**
 * BOFS_CloseDir - 关闭一个以及打开的目录
 * @dir: 目录的指针
 */
void BOFS_CloseDir(struct BOFS_Dir* dir)
{
	/* 不能关闭空目录和主超级块的根目录 */
	if(dir == NULL || dir == masterSuperBlock->rootDir){	
		return;
	}
	
	/* 释放目录项和目录结构体 */
	if(dir->dirEntry != NULL){
		BOFS_CloseDirEntry(dir->dirEntry);
	}
	kfree(dir);
}

/**
 * BOFS_OpenDirSub - 打开目录子函数
 * @dirEntry: 目录项
 * @sb: 超级块
 * 
 * 打开一个目录，创建一个目录结构，然后把目录内容读取到缓冲区中去
 * @return: 成功返回打开的目录，失败返回NULL
 */
PRIVATE struct BOFS_Dir *BOFS_OpenDirSub(struct BOFS_DirEntry *dirEntry,
	struct BOFS_SuperBlock *sb)
{
	/* 创建目录结构 */
	struct BOFS_Dir *dir = (struct BOFS_Dir *)kmalloc(sizeof(struct BOFS_Dir), GFP_KERNEL);
	if(dir == NULL){
		return NULL;
	}
	/* 绑定超级块 */
	dir->superBlock = sb;

	/* 绑定目录项 */
	dir->dirEntry = dirEntry;
	
	/* 设置当前访问位置 */
	dir->pos = 0;

	/* 从磁盘加载数据 */

	/* 获取节点信息 */
	struct BOFS_Inode inode;
	if (BOFS_LoadInodeByID(&inode, dirEntry->inode, sb)) {
		goto ToFreeDir;
	}

	dir->size = inode.size;

	/* 计算有多少数据块 */
	unsigned int blocks = DIV_ROUND_UP(inode.size, SECTOR_SIZE);
	
	/* 分配数据块那么多的扇区 */
	dir->buf = kmalloc(blocks * SECTOR_SIZE, GFP_KERNEL);
	if (dir->buf == NULL) {
		goto ToFreeDir;
	}

	unsigned int blockID = 0, lba;

	/* 用buf指针来进行数据的写入 */
	unsigned char *buf = dir->buf;
	/*we check for blocks, we need search in old dir entry*/
	while (blockID < blocks) {
		BOFS_GetInodeData(&inode, blockID, &lba, sb);

		/* 读取到buffer中去 */
		if (!BlockRead(sb->deviceID, lba, buf)) {
			printk(PART_ERROR "device %d read failed!\n", sb->deviceID);
			goto ToFreeDir;
		}
		
		/* 指向下一个数据块 */
		blockID++;
		buf += SECTOR_SIZE;
	}
	//printk(">>>read sucess!\n");
	/* 没有错误，正常返回 */
	return dir;

/* 如果发送错误，就要释放dir */
ToFreeDir:
	kfree(dir);
	return NULL;
}

/**
 * BOFS_OpenDir - 打开一个目录
 * @pathname: 路径名
 * 
 * 打开一个目录，就是从磁盘加载目录的信息到内存
 */
struct BOFS_Dir *BOFS_OpenDir(const char *pathname)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	
	/* 超级块指针 */
    struct BOFS_SuperBlock *sb = masterSuperBlock;

	struct BOFS_Dir *dir = NULL;
	
	//if it's root , we return root dir
	if (path[0] == '/' && path[1] == 0) {
		struct BOFS_DirEntry *dirEntry = (struct BOFS_DirEntry *)kmalloc(\
			sizeof(struct BOFS_DirEntry), GFP_KERNEL);
		if (dirEntry == NULL) {
			return NULL;
		}
		/* 复制根目录项给它 */
		memcpy(dirEntry, sb->rootDir->dirEntry, sizeof(struct BOFS_DirEntry));

		/* 加载根目录数据，然后再返回根目录 */
		dir = BOFS_OpenDirSub(dirEntry, sb);
		
		/* 如果打开目录失败就释放分配的目录项 */
		if (dir == NULL)
			kfree(dirEntry);
		return dir;
	}
	
	struct BOFS_DirSearchRecord record;
	int found = BOFS_SearchDir(path, &record, sb);
	
	if(!found){	//fount
		/*printk("search dir parent name %s child name %s super at %x\n",
			record.parentDir->name, record.childDir->name, record.superBlock);*/

		if (record.childDir->type == BOFS_FILE_TYPE_NORMAL) {	///nomal file
			//printk("%s is regular file!\n", path);
			
			BOFS_CloseDirEntry(record.childDir);
		} else if (record.childDir->type == BOFS_FILE_TYPE_DIRECTORY) {
			//printk("%s is dir file!\n", path);
			
			//BOFS_DumpSuperBlock((struct BOFS_SuperBlock *)record.superBlock);
			dir = BOFS_OpenDirSub(record.childDir, record.superBlock);

			/* 如果打开失败就释放资源并返回 */
			if(dir == NULL){
				BOFS_CloseDirEntry(record.parentDir);
				BOFS_CloseDirEntry(record.childDir);
				return NULL;
			}
		}
	}else{
		printk(PART_ERROR "path %s is not exist!\n", path); 
	}
	BOFS_CloseDirEntry(record.parentDir);
	
	return dir;
}

/**
 * BOFS_SearchDir - 搜索目录
 * @pathname: 路径名
 * @record: 搜索记录
 * @sb: 超级块
 * 
 * 在路径中搜索目录，搜索到后就把搜索内容填充到搜索记录中
 * @return: 成功返回1，失败返回0
 */
PUBLIC int BOFS_SearchDir(char* pathname,
	struct BOFS_DirSearchRecord *record,
	struct BOFS_SuperBlock *sb)
{
	char *p = (char *)pathname;
	char *next = NULL;

	char depth = 0;
	char name[BOFS_NAME_LEN];
	int i,j;
	
	/*alloc dir entry space*/
	struct BOFS_DirEntry *parentDir = (struct BOFS_DirEntry *)kmalloc(sizeof(struct BOFS_DirEntry), GFP_KERNEL);
	struct BOFS_DirEntry *childDir = (struct BOFS_DirEntry *)kmalloc(sizeof(struct BOFS_DirEntry), GFP_KERNEL);
	
	memset(parentDir, 0, sizeof(struct BOFS_DirEntry));
	memset(childDir, 0, sizeof(struct BOFS_DirEntry));
	
	/* 最开始初始化一下record */
	record->parentDir = parentDir;
	record->childDir = childDir;
	record->superBlock = sb;

	if(*p != '/'){	//First must be /
		return -1;
	}
	//Count how many dir 
	while(*p){
		if(*p == '/'){
			depth++;
		}
		p++;
	}
	
	p = (char *)pathname;
	for(i = 0; i < depth; i++){
		p++;	//skip '/'

		memset(name,0,BOFS_NAME_LEN);
		j = 0;
		//get a dir name
		while(*p != '/' && j < BOFS_NAME_LEN){	//if not arrive next '/'
			name[j] = *p;	// transform to A~Z
			j++;
			p++;
		}
		
		// 如果是根目录，就返回根目录信息
		if(depth == 1 && pathname[0] == '/' && pathname[1] == 0){
			record->parentDir = parentDir;

			/* 复制根目录数据给子目录 */
			memcpy(childDir, sb->rootDir->dirEntry, sizeof(struct BOFS_DirEntry));
			record->childDir = childDir;

			/* 记录子目录所在的超级块 */
			record->superBlock = sb;
			return 0;
		}
		/* 如果是在根目录下面查找，那么就需要把根目录数据复制给父目录 */
		if(i == 0){
			memcpy(parentDir, sb->rootDir->dirEntry, sizeof(struct BOFS_DirEntry));
		}
		//printk("ptr %s\n", p);
		//get root dir entry， /mnt/hda
		if(BOFS_SearchDirEntry(sb, parentDir, childDir, name)){	//find
			/* 发现子目录是挂载目录，就记录挂载目录的位置 */
			if (childDir->type == BOFS_FILE_TYPE_MOUNT && next == NULL) {
				//printk(">>meet mount dir %s name %s depth %d\n", pathname, name, i);
				next = p;
			}
			if(i == depth - 1){	//finally
				//printk("parent %s found child %s!\n", parentDir->name, childDir->name);
				
				/* 如果是在最后，并且也是挂在目录，说明我们需要访问根目录 */
				if (childDir->type == BOFS_FILE_TYPE_MOUNT) {
					return BOFS_SearchDir("/", record, (struct BOFS_SuperBlock *)childDir->mount);
				}
				/* 不是挂载目录，直接返回记录的信息 */
				record->parentDir = parentDir;
				
				record->childDir = childDir;
				
				/* 记录子目录所在的超级块 */
				record->superBlock = sb;
				return 0;		
			}else{
				memcpy(parentDir, childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir

				/* 检测是否是挂载目录，是的话，就进入挂载目录,
				遇到挂载目录，并且还没有到最后的路径，说明要删除的路径在挂载目录里面
				搜索的时候，如果是搜索的挂在目录，就相当于操作了操作了挂在的根目录，
				如：/mnt/floppy, 如果访问floppy，就相当于访问 软盘的根目录，软盘/
				*/
				if (next != NULL) {
					//printk("search under dir %s -> %s super at %x\n", p, next, childDir->mount);
					/* 挂载关联是否正确 */
					if (!childDir->mount) {
						printk("path %s mount link error!\n", pathname);
						return -1;
					}
					return BOFS_SearchDir(next, record, (struct BOFS_SuperBlock *)childDir->mount);
				}
			}
			
			//printk("find\n");
		}else{
			printk("not found\n");
			
			//printk("search:error!\n");
			record->parentDir = parentDir;
			record->childDir = NULL;
			/* 还是要记录超级块信息，当文件没有找到的时候，需要创建文件使用 */
			record->superBlock = sb;

			kfree(childDir);
			break;
		}
	}
	return -1;
}

/**
 * BOFS_OpenRootDir - 打开根目录
 * @sb: 超级块
 * 
 * 把根目录打开，加载目录的内容到内存
 * @return: 成功返回0，失败返回-1
 */
PUBLIC int BOFS_OpenRootDir(struct BOFS_SuperBlock *sb)
{	
	//BOFS_DumpSuperBlock(sb);
	
    /* 为根目录和根目录项分配内存 */
	sb->rootDir = (struct BOFS_Dir *)kmalloc(sizeof(struct BOFS_Dir),
        GFP_KERNEL);

    if (sb->rootDir == NULL) {
        return -1;
    }
    
    struct BOFS_DirEntry *dirEntry = kmalloc(sizeof(struct BOFS_DirEntry), GFP_KERNEL);
	
    if (dirEntry == NULL) {
        kfree(sb->rootDir);
        return -1;
    }

    /* 获取根目录项信息 */
	memset(sb->iobuf, 0, SECTOR_SIZE);
	/* 读取根目录的数据 */
	if (!BlockRead(sb->deviceID, sb->dataStartLba, sb->iobuf)) {
        return -1;
    }
	
	memcpy(dirEntry, sb->iobuf, sizeof(struct BOFS_DirEntry));

    sb->rootDir->dirEntry = dirEntry;
	//BOFS_DumpDirEntry(dirEntry);

    /* 根目录的目录信息不使用，在打开根目录的时候会重新创建一个信息 */
    sb->rootDir->pos = 0;
    sb->rootDir->size = 0;
	sb->rootDir->buf = NULL;
	sb->rootDir->superBlock = sb;

    return 0;
}
/**
 * BOFS_CreateNewDirEntry - 创建一个新的目录项
 * @sb: 超级块儿
 * @parentDir: 父目录
 * @childDir: 子目录
 * @name: 目录项的名字
 * 
 * 在父目录中创建一个新的目录项并保存到子目录项中
 * @return: 成功返回-1，失败返回0
 */
PRIVATE int BOFS_CreateNewDirEntry(struct BOFS_SuperBlock *sb,
	struct BOFS_DirEntry *parentDir,
	struct BOFS_DirEntry *childDir,
	char *name)
{
	/**
	 * 1.分配节点ID
	 * 2.创建节点
	 * 3.创建目录项
	 * 4.创建索引目录（.和..）
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
      
	BOFS_CreateInode(&inode, inodeID, (BOFS_IMODE_R|BOFS_IMODE_W),
		0, sb->deviceID);    

	// 创建目录项
	BOFS_CreateDirEntry(childDir, inodeID, BOFS_FILE_TYPE_DIRECTORY, 0, name);
	
	//BOFS_DumpDirEntry(childDir);
	//bofs_load_inode_by_id(&inode, childDir.inode);
	
	inode.size = sizeof(struct BOFS_DirEntry)*2;

	memset(sb->iobuf, 0, SECTOR_SIZE);
	BOFS_SyncInode(&inode, sb);

	//BOFS_DumpInode(&inode);

	//we need create . and .. under child 
	struct BOFS_DirEntry dir0, dir1;
	
	// . point to childDir .. point to parentDir
	//copy data
	BOFS_CopyDirEntry(&dir0, childDir, 0, sb);

	//reset name
	memset(dir0.name, 0,BOFS_NAME_LEN);
	//set name
	strcpy(dir0.name, ".");
	
	//BOFS_DumpDirEntry(&dir0);

	BOFS_CopyDirEntry(&dir1, parentDir, 0, sb);
	memset(dir1.name,0,BOFS_NAME_LEN);
	strcpy(dir1.name, "..");

	//BOFS_DumpDirEntry(&dir1);

	/*sync . and .. to child dir*/
	//printk("sync . dir >>>");
	if (!BOFS_SyncDirEntry(childDir, &dir0, sb)) {
		return -1;
	}
	//printk("sync .. dir>>>");
	if (!BOFS_SyncDirEntry(childDir, &dir1, sb)) {
		return -1;
	}

	//printk("sync child dir>>>");
	
	//Spin("test");

	int result = BOFS_SyncDirEntry(parentDir, childDir, sb);
	
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
 * BOFS_MakeDirSub - 创建一个目录
 * @pathname: 路径名
 * @sb: 超级块
 * 
 * 创建目录的时候，应该申请一个信号量，来正常竞争
 * @return: 成功返回0，失败返回-1
 */
PUBLIC int BOFS_MakeDirSub(const char *pathname, struct BOFS_SuperBlock *sb)
{

	int i, j;
	
	char *path = (char *)pathname;

	//printk("The path is %s\n", path);
	
	char *p = (char *)path;
	
	char *last;

	char name[BOFS_NAME_LEN];   
	
	struct BOFS_DirEntry parentDir, childDir;

    /* 第一个必须是根目录符号 */
	if(*p != '/'){
		printk("mkdir: bad path!\n");
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
		/* 记录上一次的路径 */
		last = p;
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
				printk("mkdir: path %s has exist, can't create it again!\n", path);
				return -1;
			}
		}else{
			
			/* 没找到，并且是在路径末尾 */
			if(i == depth - 1){
				/* 创建目录 */
				
				//printk("BOFS_MakeDir:depth:%d path:%s name:%s not exsit!\n",depth, pathname, name);
				// printk("parent %s\n", parentDir.name);
				/* 查看父目录类型，如果是挂载目录，就会切换到该文件系统下面去，不是就正常在master中创建 */
				if (parentDir.type == BOFS_FILE_TYPE_MOUNT) {
					//printk("meet a mount dir %s!\n", pathname);

					/* 进入挂在的目录所在的文件系统 */
					//printk("last name %s\n", last);
					
					/* 挂载关联是否正确 */
					if (!parentDir.mount) {
						printk("path %s mount link error!\n", pathname);
						return -1;
					}
					return BOFS_MakeDirSub(last, (struct BOFS_SuperBlock *)parentDir.mount);

				} else {
					/* 在master中创建 */	
					if (BOFS_CreateNewDirEntry(sb, &parentDir, &childDir, name)) {
						return -1;
					}
				}
			}else{
				printk("mkdir:can't create path:%s name:%s\n", pathname, name);
				return -1;
			}
		}
	}
	/*if dir has exist, it will arrive here.*/
	return 0;
}

/**
 * BOFS_MakeDir - 创建目录
 */
PUBLIC int BOFS_MakeDir(const char *pathname)
{
	//printk("mkdir: path is %s\n", pathname);
	/* 选择一个超级块 */
    struct BOFS_SuperBlock *sb = masterSuperBlock;

	return BOFS_MakeDirSub(pathname, sb);
}


/**
 * BOFS_MountDir - 在一个路径下面挂载一个文件系统
 * @pathname: 路径名
 * @devname: 设备名
 * 
 * 挂载文件系统时，只能把把文件系统挂载到主文件系统下面
 */
PUBLIC int BOFS_UnmountDirSub(char *pathname, struct BOFS_SuperBlock *sb)
{

	int i, j;
	
	char *path = (char *)pathname;

	//printk("The path is %s\n", path);
	
	char *p = (char *)path;
	char *next = NULL;

	char depth = 0;
	char name[BOFS_NAME_LEN];   
	
	struct BOFS_DirEntry parentDir, childDir;
	
    /* 第一个必须是根目录符号 */
	if(*p != '/'){
		printk("mkdir: bad path!\n");
		return -1;
	}

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
		
        /* 如果'/'后面没有名字，就直接返回，如果是挂载到根目录，就在这儿返回 */
		if(name[0] == 0){
			/* 创建的时候，如果没有指定名字，就只能返回。 */
			return -1;
		}
		if(i == 0){
			/* 如果是根目录，就把根目录当做父目录 */
			memcpy(&parentDir, sb->rootDir->dirEntry, sizeof(struct BOFS_DirEntry));
		}
		/*!!!! 这里还有一个问题，那就是在一个文件系统目录下面挂载一个文件系统，
		然后再在挂载的文件系统下面挂载一个文件系统。对于这种情况，我们不让它挂载到非文件系统上，
		也就是说，只能挂载到主文件系统上，不管你挂载到哪儿*/

		/* 搜索子目录 */
		if(BOFS_SearchDirEntry(sb, &parentDir, &childDir, name)){	//find
			/* 发现子目录是挂载目录，就记录挂载目录的位置 */
			if (childDir.type == BOFS_FILE_TYPE_MOUNT && next == NULL) {
				//printk(">>meet mount dir %s name %s depth %d\n", pathname, name, i);
				next = p;
			}
			

			if(i == depth - 1){
				//printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s has exsit! find\n",depth, pathname, name);
				
				/* 如果是挂载目录，才能进行卸载，不然就返回错误 */
				if (childDir.type == BOFS_FILE_TYPE_MOUNT) {
					//printk();
					childDir.type = BOFS_FILE_TYPE_DIRECTORY;
					childDir.mount = 0;
					/* 挂载完成后，要写回磁盘，才能生效 */
					if (!BOFS_SyncDirEntry(&parentDir, &childDir, sb)) {
						return -1;
					}
					//printk("unmount: path %s unmount sucess!\n", pathname);
				} else {
					/* 不是挂载目录，就返回错误 */
					printk("unmount: path %s not mount dir!\n", pathname);
					return -1;
				}	
			
			} else {	
				memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir
				
				/* 如果是有挂在目录，那么就要进入挂在目录所在文件系统 */
				if (next != NULL) {
					//printk("mount dir %s -> %s super at %x\n", p, next, childDir.mount);

					/* 挂载关联是否正确 */
					if (!childDir.mount) {
						printk("path %s mount link error!\n", pathname);
						return -1;
					}

					return BOFS_UnmountDirSub(next, (struct BOFS_SuperBlock *)childDir.mount);
				}
			}
			
			//printk("BOFS_MakeDir:depth:%d path:%s name:%s has exsit!\n",depth, pathname, name);
		}else{
			/* 在目录中没有找到子目录，就不能挂载 */
			printk(PART_ERROR "mount: path:%s not exist!\n", pathname, name);
			
			return -1;
		}
	}
	/*if dir has exist, it will arrive here.*/
	return 0;
}

/**
 * BOFS_MountDir - 在一个路径下面挂载一个文件系统
 * @pathname: 路径名
 * @devname: 设备名
 * 
 * 挂载文件系统时，只能把把文件系统挂载到主文件系统下面
 */
PUBLIC int BOFS_UnmountDir(const char *target)
{
	struct BOFS_SuperBlock *sb = masterSuperBlock;

	return BOFS_UnmountDirSub((char *)target, sb);
}

/**
 * BOFS_MountDir - 在一个路径下面挂载一个文件系统
 * @pathname: 路径名
 * @devname: 设备名
 * 
 * 挂载文件系统时，只能把把文件系统挂载到主文件系统下面
 */
PUBLIC int BOFS_MountDirSub(char *pathname, struct BOFS_SuperBlock *sb,
	struct BOFS_SuperBlock *devsb)
{

	int i, j;
	
	char *path = (char *)pathname;

	//printk("The path is %s\n", path);
	
	char *p = (char *)path;
	char *next = NULL;

	char depth = 0;
	char name[BOFS_NAME_LEN];   
	
	struct BOFS_DirEntry parentDir, childDir;
	
    /* 第一个必须是根目录符号 */
	if(*p != '/'){
		printk("mkdir: bad path!\n");
		return -1;
	}

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
		
        /* 如果'/'后面没有名字，就直接返回，如果是挂载到根目录，就在这儿返回 */
		if(name[0] == 0){
			/* 创建的时候，如果没有指定名字，就只能返回。 */
			return -1;
		}
		if(i == 0){
			/* 如果是根目录，就把根目录当做父目录 */
			memcpy(&parentDir, sb->rootDir->dirEntry, sizeof(struct BOFS_DirEntry));
		}
		/*!!!! 这里还有一个问题，那就是在一个文件系统目录下面挂载一个文件系统，
		然后再在挂载的文件系统下面挂载一个文件系统。对于这种情况，我们不让它挂载到非文件系统上，
		也就是说，只能挂载到主文件系统上，不管你挂载到哪儿*/

		//parentDir we have gotten under root dir
		if(BOFS_SearchDirEntry(sb, &parentDir, &childDir, name)){	//find
			/* 发现子目录是挂载目录，就记录挂载目录的位置 */
			if (childDir.type == BOFS_FILE_TYPE_MOUNT && next == NULL) {
				//printk(">>meet mount dir %s name %s depth %d\n", pathname, name, i);
				next = p;
			}
			
			if(i == depth - 1){
				//printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s has exsit! find\n",depth, pathname, name);
				
				/* 如果是以经挂在的目录，那么可以重新挂在
				修改类型为挂载目录项 */
				childDir.type = BOFS_FILE_TYPE_MOUNT;
				childDir.mount = (unsigned int )devsb;
				/* 挂载完成后，要写回磁盘，才能生效 */
				if (!BOFS_SyncDirEntry(&parentDir, &childDir, sb)) {
					return -1;
				}
				//BOFS_DumpDirEntry(&childDir);
				
				//printk("mount: path %s mount sucess!\n", pathname);
			
			} else {	
				memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir
				
				/* 如果是有挂在目录，那么就要进入挂在目录所在文件系统 */
				if (next != NULL) {
					//printk("mount dir %s -> %s super at %x\n", p, next, childDir.mount);

					return BOFS_MountDirSub(next, (struct BOFS_SuperBlock *)childDir.mount, devsb);
				}
			}
			
			//printk("BOFS_MakeDir:depth:%d path:%s name:%s has exsit!\n",depth, pathname, name);
		}else{
			/* 在目录中没有找到子目录，就不能挂载 */
			printk(PART_ERROR "mount: path:%s not exist!\n", pathname, name);
			
			return -1;
		}
	}
	/*if dir has exist, it will arrive here.*/
	return 0;
}

/**
 * BOFS_MountDir - 在一个路径下面挂载一个文件系统
 * @devname: 设备路径名
 * @pathname: 路径名
 * 
 * 挂载文件系统时，只能把把文件系统挂载到主文件系统下面
 */
PUBLIC int BOFS_MountDir(const char *devpath, const char *pathname)
{
	struct BOFS_SuperBlock *sb = masterSuperBlock;

	char name[BOFS_NAME_LEN];
	memset(name, 0, BOFS_NAME_LEN);

	/* 获取命名 */
	if (BOFS_PathToName(devpath, name)) {
		return -1;
	}

	/* 获取设备对应得设备记录信息 */
	
	struct Device *device = GetDeviceByID(sb->deviceID);

	if (device == NULL) {
		printk("Get device failed!\n");
		return -1;
	}

	if (device->private == NULL) {
		printk("device %s pointer is null!\n", device->name);
		return -1;
	}

	struct BOFS_SuperBlock *devsb = device->private;
	
	struct BOFS_DirSearchRecord record;
	
	/* 检测设备是否为块设备，只有块设备才能进行挂载 */
	int found = BOFS_SearchDir((char *)devpath, &record, sb);

	if(!found){
		/* 只有找到了，并且是块设备才能进行挂载 */
		if (record.childDir->type == BOFS_FILE_TYPE_BLOCK) {
			if(!BOFS_MountDirSub((char *)pathname, sb, devsb)) {
				return 0;
			} else {
				printk("device mount failed!\n");
				return -1;
			}
		}
		printk("device %s not a block device!\n", name);
		return -1;
	}

	printk("device %s not exist!\n", name);
	return -1;
}


/**
 * BOFS_RemoveDirSub - 移除一个目录项
 * @pathname: 路径名
 */
PUBLIC int BOFS_RemoveDirSub(const char *pathname, struct BOFS_SuperBlock *sb)
{

	int i, j;
	
	char *path = (char *)pathname;

	char *p = (char *)path;
	char *next = NULL;
	char depth = 0;
	char name[BOFS_NAME_LEN];
	
	struct BOFS_DirEntry parentDir, childDir;
	
    /* 第一个必须是根目录符号 */
	if(*p != '/'){
		printk("BOFS_RemoveDir: bad path!\n");
		return -1;
	}

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

        /* 准备获取目录名字 */
		memset(name, 0, BOFS_NAME_LEN);
		j = 0;

        /* 如果没有遇到'/'就一直获取字符 */
		while(*p != '/' && j < BOFS_NAME_LEN){
			name[j] = *p;
			j++;
			p++;
		}
        /* 如果'/'后面没有名字，就直接返回 */
		if(name[0] == 0){
			return -1;
		}
		
		if (i == 0) {
			/* 是根目录的话，就把根目录当做父目录 */
			memcpy(&parentDir, sb->rootDir->dirEntry, sizeof(struct BOFS_DirEntry));
		}
		/* 在根目录中搜索目录项 */
		if (BOFS_SearchDirEntry(sb, &parentDir, &childDir, name)) {	
			/* 找到了才可以移除 */

			//printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s has exsit! find\n",i, pathname, name);
			/* 发现子目录是挂载目录，就记录挂载目录的位置 */
			if (childDir.type == BOFS_FILE_TYPE_MOUNT && next == NULL) {
				//printk(">>meet mount dir %s name %s depth %d\n", pathname, name, i);
				next = p;
			}

			/* 如果是最后一个目录路径就移除它 */
			if(i == depth - 1){
				/* 不能直接删除挂载目录 */
				if (childDir.type != BOFS_FILE_TYPE_MOUNT) {
					if (childDir.type == BOFS_FILE_TYPE_BLOCK || childDir.type == BOFS_FILE_TYPE_CHAR) {
						printk("rmdir: can't remove device %s with rmdir, please use rm instead!\n", pathname);
						return -1;
					}
					/* 先释放目录项，再同步目录项 */
					BOFS_ReleaseDirEntry(sb, &childDir);
					
					//BOFS_DumpDirEntry(&childDir);

					if(BOFS_SyncDirEntry(&parentDir, &childDir, sb)){
						/* 删除一个目录项之后，不用修改父目录的大小，因为我们只把
						子目录设置成无效，这是为了以后能够恢复信息 */
						return 0;
					}
				} else {
					printk("rmdir: can't remove mount dir %s\n", pathname);
					return -1;
				}
				
			}else{
				/*
				还没有到最后一个目录，把子目录当做父目录，再次搜索
				*/
				memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir
				//printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s not last path, continue!\n",i, pathname, name);
				/* 遇到挂载目录，并且还没有到最后的路径，说明要删除的路径在挂载目录里面 */
				if (next != NULL) {
					//printk("remove under dir %s\n", next);
					/* 挂载关联是否正确 */
					if (!childDir.mount) {
						printk("path %s mount link error!\n", pathname);
						return -1;
					}
					return BOFS_RemoveDirSub(next, (struct BOFS_SuperBlock *)childDir.mount);
				}
			}
			
		} else {
			/* 没有找到，就会直接在后面返回 */
			printk("BOFS_RemoveDir:depth:%d path:%s name:%s not exsit! not find\n",depth, pathname, name);
		}
	}
	/*if dir has not exist, it will arrive here.*/
	return -1;
}

/**
 * BOFS_RemoveDir - 移除一个目录项
 * @pathname: 路径名
 */
PUBLIC int BOFS_RemoveDir(const char *pathname)
{
	//printk("rmdir: The path is %s\n", pathname);
	
    struct BOFS_SuperBlock *sb = masterSuperBlock;

	return BOFS_RemoveDirSub(pathname, sb);
}

/**
 * BOFS_ChangeCWD - 改变当前工作目录
 * 
 */
PUBLIC int BOFS_ChangeCWD(const char *pathname)
{
	struct Task *cur = CurrentTask();
	int ret = -1;
	
	char *path = (char *)pathname;

	struct BOFS_SuperBlock *sb = masterSuperBlock;

	/*root dir*/
	if(path[0] == '/' && path[1] == 0){
		memset(cur->cwd, 0, BOFS_PATH_LEN);
		strcpy(cur->cwd, pathname);
		ret = 0;
	}else{
		/*search dir*/
		struct BOFS_DirSearchRecord record;  
		memset(&record, 0, sizeof(struct BOFS_DirSearchRecord));
		int found = BOFS_SearchDir(path, &record, sb);
		//printk("bofs_chdir: %s is found!\n", path);
		if (!found) {
			//printk("bofs_chdir: %s is found!\n", path);
			if(record.childDir->type == BOFS_FILE_TYPE_DIRECTORY){
				
				memset(cur->cwd, 0, BOFS_PATH_LEN);
				strcpy(cur->cwd, pathname);
				//printk("bofs_chdir: thread pwd %s \n", cur->cwd);
				ret = 0;
			}
			BOFS_CloseDirEntry(record.childDir);
		}else {	
			printk("bofs chdir: %s isn't exist or not a directory!\n", pathname);
		}
		BOFS_CloseDirEntry(record.parentDir);
	}
	return ret;
}

PUBLIC int BOFS_GetCWD(char* buf, unsigned int size)
{
	struct Task *cur = CurrentTask();

	if (size > BOFS_PATH_LEN)
		size = BOFS_PATH_LEN;

	memcpy(buf, cur->cwd, size);
	return 0;
}


PUBLIC int BOFS_ResetName(const char *pathname, char *name)
{
	char *path = (char *)pathname;
	struct BOFS_SuperBlock *sb = masterSuperBlock;

	/*if dir is '/' */
	if(!strcmp(path, "/")){
		printk("rename: can't reset '/' name!\n");
		return -1;
	}
	
	char newPath[BOFS_PATH_LEN] = {0};
	memset(newPath, 0, BOFS_PATH_LEN);
	strcpy(newPath, pathname);
	
	int i;
	/*从后往前寻找'/'*/
	for(i = strlen(newPath)-1; i >= 0; i--){
		if(newPath[i] == '/'){
			break;
		}
	}
	i++; /*跳过'/'*/
	while(i < BOFS_PATH_LEN){
		newPath[i] = 0;
		i++;
	}
	
	strcat(newPath, name);
	
	//printk("new path :%s\n", newPath);
	/*check the name after rename*/
	if(!BOFS_Access(newPath, BOFS_F_OK)){
		printk("rename: the path name:%s has exist!\n", newPath);
		return -1;
	}
	
	int ret = -1;	// default -1
	struct BOFS_DirSearchRecord record;
	memset(&record, 0, sizeof(struct BOFS_DirSearchRecord));   // 记得初始化或清0,否则栈中信息不知道是什么
	int found = BOFS_SearchDir(path, &record, sb);
	if(!found){
		//printk("rename: find dir entry %s.\n", record.childDir->name);
		/*reset name*/
		memset(record.childDir->name, 0, BOFS_NAME_LEN);
		strcpy(record.childDir->name, name);
		/*sync dir entry*/
		if(BOFS_SyncDirEntry(record.parentDir, record.childDir, record.superBlock)){
			ret = 0;
		}
		BOFS_CloseDirEntry(record.childDir);
	} else {
		printk("%s not exist!\n", pathname);
	}
	BOFS_CloseDirEntry(record.parentDir);
	return ret;
}

