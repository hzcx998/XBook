/*
 * file:		include/hal/char/display.h
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _HAL_CHAR_DISPLAY_H
#define _HAL_CHAR_DISPLAY_H

#include <share/stdint.h>
#include <share/types.h>
#include <book/hal.h>
#include <hal/char/color.h>

#define COLOR_DEFAULT	(MAKE_COLOR(TEXT_BLACK, TEXT_WHITE))

#define SCREEN_UP -1
#define SCREEN_DOWN 1

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25

#define SCREEN_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT)

//I/O 操作
#define DISPLAY_HAL_IO_COLOR    		1
#define DISPLAY_HAL_IO_CURSOR   		2
#define DISPLAY_HAL_IO_CLEAR    		3
#define DISPLAY_HAL_IO_RDCURSOR  		4

#endif  //_HAL_CHAR_DISPLAY_H
