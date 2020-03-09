/*
 * file:		kernel/kgc/window/draw.c
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */
/* 系统内核 */
#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <video/video.h>
#include <kgc/container/draw.h>
#include <kgc/window/window.h>
#include <kgc/window/draw.h>
#include <kgc/window/message.h>
#include <kgc/window/switch.h>
#include <kgc/color.h>
#include <kgc/bar/taskbar.h>

EXTERN KGC_WindowSkin_t windowSkin;


/* 图标数据 */
PRIVATE uint8_t iconRawData[16 * 16] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,
    0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,
    0,0,0,1,0,1,0,0,0,0,1,0,0,0,0,0,
    0,0,0,1,1,0,0,0,0,0,0,1,0,0,0,0,
    0,0,0,1,0,0,0,0,0,0,0,0,1,0,0,0,
    0,0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,
    0,1,1,1,0,0,1,1,1,1,0,0,1,1,1,0,
    0,0,0,1,0,1,0,0,0,0,1,0,1,0,0,0,
    0,0,0,1,0,1,0,0,0,0,1,0,1,0,0,0,
    0,0,0,1,0,0,0,0,0,0,1,0,1,0,0,0,
    0,0,0,1,0,0,0,0,0,0,1,0,1,0,0,0,
    0,0,0,1,0,0,0,0,0,0,1,0,1,0,0,0,
    0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};


/* 最小化按钮数据 */
PRIVATE uint8_t minimRawData[16 * 16] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,
    0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,
    0,0,1,0,0,0,0,0,1,1,1,1,1,0,0,0,
    0,0,1,0,0,0,0,0,1,0,1,0,1,0,0,0,
    0,0,0,1,1,1,1,1,1,1,1,0,1,0,0,0,
    0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,
    0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};


/* 最大化按钮数据 */
PRIVATE uint8_t maximRawData[16 * 16] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,
    0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,
    0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,
    0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,
    0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,
    0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,
    0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,
    0,0,1,1,1,1,1,0,0,0,0,0,1,0,0,0,
    0,0,1,0,1,0,1,0,0,0,0,0,1,0,0,0,
    0,0,1,0,1,1,1,1,1,1,1,1,0,0,0,0,
    0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,
    0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

/* 关闭按钮数据 */
PRIVATE uint8_t closeRawData[26 * 26] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
    0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
    0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,
    0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,0,
    0,1,1,0,0,0,1,1,1,0,0,0,1,1,0,0,
    1,1,1,1,0,0,0,1,0,0,0,1,1,1,1,0,
    1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,0,
    1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,
    1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,0,
    1,1,1,1,0,0,0,1,0,0,0,1,1,1,1,0,
    0,1,1,0,0,0,1,1,1,0,0,0,1,1,0,0,
    0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,0,
    0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,
    0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,
    0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,
};


/**
 * KGC_WindowDrawPixel - 往窗口中绘制像素
 * @window: 窗口
 * @x: 窗口中的横坐标
 * @y: 窗口中的纵坐标
 * @color: 颜色
 * 
 */
PUBLIC void KGC_WindowDrawPixel(
    KGC_Window_t *window, 
    int x, 
    int y, 
    uint32_t color)
{
    /* 判断是否在窗口内 */
    if (x < 0 || x >= window->width || y < 0 || y >= window->height)
        return;
    /* 直接绘制到视图中 */
    KGC_ContainerWritePixel(window->container, window->x + x, window->y + y, color);
}

/**
 * KGC_WindowDrawRectangle - 往窗口中绘制矩形
 * @window: 窗口
 * @left: 窗口中的横坐标
 * @top: 窗口中的纵坐标
 * @width: 矩形的宽度
 * @height: 矩形的高度
 * @color: 颜色
 * 
 */
PUBLIC void KGC_WindowDrawRectangle(
    KGC_Window_t *window, 
    int left, 
    int top, 
    int width, 
    int height, 
    uint32_t color)
{
    /* 进行窗口剪切 */
    KGC_ContainerDrawRectangle(window->container, window->x + left, window->y + top,
        width, height, color);
}

