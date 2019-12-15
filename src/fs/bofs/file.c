/*
 * file:		fs/bofs/main.c
 * auther:		Jason Hu
 * time:		2019/9/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/memcache.h>
#include <book/debug.h>
#include <share/string.h>
#include <share/string.h>
#include <share/math.h>
#include <fs/bofs/file.h>
#include <fs/bofs/bitmap.h>
#include <book/device.h>
#include <book/blk-buffer.h>
#include <drivers/tty.h>
#include <fs/bofs/pipe.h>
#include <fs/bofs/fifo.h>

struct BOFS_FileDescriptor BOFS_GlobalFdTable[BOFS_MAX_FD_NR];

PUBLIC void BOFS_InitFdTable()
{
	int fdIdx = 0;
	while (fdIdx < BOFS_MAX_FD_NR) {
		
		BOFS_GlobalFdTable[fdIdx].flags = BOFS_FD_FREE;
		BOFS_GlobalFdTable[fdIdx].dirEntry = NULL;
		BOFS_GlobalFdTable[fdIdx].parentEntry = NULL;
		BOFS_GlobalFdTable[fdIdx].inode = NULL;
		BOFS_GlobalFdTable[fdIdx].pos = 0;
		BOFS_GlobalFdTable[fdIdx].superBlock = NULL;
		
		fdIdx++;
	}
}

PUBLIC int BOFS_AllocFdGlobal()
{
	uint32 fdIdx = 0;
	while (fdIdx < BOFS_MAX_FD_NR) {
		if (BOFS_GlobalFdTable[fdIdx].flags == BOFS_FD_FREE) {
			BOFS_GlobalFdTable[fdIdx].flags = BOFS_FD_USING;
			return fdIdx;
		}
		fdIdx++;
	}
	
	printk("alloc fd over max open files!\n");
	return -1;
}

PUBLIC void BOFS_FreeFdGlobal(int fd)
{
    if(fd < 0 || fd >= BOFS_MAX_FD_NR) {
		printk("fd error\n");
		return;
	}

	BOFS_GlobalFdTable[fd].flags = BOFS_FD_FREE;
    BOFS_GlobalFdTable[fd].inode = NULL;
    BOFS_GlobalFdTable[fd].dirEntry = NULL;
    BOFS_GlobalFdTable[fd].parentEntry = NULL;
    BOFS_GlobalFdTable[fd].pos = 0;
	BOFS_GlobalFdTable[fd].superBlock = NULL;
}

PUBLIC struct BOFS_FileDescriptor *BOFS_GetFileByFD(int fd)
{
	if(fd < 0 || fd >= BOFS_MAX_FD_NR) {
		printk("fd error\n");
		return NULL;
	}
	return &BOFS_GlobalFdTable[fd];
}

PUBLIC void BOFS_DumpFD(int fd)
{
	if(fd < 0 || fd >= BOFS_MAX_FD_NR) {
		printk("fd error\n");
		return;
	}
	struct BOFS_FileDescriptor *ptr = &BOFS_GlobalFdTable[fd];

	printk(PART_TIP "----File Descriptor----\n");

	printk(PART_TIP "dir entry %x parent entry %x inode %x\n",
		ptr->dirEntry, ptr->parentEntry, ptr->inode);
	
	printk(PART_TIP "fd %d flags %x pos %d\n",
		fd, ptr->flags, ptr->pos);
	
}

PUBLIC void BOFS_GlobalFdAddFlags(unsigned int globalFd, unsigned int flags)
{
	ASSERT(globalFd >= 0 && globalFd < BOFS_MAX_FD_NR);
    BOFS_GlobalFdTable[globalFd].flags |= flags;
}

PUBLIC void BOFS_GlobalFdDelFlags(unsigned int globalFd, unsigned int flags)
{
	ASSERT(globalFd >= 0 && globalFd < BOFS_MAX_FD_NR);
    BOFS_GlobalFdTable[globalFd].flags &= ~flags;
}

PUBLIC int BOFS_GlobalFdHasFlags(unsigned int globalFd, unsigned int flags)
{
	ASSERT(globalFd >= 0 && globalFd < BOFS_MAX_FD_NR);

    return (BOFS_GlobalFdTable[globalFd].flags & flags);
}


/**
 * FdLocal2Global - 把进程中的fd转换成为系统中的fd
 * @localFD: 局部文件描述符
 * 
 * 返回全局文件描述符fd
 */
PUBLIC uint32_t FdLocal2Global(uint32_t localFD)
{
    struct Task *cur = CurrentTask();
    int32_t globalFD = cur->fdTable[localFD]; /* 文件描述符表中存放的就是全局的文件描述符 */
    ASSERT(globalFD >= 0 && globalFD < BOFS_MAX_FD_NR);
    return (uint32_t )globalFD;
}

/**
 * TaskInstallFD - 任务安装文件描述符
 * @globalFdIdx: 全局描述符
 * 
 * 将全局描述符下标安装到进程或线程自己的文件描述符数组fd_table中
 * 
 * 成功返回下标,失败返回-1 
 */
PUBLIC int TaskInstallFD(int globalFdIdx) 
{
    struct Task* cur = CurrentTask();
    /* 跨过stdin,stdout,stderr */
    uint8_t localFdIdx = 0; 
    while (localFdIdx < MAX_OPEN_FILES_IN_PROC) {
        if (cur->fdTable[localFdIdx] == -1) {	// -1表示空闲，可以使用
	        cur->fdTable[localFdIdx] = globalFdIdx; // 填写全局描述符索引到局部描述符表
	        break;
        }
        localFdIdx++;
    }
    if (localFdIdx == MAX_OPEN_FILES_IN_PROC) {
        printk(PART_ERROR "local fd out of bound!\n");
        return -1;
    }
    return localFdIdx;
}

