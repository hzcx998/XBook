/*
 * file:		include/fs/bofs/inode.h
 * auther:		Jason Hu
 * time:		2019/9/5
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOFS_INODE_H
#define _BOFS_INODE_H


#include <share/stdint.h>
#include <share/types.h>

/*default fs inode nr*/
#define DEFAULT_MAX_INODE_NR 9182

/*book os file system inode*/
#define BOFS_BLOCK_NR 3
#define BOFS_Inode_RESERVED 5

#define BOFS_SECTOR_BLOCK_NR  128	/*a sector max block nr*/

/*we assume that a file max size is 8MB-64KB-512B*/

#define BOFS_BLOCK_LEVEL0  0
#define BOFS_BLOCK_LEVEL1  BOFS_SECTOR_BLOCK_NR
#define BOFS_BLOCK_LEVEL2  (BOFS_SECTOR_BLOCK_NR + BOFS_SECTOR_BLOCK_NR*BOFS_SECTOR_BLOCK_NR)

#define BOFS_IMODE_U 0X00 /*unknown mode*/
#define BOFS_IMODE_R 0X01 /*read mode*/
#define BOFS_IMODE_W 0X02 /*write mode*/
#define BOFS_IMODE_X 0X04 /*read and write mode*/

#define BOFS_IMODE_F 0X10 /*file type mode*/
#define BOFS_IMODE_D 0X20 /*directory type mode*/

/*
we assume a inode is 64 bytes
*/

struct BOFS_Inode 
{
	devid_t deviceID;	/* 设备ID */

	unsigned int id;	/*inode id*/
	
	uint16 mode;	/*file mode*/
	uint16 links;	/*links number*/
	
	uint32 size;	/*file size*/
	
	uint32 crttime;	/*create time*/
	uint32 mdftime;	/*modify time*/
	uint32 acstime;	/*access time*/
	
	uint32 flags;	/*access time*/
	
	uint32 block[BOFS_BLOCK_NR];	/*data block*/
	
	/* end 44 bytes */

	uint32 reserved[BOFS_Inode_RESERVED];	/*data block*/
}__attribute__ ((packed));;

void BOFS_CreateInode(struct BOFS_Inode *inode,
    unsigned int id,
    unsigned int mode,
    unsigned int flags,
    devid_t deviceID);

void BOFS_DumpInode(struct BOFS_Inode *inode);

void BOFS_CopyInode(struct BOFS_Inode *dst, struct BOFS_Inode *src);

void bofs_sync_inode(struct BOFS_Inode *inode);
void bofs_load_inode_by_id(struct BOFS_Inode *inode, uint32 id);
void bofs_copy_inode(struct BOFS_Inode *inode_a, struct BOFS_Inode *inode_b);
void bofs_release_inode_data(struct BOFS_Inode *inode);
void bofs_empty_inode(struct BOFS_Inode *inode);
void bofs_close_inode(struct BOFS_Inode *inode);

int bofs_get_inode_data(struct BOFS_Inode *inode, uint32 block_id, uint32 *data);
int bofs_free_inode_data(struct BOFS_Inode *inode, uint32 block_id);
void bofs_copy_inode_data(struct BOFS_Inode *inode_a, struct BOFS_Inode *inode_b);


#endif

