/*
 * file:		include/book/power.h
 * auther:		Jason Hu
 * time:		2020/3/2
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_POWER_H
#define _BOOK_POWER_H

#include <lib/stdint.h>
#include <lib/types.h>

#define REBOOT_KBD      1
#define REBOOT_BIOS     2		/*实验失败*/
#define REBOOT_CF9      3
#define REBOOT_TRIPLE   4

int SysReboot();

#endif   /* _BOOK_POWER_H */
