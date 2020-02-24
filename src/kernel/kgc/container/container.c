/*
 * file:		kernel/kgc/container/container.c
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <book/memcache.h>
#include <lib/string.h>
#include <kgc/video.h>
#include <kgc/draw.h>
#include <kgc/container/container.h>
#include <kgc/container/draw.h>
#include <kgc/input/mouse.h>
#include <kgc/window/window.h>
#include <kgc/bar/bar.h>
#include <kgc/desktop/desktop.h>

/* 容器管理器 */
PRIVATE KGC_ContainerManager_t *containerManager; 

PUBLIC KGC_ContainerManager_t *KGC_ContainerManagerrInit(int width, int height)
{
	KGC_ContainerManager_t *manager;
	manager = (KGC_ContainerManager_t *) kmalloc(sizeof(KGC_ContainerManager_t), GFP_KERNEL);
	if (manager == NULL) {
		Panic("alloc memory for container manager failed!\n");
	}

	/* map只需要1字节就够了 */
	manager->map = (uint8_t *) kmalloc(width * height, GFP_KERNEL);
	if(manager->map == NULL){
		kfree(manager);
		Panic("alloc memory for container manager map failed!\n");
	}

	manager->width = width;
	manager->height = height;
	manager->top = -1;		/* 没有容器 */

	int i;
	for(i = 0; i < KGC_MAX_CONTAINER_NR; i++){
		manager->containerTable[i].id = i;
		manager->containerTable[i].flags = KGC_CONTAINER_UNUSED;
	}
	
	return manager;
}

PUBLIC KGC_Container_t *KGC_ContainerAlloc()
{
	KGC_Container_t *container;
	int i;
	/* 遍历查找 */
	for(i = 0; i < KGC_MAX_CONTAINER_NR; i++){
		container = &containerManager->containerTable[i];
		if(container->flags == KGC_CONTAINER_UNUSED){
            container->flags = KGC_CONTAINER_USING;
            memset(container->name, 0, KGC_CONTAINER_NAME_LEN);
            return container;			
		}
	}
	return NULL;    /* 分配失败，所有容器都在使用 */
}

PUBLIC void KGC_ContainerFree(KGC_Container_t *container)
{
	int i;
	for(i = 0; i < KGC_MAX_CONTAINER_NR; i++){
		if(&containerManager->containerTable[i] == container){
			/* 释放的时候，如果正在显示，那么先隐藏 */
            if(container->z >= 0){
                /* 调整容器的Z序 */
				KGC_ContainerZ(container, -1);	/* 隐藏 */
			}
            /* 释放容器缓冲区 */
            kfree(container->buffer);
			container->flags = KGC_CONTAINER_UNUSED;
			break;
		}
	}
}

PUBLIC int KGC_ContainerInit(KGC_Container_t *container,
    char *name,
    int x, int y,
    int width, int height)
{
	container->buffer = (uint32_t *) kmalloc(width * height * KGC_CONTAINER_BPP, GFP_KERNEL);
	if (container->buffer == NULL) {
		return -1;
	}
    container->x = x;
	container->y = y;
	container->width = width;
	container->height = height;
	container->bytesPerPixel = KGC_CONTAINER_BPP;
	container->z = -1;			/* 容器为隐藏状态 */
	
    INIT_LIST_HEAD(&container->widgetListHead);
	memset(container->name, 0, KGC_CONTAINER_NAME_LEN);
	strcpy(container->name, name);
    return 0;
}

/**
 * KGC_ContainerRefreshBufferPlus - 有透明计算的刷新
 * 
 * 
 */
void KGC_ContainerRefreshBufferPlus(int x0, int y0, int x1, int y1, int z0, int z1)
{
	int h, bx, by, x, y, bx0, by0, bx1, by1;	//x,y是容器在屏幕中的位置

	KGC_Container_t *container;

	KGC_ARGB_t src_color;
	KGC_ARGB_t *dst_color;
	uint8_t alpha;

	if (x0 < 0)
        x0 = 0;
	if (y0 < 0)
        y0 = 0;
	if (x1 > containerManager->width)
        x1 = containerManager->width;
	if (y1 > containerManager->height)
        y1 = containerManager->height;

    /* 全部发送 */
	for(h = 0; h <= containerManager->top; h++){
		//获取容器
		container = containerManager->containerPtr[h];
		bx0 = x0 - container->x;
		by0 = y0 - container->y;
		bx1 = x1 - container->x;
		by1 = y1 - container->y;
		if (bx0 < 0)
            bx0 = 0;
		if (by0 < 0)
            by0 = 0;
		if (bx1 > container->width)
            bx1 = container->width;
		if (by1 > container->height)
            by1 = container->height;
		//循环写入数据
		for(by = by0; by < by1; by++){
            y = container->y + by;
			for(bx = bx0; bx < bx1; bx++){
				x = container->x + bx;
                //获取源颜色
                KGC_ContainerReadPixel(container, bx, by, (uint32_t *)&src_color);

                /* 获取缓冲区屏幕上的颜色
                x, y, dst_color
                 */

                alpha = src_color.alpha;

                dst_color->red = src_color.red*alpha/0xff + dst_color->red*(1-alpha/0xFF * alpha/0xFF);
                dst_color->green = src_color.green*alpha/0xff + dst_color->green*(1-alpha/0xFF * alpha/0xFF);
                dst_color->blue = src_color.blue*alpha/0xff + dst_color->blue*(1-alpha/0xFF * alpha/0xFF);
                dst_color->alpha = src_color.alpha*alpha/0xff + dst_color->alpha*(1-alpha/0xFF * alpha/0xFF);
                /* 写入缓冲区颜色x, y, dst_color */
			}
		}
	}
    /* 刷入到显存 */
	// x0，y0，x1，y1
}

