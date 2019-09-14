/*
 * file:		fs/bofs/inode.c
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
#include <fs/bofs/inode.h>
#include <driver/clock.h>
#include <fs/bofs/bitmap.h>
#include <share/math.h>

void BOFS_CreateInode(struct BOFS_Inode *inode,
    unsigned int id,
    unsigned int mode,
    unsigned int flags,
    devid_t deviceID)
{
	memset(inode, 0, sizeof(struct BOFS_Inode));
	inode->id = id;
	inode->mode = mode;
	inode->links = 0;
	inode->size = 0;
	
	inode->crttime = SystemDateToData();	/*create time*/
	inode->acstime = inode->mdftime = inode->crttime;	/*modify time*/
	
	inode->flags = flags;	/* 访问标志 */
	
    inode->deviceID = deviceID;	/* 设备ID */
	
	int i;
	for(i = 0; i < BOFS_BLOCK_NR; i++){
		inode->block[i] = 0;
	}
}

void BOFS_DumpInode(struct BOFS_Inode *inode)
{
    printk(PART_TIP "---- Inode ----\n");
    printk(PART_TIP "id:%d mode:%x links:%d size:%x flags:%x block start:%d\n",
        inode->id, inode->mode, inode->links, inode->size,
        inode->flags, inode->block[0]);

    printk(PART_TIP "Date: %d/%d/%d",
        DATA16_TO_DATE_YEA(inode->crttime >> 16),
        DATA16_TO_DATE_MON(inode->crttime >> 16),
        DATA16_TO_DATE_DAY(inode->crttime >> 16));
    printk(" %d:%d:%d\n",
        DATA16_TO_TIME_HOU(inode->crttime),
        DATA16_TO_TIME_MIN(inode->crttime),
        DATA16_TO_TIME_SEC(inode->crttime));

}

PUBLIC int BOFS_SyncInode(struct BOFS_Inode *inode, struct BOFS_SuperBlock *sb)
{
	uint32 sectorOffset = inode->id / sb->inodeNrInSector;
	uint32 lba = sb->inodeTableLba + sectorOffset;
	uint32 bufOffset = inode->id % sb->inodeNrInSector;
	
	//printk("BOFS sync inode: sec off:%d lba:%d buf off:%d\n", sectorOffset, lba, bufOffset);
	
	memset(sb->iobuf, 0, SECTOR_SIZE);
    if (DeviceRead(sb->deviceID, lba, sb->iobuf, 1)) {
        return -1;
    }
	struct BOFS_Inode *inode2 = (struct BOFS_Inode *)sb->iobuf;
	*(inode2 + bufOffset) = *inode;
	
    if (DeviceWrite(sb->deviceID, lba, sb->iobuf, 1)) {
        return -1;
    }
    return 0;
}

/**
 * BOFS_LoadInodeByID - 通过inode id 加载节点信息
 * @inode: 储存节点信息
 * @id: 节点id
 * @sb: 所在的超级块
 * 
 * 成功返回0，失败返回-1
 */
PUBLIC int BOFS_LoadInodeByID(struct BOFS_Inode *inode, 
    unsigned int id, 
    struct BOFS_SuperBlock *sb)
{

	uint32 sectorOffset = id / sb->inodeNrInSector;
	uint32 lba = sb->inodeTableLba + sectorOffset;
	uint32 bufOffset = id % sb->inodeNrInSector;
	
	//printk("BOFS load inode sec off:%d lba:%d buf off:%d\n", sectorOffset, lba, bufOffset);
	
	//printk("BOFS inode nr in sector %d\n", sb->inodeNrInSector);
	
	memset(sb->iobuf, 0, SECTOR_SIZE);
	if (DeviceRead(sb->deviceID, lba, sb->iobuf, 1)) {
        return -1;
    }

	struct BOFS_Inode *inode2 = (struct BOFS_Inode *)sb->iobuf;
	*inode = inode2[bufOffset];
	//BOFS_DumpInode(inode);
	return 0;
}

PUBLIC void BOFS_CopyInode(struct BOFS_Inode *dst, struct BOFS_Inode *src)
{
	memset(dst, 0, sizeof(struct BOFS_Inode));
	
	dst->id = src->id;
	
	dst->mode = src->mode;
	dst->links = src->links;
	
	dst->size = src->size;
	
	dst->crttime = src->crttime;
	dst->mdftime = src->mdftime;
	dst->acstime = src->acstime;
	
	dst->flags = src->flags;
	/*
	copy data block
	*/
}

