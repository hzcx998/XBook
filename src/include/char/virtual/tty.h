/*
 * file:		include/char/virtual/tty.h
 * auther:		Jason Hu
 * time:		2019/12/3
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_VIRTUAL_TTY_H
#define _DRIVER_VIRTUAL_TTY_H

#include <book/ioqueue.h>
#include <lib/stdint.h>
#include <lib/types.h>

enum {
    TTY_CMD_CLEAN,
    TTY_CMD_HOLD,
};
/*
PUBLIC int InitTTYDriver();
*/
#endif  /* _DRIVER_VIRTUAL_TTY_H */
