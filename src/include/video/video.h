/*
 * file:		include/video/video.h
 * auther:		Jason Hu
 * time:		2020/2/10
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_VIDEO_H
#define _DRIVER_VIDEO_H

#include <lib/stdint.h>
#include <lib/types.h>

/* 图形设备IO的命令 */
enum VideoIoctls {
    VIDEO_IOCTL_GETINFO = 1,    /* 获取图形基础信息包 */
};

/* 视频设备标志 */
enum VideoDeviceFlags {
    VIDEO_FLAGS_INIT_DONE = 1,        /* 初始化完成标志 */
};

/* 单个像素由多少位组成 */
enum BitsPerPixel {
    BITS_PER_PIXEL_8    = 8,    /* 1像素8位 */
    BITS_PER_PIXEL_16   = 16,   /* 1像素16位 */
    BITS_PER_PIXEL_24   = 24,   /* 1像素24位 */
    BITS_PER_PIXEL_32   = 32,   /* 1像素32位 */
};

/* 视频信息 */
typedef struct VideoInfo {
    uint8_t bitsPerPixel;               /* 每个像素的位数 */
    uint32_t bytesPerScanLine;          /* 单行的字节数 */
    uint32_t xResolution, yResolution;  /* 分辨率x，y */
    uint32_t phyBasePtr;                /* 物理显存的指针 */
    
    uint32_t bytesScreenSize;           /* 整个屏幕占用的字节大小 */
    uint32_t screenSize;                /* 整个屏幕占用的大小（x*y） */
    uint8_t *virBasePtr;                /* 显存映射到内存中的地址 */
} VideoInfo_t;

EXTERN VideoInfo_t videoInfo;

/* 视频设备 */
typedef struct VideoDevice {
    struct List list;                   /* 视频设备链表 */
    struct CharDevice *chrdev;	        /* 字符设备 */
    VideoInfo_t info;                   /* 视频设备信息 */
    uint8_t flags;                      /* 驱动标志 */
    
    /* 设备操作 */
    void (*writePixel)(struct VideoDevice *, int , int, uint32_t );
    void (*readPixel)(struct VideoDevice *, int , int, uint32_t *);

    /* 设备操作 */
} VideoDevice_t;

PUBLIC int RegisterVideoDevice(VideoDevice_t *vdodev,
    dev_t devno,
    char *name,
    int count,
    void *private);

PUBLIC int UnregisterVideoDevice(VideoDevice_t *vdodev);

PUBLIC int InitVideoSystem();

/* 视频点 */
typedef struct VideoPoint {
    int x, y;   /* 坐标 */
    unsigned int color; /* 颜色 */
} VideoPoint_t;

/* 视频填充矩形 */
typedef struct VideoRect {
    VideoPoint_t point; /* 点 */
    int width, height;  /* 大小 */
} VideoRect_t;

/* 视频传输位图 */
typedef struct VideoBitmap {
    unsigned int *bitmap;   /* 位图 */
    int x, y;       /* 位图位置 */
    int width, height;      /* 大小 */
} VideoBitmap_t;

PUBLIC void VideoWritePixel(VideoPoint_t *point);
PUBLIC void VideoReadPixel(VideoPoint_t *point);
PUBLIC void VideoBlank(uint32_t color);
PUBLIC void VideoFillRect(VideoRect_t *rect);
PUBLIC void VideoBitmapBlit(VideoBitmap_t *bitmap);

#endif  /* _DRIVER_VIDEO_H */