PRIVATE int BOFS_CreateFile(struct BOFS_DirEntry *parentDir,
	char *name,
	unsigned int mode,
	struct BOFS_SuperBlock *sb)
{
	/**
	 * 创建目录项和节点
	 * 1.分配节点ID
	 * 2.创建节点
	 * 3.创建目录项
	 */
	/*printk("BOFS_CreateFile: parent %x name %s mode %x sb %x\n",
		parentDir, name, mode, sb);
	*/
	/* 分配内存空间 */
	struct BOFS_DirEntry *dirEntry = (struct BOFS_DirEntry *)kmalloc(\
		sizeof(struct BOFS_DirEntry), GFP_KERNEL);
	if(dirEntry == NULL){
		printk("alloc memory for dirEntry failed!\n");
		return -1;
	}
	
	struct BOFS_Inode *inode = (struct BOFS_Inode *)kmalloc(\
		sizeof(struct BOFS_Inode), GFP_KERNEL);
	if(inode == NULL){
		printk("alloc memory for inode failed!\n");
		kfree(dirEntry);
		return -1;
	}

	/* 分配节点 */
	unsigned int inodeID = BOFS_AllocBitmap(sb, BOFS_BMT_INODE, 1); 
	if (inodeID == -1) {
		printk(PART_ERROR "BOFS alloc inode bitmap failed!\n");
		return -1;
	}
	//printk("alloc inode id %d\n", inodeID);
	
	/* 把节点id同步到磁盘 */
	BOFS_SyncBitmap(sb, BOFS_BMT_INODE, inodeID);
    
	BOFS_CreateInode(inode, inodeID, mode,
		0, sb->devno);    

	// 创建目录项，普通文件
	BOFS_CreateDirEntry(dirEntry, inodeID, BOFS_FILE_TYPE_NORMAL, name);
	
	BOFS_SyncInode(inode, sb);

	//BOFS_DumpInode(inode);

	/*2.sync dir entry to parent dir*/
	int result = BOFS_SyncDirEntry(parentDir, dirEntry, sb);
	
	if(result > 0){
		if(result == 1){
			/*3.change parent dir size*/
			struct BOFS_Inode parentInode;
			/*change parent dir parent inode size*/
			BOFS_LoadInodeByID(&parentInode, parentDir->inode, sb);
			
			parentInode.size += sizeof(struct BOFS_DirEntry);
			BOFS_SyncInode(&parentInode, sb);
		}
		
		/*4.open file*/
		int fd = BOFS_AllocFdGlobal();
		if (fd == -1) {
			printk("alloc fd for failed!\n");

			kfree(dirEntry);
			kfree(inode);
			return -1;
		}
		
		BOFS_GlobalFdTable[fd].parentEntry = parentDir;
		BOFS_GlobalFdTable[fd].dirEntry = dirEntry;
		
		BOFS_GlobalFdTable[fd].inode = inode;
		
		BOFS_GlobalFdTable[fd].pos = 0;
		BOFS_GlobalFdTable[fd].flags |= mode;

		BOFS_GlobalFdTable[fd].superBlock = sb;
		
		//we can't free dirEntry and inode, because we will use it in fd
		
		return TaskInstallFD(fd);
	}else{
		kfree(dirEntry);
		kfree(inode);
		return -1;
	}
}

PUBLIC int BOFS_Lseek(int fd, int offset, unsigned char whence)
{
	if (fd < 0 || fd >= MAX_OPEN_FILES_IN_PROC) {
		printk("bofs lseek: fd error\n");
		return -1;
	}
    unsigned int globalFD = 0;
    /* 局部fd转换成全局fd */
    globalFD = FdLocal2Global(fd);

	struct BOFS_FileDescriptor* fdptr = &BOFS_GlobalFdTable[globalFD];
	
	//printk("seek file %s\n",fdptr->dir->name);
	int newPos = 0;   //new pos must < file size
	int fileSize = (int)fdptr->inode->size;
	//printk("fsize %d\n", fileSize);

	switch (whence) {
		case BOFS_SEEK_SET: 
			newPos = offset; 
			break;
		case BOFS_SEEK_CUR: 
			newPos = (int)fdptr->pos + offset; 
			break;
		case BOFS_SEEK_END: 
			newPos = fileSize + offset;
			break;
		default :
			printk("bofs lseek: unknown whence!\n");
			break;
	}
	//printk("newPos: %d\n", newPos);
	
	if (fdptr->dirEntry->type == BOFS_FILE_TYPE_NORMAL) {	
		if (newPos < 0 || newPos > fileSize) {	 
			return -1;
		}
	}
	fdptr->pos = newPos;
	//printk("seek: %d\n", fdptr->pos);
	
	return fdptr->pos;
}

PRIVATE int BOFS_OpenFile(struct BOFS_DirEntry *parentDir,
	char *name,
	unsigned int mode,
	struct BOFS_SuperBlock *sb)
{
	/*1.alloc memory for dir entry and inode*/
	struct BOFS_DirEntry *dirEntry = (struct BOFS_DirEntry *)kmalloc(\
		sizeof(struct BOFS_DirEntry), GFP_KERNEL);
	if(dirEntry == NULL){
		printk("alloc memory for dirEntry failed!\n");
		return -1;
	}
	
	struct BOFS_Inode *inode = (struct BOFS_Inode *)kmalloc(\
		sizeof(struct BOFS_Inode), GFP_KERNEL);
	if(inode == NULL){
		printk("alloc memory for inode failed!\n");
		kfree(dirEntry);
		return -1;
	}

	//printk("open inode and dir entry ok!\n");
	/*2.load dir entry to parent dir*/
	if(BOFS_LoadDirEntry(parentDir, name, dirEntry, sb)){
		
		/*3.open file*/
		int fd = BOFS_AllocFdGlobal();
		if (fd == -1) {
			printk("alloc fd for failed!\n");

			kfree(dirEntry);
			kfree(inode);
			return -1;
		}
		
		BOFS_GlobalFdTable[fd].parentEntry = parentDir;
		BOFS_GlobalFdTable[fd].dirEntry = dirEntry;
		
		BOFS_LoadInodeByID(inode, dirEntry->inode, sb);
		
		BOFS_GlobalFdTable[fd].inode = inode;
		
		BOFS_GlobalFdTable[fd].pos = 0;
		
		BOFS_GlobalFdTable[fd].flags |= mode;

		BOFS_GlobalFdTable[fd].superBlock = sb;
		
        //printk("load dir entry ok!\n");

		//we can't free dirEntry and inode, because we will use it in fd
		return TaskInstallFD(fd);
		
	}else{
		printk("load dir entry failed!\n");
		kfree(dirEntry);
		kfree(inode);
		return -1;
	}
}

