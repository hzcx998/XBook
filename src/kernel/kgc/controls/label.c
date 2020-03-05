/*
 * file:		kernel/kgc/controls/label.c
 * auther:		Jason Hu
 * time:		2020/2/21
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <book/memcache.h>
#include <video/video.h>
#include <kgc/color.h>
#include <kgc/controls/label.h>
#include <lib/string.h>

/**
 * KGC_CreateLabel - 创建一个标签
 * 
 * 成功返回标签，失败返回NULL
 */
PUBLIC KGC_Label_t *KGC_CreateLabel()
{
    KGC_Label_t *label = kmalloc(sizeof(KGC_Label_t), GFP_KERNEL);
    if (label == NULL)
        return NULL;
    if (KGC_LabelInit(label)) {
        kfree(label);
        return NULL;
    }
    return label;
}

/**
 * KGC_LabelDestroySub - 销毁一个标签，部分操作
 * 
 */
PUBLIC void KGC_LabelDestroySub(KGC_Label_t *label)
{
    /* 释放文本 */
    if (label->text)
        kfree(label->text);

    /* 释放图形数据 */
    if (label->image.data)
        kfree(label->image.data);
}


/**
 * KGC_LabelDestroy - 销毁一个标签
 * 
 */
PUBLIC void KGC_LabelDestroy(KGC_Label_t *label)
{
    KGC_LabelDestroySub(label);
    kfree(label);
}


/**
 * KGC_LabelInit - 初始化一个标签
 * @label: 标签
 * 成功返回标签，失败返回NULL
 */
PUBLIC int KGC_LabelInit(KGC_Label_t *label)
{
    label->textMaxLength = KGC_DEFAULT_LABEL_TEXT_LEN;
    label->text = kmalloc(label->textMaxLength, GFP_KERNEL);
    if (label->text == NULL) {
        return -1;
    }
    /* 进行默认的初始化 */
    label->x = 0;
    label->y = 0;
    label->width = KGC_DEFAULT_LABEL_WIDTH;
    label->height = KGC_DEFAULT_LABEL_HEIGHT;
    label->visable = 1;
    label->disabel = 0;
    label->backColor = KGC_LABEL_BACK_COLOR;
    label->fontColor = KGC_LABEL_FONT_COLOR;
    label->disableColor = KGC_LABEL_DISABEL_COLOR;
    
    /* 设置默认标签 */
    memset(label->name, 0, KGC_LABEL_NAME_LEN);
    strcpy(label->name, "label");
    memset(label->text, 0, label->textMaxLength);
    strcpy(label->text, "text");
    label->textLength = strlen(label->text);
    label->textAlign = KGC_WIDGET_ALIGN_LEFT;   /* 默认左对齐 */
    /* 设置系统字体 */
    label->font = currentFont;
    /* 默认是文本 */
    label->type = KGC_LABEL_TEXT;
    
    /* 初始化图像 */
    KGC_LabelSetImage(label, 0, 0, NULL, 0, 0);

    /* 初始化widget */
    KGC_WidgetInit(&label->widget, KGC_WIDGET_LABEL);
    KGC_WidgetSetDraw(&label->widget, &KGC_LabelShow);
    
    return 0;
}

/**
 * KGC_LabelSetLocation - 设置标签的位置
 * @label: 标签
 * @x: 横坐标
 * @y: 纵坐标
 */
PUBLIC void KGC_LabelSetLocation(KGC_Label_t *label, int x, int y)
{
    label->x = x;
    label->y = y;
}

PRIVATE void KGC_LabelCreateImage(KGC_Image_t *image)
{
    int x, y;
	for (y = 0; y < image->height; y++) {
		for (x = 0; x < image->width; x++) {
            //printk("x%d, y%d\n", x, y);
			if (image->raw[y * image->width + x] == 0) {
                image->data[y * image->width + x] = KGCC_NONE;
            } else if (image->raw[y * image->width + x] == 1) {
				image->data[y * image->width + x] = image->borderColor;
            } else if (image->raw[y * image->width + x] == 2) {
                image->data[y * image->width + x] = image->fillColor;
            }
		}
	}
    //printk("create image done!\n");
            
}

PUBLIC void KGC_LabelSetImageAlign(KGC_Label_t *label, KGC_WidgetAlign_t align)
{
    label->imageAlign = align;
}

PUBLIC void KGC_LabelSetImage(KGC_Label_t *label, int width,
    int height, uint8_t *raw, kgcc_t border, kgcc_t fill)
{
    label->image.width = width;
    label->image.height = height;
    label->image.raw = raw;
    label->image.borderColor = border;
    label->image.fillColor = fill;
    label->imageAlign = KGC_WIDGET_ALIGN_LEFT;
    
    if (raw == NULL) {
        label->image.data = NULL;
    } else {
        label->image.data = kmalloc(width * height * KGC_CONTAINER_BPP, GFP_KERNEL);
        if (label->image.data) {
            /* 开始数据绘制 */
            /* 图形类型 */
            label->type = KGC_LABEL_IMAGE;
            //printk("will create image\n");
            /* 创建数据 */
            KGC_LabelCreateImage(&label->image);
        }
    }
}

/**
 * KGC_LabelSetSize - 设置标签的大小
 * @label: 标签
 * @width: 宽度
 * @height: 高度
 */
PUBLIC void KGC_LabelSetSize(KGC_Label_t *label, int width, int height)
{
    label->width = width;
    label->height = height;
}

/**
 * KGC_LabelSetColor - 设置标签的颜色
 * @label: 标签
 * @back: 背景色
 * @font: 字体色
 */
PUBLIC void KGC_LabelSetColor(KGC_Label_t *label, kgcc_t back, kgcc_t font)
{
    label->backColor = back;
    label->fontColor = font;
}