/*
不带透明度的窗口管理系统
KGC_ContainerRefreshMap
*/
static void KGC_ContainerRefreshBuffer(int x0, int y0, int x1, int y1, int z0, int z1)
{
	int z, bx, by, x, y, bx0, by0, bx1, by1;	//x,y是容器在屏幕中的位置

	uint8_t *map = containerManager->map;

	KGC_Container_t *container;
	
	uint8_t id;

	uint32_t color;

	if (x0 < 0)
        x0 = 0;
	if (y0 < 0)
        y0 = 0;
	if (x1 > containerManager->width)
        x1 = containerManager->width;
	if (y1 > containerManager->height)
        y1 = containerManager->height;
	
	for(z = z0; z <= z1; z++){
		//获取容器
		container = containerManager->containerPtr[z];
		id = container - containerManager->containerTable;
        //id = container->id;
		
        /* 获取刷新范围 */
		bx0 = x0 - container->x;
		by0 = y0 - container->y;
		bx1 = x1 - container->x;
		by1 = y1 - container->y;
        /* 修复范围 */
		if (bx0 < 0)
            bx0 = 0;
		if (by0 < 0)
            by0 = 0;
		if (bx1 > container->width) 
            bx1 = container->width;
		if (by1 > container->height)
            by1 = container->height;
        
        for(by = by0; by < by1; by++){
			y = container->y + by;
			for(bx = bx0; bx < bx1; bx++){
				x = container->x + bx;
				/* 照着map中的id进行刷新 */				
				if(map[(y * containerManager->width + x)] == id){
					/* 从容器中读取一个像素 */
                    KGC_ContainerReadPixel(container, bx, by, (uint32_t *)&color);

                    /* 写入到图形缓冲区 */
                    /* write(x, y, color) */
                    KGC_DrawPixel(x, y, color);
				}
			}
		}
	}
}

static void KGC_ContainerRefreshMap(int x0, int y0, int x1, int y1, int z0)
{
    // x,y 是图层在屏幕中的位置
	int z, bx, by, x, y, bx0, by0, bx1, by1;
	uint8_t *map = containerManager->map;

	KGC_ARGB_t color;
	KGC_Container_t *container;
	uint8_t id;

	if (x0 < 0)
        x0 = 0;
	if (y0 < 0)
        y0 = 0;
	if (x1 > containerManager->width)
        x1 = containerManager->width;
	if (y1 > containerManager->height)
        y1 = containerManager->height;
	
	for(z = z0; z <= containerManager->top; z++){
		/* 获取图层 */
		container = containerManager->containerPtr[z];
		id = container - containerManager->containerTable;
		//id = container->id;
		
        bx0 = x0 - container->x;
		by0 = y0 - container->y;
		bx1 = x1 - container->x;
		by1 = y1 - container->y;
		if (bx0 < 0)
            bx0 = 0;
		if (by0 < 0)
            by0 = 0;
		if (bx1 > container->width)
            bx1 = container->width;
		if (by1 > container->height)
            by1 = container->height;

		//循环写入数据
		for(by = by0; by < by1; by++){	//height*2才能写完全部，不然只有一半
			y = container->y + by;
			for(bx = bx0; bx < bx1; bx++){
				x = container->x + bx;		
				KGC_ContainerReadPixel(container, bx, by, (uint32_t *)&color);
				/* 如果像素不是透明的，就把它的容器id写进map */
				if(color.alpha){
                    /* 存放容器的id */
					map[(y * containerManager->width + x)] = id;
				}
			}
		}
	}			
}

PUBLIC void KGC_ContainerRefresh(KGC_Container_t *container, int x0, int y0, int x1, int y1)
{
	if(container->z >= 0){ 
		KGC_ContainerRefreshBuffer(container->x +  x0, container->y +  y0,
            container->x +  x1, container->y +  y1, container->z, container->z);
	}
}

/**
 * KGC_ContainerZ - 调整图层轴序
 * @container: 容器
 * @z: 轴
 * 
 */