PUBLIC int BOFS_Open(const char *pathname, unsigned int flags, struct BOFS_SuperBlock *sb)
{
    //printk("BOFS_Open start\n");
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	
	struct BOFS_DirSearchRecord record;
	
    /* 超级块指针 */
	int fd = -1;	   //default open failed
	
    /* 获取文件名 */
	char name[BOFS_NAME_LEN];
	memset(name,0,BOFS_NAME_LEN);
	if(!BOFS_PathToName(path, name)){
		//printk("path to name sucess!\n");
	}else{
		printk("path to name %s failed!\n", pathname);
		return -1;
	}
	
    //printk("path %s name %s\n", pathname, name);

	/*
	if file has exist, we will reopen it(reopen = 1).
	if file not exist, we have falgs O_CREAT, we create a file
	*/
	char reopen = 0;

	//printk("master sb %x\n", sb);
	int found = BOFS_SearchDir(path, &record, sb);
	//printk("BOFS_Open end\n");
	
	if(!found){
		//printk("find dir entry! path %s name %s\n", pathname, name);
		
		//BOFS_DumpDirEntry(record.childDir);
		if(record.childDir->type == BOFS_FILE_TYPE_DIRECTORY){	//found a dir
			printk("open: can't open a direcotry with open(), use opendir() to instead!\n");
			BOFS_CloseDirEntry(record.parentDir);
			BOFS_CloseDirEntry(record.childDir);
			
			return -1;
		}
		//printk("continue!\n");
		/* 只有普通文件才能创建 */
		if ((flags & BOFS_O_CREAT) && record.childDir->type == BOFS_FILE_TYPE_NORMAL) {
			//printk("open: %s has already exist!\n", pathname);
			if((flags & BOFS_O_RDONLY) || (flags & BOFS_O_WRONLY) || (flags & BOFS_O_RDWR)){
				reopen = 1;
				//printk("%s %s can be reopen.\n", pathname, name);
			}else{
				
				BOFS_CloseDirEntry(record.parentDir);
				BOFS_CloseDirEntry(record.childDir);
				printk("open: file %s has exist, can't create it!\n", pathname);
				return -1;
			}
		}
	}else{
		//printk("open: not found!\n");
		if (found && !(flags & BOFS_O_CREAT)) {	//not found ,not create
			printk("open: path %s, file isn't exist and without O_CR!\n", pathname);
			BOFS_CloseDirEntry(record.parentDir);
			BOFS_CloseDirEntry(record.childDir);
			
			return -1;
		}
	}
	
	/*make file mode*/
	unsigned int mode = 0;
	/*read and write set*/
	if(flags & BOFS_O_RDONLY){
		mode |= BOFS_IMODE_R;
	}else if(flags & BOFS_O_WRONLY){
		mode |= BOFS_IMODE_W;
	}else if(flags & BOFS_O_RDWR){
		mode |= BOFS_IMODE_R;
		mode |= BOFS_IMODE_W;
	}
	/*execute flags*/
	if(flags & BOFS_O_EXEC){
		mode |= BOFS_IMODE_X;
	}
	
	char append = 0;
	if(flags & BOFS_O_APPEDN){
		append = 1;
	}
	
	//printk("file mode:%x\n", mode);
	if(flags & BOFS_O_CREAT) {	
		/* 没有重打开才进行创建 */
		if(!reopen){
			fd = BOFS_CreateFile(record.parentDir, name, mode, record.superBlock);
			
			if(fd != -1){
				/*create sucess! we can't close record parent dir, we will use in fd_parent*/
				
				/*if need append, we set it here*/
				if(append){
					BOFS_Lseek(fd, 0, BOFS_SEEK_END);
				}
				
				/*close record child dir, we don't use it
				fd_dir was alloced in bofs_create_file
				*/
				//printk("create a file %s sucess!\n", name);

				//printk("create path %s file %s sucess!\n", pathname, name);
				BOFS_CloseDirEntry(record.childDir);
			}else{	
				/*create file failed, we close record dir*/
				printk("create path %s file %s failed!\n", pathname, name);
				BOFS_CloseDirEntry(record.parentDir);
				BOFS_CloseDirEntry(record.childDir);
			}
			return fd;
		}
	}
	
	/* 如果有读写属性才进行打开 */
	if((flags & BOFS_O_RDONLY) || (flags & BOFS_O_WRONLY) || (flags & BOFS_O_RDWR)){
		
		//printk("open: open the file %s\n", name);
		/*open an exsit file with O_RO,O_WO,O_RW*/

        if (record.childDir->type == BOFS_FILE_TYPE_NORMAL || 
            record.childDir->type == BOFS_FILE_TYPE_FIFO ||
            record.childDir->type == BOFS_FILE_TYPE_BLOCK || 
            record.childDir->type == BOFS_FILE_TYPE_CHAR) {
            
            /* 打开文件描述符 */
            fd = BOFS_OpenFile(record.parentDir, name, mode, record.superBlock);
            
            /* 设备文件需要打开设备 */
            if (record.childDir->type == BOFS_FILE_TYPE_BLOCK || 
            record.childDir->type == BOFS_FILE_TYPE_CHAR) {
                /* 打开设备 */
                if (DeviceOpen(BOFS_GlobalFdTable[FdLocal2Global(fd)].inode->blocks[0], 0)) {
                    printk("device open failed!\n");
                }
            }
        }
		if(fd != -1){
			/*open sucess! we can't close record parent dir, we will use it in fd_parent*/
			/*if need append, we set it here*/
			if(append){
				BOFS_Lseek(fd, 0, BOFS_SEEK_END);
			}

			/*close record child dir, we don't use it
			fd_dir was alloced in bofs_create_file
			*/
			//printk("open path %s file %s sucess!\n", pathname, name);
			BOFS_CloseDirEntry(record.childDir);
		}else{
			/*open file failed, we close record dir*/
			//printk("open path %s file %s failed!\n", pathname, name);
			BOFS_CloseDirEntry(record.parentDir);
			BOFS_CloseDirEntry(record.childDir);
		}
	}
	return fd;
}

