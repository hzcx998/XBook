/*
 * file:		fs/bofs/dir.c
 * auther:		Jason Hu
 * time:		2019/9/7
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <book/deviceio.h>
#include <driver/ide.h>
#include <driver/clock.h>
#include <book/share.h>
#include <fs/bofs/dir.h>
#include <fs/bofs/dir_entry.h>
#include <fs/bofs/bitmap.h>
#include <fs/bofs/super_block.h>
#include <book/device.h>

/*
get the name from path
*/
int BOFS_PathToName(const char *pathname, char *namebuf)
{
	char *p = (char *)pathname;
	char depth = 0;
	char name[BOFS_NAME_LEN];
	int i,j;
	
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
			return -1;
		}
		
		if(i == depth-1){	//name is what we need
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

struct BOFS_SuperBlock *BOFS_GetSuperBlockByPath(const char *pathname)
{
	int i, j;
	
	char *path = (char *)pathname;

	//printk("The path is %s\n", path);
	
	char *p = (char *)path;
	char depth = 0;
	char name[BOFS_NAME_LEN];
	
	struct BOFS_DirEntry parentDir, childDir;
	
    /* 超级块指针 */
    currentSuperBlock = masterSuperBlock;

    struct BOFS_SuperBlock *sb = currentSuperBlock;


	int isMount = 0;

    /* 第一个必须是根目录符号 */
	if(*p != '/'){
		printk("BOFS_GetSuperBlockByPath: bad path!\n");
		return NULL;
	}

	//计算一共有多少个路径/ 
	while(*p){
		if(*p == '/'){
			depth++;
		}
		p++;
	}
	
	p = (char *)path;

	//  / /bin /bin/

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
		//printk("dir name is %s\n", name);

        /* 如果'/'后面没有名字，就直接返回 */
		if(name[0] == 0){
			
			/* 判断是否为挂载的超级块 */
			if (!isMount)
				return masterSuperBlock;
			else 
				return sb;

			return NULL;
		}

		/* 如果深度是0，就说明是根目录，则进行特殊处理 */
		if(i == 0){
            /* 在根目录中搜索目录项 */
			if(BOFS_SearchDirEntry(sb, sb->rootDir->dirEntry, &childDir, name)){	
				/* 如果搜索到，说明目录已经存在，那么就保存目录项，然后在子目录项中去搜索
                子目录项变成父目录，以便进行下一次搜索。*/
                memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir
				//printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s has exsit! find\n",depth, pathname, name);

				
				/* 是在最后找到了 */
				if(i == depth - 1){
					/* 直接返回master */
					return masterSuperBlock;
				} else {
					
				}
			} else {
				/* 没找到目录 */
				return NULL;
			}
		}else{	//if not under the root dir 
            /* 如果是/mnt/目录，那么就根据/mnt/fs-sb来设置不同的超级块 */
            
			//parentDir we have gotten under root dir
			if(BOFS_SearchDirEntry(sb, &parentDir, &childDir, name)){	//find
				memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir
				
				/* 是在最后找到了 */
				if(i == depth - 1){
					/* 返回超级块 */
					return sb;
				} else {
					
				}
			} else {
				/* 没找到目录 */
				return NULL;
			}
		}
	}
	/*if dir has exist, it will arrive here.*/
	return NULL;
}

