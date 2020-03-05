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
#include <book/vmarea.h>
#include <book/bitops.h>
#include <book/device.h>
#include <book/spinlock.h>

#include <video/video.h>
#include <lib/string.h>
#include <kgc/color.h>

/* 生成5-6-5格式的像素 */
#define ARGB_TO_16(r, g, b) ((((r >> 3) & 0x1f) << 11)   | \
    (((g >> 2)& 0x3f) << 5)   | ((b >> 3) & 0x1f ))

/* ----驱动程序初始化文件导入---- */
EXTERN int InitVesaDriver();
/* ----驱动程序初始化文件导入完毕---- */

/* 当前视频设备设备号 */
PRIVATE dev_t videoDevno;

/* 指向当前显示的视频设备 */
PRIVATE struct VideoDevice *videoDeviceMaster;

/* 视频设备队列 */
PRIVATE LIST_HEAD(videoDeviceListHead);

/* 视频锁，当设定视频信息的时候，需要用锁来保护环境 */
PRIVATE SPIN_LOCK_INIT(videoLock);

/* 视频信息 */
PUBLIC VideoInfo_t videoInfo;

PRIVATE void VideoWritePixel8(struct VideoDevice *vdodev, int x, int y, uint32_t value)
{
    /* 根据像素大小选择对应得像素写入方法 */
    uint8_t *p;
    /* 获取像素位置 */
    p = vdodev->info.virBasePtr + (y * vdodev->info.xResolution + x);

    /* 写入像素数据 */
    *p = LOW8(value);
}

/**
 * VideoWritePixe16 - 写入单个像素16位
 * @value: 像素值，ARGB格式
 * 可以先根据像素宽度设置一个函数指针，指向对应的操作函数，就不用每次都进行判断了。
 */
PRIVATE void VideoWritePixe16(struct VideoDevice *vdodev, int x, int y, uint32_t value)
{
    /* 根据像素大小选择对应得像素写入方法 */
    uint8_t *p;
    /* 获取像素位置 */
    p = vdodev->info.virBasePtr + ((y * vdodev->info.xResolution + x) * 2); // 左位移1相当于乘以2

    /* 写入像素数据 */
    *(uint16_t *)p = ARGB_TO_16((value >> 16) , (value >> 8), (value));
}

/**
 * VideoWritePixe24 - 写入单个像素24位
 * @value: 像素值，ARGB格式
 * 可以先根据像素宽度设置一个函数指针，指向对应的操作函数，就不用每次都进行判断了。
 */
PRIVATE void VideoWritePixe24(struct VideoDevice *vdodev, int x, int y, uint32_t value)
{
    /* 根据像素大小选择对应得像素写入方法 */
    uint8_t *p;
    /* 获取像素位置 */
    p = vdodev->info.virBasePtr + (y * vdodev->info.xResolution + x)*3;
    
    /* 写入像素数据 */
    *(uint16_t *)p = LOW16(value);
    p += 2;
    *p = LOW8(HIGH16(value));

}

/**
 * VideoWritePixe32 - 写入单个像素32位
 * @value: 像素值，ARGB格式
 * 可以先根据像素宽度设置一个函数指针，指向对应的操作函数，就不用每次都进行判断了。
 */
PRIVATE void VideoWritePixe32(struct VideoDevice *vdodev, int x, int y, uint32_t value)
{
    /* 根据像素大小选择对应得像素写入方法 */
    uint8_t *p;
    
    /* 获取像素位置 */
    p = vdodev->info.virBasePtr + ((y * vdodev->info.xResolution + x) << 2); // 左移2位相当于乘以4
    
    /* 写入像素数据 */
    *(uint32_t *)p = value;
}


/**
 * VideoReadPixe8 - 读取单个像素8位
 * @value: 像素值，储存格式
 */
PRIVATE void VideoReadPixel8(struct VideoDevice *vdodev, int x, int y, uint32_t *value)
{
    /* 根据像素大小选择对应得像素写入方法 */
    uint8_t *p;
    /* 获取像素位置 */
    p = vdodev->info.virBasePtr + (y * vdodev->info.xResolution + x);

    *value = *p;
}

/**
 * VideoReadPixel16 - 读取单个像素16位
 * @value: 像素值，储存格式
 */
PRIVATE void VideoReadPixel16(struct VideoDevice *vdodev, int x, int y, uint32_t *value)
{
    /* 根据像素大小选择对应得像素写入方法 */
    uint8_t *p;
    /* 获取像素位置 */
    p = vdodev->info.virBasePtr + ((y * vdodev->info.xResolution + x) * 2); // 左位移1相当于乘以2

    *value = *(uint16_t *)p;
}

