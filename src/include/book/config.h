/*
 * file:		include/book/config.h
 * auther:		Jason Hu
 * time:		2019/6/3
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/**
 * 操作系统配置文件，有些可以配置的内容可以在这里进行配置，配置后重新编译内核即可
 */

#ifndef _BOOK_CONFIG_H
#define _BOOK_CONFIG_H

/**
 * ------------------------
 * 系统宏
 * ------------------------
 */
#define _BOOK32 /* 配置为BOOK32位 */



/**
 * ------------------------
 * 系统平台
 * ------------------------
 */
#define CONFIG_ARCH_X86 /* 配置为X86平台 */

/**
 * ------------------------
 * CPU的数据宽度
 * ------------------------
 */
#define CONFIG_CPU_32   /* 32位CPU */
//#define CONFIG_CPU_64 /* 64位CPU */

/**
 * ------------------------
 * 内核配置
 * ------------------------
 */

#define CONFIG_SEMAPHORE_M /* 配置多元信号量（Multivariate semaphore） */
//#define CONFIG_SEMAPHORE_B /* 配置二元信号量（Binary semaphore） */

/* 3大设备模块 */
#define CONFIG_BLOCK_DEVICE     /* 配置块设备模块 */
#define CONFIG_CHAR_DEVICE      /* 配置字符设备模块 */
#define CONFIG_NET_DEVICE       /* 配置网络设备模块 */

#define CONFIG_FILE_SYSTEM      /* 配置文件系统 */

/**
 * ------------------------
 * 内存管理配置
 * ------------------------
 */
#define CONFIG_LARGE_ALLOCS /* 如果想要用kmalloc分配128KB~4MB之间大小的内存，就需要配置此项 */

/**
 * ------------------------
 * 配置驱动
 * ------------------------
 */
#define CONFIG_DRV_KEYBOARD     /* 键盘驱动配置 */
#define CONFIG_DRV_MOUSE        /* 鼠标驱动配置 */
#define CONFIG_DRV_IDE          /* IDE驱动配置 */
#define CONFIG_DRV_RAMDISK      /* RAMDISK驱动配置 */
#define CONFIG_DRV_SERIAL       /* 串口驱动配置 */
#define CONFIG_DRV_VESA         /* VESA图形驱动配置 */
#define CONFIG_DRV_TTY          /* TTY驱动配置 */
#define CONFIG_DRV_CONSOLE      /* 控制台驱动 */

//#define CONFIG_DRV_PCNET32    /* pcnet32驱动配置 */
#define CONFIG_DRV_RTL8139    /* RTL8139驱动配置 */
//#define CONFIG_DRV_SB16       /* 声霸卡16驱动 */

/**
 * ------------------------
 * 配置调试方法
 * ------------------------
 */
//#define CONFIG_SERIAL_DEBUG     /* 串口调试，输出到其他电脑进行调试 */
#define CONFIG_CONSOLE_DEBUG    /* 串口调试，输出到其他电脑进行调试 */

/**
 * ------------------------
 * 配置显示方式
 * ------------------------
 */
#define CONFIG_DISPLAY_TEXT  /* 显示文本模式 */
#define CONFIG_DISPLAY_GRAPH /* 显示图形模式，注意loader中的图形配置 */

#endif   /*_BOOK_CONFIG_H*/