int BOFS_CloseFile(struct BOFS_FileDescriptor *fdptr)
{
	if(fdptr == NULL) {
		return -1;
	}
	
	/*close parent and child dir*/
	BOFS_CloseDirEntry(fdptr->dirEntry);
	BOFS_CloseDirEntry(fdptr->parentEntry);
	/*close inode*/
	BOFS_CloseInode(fdptr->inode);
	
	return 0;
}

/**
 * BOFS_Fsync - 同步文件数据到磁盘
 * @fd: 文件描述符
 */
PUBLIC int BOFS_Fsync(int fd)
{
	int ret = -1;   // defaut -1,error
	if (fd >= 0 && fd < MAX_OPEN_FILES_IN_PROC) {
        /* 简单起见，直接同步所有文件 */
        BlockSync();

	} else {
		printk("fd:%d failed!\n", fd);
	}
	return ret;
}

/**
 * BOFS_Close - 关闭一个文件
 * @fd: 文件描述符（局部）
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int BOFS_Close(int fd)
{
	int ret = -1;   // defaut -1,error
	if (fd >= 0 && fd < MAX_OPEN_FILES_IN_PROC) {
        
        unsigned int globalFD = FdLocal2Global(fd);

        struct BOFS_FileDescriptor *fdec = &BOFS_GlobalFdTable[globalFD];
        
        if (BOFS_IsPipe(fd)) {
            
            /* 如果引用计数为0，就需要释放管道 */
            if (--fdec->pos == 0) {
                printk("close pipe local %d global %d\n", fd, globalFD);
                kfree(fdec->inode);

                /* 释放全局描述符 */
                BOFS_FreeFdGlobal(globalFD);
            }
            ret = 0;
        } else {
            /* 如果是设备文件，就需要关闭设备 */
            if (fdec->dirEntry->type == BOFS_FILE_TYPE_BLOCK || 
                fdec->dirEntry->type == BOFS_FILE_TYPE_CHAR) {
                printk("close device %x\n", fdec->inode->blocks[0]);
                DeviceClose(fdec->inode->blocks[0]);
            }

            /* 文件关闭之前做一次强制同步，保证写入磁盘的数据能够在磁盘上 */
            BlockSync();

            /* 关闭全局文件描述符表中的文件 */
            ret = BOFS_CloseFile(fdec);
            /* 释放全局描述符 */
            BOFS_FreeFdGlobal(globalFD);

            //printk("close fd:%d success!\n", fd);
        }
        /* 释放局部文件描述符，置-1表示未使用 */
        CurrentTask()->fdTable[fd] = -1; 
        
	}else{
		printk("close fd:%d failed!\n", fd);
	}
	return ret;
}

PUBLIC int BOFS_Remove(const char *pathname, struct BOFS_SuperBlock *sb)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	//printk("The path is %s\n", pathname);
	
	if(path[0] == '/' && path[1] == 0){
		printk("remove: can't delelt / dir!\n");
		return -1;
	}
	
	 /* 获取文件名 */
	char name[BOFS_NAME_LEN];
	memset(name,0,BOFS_NAME_LEN);
	if(!BOFS_PathToName(path, name)){
		//printk("path to name sucess!\n");
	}else{
		printk("path to name failed!\n");
		return -1;
	}

	struct BOFS_DirSearchRecord record;
	
	//1.is file exist?
	int found = BOFS_SearchDir(path, &record, sb);
	if(!found){
		
		if(record.childDir->type == BOFS_FILE_TYPE_DIRECTORY){	//found a dir
			printk("remove: can't delete a direcotry with remove(), use rmdir() to instead!\n");
			BOFS_CloseDirEntry(record.parentDir);
			BOFS_CloseDirEntry(record.childDir);
			
			return -1;
		}
		/* 如果不是目录就可以删除 */
	}else{
		//printk("remove: file %s not found!\n", pathname);
		BOFS_CloseDirEntry(record.parentDir);
		BOFS_CloseDirEntry(record.childDir);
		return -1;
	}
	
	//2.file is in file table?
	uint32 fd = 0;
	while (fd < BOFS_MAX_FD_NR) {
		//if name is same and inode same, that the file we want
		if (BOFS_GlobalFdTable[fd].flags & BOFS_FD_USING){
			//printk("scan fd %d name:%s \n", fd, bofs_fd_table[fd].fd_dir->name);
			
            /* 其实还需要判断是否是同一个设备上的文件，那样就更精确了 */
            if(record.childDir->inode == BOFS_GlobalFdTable[fd].dirEntry->inode &&
                !strcmp(record.childDir->name, BOFS_GlobalFdTable[fd].dirEntry->name) &&
                record.childDir->type == BOFS_GlobalFdTable[fd].dirEntry->type){
				
                //printk("find a open file with fd %d !\n", fd);
				break;
			}
		}
		fd++;
	}
	if (fd < BOFS_MAX_FD_NR) {
		BOFS_CloseDirEntry(record.parentDir);
		BOFS_CloseDirEntry(record.childDir);
		printk("remove: file %s is in using, not allow to delete!\n", pathname);
		return -1;
	}
	//BOFS_DumpDirEntry(record.childDir);

	/* 3.empty file data */
	BOFS_ReleaseDirEntry(record.superBlock, record.childDir);
	
	/*4.sync child dir to parent dir*/
	if(BOFS_SyncDirEntry(record.parentDir, record.childDir, record.superBlock)){
		//printk("remove: delete file %s done.\n",pathname);
	
		BOFS_CloseDirEntry(record.parentDir);
		BOFS_CloseDirEntry(record.childDir);
		return 0;
	}
	BOFS_CloseDirEntry(record.parentDir);
	BOFS_CloseDirEntry(record.childDir);
	printk("remove: delete file %s faild!\n",pathname);
	
	return -1;
}

