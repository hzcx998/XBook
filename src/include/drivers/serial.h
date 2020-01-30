/*
 * file:		include/drivers/serial.h
 * auther:		Jason Hu
 * time:		2020/1/30
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_SERIAL_H
#define _DRIVER_SERIAL_H

#include <share/stdint.h>
#include <share/types.h>

PUBLIC int InitSerialDriver();

/* 如果配置了才进行编译 */
#ifdef CONFIG_SERIAL_DEBUG
PUBLIC int InitSerialDebugDriver();
PUBLIC void SerialDebugPutChar(char ch);
#endif /* CONFIG_SERIAL_DEBUG */

#endif  /* _DRIVER_SERIAL_H */
