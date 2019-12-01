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
#include <book/device.h>

/*default fs inode nr*/
#define DEFAULT_MAX_INODE_NR 9182

/*book os file system inode*/
#define BOFS_BLOCK_NR 15

#define BOFS_INODE_RESERVED ((128-92)/4)



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
#define BOFS_IMODE_V 0X20 /* device mode*/

/*
we assume a inode is 128 bytes
*/
struct BOFS_Inode 
{
	dev_t devno;	/* 所在的设备ID */

	unsigned int id;	/*inode id*/
	
	uint16 mode;	/*file mode*/
	uint16 links;	/*links number*/
	
	uint32 size;	/*file size*/
	
	uint32 crttime;	/*create time*/
	uint32 mdftime;	/*modify time*/
	uint32 acstime;	/*access time*/
	
	uint32 flags;	/*access time*/
	
    /*8+15 = 23
    23 * 4 = 92
    128 ->*/
	uint32 blocks[BOFS_BLOCK_NR];	/*data block*/
	/* end 44 bytes */

	uint32 reserved[BOFS_INODE_RESERVED];	/*data block*/
}__attribute__ ((packed));;

PUBLIC void BOFS_CreateInode(struct BOFS_Inode *inode,
    unsigned int id,
    unsigned int mode,
    unsigned int flags,
    dev_t devno);

PUBLIC void BOFS_DumpInode(struct BOFS_Inode *inode);

PUBLIC void BOFS_CopyInode(struct BOFS_Inode *dst, struct BOFS_Inode *src);

PUBLIC void BOFS_CloseInode(struct BOFS_Inode *inode);

#endif