PRIVATE int BOFS_FileWrite(struct BOFS_FileDescriptor *fdptr,
	void* buf,
	unsigned int count)
{
	
	/*step 1:
	if pos > file size, we should correcte it*/
	if(fdptr->pos > fdptr->inode->size){
		fdptr->pos = fdptr->inode->size;
	}
	
	/*step 2:
	we need to get file's data old blocks, 
	after that we can calculate data new blocks by pos and count.
	*/
	//get old blocks
	//uint32 all_blocks_old = DIV_ROUND_UP(fdptr->inode->size, SECTOR_SIZE);
	//printk("all_blocks_old:%d\n", all_blocks_old);
	//we will beyond how many bytes, we need add how many bytes beyond file size
	/*uint32 will_beyond_bytes;	
	if(fdptr->pos < fdptr->inode->size){//pos is in file size
		if((fdptr->pos + count) < fdptr->inode->size){	
			will_beyond_bytes = 0;
			//printk("below pos start:%d pos end:%d cunt:%d\n",fdptr->pos, fdptr->pos + count, count);
		}else{	//end >= size
			will_beyond_bytes = count - (fdptr->inode->size - fdptr->pos);
			//printk("above pos start:%d pos end:%d cunt:%d\n",fdptr->pos, fdptr->pos + count, count);
		}
	}else{	//pos is equal to size
		will_beyond_bytes = count;
	}*/
	/*
	uint32 will_beyond_sectors = 0;	//default is 0 sectors
	if(will_beyond_bytes > 0){//if have beyond bytes
		will_beyond_sectors = DIV_ROUND_UP(will_beyond_bytes, SECTOR_SIZE);
	}*/
	
	//uint32 all_blocks_new = all_blocks_old + will_beyond_sectors;
	
	//printk("will_beyond_bytes:%x will_beyond_sectors:%d\n",will_beyond_bytes, will_beyond_sectors);

	/*step 3:
	judge whether need we change file's size. if pos+count > size, change size!
	*/
	char changeFileSize = 0;
	if(fdptr->pos + count > fdptr->inode->size){
		changeFileSize = 1;
	}
	
    

	/*step 4:
	write data to file
	*/
	uint8 *src = buf;        // src is buffer we will write to file 
	uint32 bytesWritten = 0;	    // we have written n bytes
	uint32 sizeLeft = count;	    // how many bytes we didn't write
	uint32 sectorLba;	      // we will write data to a sector 
	uint32 sectorOffsetBytes;    // bytes offset in a sector 
	uint32 sectorLeftBytes;   // bytes left in a sector
	uint32 chunkSize;	      // every time will write chunk size to disk
	
	/*we will write data to a sector, but not every time we write is a whole sector.
	maybe it is like this. a sector, we will write to half of it. so we will clear a
	sector buf first, and then write data into sector buf, and write into disk.
	if we don't judge whether it's first time to write sector, we may clean old data
	in the disk. so we judge it use value firstWriteSector.*/
	uint8 firstWriteSector = 1;
	
	/*block id for get file data block.
	we need set it by fd pos, we will start at a pos, not always 0.
	because sometimes, pos is not start from 0, so we should set it use 
	pos/SECTOR_SIZE.
	for example:
		if (0<= pos < 512) : id = 0
		if (512<= pos < 1024) : id = 1
		if (1024<= pos < 1536) : id = 2
		...
	*/
    unsigned int blockSize = fdptr->superBlock->blockSize;

    buf8_t iobuf = kmalloc(blockSize, GFP_KERNEL);
    if (iobuf == NULL)
        return -1;

	uint32 blockID = fdptr->pos/blockSize;    

	struct BOFS_SuperBlock *sb = fdptr->superBlock;

	//printk(">>>start block id:%d\n", blockID);
	while (bytesWritten < count) {
		
		BOFS_GetInodeData(fdptr->inode, blockID, &sectorLba, sb);
		
        //printk("write block %d\n", sectorLba);

        memset(iobuf, 0, blockSize);
		
		//get remainder of pos = pos/512
		sectorOffsetBytes = fdptr->pos % blockSize;	
		sectorLeftBytes = blockSize - sectorOffsetBytes;
		
		chunkSize = sizeLeft < sectorLeftBytes ? sizeLeft : sectorLeftBytes;
		
		//printk("sector:%d off:%d left:%d chunk:%d\n", sectorLba, sectorOffsetBytes, sectorLeftBytes, chunkSize);
		
		if (firstWriteSector) {	//we need to keep first sector not
			
			if(BlockRead(sb->devno, sectorLba, iobuf)) {
				goto ToFailed;
			}
			
			firstWriteSector = 0;
			//printk("first write, need to read old data!\n");
		}
        
		memcpy(iobuf + sectorOffsetBytes, src, chunkSize);
		//sector_write(sectorLba, iobuf, 1);
		if(BlockWrite(sb->devno, sectorLba, iobuf, 0)) {
			goto ToFailed;
		}
		src += chunkSize;   // set src to next pos
		fdptr->pos += chunkSize;   
		if(changeFileSize){
			fdptr->inode->size  = fdptr->pos;    // updata file's size
		}
		bytesWritten += chunkSize;
		sizeLeft -= chunkSize;
		blockID++;
	}
	/*step 5:
	updata inode size
	*/
	BOFS_SyncInode(fdptr->inode, sb);
	
    kfree(iobuf);
	return bytesWritten;

ToFailed:
    kfree(iobuf);
    return -1;
}

