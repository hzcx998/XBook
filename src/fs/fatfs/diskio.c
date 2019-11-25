/*-----------------------------------------------------------------------*/
/* Low level disk control module for Win32              (C)ChaN, 2019    */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include <book/device.h>		/* device operate */

#include <drivers/ramdisk.h>
#include <drivers/ide.h>
#include <share/string.h>
#include <book/debug.h>


#define MAX_DRIVES	10		/* Max number of physical drives to be used */
#define	SZ_BLOCK	1024		/* Block size to be returned by GET_BLOCK_SIZE command */

#define SZ_RAMDISK	10		/* Size of drive 0 (RAM disk) [MiB] */
#define SS_RAMDISK	512		/* Sector size of drive 0 (RAM disk) [byte] */

#define MIN_READ_ONLY	1	/* Read-only drive from */
#define	MAX_READ_ONLY	1	/* Read-only drive to */



/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/

#define	BUFSIZE 128*1024	/* Size of data transfer buffer */

typedef struct {
	DSTATUS	status;
	WORD sz_sector;
	LBA_t n_sectors;
	dev_t h_drive;
    dev_t devno;
} STAT;

static int Drives;

static volatile STAT Stat[MAX_DRIVES];

static BYTE *Buffer;	/* Poiter to the data transfer buffer and ramdisk */


int get_status (
	BYTE pdrv
)
{
	volatile STAT *stat = &Stat[pdrv];
	stat->h_drive = pdrv;

	if (pdrv == 0) {	/* RAMDISK */
        stat->devno = DEV_RDA;

        /* 从磁盘读取信息 */
        DeviceIoctl(stat->devno, RAMDISK_IO_BLKZE, (int)&stat->sz_sector);
		if (stat->sz_sector < FF_MIN_SS || stat->sz_sector > FF_MAX_SS) return 0;
        DeviceIoctl(stat->devno, RAMDISK_IO_SECTORS, (int)&stat->n_sectors);
        
		stat->status = 0;
		return 1;
	}

	/* Get drive size */
	stat->status = STA_NOINIT;

    stat->devno = DEV_HDA;

    DeviceIoctl(stat->devno, IDE_IO_BLKZE, (int)&stat->sz_sector);
    if (stat->sz_sector < FF_MIN_SS || stat->sz_sector > FF_MAX_SS) return 0;
    DeviceIoctl(stat->devno, IDE_IO_SECTORS, (int)&stat->n_sectors);
	
    stat->status = 0;
	return 1;
}




/*--------------------------------------------------------------------------

   Public Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* Initialize Windows disk accesss layer                                 */
/*-----------------------------------------------------------------------*/

int assign_drives (void)
{
	BYTE pdrv, ndrv;

    /* 为缓冲区分配空间 */
	Buffer = kmalloc(BUFSIZE, GFP_KERNEL);
	if (!Buffer) return 0;

    ndrv = 2;
    
    /* 打开ramdisk磁盘 */
    DeviceOpen(DEV_RDA, 0);

    /* 打开硬盘 */
    DeviceOpen(DEV_HDA, 0);
	for (pdrv = 0; pdrv < ndrv; pdrv++) {

		if (get_status(pdrv)) {
			printk(" (drive %d %dMB, %d bytes * %d blocks)\n", pdrv, (UINT)((UINT)Stat[pdrv].sz_sector * Stat[pdrv].n_sectors / 1024 / 1024),
                Stat[pdrv].sz_sector, Stat[pdrv].n_sectors);
		} else {
			printk(" (Not Ready)\n");
		}
	}
	Drives = pdrv;

	return pdrv;
}


/*-----------------------------------------------------------------------*/
/* Initialize Disk Drive                                                 */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv		/* Physical drive nmuber */
)
{
	DSTATUS sta;

	if (pdrv >= Drives) {
		sta = STA_NOINIT;
	} else {
		get_status(pdrv);
		sta = Stat[pdrv].status;
	}

	return sta;
}