/**
 * KGC_WindowDrawBitmap - 往窗口中绘制位图
 * @window: 窗口
 * @left: 窗口中的横坐标
 * @top: 窗口中的纵坐标
 * @width: 位图的宽度
 * @height: 位图的高度
 * @bitmap: 位图地址
 * 
 */
PUBLIC void KGC_WindowDrawBitmap(
    KGC_Window_t *window, 
    int left,
    int top, 
    int width, 
    int height, 
    uint32_t *bitmap)
{
    /* 进行窗口剪切 */
    KGC_ContainerDrawBitmap(window->container, window->x + left, window->y + top,
        width, height, bitmap);
}

/**
 * KGC_WindowDrawLine - 绘制直线
 * @window: 窗口
 * @x0: 起始横坐标
 * @y0: 起始纵坐标
 * @x1: 结束横坐标
 * @y1: 结束纵坐标
 * @color: 颜色
 */
PUBLIC void KGC_WindowDrawLine(
    KGC_Window_t *window,
    int x0,
    int y0,
    int x1,
    int y1, 
    uint32_t color)
{
    KGC_ContainerDrawLine(window->container, window->x + x0, window->y + y0,
        window->x + x1, window->y + y1, color);
}

/**
 * KGC_WindowDrawChar - 绘制字符
 * @window: 窗口
 * @x: 起始横坐标
 * @y: 起始纵坐标
 * @ch: 字符
 * @color: 颜色
 */
PUBLIC void KGC_WindowDrawChar(
    KGC_Window_t *window,
    int x,
    int y,
    char ch,
    uint32_t color)
{
    KGC_ContainerDrawChar(window->container, window->x + x, window->y + y, ch, color);
}

/**
 * KGC_WindowDrawString - 绘制字符串
 * @window: 窗口
 * @x: 起始横坐标
 * @y: 起始纵坐标
 * @str: 字符串
 * @color: 颜色
 */
PUBLIC void KGC_WindowDrawString(
    KGC_Window_t *window,
    int x,
    int y,
    char *str,
    uint32_t color)
{
    KGC_ContainerDrawString(window->container, window->x + x, window->y + y, str, color);
}

/**
 * KGC_WindowRefresh - 绘制直线
 * @window: 窗口
 * @x0: 起始横坐标
 * @y0: 起始纵坐标
 * @x1: 结束横坐标
 * @y1: 结束纵坐标
 * @color: 颜色
 * 
 * 刷新时，需要刷新上面的鼠标
 */
PUBLIC void KGC_WindowRefresh(
    KGC_Window_t *window,
    int left,
    int top,
    int right,
    int bottom)
{
    /* 如果不是当前窗口，就不刷新到屏幕上 */
    /*if (window != GET_CURRENT_WINDOW())
        return;*/
    
    KGC_ContainerRefresh(window->container, window->x + left, window->y + top,
        window->x + right, window->y + bottom);
}

PUBLIC void WindowButtonCloseHandler(KGC_Button_t *button)
{
    /* 给按钮对应的窗口发送消息 */
    KGC_Window_t *window = button->label.widget.container->private;
    
    /* 发送退出信号 */
    if (window->task) {
        ForceSignal(SIGTERM, ((Task_t *)window->task)->pid);
    }
    
    /* 给当前窗口发送关闭窗口消息 */
    if (GET_CURRENT_WINDOW() == window) {       
        KGC_MessageNode_t *node = KGC_CreateMessageNode();
        if (node) {
            /* 发出退出事件 */
            node->message.type = KGC_MSG_QUIT;
            KGC_AddMessageNode(node, window);
            //printk("send close to %s\n", GET_CURRENT_WINDOW()->title);
        }
    }
}


