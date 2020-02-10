/*
 * file:		video/core/video.c
 * auther:		Jason Hu
 * time:		2020/2/10
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>

#include <video/video.h>

/* 驱动头文件 */
//#include <video/vbe/vesa.h>

/* ----驱动程序初始化文件导入---- */
EXTERN int InitVesaDriver();
/* ----驱动程序初始化文件导入完毕---- */

PRIVATE void InitGraphDrivers()
{
    
#ifdef CONFIG_DRV_VESA
    if (InitVesaDriver()) {
        printk("init serial driver failed!\n");
    }
#endif  /* CONFIG_DRV_VESA */

}

/**
 * InitVideoSystem - 初始化视频系统
 */
PUBLIC int InitVideoSystem()
{
    
    /* 初始化图形系统的驱动 */
    InitGraphDrivers();

    InitKGC();
    
    return 0;
}