/*
for data read or write, we should make a way to do it.
in a inode, we can read max sectors below blocks, so we can get 
sector use bofs_get_inode_data(); to get data lba
*/
int BOFS_GetInodeData(struct BOFS_Inode *inode,
 	uint32 blockID,
	uint32 *data,
	struct BOFS_SuperBlock *sb)
{
	uint32 indirect[3];
	uint32 offset[2];
	uint32 *data_buf0, *data_buf1;

	uint32 lba;
	int idx;
	//printk("BOFS_GetInodeData> inode:%d ",inode->id);
	if(blockID == BOFS_BLOCK_LEVEL0){
		//printk("level:%d off:%d\n", 0, blockID);
		
		/*get indirect0*/
		indirect[0] = inode->block[0];
		if(indirect[0] == 0){
			//printk("no data, now create a new sector to save it.\n");
			/*no data, alloc data*/
			idx = BOFS_AllocBitmap(sb, BOFS_BMT_SECTOR, 1);
			if(idx == -1){
				printk("alloc sector bitmap failed!\n");
				return -1;
			}
			/*alloc a sector*/
			BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
			lba = BOFS_IDX_TO_LBA(sb, idx);
			
			indirect[0] = inode->block[0] = lba;
			
			//printk("BOFS_GetInodeData: alloc a sector at:%d\n", lba);
			
			/*sync inode to save new data*/
			BOFS_SyncInode(inode, sb);

		}else{
			//printk("have data.\n");
		}
		
		*data = indirect[0];
		
	}else if(BOFS_BLOCK_LEVEL0 < blockID && blockID <= BOFS_BLOCK_LEVEL1){
		
		blockID -= (BOFS_BLOCK_LEVEL0+1);
		
		offset[0] = blockID;
		
		//printk("level:%d off:%d\n", 1, offset[0]);
		/*get indirect0*/
		indirect[0] = inode->block[1];

		if(indirect[0] == 0){
			//printk("no indirect 0 data, now create a new sector to save it.\n");
			
			/*no data, alloc data*/
			idx = BOFS_AllocBitmap(sb, BOFS_BMT_SECTOR, 1);
			if(idx == -1){
				printk("alloc sector bitmap failed!\n");
				return -1;
			}
			/*alloc a sector*/
			BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
			lba = BOFS_IDX_TO_LBA(sb, idx);
			
			indirect[0] = inode->block[1] = lba;
			/*sync inode to save new data*/
			//printk(">>>sync inode block %d %d %d", inode->block[0], inode->block[1], inode->block[2]);
		
			BOFS_SyncInode(inode, sb);

		}else{
			//printk("have data.\n");
			
		}
		
		memset(sb->databuf[0], 0, SECTOR_SIZE);

		//sector_read(indirect[0], sb->databuf[0], 1);
		if (DeviceRead(sb->deviceID, indirect[0], sb->databuf[0], 1)) {
			printk(PART_ERROR "device %d read failed!\n", sb->deviceID);
			return -1;
		}
		data_buf0 = (unsigned int *)sb->databuf[0];
		
		/*get indirect1*/
		indirect[1] = data_buf0[offset[0]];
		if(indirect[1] == 0){
			//printk("no indirect 1 data, now create a new sector to save it.\n");
			
			/*no data, alloc data*/
			idx = BOFS_AllocBitmap(sb, BOFS_BMT_SECTOR, 1);
			if(idx == -1){
				printk("alloc sector bitmap failed!\n");
				return -1;
			}
			/*alloc a sector*/
			BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
			lba = BOFS_IDX_TO_LBA(sb, idx);
			
			indirect[1] = data_buf0[offset[0]] = lba;
			/*sync buf to save new data*/
			
			//sector_write(indirect[0], sb->databuf[0], 1);
			if (DeviceWrite(sb->deviceID, indirect[0], sb->databuf[0], 1)) {
				printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
				return -1;
			}
		}else{
			//printk("have data.\n");
			
		}
		//printk("block %d %d %d", inode->block[0], inode->block[1], inode->block[2]);
		
		*data = indirect[1];
		
	}else if(BOFS_BLOCK_LEVEL1 < blockID && blockID <= BOFS_BLOCK_LEVEL2){
		/*
		we do not test here, if a dir has at least 129 dir , it will run here.
		so now, we don't test it, but we image it is OK.
		2019/3/24 by Hu Zicheng @.@
		*/
		
		blockID -= (BOFS_BLOCK_LEVEL1+1);
		
		offset[0] = blockID/BOFS_SECTOR_BLOCK_NR;
		offset[1] = blockID%BOFS_SECTOR_BLOCK_NR;
		
		//printk("level:%d off:%d off:%d\n", 2, offset[0], offset[1]);
		/*get indirect0*/
		indirect[0] = inode->block[2];
		if(indirect[0] == 0){
			/*no data, alloc data*/
			idx = BOFS_AllocBitmap(sb, BOFS_BMT_SECTOR, 1);
			if(idx == -1){
				printk("alloc sector bitmap failed!\n");
				return -1;
			}
			/*alloc a sector*/
			BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
			lba = BOFS_IDX_TO_LBA(sb, idx);
			
			indirect[0] = inode->block[2] = lba;
			/*sync inode to save new data*/
			BOFS_SyncInode(inode, sb);

		}
		
		memset(sb->databuf[0], 0, SECTOR_SIZE);
		//sector_read(indirect[0], sb->databuf[0], 1);
		if (DeviceRead(sb->deviceID, indirect[0], sb->databuf[0], 1)) {
			printk(PART_ERROR "device %d read failed!\n", sb->deviceID);
			return -1;
		}
		data_buf0 = (unsigned int *)sb->databuf[0];
		
		/*get indirect1*/
		indirect[1] = data_buf0[offset[0]];
		if(indirect[1] == 0){
			/*no data, alloc data*/
			idx = BOFS_AllocBitmap(sb, BOFS_BMT_SECTOR, 1);
			if(idx == -1){
				printk("alloc sector bitmap failed!\n");
				return -1;
			}
			/*alloc a sector*/
			BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
			lba = BOFS_IDX_TO_LBA(sb, idx);
			
			indirect[1] = data_buf0[offset[0]] = lba;
			/*sync buf to save new data*/
			//sector_write(indirect[0], sb->databuf[0], 1);
			if (DeviceWrite(sb->deviceID, indirect[0], sb->databuf[0], 1)) {
				printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
				return -1;
			}
		}
		
		memset(sb->databuf[1], 0, SECTOR_SIZE);
		//sector_read(indirect[1], sb->databuf[1], 1);
		if (DeviceRead(sb->deviceID, indirect[1], sb->databuf[1], 1)) {
			printk(PART_ERROR "device %d read failed!\n", sb->deviceID);
			return -1;
		}
		data_buf1 = (unsigned int *)sb->databuf[1];
		
		/*get indirect2*/
		indirect[2] = data_buf1[offset[1]];
		if(indirect[2] == 0){
			/*no data, alloc data*/
			idx = BOFS_AllocBitmap(sb, BOFS_BMT_SECTOR, 1);
			if(idx == -1){
				printk("alloc sector bitmap failed!\n");
				return -1;
			}
			/*alloc a sector*/
			BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
			lba = BOFS_IDX_TO_LBA(sb, idx);
			
			indirect[2] = data_buf1[offset[1]] = lba;
			/*sync buf to save new data*/
			
			//sector_write(indirect[1], sb->databuf[1], 1);
			if (DeviceWrite(sb->deviceID, indirect[1], sb->databuf[1], 1)) {
				printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
				return -1;
			}
		}
		*data = indirect[2];
	}
	return 0;
}