/**
 * VideoReadPixel24 - 读取单个像素24位
 * @value: 像素值，储存格式
 */
PRIVATE void VideoReadPixel24(struct VideoDevice *vdodev, int x, int y, uint32_t *value)
{
    /* 根据像素大小选择对应得像素写入方法 */
    uint8_t *p;
    /* 获取像素位置 */
    p = vdodev->info.virBasePtr + (y * vdodev->info.xResolution + x)*3;
    
    *value = *(uint16_t *)p;
    p += 2;
    *value += *p << 16;
}

/**
 * VideoReadPixel32 - 读取单个像素32位
 * @value: 像素值，储存格式
 */
PRIVATE void VideoReadPixel32(struct VideoDevice *vdodev, int x, int y, uint32_t *value)
{
    /* 根据像素大小选择对应得像素写入方法 */
    uint8_t *p;
    
    /* 获取像素位置 */
    p = vdodev->info.virBasePtr + ((y * vdodev->info.xResolution + x) << 2); // 左移2位相当于乘以4
    
    *value = *(uint32_t *)p;
}

/**
 * VideoWritePixel - 视频写像素
 * @x: 横坐标
 * @y: 纵坐标
 * @color: 颜色
 */
PUBLIC void VideoWritePixel(VideoPoint_t *point)
{
    if (videoDeviceMaster->writePixel) {
        /* 在屏幕范围内才绘制 */
        if (point->x >= 0 && point->y >= 0 && point->x < videoDeviceMaster->info.xResolution && 
            point->y < videoDeviceMaster->info.yResolution) {
            /* 绘制像素 */
            videoDeviceMaster->writePixel(videoDeviceMaster, point->x, point->y, point->color);
        }
    }
}

/**
 * VideoWritePixel - 视频写像素
 * @x: 横坐标
 * @y: 纵坐标
 * @color: 颜色
 */
PUBLIC void VideoReadPixel(VideoPoint_t *point)
{
    if (videoDeviceMaster->readPixel) {
        /* 在屏幕范围内才绘制 */
        if (point->x >= 0 && point->y >= 0 && point->x < videoDeviceMaster->info.xResolution && 
            point->y < videoDeviceMaster->info.yResolution) {
            /* 绘制像素 */
            videoDeviceMaster->readPixel(videoDeviceMaster, point->x, point->y, &point->color);
        }
    }
}

/**
 * VideoBlank - 视频填充空白
 * @color: 颜色
 */
PUBLIC void VideoBlank(uint32_t color)
{
    if (videoDeviceMaster->writePixel) {
        int x, y;
        for (y = 0; y < videoInfo.yResolution; y++) {
            for (x = 0; x < videoInfo.xResolution; x++) {
                /* 在屏幕范围内才绘制 */
                if (x >= 0 && y >= 0 && x < videoDeviceMaster->info.xResolution && 
                    y < videoDeviceMaster->info.yResolution) {
                    /* 绘制像素 */
                    videoDeviceMaster->writePixel(videoDeviceMaster, x, y, color);
                }
            }
        }
    }
}

/**
 * VideoFillRect - 视频填充矩形
 * @rect: 矩形
 */
PUBLIC void VideoFillRect(VideoRect_t *rect)
{
    if (videoDeviceMaster->writePixel) {
        int x, y;
        VideoPoint_t *point = &rect->point;
        for (y = 0; y < rect->height; y++) {
            for (x = 0; x < rect->width; x++) {
                /* 在屏幕范围内才绘制 */
                if (x + point->x >= 0 && y + point->y >= 0 && 
                    x + point->x < videoDeviceMaster->info.xResolution &&
                    y + point->x< videoDeviceMaster->info.yResolution) {
                    /* 绘制像素 */
                    videoDeviceMaster->writePixel(videoDeviceMaster, 
                        point->x + x, point->y + y, point->color);
                }
            }
        }
    }
}

/**
 * VideoBitmapBlit - 视频传输位图
 * @bitmap: 位图
 */
PUBLIC void VideoBitmapBlit(VideoBitmap_t *bitmap)
{
    if (videoDeviceMaster->writePixel) {
        int x, y;
        for (y = 0; y < bitmap->height; y++) {
            for (x = 0; x < bitmap->width; x++) {
                /* 在屏幕范围内才绘制 */
                if (x + bitmap->x >= 0 && y + bitmap->y >= 0 && 
                    x + bitmap->x < videoDeviceMaster->info.xResolution &&
                    y + bitmap->x< videoDeviceMaster->info.yResolution) {
                    /* 绘制像素 */
                    videoDeviceMaster->writePixel(videoDeviceMaster, 
                        bitmap->x + x, bitmap->y + y, bitmap->bitmap[y * bitmap->width + x]);
                }
            }
        }
    }
}


