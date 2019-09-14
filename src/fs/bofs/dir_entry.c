/*
 * file:		fs/bofs/dir_entry.c
 * auther:		Jason Hu
 * time:		2019/9/6
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/deviceio.h>
#include <share/string.h>
#include <driver/ide.h>
#include <fs/bofs/dir_entry.h>
#include <fs/bofs/bitmap.h>
#include <share/math.h>

PUBLIC void BOFS_CreateDirEntry(struct BOFS_DirEntry *dirEntry,
    unsigned int inode,
    unsigned short type,
	unsigned int mount,
    char *name)
{
	memset(dirEntry, 0, sizeof(struct BOFS_DirEntry));
	dirEntry->inode = inode;
	dirEntry->type = type;
	dirEntry->mount = mount;
	strcpy(dirEntry->name, name);
}

PUBLIC void BOFS_DumpDirEntry(struct BOFS_DirEntry *dirEntry)
{
    printk(PART_TIP "---- Dir Entry ----\n");
    printk(PART_TIP "inode:%d type:%x name:%s\n",
		dirEntry->inode, dirEntry->type, dirEntry->name);
}

PUBLIC int BOFS_CopyDirEntry(struct BOFS_DirEntry *dst,
    struct BOFS_DirEntry *src, char copyInode,
    struct BOFS_SuperBlock *sb)
{
	/*copy dir entry*/
	memset(dst, 0, sizeof(struct BOFS_DirEntry));
	memcpy(dst, src,sizeof(struct BOFS_DirEntry));
	
	/*if we copy dir for a new file, we nee copy inode.
	if we just copy dir for dir info, we don't need inode*/
	if(copyInode){

		struct BOFS_Inode inodeA, inodeB;
		/*load inode*/
		BOFS_LoadInodeByID(&inodeB, src->inode, sb);
		
		BOFS_CopyInode(&inodeA, &inodeB);
		
		/*alloc inode for new one*/
		inodeA.id = BOFS_AllocBitmap(sb, BOFS_BMT_INODE, 1);
        if (inodeA.id == -1)
            return -1;

		dst->inode = inodeA.id;
		BOFS_SyncBitmap(sb, BOFS_BMT_INODE, dst->inode);
		
		/*sync new inode*/
		BOFS_SyncInode(&inodeA, sb);
	}
    return 0;
}

PUBLIC bool BOFS_SearchDirEntry(struct BOFS_SuperBlock *sb,
	struct BOFS_DirEntry *parentDir,
	struct BOFS_DirEntry *childDir,
	char *name)
{
	
	/*1.read parent data*/
	struct BOFS_Inode parentInode;
	BOFS_LoadInodeByID(&parentInode, parentDir->inode, sb);
	//Spin("test");
	//BOFS_DumpInode(&parentInode);

	/*we need read data in parent inode
	we read a sector once.if not finish, read again.
	*/
	uint32 lba;
	uint32 blockID = 0;
	struct BOFS_DirEntry *dirEntry = (struct BOFS_DirEntry *)sb->iobuf;
	int i;
	
	uint32 blocks = DIV_ROUND_UP(parentInode.size, SECTOR_SIZE);
	/*we check for blocks, we need search in old dir entry*/
	while (blockID < blocks) {
		BOFS_GetInodeData(&parentInode, blockID, &lba, sb);
		//printk("inode data: id:%d lba:%d\n", blockID, lba);
		
		memset(sb->iobuf, 0, SECTOR_SIZE);
		//sector_read(lba, sb->iobuf, 1);
		if (DeviceRead(sb->deviceID, lba, sb->iobuf, 1)) {
			printk(PART_ERROR "device %d read failed!\n", sb->deviceID);
			return -1;
		}
		
		/*scan a sector*/
		for(i = 0; i < BOFS_DIR_NR_IN_SECTOR; i++){
			/*this dir has name*/
			if(dirEntry[i].name[0] != '\0'){
				//printk("dir entry name ->%s , search for dir entry ->%s\n", dirEntry[i].name, name);
				if(!strcmp(dirEntry[i].name, name) && dirEntry[i].type != BOFS_FILE_TYPE_INVALID){
					/*same dir entry*/
					memcpy(childDir, &dirEntry[i], sizeof(struct BOFS_DirEntry));
					//printk("search success!\n");
					return true;
				}
			}else{
				//printk("search failed!\n");
				return false;
			}
		}
		blockID++;
	}
	
	return false;
}

/*
sync a child dir entry to parent dir entry
return 0 is failed, 
return 1 is success with get a not invalid for dir entry, 
return 2 is success with get a invalid for dir entry.
*/
int BOFS_SyncDirEntry(struct BOFS_DirEntry *parentDir,
	struct BOFS_DirEntry *childDir,
    struct BOFS_SuperBlock *sb)
{
	/*1.read parent data*/
	struct BOFS_Inode parentInode;
	
    unsigned char *iobuf = kmalloc(SECTOR_SIZE, GFP_KERNEL);
    if (iobuf == NULL)
        return -1;
    
	BOFS_LoadInodeByID(&parentInode, parentDir->inode, sb);
	
	//BOFS_DumpInode(&parentInode);

