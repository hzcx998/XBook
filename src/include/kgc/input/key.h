/*
 * file:		include/kgc/input/key.h
 * auther:		Jason Hu
 * time:		2020/2/20
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _KGC_INPUT_KEY_H
#define _KGC_INPUT_KEY_H

#include <lib/types.h>
#include <lib/stdint.h>

#include <kgc/even.h>

PUBLIC void KGC_KeyDown(KGC_KeyboardEven_t *even);
PUBLIC void KGC_KeyUp(KGC_KeyboardEven_t *even);

#endif   /* _KGC_INPUT_KEY_H */
