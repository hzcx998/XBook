/*
 * file:		kernel/kgc/core.c
 * auther:		Jason Hu
 * time:		2020/1/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* KGC-kernel graph core 内核图形核心 */

/* 系统内核 */
#include <book/config.h>

#ifdef CONFIG_DISPLAY_GRAPH

#include <book/arch.h>
#include <book/debug.h>
#include <book/device.h>
#include <book/memcache.h>
#include <book/vmarea.h>
#include <book/bitops.h>
#include <book/mutex.h>
#include <book/kgc.h>

#include <lib/string.h>
#include <lib/math.h>

/* 图形系统 */
#include <kgc/draw.h>
#include <kgc/even.h>
#include <kgc/button.h>
#include <kgc/input.h>
#include <kgc/handler.h>
#include <kgc/video.h>

struct KGC_InOutDevice {
    dev_t keyboard;     /* [in]  键盘 */ 
    dev_t mouse;        /* [in]  鼠标 */ 
    dev_t screen;       /* [out] 屏幕 */
};

struct KGC_Private {
    uint32_t bytesScreenSize;       /* 以字节单位的屏幕大小 */
    uint32_t pixelScreenSize;       /* 以像素为单位的屏幕大小 */
    
    uint8_t *txBuffer;              /* 图形缓冲区，用于数据传输中转 */
    
    struct KGC_InOutDevice device; /* 输入输出设备 */
    Mutex_t txLock;                 /* 传输锁，数据传输时需要上锁，保证数据不被破坏 */
    KGC_Even_t even;             /* 用来获取图形输入信息 */
    KGC_InputMousePacket_t mousePackets[MAX_MOUSE_PACKET_NR];
};

/* 图形系统实体 */
PUBLIC struct KGC_Private kgcPrivate;
/* 指向图形系统的指针 */
PRIVATE struct KGC_Private *this;

PUBLIC KGC_VideoInfo_t videoInfo;     /* 视频信息 */

/**
 * KGC_PollInput - 轮询获取输入
 * 
 * @return: 有数据则返回1，无数据则返回0
 */
PRIVATE int KGC_PollEven()
{
    /* 指向输入指针 */
    KGC_Even_t *even = &this->even;
    KGC_InputMousePacket_t *packet = this->mousePackets;
    int retval = 0;
    uint32_t key = 0;
    uint32_t count = 0;
    /* 事件处理 */
    if (!DeviceRead(this->device.keyboard, 0, &key, sizeof(key))) {
        retval = 1;
        /* 对原始数据进行处理，生成通用按键。按键按下，按键弹起，组合按键 */
        KGC_EvenKeyboardKey(key, &even->key);
        KGC_EvenHandler(even);
    }
    if (!DeviceRead(this->device.mouse, 0, &packet[0], (unsigned int)&count)) {
        retval = 1;
        /* 处理每一个数据包 */
        while (count--) {
            /* 鼠标移动事件及其处理 */
            KGC_EvenMouseMotion(packet[count].xIncrease, packet[count].yIncrease, packet[count].zIncrease, &even->motion);
            KGC_EvenHandler(even);
            /* 鼠标按钮事件及其处理 */
            KGC_EvenMouseButton(packet[count].button, &even->button);
            KGC_EvenHandler(even);
        } 
    }
    return retval;
}

/**
 * KGC_Task - 图形处理任务
 * @arg: 参数
 */
PRIVATE void KGC_Task(void *arg)
{
    while (1) {
        /* 输入设备轮训 */
        if (KGC_PollEven()) {
            /* 把获取到的数据发送到交互界面（桌面，窗口，输入框...） */
            /* 有事件触发 */
        }
    }
}

/**
 * SysGraphWrite - 保留的系统调用函数
 * 
 */
PUBLIC void SysGraphWrite(int offset, int size, void *buffer)
{
    KGC_CoreDraw(offset, size, buffer, 0);
}

/** 
 * KGC_CoreDraw - 核心绘制
 * @position: 位置（x << 16 | y）
 * @area: 区域（width << 16 | height）
 * @buffer: 数据缓冲区，为NULL时，color生效，否则使用buffer里面的数据
 * @color: 颜色
 * 
 * 提供2套方案：
 * 1.直接绘制缓冲区里面的数据到传输缓冲区
 * 2.通过给定参数，往传输缓冲区里面绘制数据
 */
