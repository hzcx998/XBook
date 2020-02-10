/*
 * file:		include/char/uart/serial.h
 * auther:		Jason Hu
 * time:		2020/1/30
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_UART_SERIAL_H
#define _DRIVER_UART_SERIAL_H

#include <lib/stdint.h>
#include <lib/types.h>
/*
PUBLIC int InitSerialDriver();
*/
/* 如果配置了才进行编译 */
#ifdef CONFIG_SERIAL_DEBUG
PUBLIC int InitSerialDebugDriver();
PUBLIC void SerialDebugPutChar(char ch);
#endif /* CONFIG_SERIAL_DEBUG */

#endif  /* _DRIVER_UART_SERIAL_H */
