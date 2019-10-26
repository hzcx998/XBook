#ifndef _BOFS_SUPER_BLOCK_H
#define _BOFS_SUPER_BLOCK_H

#include <share/stdint.h>
#include <fs/bofs/dir.h>
#include <book/bitmap.h>
#include <fs/bofs/inode.h>

/* 超级块魔数 */
#define BOFS_SUPER_BLOCK_MAGIC 0x19980325

/* 超级块的I/Obuffer数量 */
#define BOFS_SUPER_BLOCK_IOBUF_NR 3

struct BOFS_SuperBlock
{
	/*disk info*/
	uint32 magic;
	uint32 deviceID;		/*this fs install on which dev*/
	uint32 totalSectors;	/*will use how many sectors*/
	
	uint32 superBlockLba;
	
	uint32 sectorBitmapLba;	/*sector bitmap start at lba*/
	uint32 sectorBitmapSectors;	/*sector bitmap use sectors*/
	
	/*fs info*/
	uint32 inodeBitmapLba;	/*inode bitmap start at lba*/
	uint32 inodeBitmapSectors;	/*inode bitmap use sectors*/
	
	uint32 inodeTableLba;	/*inode table start at lba*/
	uint32 inodeTableSectors;	/*inode table use sectors*/
	
	uint32 dataStartLba;
	/*inode info*/
	uint32 rootInodeId;	/*root inode start at lba*/
	uint32 inodeNrInSector;	/*how many inode a sector can store*/
	
	unsigned int maxInodes;	/* 最多有多少个节点 */

	/*use in ram*/
	struct Bitmap sectorBitmap;	/*sector manager bitmap*/
	struct Bitmap inodeBitmap;		/*inode manager bitmap*/
	struct BOFS_Dir *rootDir;
	
	/* IO缓冲区, 默认是3个数据缓冲区 */
	unsigned char *databuf[BOFS_SUPER_BLOCK_IOBUF_NR];
	
	/* IO缓冲区, 默认是1个I/O缓冲区 */
	unsigned char *iobuf;
	
}__attribute__ ((packed));;

EXTERN struct BOFS_SuperBlock *masterSuperBlock;

/* 当前的超级块 */
EXTERN struct BOFS_SuperBlock *currentSuperBlock;

int BOFS_InitSuperBlockTable();
struct BOFS_SuperBlock *BOFS_AllocSuperBlock();
void BOFS_FreeSuperBlock(struct BOFS_SuperBlock *sb);
void BOFS_DumpSuperBlock(struct BOFS_SuperBlock *sb);
int BOFS_SuperBlockLoadBuffer(struct BOFS_SuperBlock *sb);

/* dir */
struct BOFS_DirSearchRecord
{
   struct BOFS_DirEntry *parentDir;
   struct BOFS_DirEntry *childDir;
   struct BOFS_SuperBlock *superBlock;	/* 搜索到的子目录所在的超级块 */
};

int BOFS_OpenRootDir(struct BOFS_SuperBlock *sb);

int BOFS_SearchDir(char* pathname,
	struct BOFS_DirSearchRecord *record,
	struct BOFS_SuperBlock *sb);

/* dir entry */
int BOFS_CopyDirEntry(struct BOFS_DirEntry *dst,
    struct BOFS_DirEntry *src, char copyInode,
    struct BOFS_SuperBlock *sb);
	
bool BOFS_SearchDirEntry(struct BOFS_SuperBlock *sb,
	struct BOFS_DirEntry *parentDir,
	struct BOFS_DirEntry *childDir,
	char *name);

int BOFS_SyncDirEntry(struct BOFS_DirEntry *parentDir,
    struct BOFS_DirEntry *childDir,
    struct BOFS_SuperBlock *sb);

void BOFS_ReleaseDirEntry(struct BOFS_SuperBlock *sb,
	struct BOFS_DirEntry *childDir);

bool BOFS_LoadDirEntry(struct BOFS_DirEntry *parentDir, 
	char *name,
	struct BOFS_DirEntry *childDir,
	struct BOFS_SuperBlock *sb);

/* inode */
int BOFS_GetInodeData(struct BOFS_Inode *inode,
 	uint32 blockID,
	uint32 *data,
	struct BOFS_SuperBlock *sb);

int BOFS_SyncInode(struct BOFS_Inode *inode, 
    struct BOFS_SuperBlock *sb);


int BOFS_LoadInodeByID(struct BOFS_Inode *inode, 
    unsigned int id, 
    struct BOFS_SuperBlock *sb);

void BOFS_ReleaseInodeData(struct BOFS_SuperBlock *sb,
	struct BOFS_Inode *inode);

int BOFS_EmptyInode(struct BOFS_Inode *inode,
	struct BOFS_SuperBlock *sb);


#endif

