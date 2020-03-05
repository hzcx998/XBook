/*
 * file:		kernel/kgc/font/font.c
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/arch.h>
#include <book/debug.h>
#include <book/kgc.h>
#include <video/video.h>
#include <kgc/font/font.h>
#include <lib/string.h>

/* 导入字体注册 */
#ifdef CONFIG_FONT_STANDARD
EXTERN void KGC_RegisterFontStandard();
#endif

#ifdef CONFIG_FONT_SIMSUN
EXTERN void KGC_RegisterFontSimsun();
#endif

/* 字体表，存放字体信息 */
KGC_Font_t fontTable[KGC_MAX_FONT_NR];

KGC_Font_t *currentFont;

PUBLIC KGC_Font_t *KGC_GetFont(char *name)
{
    int i;
	for (i = 0; i < KGC_MAX_FONT_NR; i++) {
		if (!strcmp(fontTable[i].name, name)) {
			return &fontTable[i];
		}
	}
	return NULL;
}

PUBLIC int KGC_RegisterFont(KGC_Font_t *font)
{
    int i;
	for (i = 0; i < KGC_MAX_FONT_NR; i++) {
		if (fontTable[i].addr == NULL) {
			/* 注册数据 */
            fontTable[i] = *font;
			return 0;
		}
	}
	return -1;
}

PUBLIC int KGC_UnregisterFont(KGC_Font_t *font)
{
    int i;
	for (i = 0; i < KGC_MAX_FONT_NR; i++) {
		if (&fontTable[i] == font) {
			fontTable[i].width = 0;
			fontTable[i].height = 0;
			fontTable[i].addr = NULL;		
			return 0;
		}
	}
	return -1;
}

PUBLIC KGC_Font_t *KGC_SelectFont(char *name)
{
    KGC_Font_t *font = KGC_GetFont(name);
    if (font)
        currentFont = font;
    return font;
}

PUBLIC void KGC_InitFont()
{
    int i;
	for (i = 0; i < KGC_MAX_FONT_NR; i++) {
		fontTable[i].addr = NULL;
		fontTable[i].width = fontTable[i].height = 0;
	}
	currentFont = NULL;
    /* 注册字体 */
    
#ifdef CONFIG_FONT_SIMSUN
    KGC_RegisterFontSimsun();
#endif /* CONFIG_FONT_SIMSUN */

#ifdef CONFIG_FONT_STANDARD
    KGC_RegisterFontStandard();
#endif /* CONFIG_FONT_STANDARD */

    /* 选择一个默认字体 */
    KGC_SelectFont("Standard");
}
