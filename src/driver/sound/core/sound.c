/*
 * file:		sound/core/sound.c
 * auther:		Jason Hu
 * time:		2020/1/17
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/arch.h>
#include <lib/string.h>
#include <sound/sound.h>
//#include <sound/buzzer/pcspeaker.h>
//#include <sound/sb/sb16.h>

/* ----驱动程序初始化文件导入---- */
EXTERN int InitPcspeakerDriver();
/* ----驱动程序初始化文件导入完毕---- */

PUBLIC int InitSoundSystem()
{
    
#ifdef CONFIG_SB16
    if (InitSoundBlaster16Driver()) {
        return -1;
    }    
#endif /* CONFIG_SB16 */

    if (InitPcspeakerDriver()) {
        printk("init pc speaker failed!\n");
    }

    //while(1);
    return 0;
}