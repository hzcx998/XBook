/*
 * file:		include/drivers/tty.h
 * auther:		Jason Hu
 * time:		2019/12/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_TTY_H
#define _DRIVER_TTY_H

#include <share/stdint.h>
#include <share/types.h>
#include <book/ioqueue.h>
#include <drivers/console.h>
#include <drivers/keyboard.h>

enum {
    TTY_CMD_CLEAN,
    TTY_CMD_HOLD,
};


PUBLIC int InitTTYDriver();

#endif  /* _DRIVER_TTY_H */