/**
 * KGC_LabelSetName - 设置标签的名字
 * @label: 标签
 * @back: 背景色
 * @font: 字体色
 */
PUBLIC void KGC_LabelSetName(KGC_Label_t *label, char *name)
{
    label->nameLength = strlen(name);
    if (label->nameLength > KGC_LABEL_NAME_LEN - 1)
        label->nameLength = KGC_LABEL_NAME_LEN - 1;

    memset(label->name, 0, KGC_LABEL_NAME_LEN);
    memcpy(label->name, name, label->nameLength);
    /* 末尾填'\0' */
    label->name[label->nameLength] = '\0';
}


/**
 * KGC_LabelSetTextMaxLength - 设置标签的最大长度
 * @label: 标签
 * @length: 最大长度
 * 
 * 设置文本前必须调用一次
 */
PUBLIC int KGC_LabelSetTextMaxLength(KGC_Label_t *label, int length)
{
    /* 对长度进行限制 */
    if (length > KGC_MAX_LABEL_TEXT_LEN - 1)
        length = KGC_MAX_LABEL_TEXT_LEN - 1;
    
    char *old = NULL;
    /* 如果已经有数据，那么保存旧数据地址 */
    if (label->text != NULL) {
        old = label->text;
    }
    
    /* 分配文本 */
    label->text = kmalloc(length, GFP_KERNEL);
    if (label->text == NULL) {
        /* 如果分配失败，恢复原来的数据 */
        label->text = old;
        return -1;
    }
    /* 分配新数据成功，并且原来也有数据，那么就把原来的数据释放掉 */
    if (old != NULL) {
        kfree(old);
    }

    label->textMaxLength = length;
    /* 数据清空 */
    memset(label->text, 0, length);
    return 0;
}

/**
 * KGC_LabelSetText - 设置标签的文本
 * @label: 标签
 * @text: 文本
 */
PUBLIC void KGC_LabelSetText(KGC_Label_t *label, char *text)
{
    int length = strlen(text);
    /* 修复长度 */
    if (length > label->textMaxLength - 1)
        length = label->textMaxLength - 1;
    
    /* 设置文本 */
    memcpy(label->text, text, length);
    /* 末尾填'\0' */
    label->text[length] = '\0';
    label->textLength = length;
    /* 文本类型 */
    label->type = KGC_LABEL_TEXT;
}

/**
 * KGC_LabelSetFont - 设置标签的字体
 * @label: 标签
 * @fontName: 字体名
 */
PUBLIC int KGC_LabelSetFont(KGC_Label_t *label, char *fontName)
{
    KGC_Font_t *font = KGC_GetFont(fontName);
    
    /* 找到才设置，没找到就不设置 */
    if (!font)
        return -1;

    label->font = font;
    return 0;
}

/**
 * KGC_LabelAdd - 添加标签
 * @label: 标签
 * @container: 容器
 * 
 * 添加标签到容器上
 */
PUBLIC void KGC_LabelAdd(KGC_Label_t *label, KGC_Container_t *container)
{
    KGC_AddWidget(&label->widget, container);
}


/**
 * KGC_DelLabel - 删除标签
 * @label: 标签
 * 
 * 从容器上删除标签
 */
PUBLIC void KGC_DelLabel(KGC_Label_t *label)
{
    KGC_DelWidget(&label->widget);
}

/**
 * KGC_LabelShow - 显示标签
 * @label: 标签
 */
PUBLIC void KGC_LabelShow(KGC_Widget_t *widget)
{
    /* 转换成标签 */
    KGC_Label_t *label = (KGC_Label_t *)widget;
    /* 先绘制背景 */
    KGC_ContainerDrawRectangle(widget->container,
        label->x, label->y, label->width, label->height, label->backColor);
    
    /* 可见才绘制 */
    if (likely(label->visable)) {
        int x, y;
        if (label->type == KGC_LABEL_TEXT) {

            y = label->y + label->height / 2 - label->font->height / 2;
            
            switch (label->textAlign)
            {
            case KGC_WIDGET_ALIGN_LEFT:
                x = label->x;
                break;
            case KGC_WIDGET_ALIGN_CENTER:
                x = label->x + label->width / 2 - (label->textLength * label->font->width) / 2 ;
                break;
            case KGC_WIDGET_ALIGN_RIGHT:
                x = label->x + label->width - (label->textLength * label->font->width);
                break;
            default:
                break;
            }
            kgcc_t color;
            /* 选择显示颜色 */
            if (likely(!label->disabel)) {
                color = label->fontColor;
            } else {
                color = label->disableColor;
            }
            
            /* 再绘制文本 */
            KGC_ContainerDrawStringWithFont(widget->container,
                x, y, label->text, color, label->font);    
        } else if (label->type == KGC_LABEL_IMAGE) {
            y = label->y + label->height / 2 - label->image.height / 2;
            
            switch (label->imageAlign)
            {
            case KGC_WIDGET_ALIGN_LEFT:
                x = label->x;
                break;
            case KGC_WIDGET_ALIGN_CENTER:
                x = label->x + label->width / 2 - label->image.width / 2;
                
                break;
            case KGC_WIDGET_ALIGN_RIGHT:
                x = label->x + label->width - label->image.width;
                break;
            default:
                break;
            }
            /* 绘制数据 */
            KGC_ContainerDrawBitmap(widget->container, x, y, label->image.width, label->image.height, label->image.data);
        }
    }
    /* 刷新标签 */
    KGC_ContainerRefresh(widget->container, label->x, label->y,
        label->x + label->width, label->y + label->height);
    //printk("show %s\n", label->text);
}
