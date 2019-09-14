/*
 * file:		fs/bofs/dir.c
 * auther:		Jason Hu
 * time:		2019/9/7
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/slab.h>
#include <book/debug.h>
#include <share/string.h>
#include <book/deviceio.h>
#include <share/string.h>
#include <driver/ide.h>
#include <fs/bofs/dir.h>
#include <fs/bofs/dir_entry.h>
#include <fs/bofs/bitmap.h>

PUBLIC int BOFS_OpenRootDir(struct BOFS_SuperBlock *sb)
{	
    /* 为根目录和根目录项分配内存 */
	sb->rootDir = (struct BOFS_Dir *)kmalloc(sizeof(struct BOFS_Dir),
        GFP_KERNEL);
	
    if (sb->rootDir == NULL) {
        return -1;
    }
    
    struct BOFS_DirEntry *dirEntry = (unsigned char *)kmalloc(SECTOR_SIZE, GFP_KERNEL);
	
    if (dirEntry == NULL) {
        kfree(sb->rootDir);
        return -1;
    }

    /* 获取根目录项信息 */
	
	memset(dirEntry, 0, SECTOR_SIZE);
	/* 读取根目录的数据 */
	if (DeviceRead(sb->deviceID, sb->dataStartLba, dirEntry, 1)) {
        
        kfree(sb->rootDir);
        kfree(dirEntry);
        return -1;
    }
    
    sb->rootDir->dirEntry = dirEntry;

    BOFS_DumpDirEntry(sb->rootDir->dirEntry);
	
    /* 获取根目录其它信息 */
    sb->rootDir->sectorLba = sb->dataStartLba;
    sb->rootDir->dirPos = 0;
    sb->rootDir->bufStatus = 0;
    memset(sb->rootDir->dirBuf, 0, BOFS_BLOCK_SIZE);

    return 0;
}

PRIVATE int BOFS_CreateNewDirEntry(struct BOFS_SuperBlock *sb,
	struct BOFS_DirEntry *parentDir,
	struct BOFS_DirEntry *childDir)
{
	/**
	 * 1.分配节点ID
	 * 2.创建节点
	 * 3.创建目录项
	 * 4.创建索引目录（.和..）
	 */
	struct BOFS_Inode inode;
	
	inodeid_t inodeID = BOFS_AllocBitmap(, BOFS_BMT_INODE, 1); 
	if (inodeID == -1) {
		printk(PART_ERROR "BOFS alloc inode bitmap failed!\n");
		return -1;
	}
	/* 把节点id同步到磁盘 */
	BOFS_SyncBitmap(BOFS_BMT_INODE, inodeID);

	BOFS_CreateInode(&inode, inodeID, (BOFS_IMODE_R|BOFS_IMODE_W),
		0, sb->deviceID);    

	// 创建目录项
	BOFS_CreateDirEntry(childDir, inodeID, name, BOFS_FILE_TYPE_DIRECTORY);
	
	//bofs_load_inode_by_id(&inode, childDir.inode);
	
	inode.size = sizeof(struct BOFS_DirEntry)*2;

	memset(iobuf, 0, SECTOR_SIZE);
	BOFS_SyncInode(&inode, sb);

	//we need create . and .. under child 
	struct BOFS_DirEntry dir0, dir1;
	
	// . point to childDir .. point to parentDir
	//copy data
	BOFS_CopyDirEntry(&dir0, childDir, 0);

	//reset name
	memset(dir0.name, 0,BOFS_NAME_LEN);
	//set name
	strcpy(dir0.name, ".");
	
	BOFS_CopyDirEntry(&dir1, parentDir, 0);
	memset(dir1.name,0,BOFS_NAME_LEN);
	strcpy(dir1.name, "..");
	
	/*sync . and .. to child dir*/
	//printk("sync . dir >>>");
	BOFS_SyncDirEntry(childDir, &dir0, sb);
	//printk("sync .. dir>>>");
	BOFS_SyncDirEntry(childDir, &dir1, sb);
	//printk("sync child dir>>>");
	
	int sync_ret = BOFS_SyncDirEntry(parentDir, childDir);
	
