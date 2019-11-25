/*
 * file:		fs/fatfs/init.c
 * auther:		Jason Hu
 * time:		2019/11/18
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/types.h>   /* 文件系统接口 */
#include <fs/interface.h>   /* 文件系统接口 */
#include <book/debug.h>   /* 文件系统接口 */
#include <book/memcache.h>   /* 文件系统接口 */
#include <book/arch.h>   /* 文件系统接口 */
#include <share/string.h> 
#include <share/vsprintf.h> 

#include "diskio.h"     /* 获取磁盘io接口 */
#include "ff.h"     /* 获取磁盘io接口 */



void put_rc (FRESULT rc)
{
	const WCHAR *p =
		"OK\0DISK_ERR\0INT_ERR\0NOT_READY\0NO_FILE\0NO_PATH\0INVALID_NAME\0"
		"DENIED\0EXIST\0INVALID_OBJECT\0WRITE_PROTECTED\0INVALID_DRIVE\0"
		"NOT_ENABLED\0NO_FILE_SYSTEM\0MKFS_ABORTED\0TIMEOUT\0LOCKED\0"
		"NOT_ENOUGH_CORE\0TOO_MANY_OPEN_FILES\0INVALID_PARAMETER\0";
	FRESULT i;

	for (i = 0; i != rc && *p; i++) {
		while(*p++) ;
	}
	printk(L"rc=%u FR_%s\n", (UINT)rc, p);
}


/* 
1.检测是否已经分区
2.检测是否已近有文件系统
 */

void FatFsDiskPartition()
{
    /**
     * 0驱动器->ramdisk
     * 1驱动器->hda
     * ...
     */

    /* 单个分区 */
    /*LBA_t ramList[] = {100, 0};
    void *work = kmalloc(FF_MAX_SS, GFP_KERNEL);
    if (work == NULL) {
        printk("kmalloc for ff workspace failed!\n");
    }
    f_fdisk(0, ramList, work);*/
    void *work = kmalloc(FF_MAX_SS, GFP_KERNEL);
    if (work == NULL) {
        printk("kmalloc for ff workspace failed!\n");
    }
    /* 多个分区 */
    //LBA_t ideList[] = {50, 50, 0};
    /* 分区前先检测是否已经分区好了，猜测是在0扇区进行了分区操作 */
    /*FRESULT ret = f_fdisk(1, ideList, work);
    printk("ret:%d\n", ret);*/

    /* 在分区上建立文件系统 */
    f_mkfs("/RAM", 0, work, FF_MAX_SS);
    //f_mkfs("/HDA", 0, work, FF_MAX_SS);

    /*f_mkfs("1:", 0, work, FF_MAX_SS);
    f_mkfs("2:", 0, work, FF_MAX_SS);*/
    
}

FRESULT scan_files (
    char* path        /* Start node to be scanned (***also used as work area***) */
)
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;

    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                printk(fno.fname);
                res = scan_files(path);                    /* Enter the directory */
                if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* It is a file. */
                printk("%s/%s\n", path, fno.fname);
            }
        }
        f_closedir(&dir);
    }

    return res;
}

/**
 * InitFatFs - 初始化fatfs
 * 
 */
PUBLIC void InitFatFs()
{
    PART_START("FatFS");
    /* 约定FatFs的drive */
    assign_drives();
    
    FatFsDiskPartition();

    //Spin("test");

    /* Filesystem object */
    /*FATFS *fs  = kmalloc(sizeof(FATFS), GFP_KERNEL);           
    if (fs == NULL) {
        printk("kmalloc for fatfs failed!\n");
    }
    f_mount(fs, "0:", 0);*/

    FATFS *fs  = kmalloc(sizeof(FATFS), GFP_KERNEL);           /* Filesystem object */
    if (fs == NULL) {
        printk("kmalloc for fatfs failed!\n");
    }
 
    f_mount(fs, "/RAM", 0);

    fs  = kmalloc(sizeof(FATFS), GFP_KERNEL);           /* Filesystem object */
    if (fs == NULL) {
        printk("kmalloc for fatfs failed!\n");
    }
    
    f_mount(fs, "/HDA", 0);

    FIL fil;            /* File object */
    FRESULT res;        /* API result code */
    UINT bw, br;            /* Bytes written */

    FRESULT fr;
    TCHAR str[64];

    fr = f_getcwd(str, 64);  /* Get current directory path */
    printk("cwd:%s\n",str);

    f_chdrive("/HDA");
    
    //f_chdir("/");

    fr = f_getcwd(str, 64);  /* Get current directory path */
    printk("cwd:%s\n",str);

    res = f_mkdir("/HDA/bin");
    if (res != FR_OK) {
        printk("mkdir failed!\n");
    }
    /*
    char buff[256];
    strcpy(buff, "/HDA");
    res = scan_files(buff);
    if (res != FR_OK) {
        printk("scan_files failed!\n");
    }*/
    
    res = f_open(&fil, "/HDA/bin/nasm.elf", FA_OPEN_EXISTING | FA_WRITE | FA_READ);
    if (res != FR_OK) {
        printk("file open failed!\n");
    }

    //Spin("test");

    /* Create a file as new */
    /*res = f_open(&fil, "HDA:hello.txt",  FA_WRITE | FA_READ);
    if (res != FR_OK) {
        printk("file open failed!\n");
    }*/

    //Spin("test");
     char buf[32];

    f_read(&fil, buf, 15, &br);
    if (br != 15) {
        printk("file read failed!\n");
    }
    printk(buf);

    /* Write a message */
    f_write(&fil, "Hello, World! \n", 15, &bw);
    if (bw != 15) {
        printk("file write failed!\n");
    }
    
    f_lseek(&fil, 0);

   
    f_read(&fil, buf, 15, &br);
    if (br != 15) {
        printk("file read failed!\n");
    }
    printk(buf);

    /* Close the file */
    f_close(&fil);
    
    /* Create a file as new */
    res = f_open(&fil, "/RAM/hello.txt", FA_CREATE_NEW | FA_WRITE | FA_READ);
    if (res != FR_OK) {
        printk("file open failed!\n");
    }

    int len = strlen(buf);

    buf[0] = 'b';
    /* Write a message */
    f_write(&fil, buf, len, &bw);
    if (bw != len) {
        printk("file write failed!\n");
    }
    
    f_lseek(&fil, 0);

    memset(buf, 0, 32);

    f_read(&fil, buf, 15, &br);
    if (br != 15) {
        printk("file read failed!\n");
    }
    printk(buf);

    /* Close the file */
    f_close(&fil);
    
    /* Unregister work area */
    f_mount(0, "1:", 0);

    /* Unregister work area */
    f_mount(0, "2:", 0);
    


    /* 初始化FatFs */
    PART_END();
}