PUBLIC int BOFS_Write(int fd, void *buf, unsigned int count)
{
	if (count == 0) {
		//printk("fread: count zero\n");
		return 0;
	}
    int ret = -1;
    unsigned int globalFD = 0;
    
    if (fd < 0 || fd >= MAX_OPEN_FILES_IN_PROC) {
		printk("bofs write: fd error\n");
		return -1;
    }

    if (BOFS_IsPipe(fd)) {
        return BOFS_PipeWrite(fd, buf, count);
    } else {    /* 文件写入 */
        /* 局部fd转换成全局fd */
        globalFD = FdLocal2Global(fd);
        
        struct BOFS_FileDescriptor* wrFile = &BOFS_GlobalFdTable[globalFD];
        
        /*we need compare inode mode with flags*/
        /*flags have read*/
        if(wrFile->flags & BOFS_O_WRONLY || wrFile->flags & BOFS_O_RDWR){
            /*inode mode can write*/
            if(wrFile->inode->mode & BOFS_IMODE_W){
                if (wrFile->dirEntry->type == BOFS_FILE_TYPE_NORMAL) {
                    ret = BOFS_FileWrite(wrFile, buf, count);
                } else if (wrFile->dirEntry->type == BOFS_FILE_TYPE_CHAR) {
                    /* 字符设备文件，读取一个字符并返回 */
                    char *p = (char *)buf;
                    while (count-- > 0) {
                        ret = DevicePutc(wrFile->inode->blocks[0], *p++);
                        if (ret == -1) {
                            break;
                        }
                        ret = 0;
                    }
                } else if (wrFile->dirEntry->type == BOFS_FILE_TYPE_BLOCK) {
                    if (DeviceWrite(wrFile->inode->blocks[0], wrFile->pos, buf, count)) {
                        ret = -1;
                    } else {
                        ret = 0;
                    }
                } else if (wrFile->dirEntry->type == BOFS_FILE_TYPE_FIFO) {
                    //printk("fifo write!\n");
                    BOFS_FifoWrite(wrFile, buf, count);

                    ret = 0;
                }

            }else{
                printk("not allowed to write inode without BOFS_IMODE_W.\n");
            }
        } else {
            printk("bofs fwrite: not allowed to write file without flag BOFS_O_RDWR or BOFS_O_WRONLY\n");
        }
    }
	return ret;
}

PUBLIC int BOFS_Ioctl(int fd, int cmd, int arg)
{
	if (fd < 0 || fd >= MAX_OPEN_FILES_IN_PROC) {
		printk("bofs fwrite: fd error\n");
		return -1;
	}
    int ret = 0;
	unsigned int globalFD = 0;
    /* 局部fd转换成全局fd */
    globalFD = FdLocal2Global(fd);

    struct BOFS_FileDescriptor* file = &BOFS_GlobalFdTable[globalFD];
	
	if (file->dirEntry->type == BOFS_FILE_TYPE_NORMAL) {
		printk("ioctl: normal file not support ioctl!\n");
		ret = -1;
	} else if (file->dirEntry->type == BOFS_FILE_TYPE_BLOCK || 
            file->dirEntry->type == BOFS_FILE_TYPE_CHAR) {
        
        if (DeviceIoctl(file->inode->blocks[0], cmd, arg)) {
            ret = -1;
        }
	}
	return ret;
}

PRIVATE int BOFS_FileRead( struct BOFS_FileDescriptor *fdptr, void* buf, uint32 count)
{
    //printk("file read start!\n");

	/*step 1:
	calculate that we can read how many bytes
	*/
	uint32 size = count, sizeLeft = count;
	//printk("pos:%d count:%d size:%d\n", fdptr->pos, count, fdptr->inode->size);

	//check read bytes
	if ((fdptr->pos + count) > fdptr->inode->size){
		size = fdptr->inode->size - fdptr->pos;
		sizeLeft = size;
		//printk("sizeLeft:%d\n", sizeLeft);
		
		if (sizeLeft == 0) {	   // if read at the end of file, return 0
			return -1;
		}
	}
	
	/*step 2:
	calculate that we read data from which block id(sector)
	*/
	/*block id for get file data block.
	we need set it by fd pos, we will start at a pos, not always 0.
	because sometimes, pos is not start from 0, so we should set it use 
	pos/SECTOR_SIZE.
	for example:
		if (0<= pos < 512) : id = 0
		if (512<= pos < 1024) : id = 1
		if (1024<= pos < 1536) : id = 2
		...
	*/
    unsigned int blockSize = fdptr->superBlock->blockSize;

	uint32 blockID = fdptr->pos/blockSize;    
	//printk(">>>start block id:%d\n", blockID);
	
	/*step 3:
	read data to buf
	*/
	uint8 *dst = buf;        // src is buffer we will write to file 
	uint32 bytesRead = 0;	    // we have written n bytes
	uint32 sectorLba;	      // we will write data to a sector 
	uint32 sectorOffsetBytes;    // bytes offset in a sector 
	uint32 sectorLeftBytes;   // bytes left in a sector
	uint32 chunkSize;	      // every time will write chunk size to disk
	
    buf8_t iobuf = kmalloc(blockSize, GFP_KERNEL);
    if (iobuf == NULL)
        return -1;

	struct BOFS_SuperBlock *sb = fdptr->superBlock;

    //printk("file read start!\n");

	while (bytesRead < size) {
		//printk("get inode data");
		/*BOFS_DumpSuperBlock(sb);
		BOFS_DumpInode(fdptr->inode);*/
		BOFS_GetInodeData(fdptr->inode, blockID, &sectorLba, sb);

		//printk("read block %d\n", sectorLba);
		//get remainder of pos = pos/512
		sectorOffsetBytes = fdptr->pos % blockSize;	
		sectorLeftBytes = blockSize - sectorOffsetBytes;
		
		chunkSize = sizeLeft < sectorLeftBytes ? sizeLeft : sectorLeftBytes;
		
		//printk("sector:%d off:%d left:%d chunk:%d\n", sectorLba, sectorOffsetBytes, sectorLeftBytes, chunkSize);
		
		//clean buf and read data to buf
		//memset(sb->iobuf, 0, blockSize);
		//printk("read done!\n");
		if(BlockRead(sb->devno, sectorLba, iobuf)) {
			goto ToFailed;
		}
		//printk("read done!\n");
		/*copy disk data to out buf*/
		memcpy(dst, iobuf + sectorOffsetBytes, chunkSize);
		
		dst += chunkSize;
		fdptr->pos += chunkSize;
		bytesRead += chunkSize;
		sizeLeft -= chunkSize;
		blockID++;
	}
	
    //printk("read ok!\n");
    kfree(iobuf);
	return bytesRead;

ToFailed:
    kfree(iobuf);
    printk("read failed!\n");
    return -1;
}