	if(sync_ret > 0){	//successed
		if(sync_ret != 2){
			/*change parent dir inode size*/
			BOFS_LoadInodeByID(&inode, parentDir->inode);
			
			inode.size += sizeof(struct BOFS_DirEntry);
			BOFS_SyncInode(&inode, sb);
			
		}
		return 0;
	}
}


/**
 * BOFS_MakeDir - 创建一个目录
 * @pathname: 路径名
 * 
 * 创建目录的时候，应该申请一个信号量，来正常竞争
 */
PUBLIC int BOFS_MakeDir(const char *pathname)
{
	/* judge witch drive we will create dir on */
	int i, j;
	
	char *path = (char *)pathname;

	printk("The path is %s\n", path);
	
	char *p = (char *)path;
	char depth = 0;
	char name[BOFS_NAME_LEN];
	
	struct BOFS_DirEntry parentDir, childDir;
	
    /* 超级块指针 */
    currentSuperBlock = masterSuperBlock;

    struct BOFS_SuperBlock *sb = currentSuperBlock;



    /* 第一个必须是根目录符号 */
	if(*p != '/'){
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

    unsigned char *iobuf = kmalloc(SECTOR_SIZE, GFP_KERNEL);
    if (iobuf == NULL)
        return -1;
        
    
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
			if(BOFS_SearchDirEntry(sb->rootDir->dirEntry, &childDir, name)){	
				/* 如果搜索到，说明目录已经存在，那么就保存目录项，然后在子目录项中去搜索
                子目录项变成父目录，以便进行下一次搜索。*/
                memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir
				printk("BOFS_SearchDirEntry:depth:%d path:%s name:%s has exsit! find\n",depth, pathname, name);
			}else{
                /* 在根目录中没有找到子目录，就可以创建这个目录 */

				printk("BOFS_MakeDir:depth:%d path:%s name:%s not exsit! not find\n",depth, pathname, name);
				
				/* 检测是否是最后一个目录不存在，如果是，就创建这个目录 */
				if(i == depth - 1){
					printk("BOFS_MakeDir: create path:%s name:%s\n", pathname, name);
					
					if (BOFS_CreateNewDirEntry(sb, sb->rootDir->dirEntry, &childDir)) {
						return -1;
					}

                    /**
                     * 1.分配节点ID
                     * 2.创建节点
                     * 3.创建目录项
                     * 4.创建索引目录（.和..）
                     */
                    struct BOFS_Inode inode;
					
                    inodeid_t inodeID = BOFS_AllocBitmap(, BOFS_BMT_INODE, 1); 
                    if (inodeID == -1) {
                        printk(PART_ERROR "BOFS alloc inode bitmap failed!\n");
                        return -1;
                    }
                    /* 把节点id同步到磁盘 */
                    BOFS_SyncBitmap(BOFS_BMT_INODE, inodeID);

                    BOFS_CreateInode(&inode, inodeID, (BOFS_IMODE_R|BOFS_IMODE_W),
                        0, sb->deviceID);    

                    // 创建目录项
					BOFS_CreateDirEntry(&childDir, inodeID, name, BOFS_FILE_TYPE_DIRECTORY);
					
					//bofs_load_inode_by_id(&inode, childDir.inode);
					
					inode.size = sizeof(struct BOFS_DirEntry)*2;

                    memset(iobuf, 0, SECTOR_SIZE);
					BOFS_SyncInode(&inode, sb);

					//we need create . and .. under child 
					struct BOFS_DirEntry dir0, dir1;
					
					// . point to childDir .. point to parentDir
					//copy data
					BOFS_CopyDirEntry(&dir0, &childDir, 0);

					//reset name
					memset(dir0.name, 0,BOFS_NAME_LEN);
					//set name
					strcpy(dir0.name, ".");
					
					BOFS_CopyDirEntry(&dir1, sb->root_dir->dirEntry, 0);
					memset(dir1.name,0,BOFS_NAME_LEN);
					strcpy(dir1.name, "..");
					
					/*sync . and .. to child dir*/
					//printk("sync . dir >>>");
					BOFS_SyncDirEntry(&childDir, &dir0, sb);
					//printk("sync .. dir>>>");
					BOFS_SyncDirEntry(&childDir, &dir1, sb);
					//printk("sync child dir>>>");
					
					int sync_ret = BOFS_SyncDirEntry(sb->rootDir->dirEntry, &childDir);
					
					if(sync_ret > 0){	//successed
						if(sync_ret != 2){
							/*change parent dir inode size*/
							BOFS_LoadInodeByID(&inode, sb->rootDir->dirEntry->inode);
							
							inode.size += sizeof(struct BOFS_DirEntry);
							BOFS_SyncInode(&inode, sb);
							
						}
						return 0;
					}
				}else{
					printk("mkdir:can create path:%s name:%s\n", pathname, name);
					
					return -1;
				}
			}
		}else{	//if not under the root dir 
            /* 如果是/mnt/目录，那么就根据/mnt/fs-sb来设置不同的超级块 */
            
			//parentDir we have gotten under root dir
			if(BOFS_SearchDirEntry(&parentDir, &childDir, name)){	//find
				memcpy(&parentDir, &childDir, sizeof(struct BOFS_DirEntry));	//childDir become parentDir
				//printk("bofs_create_dir:depth:%d path:%s name:%s has exsit!\n",depth, pathname, name);
			}else{
				//printk("bofs_create_dir:depth:%d path:%s name:%s not exsit!\n",depth, pathname, name);
				/*check whether is last dir name*/
				if(i == depth - 1){	//create it
					
					if (BOFS_CreateNewDirEntry(sb, &parentDir, &childDir)) {
						return -1;
					}
					
                    /**
                     * 1.分配节点ID
                     * 2.创建节点
                     * 3.创建目录项
                     * 
                     */
                    struct BOFS_Inode inode;
					
                    inodeid_t inodeID = BOFS_AllocBitmap(, BOFS_BMT_INODE, 1); 
                    if (inodeID == -1) {
                        printk(PART_ERROR "BOFS alloc inode bitmap failed!\n");
                        return -1;
                    }

                    /* 把节点id同步到磁盘 */
                    BOFS_SyncBitmap(BOFS_BMT_INODE, inodeID);

                    BOFS_CreateInode(&inode, inodeID, (BOFS_IMODE_R|BOFS_IMODE_W),
                        0, sb->deviceID);    

                    // 创建目录项
					BOFS_CreateDirEntry(&childDir, inodeID, name, BOFS_FILE_TYPE_DIRECTORY);
					
					//bofs_load_inode_by_id(&inode, childDir.inode);
					
					inode.size = sizeof(struct BOFS_DirEntry)*2;

                    memset(iobuf, 0, SECTOR_SIZE);
					BOFS_SyncInode(&inode, sb);

					//we need create . and .. under child 
					struct BOFS_DirEntry dir0, dir1;
					
					// . point to childDir .. point to parentDir
					//copy data
					BOFS_CopyDirEntry(&dir0, &childDir, 0);

					//reset name
					memset(dir0.name, 0,BOFS_NAME_LEN);
					//set name
					strcpy(dir0.name, ".");
					
					BOFS_CopyDirEntry(&dir1, &parentDir, 0);
					memset(dir1.name,0,BOFS_NAME_LEN);
					strcpy(dir1.name, "..");
					
					/*sync . and .. to child dir*/
					//printk("sync . dir >>>");
					BOFS_SyncDirEntry(&childDir, &dir0, sb);
					//printk("sync .. dir>>>");
					BOFS_SyncDirEntry(&childDir, &dir1, sb);
					//printk("sync child dir>>>");
					
					int sync_ret = BOFS_SyncDirEntry(&parentDir, &childDir);
					
					if(sync_ret > 0){	//successed
						if(sync_ret != 2){
							/*change parent dir inode size*/
							BOFS_LoadInodeByID(&inode, parentDir.inode);
							
							inode.size += sizeof(struct BOFS_DirEntry);
							BOFS_SyncInode(&inode, sb);
							
						}
						return 0;
					}
				}else{
					printk("bofs_create_dir:can create path:%s name:%s\n", pathname, name);
					return -1;
				}
			}
		}
	}
	/*if dir has exist, it will arrive here.*/
	return 0;
}
