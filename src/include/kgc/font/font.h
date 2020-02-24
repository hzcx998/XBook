/*
 * file:		include/kgc/font/font.h
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

/* 鼠标按钮 */
#ifndef _KGC_FONT_H
#define _KGC_FONT_H

#include <lib/types.h>
#include <lib/stdint.h>


#define KGC_MAX_FONT_NR 10

#define KGC_FONT_NAME_LEN 24
#define KGC_FONT_COPYRIGHT_NAME_LEN 24

typedef struct KGC_Font {
	char name[KGC_FONT_NAME_LEN];	            /* 字体名字 */
	char copyright[KGC_FONT_COPYRIGHT_NAME_LEN];	    /* 字体版权 */
	uint8_t *addr;			/* 字体数据地址 */
	int width, height;		/* 字体大小 */
} KGC_Font_t;

EXTERN KGC_Font_t *currentFont;

PUBLIC void KGC_InitFont();
PUBLIC int KGC_RegisterFont(KGC_Font_t *font);
PUBLIC KGC_Font_t *KGC_GetFont(char *name);
PUBLIC int KGC_UnregisterFont(KGC_Font_t *font);
PUBLIC KGC_Font_t *KGC_SelectFont(char *name);

#endif   /* _KGC_FONT_H */
