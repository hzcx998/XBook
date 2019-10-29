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
#include <driver/ide.h>
#include <fs/partition.h>
#include <fs/bofs/file.h>
#include <fs/bofs/bitmap.h>
#include <book/device.h>
#include <book/blk-buffer.h>

struct BOFS_FileDescriptor BOFS_GlobalFdTable[BOFS_MAX_FD_NR];

void BOFS_InitFdTable()
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

int BOFS_AllocFdGlobal()
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

void BOFS_FreeFdGlobal(int fd)
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

struct BOFS_FileDescriptor *BOFS_GetFileByFD(int fd)
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

int BOFS_CreateFile(struct BOFS_DirEntry *parentDir,
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
		0, sb->deviceID);    

	// 创建目录项，普通文件
	BOFS_CreateDirEntry(dirEntry, inodeID, BOFS_FILE_TYPE_NORMAL, 0, name);
	
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
		
		return fd;
	}else{
		kfree(dirEntry);
		kfree(inode);
		return -1;
	}
}

PUBLIC int BOFS_Lseek(int fd, int offset, unsigned char whence)
{
	if (fd < 0 || fd >= BOFS_MAX_FD_NR) {
		printk("bofs lseek: fd error\n");
		return -1;
	}

	struct BOFS_FileDescriptor* fdptr = &BOFS_GlobalFdTable[fd];
	
	//printk("seek file %s\n",fdptr->dir->name);
	int newPos = 0;   //new pos must < file size
	int fileSize = (int)fdptr->inode->size;
	
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
	} else if (fdptr->dirEntry->type == BOFS_FILE_TYPE_NORMAL) {
		/* 设备扇区数范围之内才可以 */
		if (newPos < 0) {
			return -1;
		}
	}
	fdptr->pos = newPos;
	//printk("seek: %d\n", fdptr->pos);
	
	return fdptr->pos;
}

int BOFS_OpenFile(struct BOFS_DirEntry *parentDir,
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
		
		//we can't free dirEntry and inode, because we will use it in fd
		//bofs_exhibit_fd(fd);
		return fd;
		
	}else{
		printk("load dir entry failed!\n");
		kfree(dirEntry);
		kfree(inode);
		return -1;
	}
	
}