/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0) */
)
{
	DSTATUS sta;

	if (pdrv >= Drives) {
		sta = STA_NOINIT;
	} else {
		sta = Stat[pdrv].status;
	}

	return sta;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,			/* Physical drive nmuber (0) */
	BYTE *buff,			/* Pointer to the data buffer to store read data */
	LBA_t sector,		/* Start sector number (LBA) */
	UINT count			/* Number of sectors to read */
)
{
	DWORD nc;
	DSTATUS res;

	if (pdrv >= Drives || Stat[pdrv].status & STA_NOINIT) {
		return RES_NOTRDY;
	}

	nc = (DWORD)count * Stat[pdrv].sz_sector;
	
	if (pdrv) {	/* Physical dirve */
		if (nc > BUFSIZE) {
			res = RES_PARERR;
		} else {
			
            if (DeviceRead(Stat[pdrv].devno, sector, Buffer, count)) {
                /* 失败 */
                res = RES_ERROR;
            } else {
                /* 成功就从缓冲中复制 */
                memcpy(buff, Buffer, nc);
                res = RES_OK;
            }
		}
	} else {	/* RAM disk */
		if (nc >= Stat[pdrv].sz_sector * Stat[pdrv].n_sectors) {
			res = RES_ERROR;
		} else {
            if (DeviceRead(Stat[pdrv].devno, sector, Buffer, count)) {
                /* 失败 */
                res = RES_ERROR;
            } else {
                /* 成功就从缓冲中复制 */
                memcpy(buff, Buffer, nc);
                res = RES_OK;
            }
		}
	}
	return res;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0) */
	const BYTE *buff,	/* Pointer to the data to be written */
	LBA_t sector,		/* Start sector number (LBA) */
	UINT count			/* Number of sectors to write */
)
{
	DWORD nc = 0;
	DRESULT res;

	if (pdrv >= Drives || Stat[pdrv].status & STA_NOINIT) {
		return RES_NOTRDY;
	}

	res = RES_OK;

	if (Stat[pdrv].status & STA_PROTECT) {
		res = RES_WRPRT;
	} else {
		nc = (DWORD)count * Stat[pdrv].sz_sector;
		if (nc > BUFSIZE) res = RES_PARERR;
	}

	if (pdrv >= 0) {	/* Physical drives */
		//if (pdrv >= MIN_READ_ONLY && pdrv <= MAX_READ_ONLY) res = RES_WRPRT;
        
		if (res == RES_OK) {
			
            memcpy(Buffer, buff, nc);
            if (DeviceWrite(Stat[pdrv].devno, sector, Buffer, count)) {
                /* 失败 */
                res = RES_ERROR;
            }
		}
	} else {	/* RAM disk */

        if (nc >= Stat[pdrv].sz_sector * Stat[pdrv].n_sectors) {
			res = RES_ERROR;
		} else {

            memcpy(Buffer, buff, nc);
            if (DeviceWrite(Stat[pdrv].devno, sector, Buffer, count)) {
                /* 失败 */
                res = RES_ERROR;
            }
		}
	}

	return res;
}

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0) */
	BYTE ctrl,		/* Control code */
	void *buff		/* Buffer to send/receive data */
)
{
	DRESULT res;

	if (pdrv >= Drives || (Stat[pdrv].status & STA_NOINIT)) {
		return RES_NOTRDY;
	}

	res = RES_PARERR;
	switch (ctrl) {
	case CTRL_SYNC:			/* Nothing to do */
		res = RES_OK;
		break;

	case GET_SECTOR_COUNT:	/* Get number of sectors on the drive */
		*(LBA_t*)buff = Stat[pdrv].n_sectors;
		res = RES_OK;
		break;

	case GET_SECTOR_SIZE:	/* Get size of sector for generic read/write */
		*(WORD*)buff = Stat[pdrv].sz_sector;
		res = RES_OK;
		break;

	case GET_BLOCK_SIZE:	/* Get internal block size in unit of sector */
		*(DWORD*)buff = SZ_BLOCK;
		res = RES_OK;
		break;

	case 200:				/* Load disk image file to the RAM disk (drive 0) */
		{
			
		}
		break;

	}

	return res;
}