PUBLIC void WindowButtonMinimHandler(KGC_Button_t *button)
{
    /* 给按钮对应的窗口发送消息 */
    KGC_Window_t *window = button->label.widget.container->private;
    if (window) {
        /* 隐藏当前窗口 */
        KGC_ContainerZ(window->container, -1);
        
        /* 窗口隐藏后，不指向窗口。 */
        currentWindow = NULL;
        
        /* 切换到最顶层的窗口 */
        if (KGC_SwitchTopWindow() == -1) {
            /* 没有顶层窗口了，激活窗口控制 */
            KGC_WindowControllerActive(NULL);
        }
    }
}

PUBLIC void WindowButtonMaximHandler(KGC_Button_t *button)
{
    /* 给按钮对应的窗口发送消息 */

}

/**
 * KGC_WindowPaint - 绘制窗口
 * 
 * 窗口创建时，需要绘制一个窗口，可以根据参数进行调整
 */
PUBLIC void KGC_WindowPaint(KGC_Window_t *window)
{
    KGC_Container_t *container = window->container;

    /* 控件位置 */
    int widgetLeft, widgetRight, widgetTop;
    
    /* 边框和标题栏都有 */
    if ((window->style & KGC_WINSTYPLE_CAPTION) && (window->style & KGC_WINSTYPLE_BORDER)) {
        /* 先绘制边框 */
        KGC_ContainerDrawRectangle(container, 0, 0, container->width, 1, windowSkin.boderColor);
        KGC_ContainerDrawRectangle(container, 0, 0, 1, container->height, windowSkin.boderColor);
        KGC_ContainerDrawRectangle(container, container->width - 1, 0, 1, container->height, windowSkin.boderColor);
        KGC_ContainerDrawRectangle(container, 0, container->height - 1, container->width, 1, windowSkin.boderColor);
        
        KGC_ContainerDrawRectangle(container, 1, 1, container->width - 2, 24, windowSkin.captionColor);
        
        KGC_ContainerDrawRectangle(container, 1, 1 + 24, container->width - 2, container->height - (2 + 24), windowSkin.blankColor);
        
        widgetLeft = 1;
        widgetTop = 1;
        widgetRight = container->width - 1;
        
    } else if ((window->style & KGC_WINSTYPLE_CAPTION)) {
        KGC_ContainerDrawRectangle(container, 1, 1, container->width - 2, 24, windowSkin.captionColor);

        KGC_ContainerDrawRectangle(container, 0, 24, container->width, container->height - 24, windowSkin.blankColor);
        widgetLeft = 0;
        widgetTop = 0;
        widgetRight = container->width - 1;
        
    } else if ((window->style & KGC_WINSTYPLE_BORDER)) {
        KGC_ContainerDrawRectangle(container, 0, 0, container->width, 1, windowSkin.boderColor);
        KGC_ContainerDrawRectangle(container, 0, 0, 1, container->height, windowSkin.boderColor);
        KGC_ContainerDrawRectangle(container, container->width - 1, 0, 1, container->height, windowSkin.boderColor);
        KGC_ContainerDrawRectangle(container, 1, container->height - 1, container->width, 1, windowSkin.boderColor);
        
        KGC_ContainerDrawRectangle(container, 1, 1, container->width - 2, container->height - 2, windowSkin.blankColor);
    } else {
        KGC_ContainerDrawRectangle(container, 0, 0, container->width, container->height, windowSkin.blankColor);    
    }

    /* 绘制控件 */
    if (window->style & KGC_WINSTYPLE_CAPTION) {    /* 有标题栏才添加 */
        
        KGC_WindowWidget_t *widgets = &window->widgets;
        /* 默认添加图标和关闭按钮 */
        
        widgets->icon = KGC_CreateLabel();
        if (widgets->icon) {
            KGC_LabelSetLocation(widgets->icon, widgetLeft, widgetTop);
            KGC_LabelSetSize(widgets->icon, 24, 24);
            KGC_LabelSetImage(widgets->icon, 16, 16, iconRawData, KGCC_WHITE, KGCC_BLACK);
            KGC_LabelSetImageAlign(widgets->icon, KGC_WIDGET_ALIGN_CENTER);
            KGC_LabelSetName(widgets->icon, "icon");
            KGC_LabelAdd(widgets->icon, container); /* 添加到窗口的容器上 */
            KGC_LabelShow(&widgets->icon->widget);
        }
        
        widgets->close = KGC_CreateButton();
        if (widgets->close) {
            widgets->close->label.textAlign = KGC_WIDGET_ALIGN_CENTER; 
            KGC_ButtonSetLocation(widgets->close, widgetRight - 24, widgetTop);
            KGC_ButtonSetSize(widgets->close, 24, 24);
            KGC_ButtonSetImage(widgets->close, 16, 16, closeRawData, KGCC_WHITE, KGCC_BLACK);
            KGC_ButtonSetImageAlign(widgets->close, KGC_WIDGET_ALIGN_CENTER);
            KGC_ButtonSetName(widgets->close, "close");
            KGC_ButtonAdd(widgets->close, container);
            KGC_ButtonSetHandler(widgets->close, &WindowButtonCloseHandler);
            KGC_ButtonShow(&widgets->close->label.widget);
        }

        widgets->maxim = KGC_CreateButton();
        if (widgets->maxim) { 
            widgets->maxim->label.textAlign = KGC_WIDGET_ALIGN_CENTER; 
            KGC_ButtonSetLocation(widgets->maxim, widgetRight - 24*2, widgetTop);
            KGC_ButtonSetSize(widgets->maxim, 24, 24);
            KGC_ButtonSetImage(widgets->maxim, 16, 16, maximRawData, KGCC_WHITE, KGCC_BLACK);
            KGC_ButtonSetImageAlign(widgets->maxim, KGC_WIDGET_ALIGN_CENTER);
            KGC_ButtonSetName(widgets->maxim, "max");
            KGC_ButtonAdd(widgets->maxim, container);
            KGC_ButtonSetHandler(widgets->maxim, &WindowButtonMaximHandler);
            
            if (!(window->style & KGC_WINSTYPLE_MAXIMIZEBOX)) {
                widgets->maxim->label.disabel = 1;
            }

            KGC_ButtonShow(&widgets->maxim->label.widget);
        }
        
        widgets->minim = KGC_CreateButton();
        if (widgets->minim) {
            widgets->minim->label.textAlign = KGC_WIDGET_ALIGN_CENTER; 
            KGC_ButtonSetLocation(widgets->minim, widgetRight - 24*3, widgetTop);
            KGC_ButtonSetSize(widgets->minim, 24, 24);
            KGC_ButtonSetName(widgets->minim, "min");
            KGC_ButtonSetImage(widgets->minim, 16, 16, minimRawData, KGCC_WHITE, KGCC_BLACK);
            KGC_ButtonSetImageAlign(widgets->minim, KGC_WIDGET_ALIGN_CENTER);
            KGC_ButtonAdd(widgets->minim, container);
            KGC_ButtonSetHandler(widgets->minim, &WindowButtonMinimHandler); 
            
            if (!(window->style & KGC_WINSTYPLE_MINIMIZEBOX)) {
                widgets->minim->label.disabel = 1;
            }

            KGC_ButtonShow(&widgets->minim->label.widget);
        }
        widgets->title = KGC_CreateLabel();
        if (widgets->title) {
            widgets->title->textAlign = KGC_WIDGET_ALIGN_LEFT; 
            KGC_LabelSetLocation(widgets->title, widgetLeft + 24, widgetTop);
            KGC_LabelSetSize(widgets->title, container->width - 24*4 - 2, 24);
            KGC_LabelSetText(widgets->title, window->title);
            KGC_LabelSetName(widgets->title, "title");
            KGC_LabelAdd(widgets->title, container);
            KGC_LabelShow(&widgets->title->widget);        
        }
    }
}