void BOFS_ListDir(const char *pathname, int level)
{
	printk("List dir path: %s\n", pathname);
	//open a dir
	struct BOFS_Dir *dir = BOFS_OpenDir(pathname);

	if(dir == NULL){
		return;
	}
	
	struct BOFS_SuperBlock *sb = BOFS_GetSuperBlockByPath(pathname);

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
			}
			BOFS_LoadInodeByID(&inode, dirEntry->inode, sb);
			
			printk("%d/%d/%d ",
				DATA16_TO_DATE_YEA(inode.crttime>>16),
				DATA16_TO_DATE_MON(inode.crttime>>16),
				DATA16_TO_DATE_DAY(inode.crttime>>16));
			printk("%d:%d:%d ",
				DATA16_TO_TIME_HOU(inode.crttime&0xffff),
				DATA16_TO_TIME_MIN(inode.crttime&0xffff),
				DATA16_TO_TIME_SEC(inode.crttime&0xffff));
			printk("%c %d %s \n", type, inode.size, dirEntry->name);
		}else if(level == 1){
			if(dirEntry->type != BOFS_FILE_TYPE_INVALID){
				if(dirEntry->type == BOFS_FILE_TYPE_DIRECTORY){
					type = 'd';
				}else if(dirEntry->type == BOFS_FILE_TYPE_NORMAL){
					type = 'f';
				}
				BOFS_LoadInodeByID(&inode, dirEntry->inode, sb);
			
				printk("%d/%d/%d ",
					DATA16_TO_DATE_YEA(inode.crttime>>16),
					DATA16_TO_DATE_MON(inode.crttime>>16),
					DATA16_TO_DATE_DAY(inode.crttime>>16));
				printk("%d:%d:%d ",
					DATA16_TO_TIME_HOU(inode.crttime&0xffff),
					DATA16_TO_TIME_MIN(inode.crttime&0xffff),
					DATA16_TO_TIME_SEC(inode.crttime&0xffff));
				printk("%c %d %s \n", type, inode.size, dirEntry->name);
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

void BOFS_RewindDir(struct BOFS_Dir *dir)
{
	dir->pos = 0;
}

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

PRIVATE struct BOFS_Dir *BOFS_OpenDirSub(struct BOFS_DirEntry *dirEntry,
	struct BOFS_SuperBlock *sb)
{
	/* 创建目录结构 */
	struct BOFS_Dir *dir = (struct BOFS_Dir *)kmalloc(sizeof(struct BOFS_Dir), GFP_KERNEL);
	if(dir == NULL){
		return NULL;
	}
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
		if (DeviceRead(sb->deviceID, lba, buf, 1)) {
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
		printk("search dir parent name %s child name %s super at %x\n",
			record.parentDir->name, record.childDir->name, record.superBlock);

		if (record.childDir->type == BOFS_FILE_TYPE_NORMAL) {	///nomal file
			//printk("%s is regular file!\n", path);
			
			BOFS_CloseDirEntry(record.childDir);
		} else if (record.childDir->type == BOFS_FILE_TYPE_DIRECTORY) {
			//printk("%s is dir file!\n", path);
			
			BOFS_DumpSuperBlock((struct BOFS_SuperBlock *)record.superBlock);
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
			/* 父目录会被释放 */
			record->parentDir = NULL;
			kfree(parentDir);

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
				printk(">>meet mount dir %s name %s depth %d\n", pathname, name, i);
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
					printk("search under dir %s -> %s super at %x\n", p, next, childDir->mount);
					return BOFS_SearchDir(next, record, (struct BOFS_SuperBlock *)childDir->mount);
				}
			}
			
			//printk("find\n");
		}else{
			printk("not found\n");

			//printk("search:error!\n");
			record->parentDir = sb->rootDir->dirEntry;
			record->childDir = NULL;
			
			kfree(childDir);
			break;
		}
	}
	return -1;
}

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
	if (DeviceRead(sb->deviceID, sb->dataStartLba, sb->iobuf, 1)) {
        return -1;
    }
	
	memcpy(dirEntry, sb->iobuf, sizeof(struct BOFS_DirEntry));

    sb->rootDir->dirEntry = dirEntry;
	//BOFS_DumpDirEntry(dirEntry);

    /* 根目录的目录信息不使用，在打开根目录的时候会重新创建一个信息 */
    sb->rootDir->pos = 0;
    sb->rootDir->size = 0;
	sb->rootDir->buf = NULL;

    return 0;
}

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
	
	unsigned int inodeID = BOFS_AllocBitmap(sb, BOFS_BMT_INODE, 1); 
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

	int sync_ret = BOFS_SyncDirEntry(parentDir, childDir, sb);
	
	if(sync_ret > 0){	//successed
		if(sync_ret == 1){
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
 * BOFS_MakeDir - 创建一个目录
 * @pathname: 路径名
 * 
 * 创建目录的时候，应该申请一个信号量，来正常竞争
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

		/* 如果深度是0，就说明是根目录，则进行特殊处理 */
		if(i == 0){
            /* 在根目录中搜索目录项 */
			if(BOFS_SearchDirEntry(sb, sb->rootDir->dirEntry, &childDir, name)){	
				/* 如果搜索到，说明目录已经存在，那么就保存目录项，然后在子目录项中去搜索
                子目录项变成父目录，以便进行下一次搜索。*/
                memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir
				//printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s has exsit! find\n",depth, pathname, name);

				/* 如果已经存在，并且位于末尾，说明我们想要创建一个已经存在的目录，返回失败 */
				if(i == depth - 1){
					printk("mkdir: path %s has exist, can't create it again!\n", path);
					return -1;
				}
			}else{
                /* 在根目录中没有找到子目录，就可以创建这个目录 */
	
				//printk("BOFS_MakeDir:depth:%d path:%s name:%s not exsit! not find\n",depth, pathname, name);
	
				/* 检测是否是最后一个目录不存在，如果是，就创建这个目录 */
				if(i == depth - 1){
					//printk("BOFS_MakeDir:depth:%d path:%s name:%s not exsit!\n",depth, pathname, name);

					//printk("BOFS_MakeDir: create path:%s name:%s\n", pathname, name);
					
					if (BOFS_CreateNewDirEntry(sb, sb->rootDir->dirEntry, &childDir, name)) {
						return -1;
					}
				}else{
					//printk("mkdir:can't create path:%s name:%s\n", pathname, name);
					
					return -1;
				}
			}
		}else{	//if not under the root dir 
            /* 如果是/mnt/目录，那么就根据/mnt/fs-sb来设置不同的超级块 */
            
			//parentDir we have gotten under root dir
			if(BOFS_SearchDirEntry(sb, &parentDir, &childDir, name)){	//find
				memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir
				//printk("BOFS_MakeDir:depth:%d path:%s name:%s has exsit!\n",depth, pathname, name);

				/* 如果已经存在，并且位于末尾，说明我们想要创建一个已经存在的目录，返回失败 */
				if(i == depth - 1){
					printk("mkdir: path %s has exist, can't create it again!\n", path);
					return -1;
				}
			}else{
				
				/*check whether is last dir name*/
				if(i == depth - 1){	//create it
					//printk("BOFS_MakeDir:depth:%d path:%s name:%s not exsit!\n",depth, pathname, name);
					// printk("parent %s\n", parentDir.name);
					/* 查看父目录类型，如果是挂载目录，就会切换到该文件系统下面去，不是就正常在master中创建 */
					if (parentDir.type == BOFS_FILE_TYPE_MOUNT) {
						printk("meet a mount dir %s!\n", pathname);

						/* 进入挂在的目录所在的文件系统 */
						printk("last name %s\n", last);
						
						return BOFS_MakeDirSub(last, (struct BOFS_SuperBlock *)parentDir.mount);

					} else {
						/* 在master中创建 */	
						if (BOFS_CreateNewDirEntry(sb, &parentDir, &childDir, name)) {
							return -1;
						}
					}
				}else{
					//printk("mkdir:can't create path:%s name:%s\n", pathname, name);
					return -1;
				}
			}
		}
	}
	/*if dir has exist, it will arrive here.*/
	return 0;
}