int BOFS_FreeInodeData(struct BOFS_SuperBlock *sb,
	struct BOFS_Inode *inode,
	uint32 blockID)
{
	uint32 indirect[3];
	uint32 offset[2];
	uint32 *data_buf0, *data_buf1;
	
	uint32 idx;
	int i;
	uint8 flags;
	
	//printk("bofs_free_inode_data> ");
	
	if(blockID == BOFS_BLOCK_LEVEL0){
		/*get indirect0*/
		indirect[0] = inode->block[0];
		if(indirect[0] != 0){
			//printk("have data:%d\n", indirect[0]);
			memset(sb->databuf[0], 0, SECTOR_SIZE);
			//sector_write(indirect[0], sb->databuf[0], 1);
			if (DeviceWrite(sb->deviceID, indirect[0], sb->databuf[0], 1)) {
				printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
				return -1;
			}
			idx = BOFS_LBA_TO_IDX(sb, indirect[0]);
			/*free sector bitmap*/
			BOFS_FreeBitmap(sb, BOFS_BMT_SECTOR, idx);
			/*we need sync to disk*/
			BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
			
			inode->block[0] = 0;
			/*sync inode to save empty data*/
			BOFS_SyncInode(inode, sb);
			printk("<<<lba:%d\n", indirect[0]);
			
		}else{
			printk("no data.\n");
		}
		return indirect[0];
	}else if(BOFS_BLOCK_LEVEL0 < blockID && blockID <= BOFS_BLOCK_LEVEL1){
		
		blockID -= (BOFS_BLOCK_LEVEL0+1);
		
		offset[0] = blockID;
		
		//printk("level:%d off:%d\n", 1, offset[0]);
		/*get indirect0*/
		indirect[0] = inode->block[1];
		if(indirect[0] != 0){
			//printk("have indirect 0 data:%d\n", indirect[0]);
			memset(sb->databuf[0], 0, SECTOR_SIZE);
			//sector_read(indirect[0], sb->databuf[0], 1);
			if (DeviceRead(sb->deviceID, indirect[0], sb->databuf[0], 1)) {
				printk(PART_ERROR "device %d read failed!\n", sb->deviceID);
				return -1;
			}

			data_buf0 = (unsigned int *)sb->databuf[0];
			
			/*get indirect1*/
			indirect[1] = data_buf0[offset[0]];
			if(indirect[1] != 0){
				//printk("have indirect 1 data:%d\n", indirect[1]);
				memset(sb->databuf[1], 0, SECTOR_SIZE);
				//sector_write(indirect[1], sb->databuf[1], 1);
				if (DeviceWrite(sb->deviceID, indirect[1], sb->databuf[1], 1)) {
					printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
					return -1;
				}
				idx = BOFS_LBA_TO_IDX(sb, indirect[1]);
				/*free sector bitmap*/
				BOFS_FreeBitmap(sb, BOFS_BMT_SECTOR, idx);
				/*we need sync to disk*/
				BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
			
				data_buf0[offset[0]] = 0;
				/*write to disk for next time check.
				every time we change indirect block's data,
				we need write to disk to save this result.
				*/
				//sector_write(indirect[0], sb->databuf[0], 1);
				if (DeviceWrite(sb->deviceID, indirect[0], sb->databuf[0], 1)) {
					printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
					return -1;
				}
				//printk("@@@check empty!\n");
				flags = 0;
				/*check indirect, if empty, we release it*/
				for(i = 0; i < BOFS_SECTOR_BLOCK_NR; i++){
					if(data_buf0[i] != 0){
						/*not empty*/
						flags = 1;
						//printk("buf[%d] = %d ", i, data_buf0[i]);
						//printk(">>indirect block no empty!\n");
						break;
					}
				}
				/*indirect sector is empty, we will release it*/
				if(!flags){
					//printk("level 1 indirect 1 is empty!\n");
					memset(sb->databuf[0], 0, SECTOR_SIZE);
					//sector_write(indirect[0], sb->databuf[0], 1);
					if (DeviceWrite(sb->deviceID, indirect[0], sb->databuf[0], 1)) {
						printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
						return -1;
					}
					
					//printk("##sector!\n");
					idx = BOFS_LBA_TO_IDX(sb, indirect[0]);
					/*free sector bitmap*/
					BOFS_FreeBitmap(sb, BOFS_BMT_SECTOR, idx);
					/*we need sync to disk*/
					BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
			
					inode->block[1] = 0;
					//printk("&&&&&sync to inode!\n");
					/*sync inode to save empty data*/
					BOFS_SyncInode(inode, sb);
				}
			}else{
				//printk("no data.\n");
			}
		}else{
			//printk("no data.\n");
		}
		return indirect[1];
	}else if(BOFS_BLOCK_LEVEL1 < blockID && blockID <= BOFS_BLOCK_LEVEL2){
		/*
		we do not test here, if a dir has at least 129 dir , it will run here.
		so now, we don't test it, but we image it is OK.
		2019/3/24 by Hu Zicheng @.@
		*/
		
		blockID -= (BOFS_BLOCK_LEVEL1+1);
		
		offset[0] = blockID/BOFS_SECTOR_BLOCK_NR;
		offset[1] = blockID%BOFS_SECTOR_BLOCK_NR;
		
		//printk("level:%d off:%d off:%d\n", 2, offset[0], offset[1]);
		/*get indirect0*/
		indirect[0] = inode->block[2];
		if(indirect[0] != 0){
			
			memset(sb->databuf[0], 0, SECTOR_SIZE);
			//sector_read(indirect[0], sb->databuf[0], 1);
			if (DeviceRead(sb->deviceID, indirect[0], sb->databuf[0], 1)) {
				printk(PART_ERROR "device %d read failed!\n", sb->deviceID);
				return -1;
			}
			data_buf0 = (unsigned int *)sb->databuf[0];
			
			/*get indirect1*/
			indirect[1] = data_buf0[offset[0]];
			if(indirect[1] != 0){
				
				memset(sb->databuf[1], 0, SECTOR_SIZE);
				//sector_read(indirect[1], sb->databuf[1], 1);
				if (DeviceRead(sb->deviceID, indirect[1], sb->databuf[1], 1)) {
					printk(PART_ERROR "device %d read failed!\n", sb->deviceID);
					return -1;
				}
				data_buf1 = (unsigned int *)sb->databuf[1];
				
				indirect[2] = data_buf1[offset[1]];
				
				if(indirect[2] != 0){
					memset(sb->databuf[2], 0, SECTOR_SIZE);
					//sector_write(indirect[2], sb->databuf[2], 1);
					if (DeviceWrite(sb->deviceID, indirect[2], sb->databuf[2], 1)) {
						printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
						return -1;
					}
					idx = BOFS_LBA_TO_IDX(sb, indirect[2]);
					/*free sector bitmap*/
					BOFS_FreeBitmap(sb, BOFS_BMT_SECTOR, idx);
					/*we need sync to disk*/
					BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
			
					data_buf1[offset[1]] = 0;
					/*write to disk for next time check.
					every time we change indirect block's data,
					we need write to disk to save this result.
					*/
					//sector_write(indirect[1], sb->databuf[1], 1);
					if (DeviceWrite(sb->deviceID, indirect[1], sb->databuf[1], 1)) {
						printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
						return -1;
					}
					flags = 0;
					/*check indirect, if empty, we release it*/
					for(i = 0; i < BOFS_SECTOR_BLOCK_NR; i++){
						if(data_buf1[i] != 0){
							/*not empty*/
							flags = 1;
						}
					}
					/*indirect sector is empty, we will release it*/
					if(!flags){
						memset(sb->databuf[1], 0, SECTOR_SIZE);
						//sector_write(indirect[1], sb->databuf[1], 1);
						if (DeviceWrite(sb->deviceID, indirect[1], sb->databuf[1], 1)) {
							printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
							return -1;
						}
						idx = BOFS_LBA_TO_IDX(sb, indirect[1]);
						/*free sector bitmap*/
						BOFS_FreeBitmap(sb, BOFS_BMT_SECTOR, idx);
						/*we need sync to disk*/
						BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
						
						data_buf0[offset[0]] = 0;
						/*write to disk for next time check.
						every time we change indirect block's data,
						we need write to disk to save this result.
						*/
						//sector_write(indirect[0], sb->databuf[0], 1);
						if (DeviceWrite(sb->deviceID, indirect[0], sb->databuf[0], 1)) {
							printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
							return -1;
						}
						flags = 0;
						/*check indirect, if empty, we release it*/
						for(i = 0; i < BOFS_SECTOR_BLOCK_NR; i++){
							if(data_buf0[i] != 0){
								/*no empty*/
								flags = 1;
							}
						}
						/*indirect sector is empty, we will release it*/
						if(!flags){
							//printk("level 2 indirect 1 is empty!\n");
							
							memset(sb->databuf[0], 0, SECTOR_SIZE);
							//sector_write(indirect[0], sb->databuf[0], 1);
							if (DeviceWrite(sb->deviceID, indirect[0], sb->databuf[0], 1)) {
								printk(PART_ERROR "device %d write failed!\n", sb->deviceID);
								return -1;
							}
							idx = BOFS_LBA_TO_IDX(sb, indirect[0]);
							/*free sector bitmap*/
							BOFS_FreeBitmap(sb, BOFS_BMT_SECTOR, idx);
							/*we need sync to disk*/
							BOFS_SyncBitmap(sb, BOFS_BMT_SECTOR, idx);
			
							inode->block[2] = 0;
							
							/*sync inode to save empty data*/
							BOFS_SyncInode(inode, sb);
						}
					}
					
				}
			}
		}
		return indirect[2];
	}
	return -1;
}

/*
we need inode size to release data
*/
void BOFS_ReleaseInodeData(struct BOFS_SuperBlock *sb,
	struct BOFS_Inode *inode)
{
	int i;
	uint32 blocks = DIV_ROUND_UP(inode->size, SECTOR_SIZE);
	
	printk("inode:%d size:%d blocks:%d\n",inode->id,inode->size, blocks);
	/*scan back to*/
	for(i = 0; i < blocks; i++){
		BOFS_FreeInodeData(sb, inode, i);
	}
}