/**
 * KGC_WindowPaintActive - 绘制激活窗口
 * @window: 要绘制的窗口
 * @active: 激活
 * 
 */
PUBLIC void KGC_WindowPaintActive(KGC_Window_t *window, int active)
{
    KGC_Container_t *container = window->container;
    uint32_t border;
    uint32_t caption;
    
    if (active) {
        border = windowSkin.boderActiveColor;    
        caption = windowSkin.boderActiveColor;
    } else {
        border = windowSkin.boderColor;     
        caption = windowSkin.captionColor;
    }
    /* 将标题栏的控件的背景色都设置为激活状态颜色 */
    if (window->style & KGC_WINSTYPLE_CAPTION) {
        window->widgets.icon->backColor = caption;
        KGC_LabelShow(&window->widgets.icon->widget);
        
        window->widgets.title->backColor = caption;
        KGC_LabelShow(&window->widgets.title->widget);

        window->widgets.close->label.backColor = caption;
        window->widgets.close->defaultColor = caption;
        
        KGC_LabelShow(&window->widgets.close->label.widget);
        
        window->widgets.maxim->label.backColor = caption;
        window->widgets.maxim->defaultColor = caption;
        
        KGC_LabelShow(&window->widgets.maxim->label.widget);
        
        window->widgets.minim->label.backColor = caption;
        window->widgets.minim->defaultColor = caption;
        
        KGC_LabelShow(&window->widgets.minim->label.widget);
        
    }
    KGC_ContainerDrawRectanglePlus(container, 0, 0, container->width, 1, border);
    KGC_ContainerDrawRectanglePlus(container, 0, 0, 1, container->height, border);
    KGC_ContainerDrawRectanglePlus(container, container->width - 1, 0, 1, container->height, border);
    KGC_ContainerDrawRectanglePlus(container, 1, container->height - 1, container->width, 1, border);
}


