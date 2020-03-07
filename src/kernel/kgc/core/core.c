/*
 * file:		kernel/kgc/core/core.c
 * auther:		Jason Hu
 * time:		2020/1/31
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* KGC-kernel graph core 内核图形核心 */

/* 系统内核 */
#include <book/config.h>


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
#include <kgc/handler.h>
#include <kgc/font/font.h>
#include <kgc/container/container.h>

#include <clock/clock.h>

#include <input/input.h>
#include <video/video.h>

/**
 * KGC_AllocBuffer - 分配缓冲区
 * @size: 缓冲区大小
 * 
 * 根据大小选择不同的分配方式
 * 
 * 成功返回缓冲区地址，失败返回NULL
 */
PUBLIC void *KGC_AllocBuffer(size_t size)
{
    if (size < MAX_MEM_CACHE_SIZE) {
        return kmalloc(size, GFP_KERNEL);
    } else {
        return vmalloc(size);    
    }
}

/**
 * KGC_KeyboardInput - 图形核心键盘输入
 */
PUBLIC void KGC_KeyboardInput(uint32_t key)
{
    /* 指向输入指针 */
    KGC_Even_t even;

    memset(&even, 0, sizeof(KGC_Even_t));
    /* 对原始数据进行处理，生成通用按键。按键按下，按键弹起，组合按键 */
    KGC_EvenKeyboardKey(key, &even.key);
    KGC_EvenHandler(&even);
}

/**
 * KGC_TimerOccur - 时钟产生
 * 
 */
PUBLIC void KGC_TimerOccur()
{
    /* 指向输入指针 */
    KGC_Even_t even;

    memset(&even, 0, sizeof(KGC_Even_t));
        
    /* 时钟处理 */
    KGC_EvenTimer(systicks, &even.timer);
    KGC_EvenHandler(&even);
            
}

/**
 * KGC_MouseInput - 处理鼠标输入
 * 
 */
PUBLIC void KGC_MouseInput(InputBufferMouse_t *packet)
{
    /* 指向输入指针 */
    KGC_Even_t even;
    /* 鼠标移动事件及其处理 */
    memset(&even, 0, sizeof(KGC_Even_t));
    KGC_EvenMouseMotion(packet->xIncrease, packet->yIncrease, packet->zIncrease, &even.motion);
    KGC_EvenHandler(&even);

    memset(&even, 0, sizeof(KGC_Even_t));
    /* 鼠标按钮事件及其处理 */
    KGC_EvenMouseButton(packet->button, &even.button);
    KGC_EvenHandler(&even);
}

/**
 * InitKGC() - 初始化内核图形核心
 * 
 */
PUBLIC int InitKGC()
{
#ifdef CONFIG_DISPLAY_GRAPH
    /* 初始化字体 */
    KGC_InitFont();    
    /* 初始化容器 */
    KGC_InitContainer();
#endif /* CONFIG_DISPLAY_GRAPH */
    return 0;
}