PUBLIC int BOFS_Read(int fd, void *buf, unsigned int count)
{
    int ret = -1;
    unsigned int globalFD = 0;
    
    if (fd < 0 || fd >= MAX_OPEN_FILES_IN_PROC) {
		printk("bofs fread: fd error\n");
		return -1;
    } 
    if (count == 0) {
        return -1;
    }

    if (BOFS_IsPipe(fd)) {
        return BOFS_PipeRead(fd, buf, count);
    } else {    /* 文件读取 */
        
        /* 局部fd转换成全局fd */
        globalFD = FdLocal2Global(fd);

        struct BOFS_FileDescriptor* rdFile = &BOFS_GlobalFdTable[globalFD];
        
        /*we need compare inode mode with flags*/
        /*flags have read*/
        if(rdFile->flags & BOFS_O_RDONLY || rdFile->flags & BOFS_O_RDWR){
            /*inode mode can read*/
            if(rdFile->inode->mode & BOFS_IMODE_R){

                /* 普通文件 */
                if (rdFile->dirEntry->type == BOFS_FILE_TYPE_NORMAL) {
                    ret = BOFS_FileRead(rdFile, buf, count);    
                } else if (rdFile->dirEntry->type == BOFS_FILE_TYPE_CHAR) {
                    /* 字符设备文件，读取一个字符并返回 */
                    ret = DeviceGetc(rdFile->inode->blocks[0]);
                    if (ret != 0) {
                        //printk("getc from device:%x\n", ret);
                        memcpy(buf, &ret, count);
                        ret = 0;
                    } else {
                        ret = -1;
                    }
                } else if (rdFile->dirEntry->type == BOFS_FILE_TYPE_BLOCK) {
                    
                    if (DeviceRead(rdFile->inode->blocks[0], rdFile->pos, buf, count) == -1) {
                        //printk("read dev %x at %d for count %d\n", rdFile->inode->blocks[0], rdFile->pos, count);
                        ret = -1;
                    } else {
                        ret = 0;
                    }
                } else if (rdFile->dirEntry->type == BOFS_FILE_TYPE_FIFO) {
                    //printk("fifo read!\n");
                    BOFS_FifoRead(rdFile, buf, count);
                    
                    ret = 0;
                }
            }else{
                printk("not allowed to read inode without BOFS_IMODE_R.\n");
            }
        } else {
            printk("fread: not allowed to read file without flag BOFS_O_RDONLY or BOFS_O_RDWR.\n");
        }
    }

	return ret;
}

PRIVATE int BOFS_IsPathExist(const char* pathname, struct BOFS_SuperBlock *sb)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	
	/*if dir is '/' */
	if(!strcmp(path, "/")){
		return 0;
	}

	int ret = -1;	//default -1
	struct BOFS_DirSearchRecord record;
	memset(&record, 0, sizeof(struct BOFS_DirSearchRecord));   // 记得初始化或清0,否则栈中信息不知道是什么
	int found = BOFS_SearchDir(path, &record, sb);
	if(!found){
		/* 找到就说明存在 */
		BOFS_CloseDirEntry(record.childDir);
		ret = 0;
	}else{
		printk("%s not exist!\n", pathname);
	}
	BOFS_CloseDirEntry(record.parentDir);
	return ret;
}

PRIVATE int BOFS_IsPathModeTrue(const char* pathname, int mode, struct BOFS_SuperBlock *sb)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	
	/*if dir is '/' */
	if(!strcmp(path, "/")){
		return 0;
	}

	int ret = -1;	//default -1
	struct BOFS_DirSearchRecord record;
	memset(&record, 0, sizeof(struct BOFS_DirSearchRecord));   // 记得初始化或清0,否则栈中信息不知道是什么
	int found = BOFS_SearchDir(path, &record, sb);
	if(!found){
		struct BOFS_Inode inode;
		BOFS_LoadInodeByID(&inode, record.childDir->inode, record.superBlock);
		
		if(inode.mode & mode){
			ret = 0;
		}

		BOFS_CloseDirEntry(record.childDir);
		
	}else{
		printk("%s not exist!\n", pathname);
	}
	BOFS_CloseDirEntry(record.parentDir);
	return ret;
}

/**
 * BOFS_Access - 检测文件是否有属性
 * @pathname: 路径名
 * @mode: 检测模式模式
 * @sb: 超级块
 * 
 * 如果文件有该属性就返回0，没有就返回-1
 */
PUBLIC int BOFS_Access(const char *pathname, mode_t mode, struct BOFS_SuperBlock *sb)
{

	int inodeMode = 0;
	if(mode & BOFS_F_OK){
		if(!BOFS_IsPathExist(pathname, sb)){
			return 0;
		}
	}

    switch (mode)
    {
    case BOFS_X_OK:
        inodeMode = BOFS_IMODE_X;
        break;
    case BOFS_R_OK:
        inodeMode = BOFS_IMODE_R;
        break;
    case BOFS_W_OK:
        inodeMode = BOFS_IMODE_W;
        break;
    default:
        break;
    }

	if(!BOFS_IsPathModeTrue(pathname, inodeMode, sb)){
		return 0;
	}
	return -1;
}