PUBLIC void KGC_CoreDraw(uint32_t position, uint32_t area, void *buffer, uint32_t color)
{
    /* 如果有缓冲区，就绘制缓冲区里面的内容，不然就绘制单色 */
    if (buffer) {
        MutexLock(&this->txLock);
        /* copy to kernel */
        memcpy(this->txBuffer, buffer, HIGH16(area) * LOW16(area) << 2);

        /* 传输数据给图形驱动程序 */
        DeviceWrite(this->device.screen, position, this->txBuffer, area);
        
        MutexUnlock(&this->txLock);
    } else {
        uint32_t *p = (uint32_t *)this->txBuffer;
        
        uint32_t length = HIGH16(area) * LOW16(area);
        //printk(">>>draw\n");
        MutexLock(&this->txLock);
        while (length--) {
            *p++ = color;
        }
        /* 传输数据给图形驱动程序 */
        DeviceWrite(this->device.screen, position, this->txBuffer, area);
        
        MutexUnlock(&this->txLock);
    }
}

/**
 * InitKGCInfo - 初始化基础信息
 * 
 */
PRIVATE int InitKGCInfo()
{
    /* 设置this指针指向系统数据 */
    this = &kgcPrivate;

    /* 初始化输出设备 */
    this->device.keyboard   = DEV_TTY3;
    this->device.mouse      = DEV_MOUSE;
    this->device.screen     = DEV_VIDEO;
    
    /* 初始化互斥锁 */
    MutexInit(&this->txLock);

    return 0;
}

/**
 * KGC_OpenDevice - 打开对应的设备
 * 
 */
PRIVATE int KGC_OpenDevice()
{
    
    KGC_VideoInfo_t *video = &videoInfo;
    
    /* 输出设备 */
    if (DeviceOpen(this->device.screen, 0)) {
        printk("open graph device failed!\n");
        return -1;
    }

    /* 获取设备基础信息 */
    DeviceIoctl(this->device.screen, GRAPH_IO_BASE_PACKT, (int)video);
    
    /* 计算参数 */
    this->bytesScreenSize = video->bytesPerScanLine * video->yResolution;
    this->pixelScreenSize = video->xResolution * video->yResolution;
    
    /* 根据不同的缓冲区大小分配缓冲区内存 */
    if (this->bytesScreenSize <= MAX_MEM_CACHE_SIZE) {
        printk("alloc buffer with kmalloc.\n");
        this->txBuffer = kmalloc(this->bytesScreenSize, GFP_KERNEL);
        if (this->txBuffer == NULL) {
            printk("kmalloc for graph system buffer failed!");
            return -1;
        }
        
    } else {
        printk("alloc buffer with vmalloc.\n");
        
        /* 超过kmalloc可分配的最大内存，使用vmalloc分配 */
        this->txBuffer = vmalloc(this->bytesScreenSize);
        if (this->txBuffer == NULL) {
            printk("vmalloc for graph system buffer failed!");
            return -1;
        }
    }
    /* 绘制黑色背景 */
    KGC_DrawRectangle(0, 0, videoInfo.xResolution, videoInfo.yResolution, KGCC_BLACK);

    /* 打开键盘设备 */
    if (DeviceOpen(this->device.keyboard, 0)) {
        printk("graph open keyboard device failed!\n");
        return -1;
    }
    /* 打开鼠标设备 */
    if (DeviceOpen(this->device.mouse, 0)) {
        printk("graph open mouse device failed!\n");
        return -1;
    }

    return 0;
}

PRIVATE void KGC_Test()
{
    
}
#endif /* CONFIG_DISPLAY_GRAPH */

/**
 * InitKGC() - 初始化内核图形核心
 * 
 */
PUBLIC int InitKGC()
{
#ifdef CONFIG_DISPLAY_GRAPH
    
    /* 初始化上半部分 */
    InitKGCInfo();
    
    if (KGC_OpenDevice()) {
        printk("KGC open device failed!\n");
        return -1;
    }

    /* 打开一个任务来处理输入设备 */
    ThreadStart("graph", 3, KGC_Task, NULL);

    KGC_Test();

#endif /* CONFIG_DISPLAY_GRAPH */
    //while (1);  
    return 0;
}