	/*we need read data in parent inode
	we read a sector once.if not finish, read again.
	*/
	uint32 lba;
	uint32 blockID = 0;
	struct BOFS_DirEntry *dirEntry = (struct BOFS_DirEntry *)sb->iobuf;
	int i;
	
	//printk("sync child name %s\n", childDir->name);

	uint32 blocks = DIV_ROUND_UP(parentInode.size, SECTOR_SIZE);
	/*we check for blocks + 1, we need a new palce to store the dir entry*/
	while(blockID <= blocks){	
		BOFS_GetInodeData(&parentInode, blockID, &lba, sb);
		//printk("inode data: id:%d lba:%d\n", blockID, lba);
		memset(sb->iobuf, 0, SECTOR_SIZE);
		
		//sector_read(lba, sb->iobuf, 1);
		if (DeviceRead(sb->deviceID, lba, sb->iobuf, 1)) {
			printk(PART_ERROR "device %d read failed!\n", sb->deviceID);
			return 0;
		}
		/*scan a sector*/
		for(i = 0; i < BOFS_DIR_NR_IN_SECTOR; i++){
			if(!strcmp(dirEntry[i].name, childDir->name) &&\
				dirEntry[i].type != BOFS_FILE_TYPE_INVALID){
				/*same dir entry
				
				change a file info
				
				*/
				memcpy(&dirEntry[i], childDir, sizeof(struct BOFS_DirEntry));
				//sector_write(lba, sb->iobuf, 1);
				if (DeviceWrite(sb->deviceID, lba, sb->iobuf, 1)) {
					printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
				}
				/*printk("BOFS_SyncDirEntry: same dir entry, just change info\n");
				printk("scan: id:%d lba:%d\n", blockID, lba);
				*/
				return 1;
				
			}else if(!strcmp(dirEntry[i].name, childDir->name) &&\
				dirEntry[i].type == BOFS_FILE_TYPE_INVALID){
				/*same dir entry
				
				new dir entry pos
				
				*/
				memcpy(&dirEntry[i], childDir, sizeof(struct BOFS_DirEntry));
				//sector_write(lba, sb->iobuf, 1);
				if (DeviceWrite(sb->deviceID, lba, sb->iobuf, 1)) {
					printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
				}
				/*printk("BOFS_SyncDirEntry: same dir entry, but is invalid\n");
				printk("scan: id:%d lba:%d\n", blockID, lba);
				*/
				/*we will use a dir entry which is invaild!*/
				return 2;
			}else if(dirEntry[i].name[0] != '\0' &&\
				dirEntry[i].inode == childDir->inode &&\
				childDir->type != BOFS_FILE_TYPE_INVALID){
				/*same dir entry but name different and not invaild
				rename a file
				*/
				memcpy(&dirEntry[i], childDir, sizeof(struct BOFS_DirEntry));
				//sector_write(lba, sb->iobuf, 1);
				if (DeviceWrite(sb->deviceID, lba, sb->iobuf, 1)) {
					printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
				}
				/*printk("same dir entry but name different\n");
				//printk(">>disk inode:%d child dir inode:%d\n", dirEntry[i].inode, childDir->inode);
				printk("scan: id:%d lba:%d\n", blockID, lba);
				*/
				return 1;
			}else if(dirEntry[i].name[0] == '\0'){
				/*empty dir entry
				new a file
				*/
				memcpy(&dirEntry[i], childDir, sizeof(struct BOFS_DirEntry));
				//sector_write(lba, sb->iobuf, 1);
				if (DeviceWrite(sb->deviceID, lba, sb->iobuf, 1)) {
					printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
				}
				/*printk("empty dir entry\n");
				
				printk("scan: id:%d lba:%d at %x\n", blockID, lba, lba*SECTOR_SIZE);
				*/
				return 1;
			}
			
		}
		blockID++;
		//Spin("test");
	}
	return 0;
}

void BOFS_ReleaseDirEntry(struct BOFS_SuperBlock *sb,
	struct BOFS_DirEntry *childDir)
{
	/*1.read parent data*/
	struct BOFS_Inode childInode;
	BOFS_LoadInodeByID(&childInode, childDir->inode, sb);
	
	BOFS_DumpInode(&childInode);

	/*release child data*/
	BOFS_ReleaseInodeData(sb, &childInode);
	
	/*free inode bitmap*/
	BOFS_FreeBitmap(sb, BOFS_BMT_INODE, childDir->inode);
	/*free inode bitmap*/
	BOFS_SyncBitmap(sb, BOFS_BMT_INODE, childDir->inode);
	
	/*we can empty inode for debug
	if we not empty inode , we can recover dir inode info
	*/
	//bofs_empty_inode(&childInode);
	
	/*
	we do not set inode to 0, so that we can recover 
	dir inode, but we can't recover data.
	*/
	/*child_dir->inode = 0;*/

	/*change file type to INVALID, so that we can search it but not use it*/
	childDir->type = BOFS_FILE_TYPE_INVALID;
}

/**
 * BOFS_CloseDirEntry - 关闭打开的目录项
 * @dirEntry: 目录项
 */
void BOFS_CloseDirEntry(struct BOFS_DirEntry *dirEntry)
{
	if(dirEntry == NULL){
		return;
	}
	kfree(dirEntry);
}