/**
 * KGC_WindowPaintMoving - 绘制移动中的窗口容器
 * @container: 移动的容器
 * @window: 要移动的窗口
 * @draw: 绘制窗口与否
 */
PUBLIC void KGC_WindowPaintMoving(KGC_Container_t *container, KGC_Window_t *window, int draw)
{ 
    uint32_t color;
    int width;

    /* 先设置宽度 */
    width = window->container->width;
        
    /* 如果有绘制，就根据绘制窗口绘制容器 */
    if (draw) {
        color = windowSkin.boderColor;
        /* 绘制边框 */
        KGC_ContainerDrawRectangle(container, 1, 1, width-1, 1, KGCC_BLACK);
        KGC_ContainerDrawRectangle(container, 1, 1, 1, container->height - 1, KGCC_BLACK);
        KGC_ContainerDrawRectangle(container, width - 1, 1, 1, container->height - 1, KGCC_BLACK);
        KGC_ContainerDrawRectangle(container, 1, container->height - 1, width - 1, 1, KGCC_BLACK);

        KGC_ContainerDrawRectangle(container, 0, 0, width-1, 1, KGCC_WHITE);
        KGC_ContainerDrawRectangle(container, 0, 0, 1, container->height - 1, KGCC_WHITE);
        KGC_ContainerDrawRectangle(container, width - 2, 0, 1, container->height - 1, KGCC_WHITE);
        KGC_ContainerDrawRectangle(container, 0, container->height - 2, width - 1, 1, KGCC_WHITE);
    } else {    /* 没有就清除容器 */
        color = KGCC_NONE;
        /* 先绘制边框 */
        KGC_ContainerDrawRectangle(container, 0, 0, width, 2, color);
        KGC_ContainerDrawRectangle(container, 0, 0, 2, container->height, color);
        KGC_ContainerDrawRectangle(container, width - 2, 0, 2, container->height , color);
        KGC_ContainerDrawRectangle(container, 0, container->height - 2, width, 2, color);
        KGC_ContainerDrawRectangle(container, 0, container->height - 2, width, 2, color);

        /* 再恢复宽度 */
        width = videoInfo.xResolution;
    }
}