PUBLIC int BOFS_GetMode(const char* pathname, struct BOFS_SuperBlock *sb)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
    
	/*if dir is '/' */
	if(!strcmp(path, "/")){
		return 0;
	}
    
	int ret = 0;	//default -1
	struct BOFS_DirSearchRecord record;
	memset(&record, 0, sizeof(struct BOFS_DirSearchRecord));   // 记得初始化或清0,否则栈中信息不知道是什么
	int found = BOFS_SearchDir(path, &record, sb);

	if(!found){
		struct BOFS_Inode inode;
		BOFS_LoadInodeByID(&inode, record.childDir->inode, record.superBlock);
		/*get mode*/
		ret = inode.mode;
		//printk("_get mode %x\n", ret);
		BOFS_CloseDirEntry(record.childDir);
	} else {
		printk("%s not exist!\n", pathname);
	}
	BOFS_CloseDirEntry(record.parentDir);

	return ret;
}

PUBLIC int BOFS_ChangeMode(const char* pathname, mode_t mode, struct BOFS_SuperBlock *sb)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	
	/*if dir is '/' */
	if(!strcmp(path, "/")){
		return 0;
	}

	int ret = -1;	//default -1
	struct BOFS_DirSearchRecord record;
	memset(&record, 0, sizeof(struct BOFS_DirSearchRecord));   // 记得初始化或清0,否则栈中信息不知道是什么
	int found = BOFS_SearchDir(path, &record, sb);

	if(!found){
		struct BOFS_Inode inode;
		BOFS_LoadInodeByID(&inode, record.childDir->inode, record.superBlock);
		/*get mode*/
		inode.mode = mode;
		BOFS_SyncInode(&inode, record.superBlock);
		
		/* 如果已经在fd表中打开，就要更新进去 */
		int fd;
		for(fd = 0; fd < BOFS_MAX_FD_NR; fd++){
			if (BOFS_GlobalFdTable[fd].flags & BOFS_FD_USING) {
				/* 如果修改了一个正在使用中的节点的属性，需要修改 */
				if (BOFS_GlobalFdTable[fd].dirEntry->inode == record.childDir->inode) {
					BOFS_GlobalFdTable[fd].inode->mode = mode;
					
					//printk("set file mode in using\n");
					break;
				}
			}
		}

		BOFS_CloseDirEntry(record.childDir);
		ret = 0;
	} else {
		printk("%s not exist!\n", pathname);
	}
	BOFS_CloseDirEntry(record.parentDir);
	
	return ret;
}

PUBLIC int BOFS_Stat(const char *pathname,
    struct BOFS_Stat *buf,
    struct BOFS_SuperBlock *sb)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);

	struct BOFS_Inode inode;
	
	/* 如果是根目录，就直接返回根目录信息 */
	if(!strcmp(path, "/")){
		BOFS_LoadInodeByID(&inode, sb->rootDir->dirEntry->inode, sb);
		
		buf->size = inode.size;
		buf->mode = inode.mode;
		buf->devno = inode.devno;
        buf->devno2 = 0;
        buf->inode = inode.id;
        
        buf->crttime = inode.crttime;
        buf->mdftime = inode.mdftime;
        
        buf->acstime = inode.acstime;
        buf->blksize = sb->blockSize;
        buf->blocks = DIV_ROUND_UP(inode.size, sb->blockSize);
		return 0;
	}
	
	int ret = -1;	// default -1
	struct BOFS_DirSearchRecord record;
	memset(&record, 0, sizeof(struct BOFS_DirSearchRecord));   // 记得初始化或清0,否则栈中信息不知道是什么
	int found = BOFS_SearchDir(path, &record, sb);

	if (!found) {
		
		BOFS_LoadInodeByID(&inode, record.childDir->inode, record.superBlock);
		//BOFS_DumpInode(&inode);
		buf->size = inode.size;
		buf->mode = inode.mode;
		buf->devno = inode.devno;
        buf->inode = inode.id;
        
        buf->crttime = inode.crttime;
        buf->mdftime = inode.mdftime;
        
        buf->acstime = inode.acstime;
        buf->blksize = sb->blockSize;
        buf->blocks = DIV_ROUND_UP(inode.size, sb->blockSize);
		
		//BOFS_DumpDirEntry(record.childDir);
		//BOFS_DumpInode(&inode);
		
		BOFS_CloseDirEntry(record.childDir);
		ret = 0;
	} else {
		printk("stat: %s not found\n", path);
	}
	BOFS_CloseDirEntry(record.parentDir);
	
	return ret;
}

PUBLIC void BOFS_UpdateInodeOpenCounts(struct Task *task)
{
    int localFd = 0, globalFd = 0;
    while (localFd < MAX_OPEN_FILES_IN_PROC) {
        globalFd = task->fdTable[localFd];
        ASSERT(globalFd < BOFS_MAX_FD_NR);
        
        /* 是已经使用中的文件 */
        if (globalFd != -1) {
            if (BOFS_IsPipe(localFd)) {
                BOFS_GlobalFdTable[globalFd].pos++;
            }
        }
        localFd++;
    }
}

PUBLIC void BOFS_ReleaseTaskFiles(struct Task *task)
{
    int localFd = 0, globalFd = 0;
    while (localFd < MAX_OPEN_FILES_IN_PROC) {
        globalFd = task->fdTable[localFd];
        ASSERT(globalFd < BOFS_MAX_FD_NR);
        /* 是已经使用中的文件 */
        if (globalFd != -1) {
            if (BOFS_IsPipe(localFd)) {
                /* 如果引用计数为0，就需要释放管道 */
                if (--BOFS_GlobalFdTable[globalFd].pos == 0) {
                    printk("close pipe local %d global %d\n", localFd, globalFd);
                
                    kfree(BOFS_GlobalFdTable[globalFd].inode);

                    /* 释放全局描述符 */
                    BOFS_FreeFdGlobal(globalFd);
                }
            } else {
                /* 其它文件，直接关闭 */
                BOFS_Close(localFd);
            }
        }
        localFd++;
    }
}


/**
 * BOFS_InitFile - 初始化文件相关
 */
PUBLIC void BOFS_InitFile()
{
    
}