PUBLIC int BOFS_MakeDir(const char *pathname)
{
	printk("mkdir: path is %s\n", pathname);
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
PUBLIC int BOFS_MountDir(const char *pathname, char *devname)
{
	struct BOFS_SuperBlock *sb = masterSuperBlock;

	int i, j;
	
	char *path = (char *)pathname;

	//printk("The path is %s\n", path);
	
	char *p = (char *)path;

	char depth = 0;
	char name[BOFS_NAME_LEN];   
	
	struct BOFS_DirEntry parentDir, childDir;
	
	struct BOFS_SuperBlock *devsb = GetDevicePointer(devname);
	if (devsb == NULL) {
		printk("Get device pointer failed!");
		return -1;
	}
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
		
        /* 如果'/'后面没有名字，就直接返回 */
		if(name[0] == 0){
			/* 创建的时候，如果没有指定名字，就只能返回。 */
			return -1;
		}

		/* 如果深度是0，就说明是根目录，则进行特殊处理 */
		if(i == 0){

            /* 在根目录中搜索目录项 */
			if(BOFS_SearchDirEntry(sb, sb->rootDir->dirEntry, &childDir, name)){	
				/* 如果已经存在，并且位于末尾 */
				if(i == depth - 1){
					//printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s has exsit! find\n",depth, pathname, name);
					/* 如果该目录已经是一个挂载目录，那么就不能再次挂载 */
					/*if (childDir.type == BOFS_FILE_TYPE_MOUNT) {
						printk("mount: path %s has mount, can't mount it again!\n", path);
					} else {*/
						/* 可以进行挂载，挂载到根目录下面的一个目录下面 */

						/* 修改类型为挂载目录项 */
						childDir.type = BOFS_FILE_TYPE_MOUNT;
						childDir.mount = (unsigned int )devsb;
						/* 挂载完成后，要写回磁盘，才能生效 */
						if (!BOFS_SyncDirEntry(&parentDir, &childDir, sb)) {
							return -1;
						}
						//BOFS_DumpDirEntry(&childDir);
						
						printk("mount: path %s mount sucess!\n", pathname);
					//}
				}
				/* 如果搜索到，说明目录已经存在，那么就保存目录项，然后在子目录项中去搜索
                子目录项变成父目录，以便进行下一次搜索。*/
                memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir
				
			}else{
                /* 在根目录中没有找到子目录，就不能挂载 */

				printk(PART_ERROR "mount: path:%s not exist!\n", pathname, name);
				
				return -1;
			}
		}else{	//if not under the root dir 
            /* 如果是/mnt/目录，那么就根据/mnt/fs-sb来设置不同的超级块 */
            
			/*!!!! 这里还有一个问题，那就是在一个文件系统目录下面挂载一个文件系统，
			然后再在挂载的文件系统下面挂载一个文件系统。对于这种情况，我们不让它挂载到非文件系统上，
			也就是说，只能挂载到主文件系统上，不管你挂载到哪儿*/

			//parentDir we have gotten under root dir
			if(BOFS_SearchDirEntry(sb, &parentDir, &childDir, name)){	//find
				if(i == depth - 1){
					//printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s has exsit! find\n",depth, pathname, name);
					/* 如果该目录已经是一个挂载目录，那么就不能再次挂载 */
					/*if (childDir.type == BOFS_FILE_TYPE_MOUNT) {
						printk("mount: path %s has mount, can't mount it again!\n", path);
					} else {*/
						/* 可以进行挂载，挂载到非根目录下面的一个目录 */

						/* 修改类型为挂载目录项 */
						childDir.type = BOFS_FILE_TYPE_MOUNT;
						childDir.mount = (unsigned int )devsb;
						/* 挂载完成后，要写回磁盘，才能生效 */
						if (!BOFS_SyncDirEntry(&parentDir, &childDir, sb)) {
							return -1;
						}
						//BOFS_DumpDirEntry(&childDir);
						
						printk("mount: path %s mount sucess!\n", pathname);
					//}
				}
				
				memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir
				//printk("BOFS_MakeDir:depth:%d path:%s name:%s has exsit!\n",depth, pathname, name);
			}else{
				/* 在目录中没有找到子目录，就不能挂载 */
				printk(PART_ERROR "mount: path:%s not exist!\n", pathname, name);
				
				return -1;
			}
		}
	}
	/*if dir has exist, it will arrive here.*/
	return 0;
}