PUBLIC int BOFS_Open(const char *pathname, unsigned int flags)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	
	struct BOFS_DirSearchRecord record;
	
    /* 超级块指针 */
    struct BOFS_SuperBlock *sb = masterSuperBlock;

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
	
	/*
	if file has exist, we will reopen it(reopen = 1).
	if file not exist, we have falgs O_CREAT, we create a file
	*/
	char reopen = 0;

	//printk("master sb %x\n", sb);
	int found = BOFS_SearchDir(path, &record, sb);
	
	if(!found){
		//printk("find dir entry! path %s name %s\n", pathname, name);
		
		//BOFS_DumpDirEntry(record.childDir);
		if(record.childDir->type == BOFS_FILE_TYPE_MOUNT){	//found a dir
			printk("open: can't open a mount direcotry!\n");
			BOFS_CloseDirEntry(record.parentDir);
			BOFS_CloseDirEntry(record.childDir);
			return -1;
		} else if(record.childDir->type == BOFS_FILE_TYPE_DIRECTORY){	//found a dir
			printk("open: can't open a direcotry with open(), use opendir() to instead!\n");
			BOFS_CloseDirEntry(record.parentDir);
			BOFS_CloseDirEntry(record.childDir);
			
			return -1;
		} else if(record.childDir->type == BOFS_FILE_TYPE_BLOCK || record.childDir->type == BOFS_FILE_TYPE_CHAR){
			/* 已经找到设备文件 */
			if (flags & BOFS_O_CREAT) {
				/* 设备文件不能创建 */
				printk("open: can't create device file!\n");
				BOFS_CloseDirEntry(record.parentDir);
				BOFS_CloseDirEntry(record.childDir);
				
				return -1;
			}
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
	
	/* 如果有读写属性才进行打卡 */
	if((flags & BOFS_O_RDONLY) || (flags & BOFS_O_WRONLY) || (flags & BOFS_O_RDWR)){
		
		//printk("open: open the file %s\n", name);
		/*open an exsit file with O_RO,O_WO,O_RW*/
		fd = BOFS_OpenFile(record.parentDir, name, mode, record.superBlock);
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
			printk("open path %s file %s failed!\n", pathname, name);
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

PUBLIC int BOFS_Close(int fd)
{
	int ret = -1;   // defaut -1,error
	if (fd >= 0 && fd < BOFS_MAX_FD_NR) {
		ret = BOFS_CloseFile(&BOFS_GlobalFdTable[fd]);
		
		BOFS_FreeFdGlobal(fd);
		//printk("close fd:%d success!\n", fd);
	}else{
		printk("close fd:%d failed!\n", fd);
	}
	return ret;
}

PUBLIC int BOFS_Unlink(const char *pathname)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	//printk("The path is %s\n", pathname);
	
	if(path[0] == '/' && path[1] == 0){
		printk("unlink: can't delelt / dir!\n");
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
	
    /* 超级块指针 */
    struct BOFS_SuperBlock *sb = masterSuperBlock;

	//1.is file exist?
	int found = BOFS_SearchDir(path, &record, sb);
	if(!found){
		
		if(record.childDir->type == BOFS_FILE_TYPE_DIRECTORY){	//found a dir
			printk("unlink: can't delete a direcotry with unlink(), use rmdir() to instead!\n");
			BOFS_CloseDirEntry(record.parentDir);
			BOFS_CloseDirEntry(record.childDir);
			
			return -1;
		} else if(record.childDir->type == BOFS_FILE_TYPE_MOUNT){	//found a mount dir
			printk("unlink: can't delete a mount with unlink(), use unmount() to instead!\n");
			BOFS_CloseDirEntry(record.parentDir);
			BOFS_CloseDirEntry(record.childDir);
			
			return -1;
		}
		/* 如果不是目录就可以删除 */
	}else{
		printk("unlink: file %s not found!\n", pathname);
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
			if(record.childDir->inode == BOFS_GlobalFdTable[fd].dirEntry->inode){
				//printk("find a open file with fd %d !\n", fd);
				break;
			}
		}
		fd++;
	}
	if (fd < BOFS_MAX_FD_NR) {
		BOFS_CloseDirEntry(record.parentDir);
		BOFS_CloseDirEntry(record.childDir);
		printk("unlink: file %s is in using, not allow to delete!\n", pathname);
		return -1;
	}
	//BOFS_DumpDirEntry(record.childDir);

	/*3.empty file data*/
	BOFS_ReleaseDirEntry(record.superBlock, record.childDir);
	
	/*4.sync child dir to parent dir*/
	if(BOFS_SyncDirEntry(record.parentDir, record.childDir, record.superBlock)){
		//printk("unlink: delete file %s done.\n",pathname);
	
		BOFS_CloseDirEntry(record.parentDir);
		BOFS_CloseDirEntry(record.childDir);
		return 0;
	}
	BOFS_CloseDirEntry(record.parentDir);
	BOFS_CloseDirEntry(record.childDir);
	printk("unlink: delete file %s faild!\n",pathname);
	
	return -1;
}

int BOFS_FileWrite( struct BOFS_FileDescriptor *fdptr,
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
	uint32 blockID = fdptr->pos/SECTOR_SIZE;    
	
	//printk(">>>start block id:%d\n", blockID);
	
	struct BOFS_SuperBlock *sb = fdptr->superBlock;

	while (bytesWritten < count) {
		
		BOFS_GetInodeData(fdptr->inode, blockID, &sectorLba, sb);
		memset(sb->iobuf, 0, SECTOR_SIZE);
		
		//get remainder of pos = pos/512
		sectorOffsetBytes = fdptr->pos % SECTOR_SIZE;	
		sectorLeftBytes = SECTOR_SIZE - sectorOffsetBytes;
		
		chunkSize = sizeLeft < sectorLeftBytes ? sizeLeft : sectorLeftBytes;
		
		//printk("sector:%d off:%d left:%d chunk:%d\n", sectorLba, sectorOffsetBytes, sectorLeftBytes, chunkSize);
		
		if (firstWriteSector) {	//we need to keep first sector not
			
			if(!BlockRead(sb->deviceID, sectorLba, sb->iobuf)) {
				return -1;
			}
			
			firstWriteSector = 0;
			//printk("first write, need to read old data!\n");
		}
		memcpy(sb->iobuf + sectorOffsetBytes, src, chunkSize);
		//sector_write(sectorLba, sb->iobuf, 1);
		if(!BlockWrite(sb->deviceID, sectorLba, sb->iobuf, 0)) {
			return -1;
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
	
	return bytesWritten;
}

PUBLIC int BOFS_Write(int fd, void* buf, unsigned int count)
{
	if (fd < 0 || fd >= BOFS_MAX_FD_NR) {
		printk("bofs fwrite: fd error\n");
		return -1;
	}
	if(count == 0) {
		printk("bofs fwrite: count zero\n");
		return 0;
	}
	
    struct BOFS_FileDescriptor* wrFile = &BOFS_GlobalFdTable[fd];
	
	/*we need compare inode mode with flags*/
	/*flags have write*/
    if(wrFile->flags & BOFS_O_WRONLY || wrFile->flags & BOFS_O_RDWR){
		/*inode mode can write*/
		if(wrFile->inode->mode & BOFS_IMODE_W){
			int bytesWritten  = -1;
			
			//printk("write type %x\n", wrFile->dirEntry->type);
			
			if (wrFile->dirEntry->type == BOFS_FILE_TYPE_NORMAL) {
				bytesWritten = BOFS_FileWrite(wrFile, buf, count);
			} else if (wrFile->dirEntry->type == BOFS_FILE_TYPE_BLOCK) {
				
				unsigned int blocks = DIV_ROUND_UP(count, SECTOR_SIZE);
				//printk("write blocks %d at %d\n", blocks, wrFile->pos);
				/*int i;
				for (i = 0; i < blocks; i++)
					if (BlockWrite(wrFile->inode->otherDeviceID, wrFile->pos + i,
						buf + i * SECTOR_SIZE, 0)) {
					}
				}*/
				/* 写入成功 */
				bytesWritten = blocks * SECTOR_SIZE;
				wrFile->pos++;
			} else if (wrFile->dirEntry->type == BOFS_FILE_TYPE_CHAR) {
				
			}
			
			return bytesWritten;

		}else{
			printk("not allowed to write inode without BOFS_IMODE_W.\n");
		}
	} else {
		printk("bofs fwrite: not allowed to write file without flag BOFS_O_RDWR or BOFS_O_WRONLY\n");
	}
	return -1;
}

PUBLIC int BOFS_Ioctl(int fd, int cmd, int arg)
{
	if (fd < 0 || fd >= BOFS_MAX_FD_NR) {
		printk("bofs fwrite: fd error\n");
		return -1;
	}
	
    struct BOFS_FileDescriptor* file = &BOFS_GlobalFdTable[fd];
	

	if (file->dirEntry->type == BOFS_FILE_TYPE_NORMAL) {
		printk("ioctl: normal file not support ioctl!\n");
		return -1;
	} else if (file->dirEntry->type == BOFS_FILE_TYPE_BLOCK) {
		/* 对块设备进行IOCTL */
		if (DeviceIoctl(file->inode->otherDeviceID, cmd, arg)) {
			return -1;
		}
	} else if (file->dirEntry->type == BOFS_FILE_TYPE_CHAR) {
		
	}
	return 0;
}



int BOFS_FileRead( struct BOFS_FileDescriptor *fdptr, void* buf, uint32 count)
{
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
		
		if (size == 0) {	   // if read at the end of file, return 0
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
	uint32 blockID = fdptr->pos/SECTOR_SIZE;    
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
	
	struct BOFS_SuperBlock *sb = fdptr->superBlock;

	while (bytesRead < size) {
		//printk("get inode data");
		/*BOFS_DumpSuperBlock(sb);
		BOFS_DumpInode(fdptr->inode);*/
		BOFS_GetInodeData(fdptr->inode, blockID, &sectorLba, sb);
		
		//printk("get inode data done");
		//get remainder of pos = pos/512
		sectorOffsetBytes = fdptr->pos % SECTOR_SIZE;	
		sectorLeftBytes = SECTOR_SIZE - sectorOffsetBytes;
		
		chunkSize = sizeLeft < sectorLeftBytes ? sizeLeft : sectorLeftBytes;
		
		//printk("sector:%d off:%d left:%d chunk:%d\n", sectorLba, sectorOffsetBytes, sectorLeftBytes, chunkSize);
		
		//clean buf and read data to buf
		//memset(sb->iobuf, 0, SECTOR_SIZE);
		//printk("read done!\n");
		if(!BlockRead(sb->deviceID, sectorLba, sb->iobuf)) {
			return -1;
		}
		//printk("read done!\n");
		/*copy disk data to out buf*/
		memcpy(dst, sb->iobuf + sectorOffsetBytes, chunkSize);
		
		dst += chunkSize;
		fdptr->pos += chunkSize;
		bytesRead += chunkSize;
		sizeLeft -= chunkSize;
		blockID++;
	}
	
	return bytesRead;
}

PUBLIC int BOFS_Read(int fd, void* buf, unsigned int count)
{
	if (fd < 0 || fd >= BOFS_MAX_FD_NR) {
		printk("bofs fread: fd error\n");
		return -1;
	}
	if (count == 0) {
		printk("fread: count zero\n");
		return 0;
	}
	
	struct BOFS_FileDescriptor* rdFile = &BOFS_GlobalFdTable[fd];
	
	/*we need compare inode mode with flags*/
	/*flags have read*/
    if(rdFile->flags & BOFS_O_RDONLY || rdFile->flags & BOFS_O_RDWR){
		/*inode mode can read*/
		if(rdFile->inode->mode & BOFS_IMODE_R){

			int bytesRead = -1;

			//printk("read type %x\n", rdFile->dirEntry->type);
			
			if (rdFile->dirEntry->type == BOFS_FILE_TYPE_NORMAL) {
				//printk("do file read!\n");
				bytesRead = BOFS_FileRead(rdFile, buf, count);
				//printk("do file read done!\n");

			} else if (rdFile->dirEntry->type == BOFS_FILE_TYPE_BLOCK) {
				//bytesRead = BOFS_FileRead(rdFile, buf, count);
				
				unsigned int blocks = DIV_ROUND_UP(count, SECTOR_SIZE);
				//printk("read blocks %d at %d\n", blocks, rdFile->pos);
				/*if (BlockRead(rdFile->inode->otherDeviceID, rdFile->pos, buf, blocks)) {
					
					bytesRead = blocks * SECTOR_SIZE;
					rdFile->pos++;
				}*/
			} else if (rdFile->dirEntry->type == BOFS_FILE_TYPE_CHAR) {
				//bytesRead = BOFS_FileRead(rdFile, buf, count);
			}
			
			return bytesRead;
		}else{
			printk("not allowed to read inode without BOFS_IMODE_R.\n");
		}
	} else {
		printk("fread: not allowed to read file without flag BOFS_O_RDONLY or BOFS_O_RDWR.\n");
	}
	return -1;
}

int BOFS_IsPathExist(const char* pathname)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	
	/*if dir is '/' */
	if(!strcmp(path, "/")){
		return 0;
	}

	/* 超级块指针 */
    struct BOFS_SuperBlock *sb = masterSuperBlock;

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

int BOFS_IsPathModeRight(const char* pathname, int mode)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	
	/*if dir is '/' */
	if(!strcmp(path, "/")){
		return 0;
	}

	/* 超级块指针 */
    struct BOFS_SuperBlock *sb = masterSuperBlock;

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

PUBLIC int BOFS_Access(const char *pathname, int mode)
{
	int inodeMode = 0;
	if(mode & BOFS_F_OK){
		if(!BOFS_IsPathExist(pathname)){
			return 0;
		}
	}

	if (mode & BOFS_X_OK) {
		inodeMode |= BOFS_IMODE_X;
	}
	if (mode & BOFS_R_OK) {
		inodeMode |= BOFS_IMODE_R;
	}
	if (mode & BOFS_W_OK) {
		inodeMode |= BOFS_IMODE_W;
	}

	if(!BOFS_IsPathModeRight(pathname, inodeMode)){
		return 0;
	}
	return -1;
}

PUBLIC int BOFS_GetMode(const char* pathname)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	
	/*if dir is '/' */
	if(!strcmp(path, "/")){
		return 0;
	}

	/* 超级块指针 */
    struct BOFS_SuperBlock *sb = masterSuperBlock;

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

PUBLIC int BOFS_SetMode(const char* pathname, int mode)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	
	/*if dir is '/' */
	if(!strcmp(path, "/")){
		return 0;
	}

	/* 超级块指针 */
    struct BOFS_SuperBlock *sb = masterSuperBlock;

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
		
		/* 如果以及载fd表中打开，就要更新进去 */
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

PUBLIC int BOFS_Stat(const char *pathname, struct BOFS_Stat *buf)
{
	char *path = (char *)pathname;
	//printk("The path is %s\n", pathname);
	
	/* 超级块指针 */
    struct BOFS_SuperBlock *sb = masterSuperBlock;

	struct BOFS_Inode inode;
	
	/* 如果是根目录，就直接返回根目录信息 */
	if(!strcmp(path, "/")){
		BOFS_LoadInodeByID(&inode, sb->rootDir->dirEntry->inode, sb);
		
		buf->type = sb->rootDir->dirEntry->type;
		buf->size = inode.size;
		buf->mode = inode.mode;
		buf->device = inode.deviceID;
		
		return 0;
	}
	
	int ret = -1;	// default -1
	struct BOFS_DirSearchRecord record;
	memset(&record, 0, sizeof(struct BOFS_DirSearchRecord));   // 记得初始化或清0,否则栈中信息不知道是什么
	int found = BOFS_SearchDir(path, &record, sb);

	if (!found) {
		
		BOFS_LoadInodeByID(&inode, record.childDir->inode, record.superBlock);
		
		buf->type = record.childDir->type;
		buf->size = inode.size;
		buf->mode = inode.mode;
		buf->device = inode.deviceID;
		
		BOFS_DumpDirEntry(record.childDir);
		BOFS_DumpInode(&inode);
		
		BOFS_CloseDirEntry(record.childDir);
		ret = 0;
	} else {
		printk("stat: %s not found\n", path);
	}
	BOFS_CloseDirEntry(record.parentDir);
	
	return ret;
}