void KGC_ContainerZ(KGC_Container_t *container, int z)
{
	int h, old = container->z;
	/* 修正高度 */
	if (z > containerManager->top + 1)
        z = containerManager->top + 1;
	if (z < -1)
        z = -1;
	
	container->z = z;		/* 设定高度 */
	
	/* 在containerPtr中重新排列 */
	if(old > z){	/* 设置的高度比以前自己高度低 */
		if(z >= 0){	//如果新的高度为显示（>=0）
			//把 old  和 new 之间的图层提高一层， 把old覆盖，然后new那一层添加为new图层
			//中间图层升高
			for(h = old; h > z; h--){
				containerManager->containerPtr[h] = containerManager->containerPtr[h - 1];	//把图层提高
				containerManager->containerPtr[h]->z = h;				//设置图层高度信息				
			}
			containerManager->containerPtr[z] = container;		//写入图层
			KGC_ContainerRefreshMap(container->x, container->y, container->x + container->width, container->y + container->height, container->z + 1);
			KGC_ContainerRefreshBuffer(container->x, container->y, container->x + container->width, container->y + container->height, container->z + 1, old);
		}else{	/*设置成了隐藏*/
			if(containerManager->top > old){
				//把原来图层和最高层之间的图层往低端降低
				for(h = old; h < containerManager->top; h++){
					containerManager->containerPtr[h] = containerManager->containerPtr[h + 1];	//把图层提高
					containerManager->containerPtr[h]->z = h;				//设置图层高度信息				
				}
			}
			containerManager->top--;		//隐藏了一个图层，最高层--
			KGC_ContainerRefreshMap(container->x, container->y, container->x + container->width, container->y + container->height, 0);
			KGC_ContainerRefreshBuffer(container->x, container->y, container->x + container->width, container->y + container->height, 0, old - 1);
		}
	} else if(old < z){ //设置的高度比以前自己高度高
		if(old >= 0){	//以前处于显示中
			//把old 和new height中间的图层下降，然后填写新的相关信息
			for(h = old; h < z; h++){
				containerManager->containerPtr[h] = containerManager->containerPtr[h + 1];	//把图层降低
				containerManager->containerPtr[h]->z = h;				//设置图层高度信息				
			}
			containerManager->containerPtr[z] = container;		//写入图层
		}else{	//原来处于隐藏状态
			//操作最高层和height之间的图层，将它们提高一层
			for(h = containerManager->top; h >= z; h--){
				containerManager->containerPtr[h + 1] = containerManager->containerPtr[h];	//把图层提高
				containerManager->containerPtr[h + 1]->z = h + 1;			//设置图层高度信息				
			}
			containerManager->containerPtr[z] = container;		//写入图层
			containerManager->top++;		//增加显示了一个图层，最高层++
			
		}
		KGC_ContainerRefreshMap(container->x, container->y, container->x + container->width, container->y + container->height, z);
		KGC_ContainerRefreshBuffer(container->x, container->y, container->x + container->width, container->y + container->height, z, z);
	}
}

/**
 * KGC_ContainerAboveTopZ - 添加一个图层到顶层上面
 * 
 * 新的图层会在最高图层上面
 */
PUBLIC void KGC_ContainerAboveTopZ(KGC_Container_t *container)
{
    KGC_ContainerZ(container, containerManager->top + 1);
}


/**
 * KGC_ContainerBelowTopZ - 设置z轴在顶部下面
 * 
 * 改变图层位置，不会添加新图层
 */
PUBLIC void KGC_ContainerBelowTopZ(KGC_Container_t *container)
{
    KGC_ContainerZ(container, containerManager->top - 1);
}

/**
 * KGC_ContainerAtTopZ - 设置z轴在顶部
 * 
 * 添加一个新的图层位于top，然后top+1
 */
PUBLIC void KGC_ContainerAtTopZ(KGC_Container_t *container)
{
    KGC_ContainerZ(container, containerManager->top);
}

PUBLIC void KGC_ContainerSlide(KGC_Container_t *container, int x, int y)
{
	int oldX = container->x, oldY = container->y;
	container->x = x;
	container->y = y;
	if(container->z >= 0){	/* 如果改变位置后图层为显示状态 */
		KGC_ContainerRefreshMap(oldX, oldY, oldX + container->width, oldY + container->height, 0);
		KGC_ContainerRefreshMap(x, y, x + container->width, y + container->height, container->z);
		
		KGC_ContainerRefreshBuffer(oldX, oldY, oldX + container->width, oldY + container->height, 0, container->z - 1);
		KGC_ContainerRefreshBuffer(x, y, x + container->width, y + container->height, container->z, container->z);
	}
}

PUBLIC void KGC_InitContainer()
{
    
    /* 初始化容器管理器 */
    containerManager = KGC_ContainerManagerrInit(videoInfo.xResolution, videoInfo.yResolution);
    if (containerManager == NULL) {
        Panic("alloc memory for container manager failed!\n");
    }
    
    KGC_InitDesktopContainer();
    KGC_InitMouseContainer();
    KGC_InitBarContainer();
    KGC_InitWindowContainer();
}