/**
 * BOFS_RemoveDir - 移除一个目录项
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

		/* 如果深度是0，就说明是根目录，则进行特殊处理 */
		if(i == 0){
            /* 在根目录中搜索目录项 */
			if (BOFS_SearchDirEntry(sb, sb->rootDir->dirEntry, &childDir, name)) {	
				/* 找到了才可以移除 */

				//printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s has exsit! find\n",i, pathname, name);
				/* 发现子目录是挂载目录，就记录挂载目录的位置 */
				if (childDir.type == BOFS_FILE_TYPE_MOUNT && next == NULL) {
					printk(">>meet mount dir %s name %s depth %d\n", pathname, name, i);
					next = p;
				}

				/* 如果是最后一个目录路径就移除它 */
				if(i == depth - 1){
					/* 不能直接删除挂载目录 */
					if (childDir.type != BOFS_FILE_TYPE_MOUNT) {
						
						
						/* 先释放目录项，再同步目录项 */
						BOFS_ReleaseDirEntry(sb, &childDir);
						
						BOFS_DumpDirEntry(&childDir);

						if(BOFS_SyncDirEntry(sb->rootDir->dirEntry, &childDir, sb)){
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
					printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s not last path, continue!\n",i, pathname, name);
					/* 遇到挂载目录，并且还没有到最后的路径，说明要删除的路径在挂载目录里面 */
					if (next != NULL) {
						printk("remove under dir %s\n", next);
						return BOFS_RemoveDirSub(next, (struct BOFS_SuperBlock *)childDir.mount);
					}
				}
				
			} else {
				/* 没有找到，就会直接在后面返回 */
				printk("BOFS_RemoveDir:depth:%d path:%s name:%s not exsit! not find\n",depth, pathname, name);
			}
		}else{
            /* 如果是/mnt/目录，那么就根据/mnt/fs-sb来设置不同的超级块 */
            
			//parentDir we have gotten under root dir
			if(BOFS_SearchDirEntry(sb, &parentDir, &childDir, name)){	//find
				
				//printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s has exsit! find\n",i, pathname, name);
				
				/* 发现子目录是挂载目录，就记录挂载目录的位置 */
				if (childDir.type == BOFS_FILE_TYPE_MOUNT && next == NULL) {
					printk(">>meet mount dir %s name %s depth %d\n", pathname, name, i);
					next = p;
				}
				
				/* 如果是最后一个目录路径就移除它 */
				if(i == depth - 1){
					if (childDir.type != BOFS_FILE_TYPE_MOUNT) {
						/* 擦除目录项 */
						/*if (BOFS_CreateNewDirEntry(sb, &parentDir, &childDir, name)) {
							return 0;
						}*/
						printk("release dir entry %s\n",childDir.name);

						/* 先释放目录项，再同步目录项 */
						BOFS_ReleaseDirEntry(sb, &childDir);
						
						printk("sync dir entry %s\n",childDir.name);
						
						if(BOFS_SyncDirEntry(&parentDir, &childDir, sb)){
							return 0;
						}
					}  else {
						printk("rmdir: can't remove mount dir %s\n", pathname);
						return -1;
					}
				}else{
					memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir
					//printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s not last path, continue!\n",depth, pathname, name);
					/* 遇到挂载目录，并且还没有到最后的路径，说明要删除的路径在挂载目录里面 */
					if (next != NULL) {
						printk("remove under dir %s\n", next);
						return BOFS_RemoveDirSub(next, (struct BOFS_SuperBlock *)childDir.mount);
					}	
				}

				
			} else{
				/* 没有找到，就会直接在后面返回 */
				printk("BOFS_RemoveDir:depth:%d path:%s name:%s not exsit! not find\n",i, pathname, name);
			}
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
	printk("rmdir: The path is %s\n", pathname);
	
    struct BOFS_SuperBlock *sb = masterSuperBlock;

	return BOFS_RemoveDirSub(pathname, sb);
}