/**
 * GetVideoDevice - 获取一个视频设备
 * @devno: 设备号
 * 
 * 返回视频设备
 */
PUBLIC VideoDevice_t *GetVideoDevice(dev_t devno)
{
    VideoDevice_t *device;

    ListForEachOwner (device, &videoDeviceListHead, list) {
        if (device->chrdev->super.devno == devno) {
            return device;
        }
    }
    return NULL;
}
PRIVATE void SetVideoDevice(dev_t devno)
{
    unsigned long flags = SpinLockIrqSave(&videoLock);
    
    VideoDevice_t *vdodev = GetVideoDevice(devno);

    if (vdodev)
        videoDeviceMaster = vdodev;

    SpinUnlockIrqSave(&videoLock, flags);
}

PRIVATE void SetVideoInfo(dev_t devno)
{
    unsigned long flags = SpinLockIrqSave(&videoLock);
    
    VideoDevice_t *vdodev = GetVideoDevice(devno);

    /* 从主设备获取设备信息 */
    memcpy(&videoInfo, &vdodev->info, sizeof(VideoInfo_t));
    
    SpinUnlockIrqSave(&videoLock, flags);
}

PUBLIC int RegisterVideoDevice(VideoDevice_t *vdodev,
    dev_t devno,
    char *name,
    int count,
    void *private)
{
    if (!vdodev)
        return -1;

    /* 计算屏幕字节大小 */
    vdodev->info.bytesScreenSize = vdodev->info.bytesPerScanLine * vdodev->info.yResolution;
    
    /* 设置函数处理指针 */
    switch (vdodev->info.bitsPerPixel)
    {
    case BITS_PER_PIXEL_8:
        vdodev->writePixel = VideoWritePixel8;
        vdodev->readPixel = VideoReadPixel8;
            
        break;
    case BITS_PER_PIXEL_16:
        vdodev->writePixel = VideoWritePixe16;    
        vdodev->readPixel = VideoReadPixel16;
        
        break;
    case BITS_PER_PIXEL_24:
        vdodev->writePixel = VideoWritePixe24;    
        vdodev->readPixel = VideoReadPixel24;
        
        break;
    case BITS_PER_PIXEL_32:
        vdodev->writePixel = VideoWritePixe32;    
        vdodev->readPixel = VideoReadPixel32;
        
        break;
    default:
        vdodev->writePixel = NULL;    
        vdodev->readPixel = NULL;
        
        break;
    }

    /* 对设备的显存进行IO映射到虚拟地址，就可以访问该地址来访问显存 */
    vdodev->info.virBasePtr = IoRemap(vdodev->info.phyBasePtr, vdodev->info.bytesScreenSize);
    if (vdodev->info.virBasePtr == NULL) {
        printk("IO remap for video driver failed!\n");
        return -1;
    }

    vdodev->chrdev = AllocCharDevice(devno);
	if (vdodev->chrdev == NULL) {
		printk(PART_ERROR "alloc char device for input failed!\n");
        IoUnmap(vdodev->info.virBasePtr);
		return -1;
	}
	/* 初始化字符设备信息 */
	CharDeviceInit(vdodev->chrdev, count, private);
	CharDeviceSetup(vdodev->chrdev, NULL);

	CharDeviceSetName(vdodev->chrdev, name);

    /* 把字符设备添加到系统 */
	AddCharDevice(vdodev->chrdev);

    /* 添加到视频设备队列 */
    ListAddTail(&vdodev->list, &videoDeviceListHead);

    vdodev->flags = VIDEO_FLAGS_INIT_DONE;
    return 0;
}

PUBLIC int UnregisterVideoDevice(VideoDevice_t *vdodev)
{
    if (!vdodev)
        return -1;
    
    if (vdodev->info.virBasePtr)
        IoUnmap(vdodev->info.virBasePtr);
    
	DelCharDevice(vdodev->chrdev);
    FreeCharDevice(vdodev->chrdev);

    /* 删除链表 */
    ListDel(&vdodev->list);
    return 0;
}

PRIVATE void InitVideoDrivers()
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
    InitVideoDrivers();

    /* 视频设备 */
    videoDevno = DEV_VIDEO;

    SetVideoDevice(videoDevno);
    SetVideoInfo(videoDevno);
    
#ifdef CONFIG_DISPLAY_GRAPH
    InitKGC();
#endif /* CONFIG_DISPLAY_GRAPH */

    return 0;
}
