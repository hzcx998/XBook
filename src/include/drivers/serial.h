/***************************************************
*		 Copyright (c) 2018 MINE 田宇
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of version 2 of the GNU General Public
* License as published by the Free Software Foundation.
*
***************************************************/

/*
 * file:		include/drivers/serial.h
 * auther:		Jason Hu
 * time:		2020/1/4
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_SERIAL_H
#define _DRIVER_SERIAL_H

#include <share/stdint.h>
#include <share/types.h>

#define COM1           0x3f8
#define COM2           0x2f8
#define COM3           0x3e8
#define COM4           0x2e8

// Serial port offsets
#define RECEIVE_BUFFER_REG		0
#define TRANSMITTER_HOLDING_REG		0
#define DIVISOR_LATCH_LOW_REG		0

#define INTERRUPT_ENABLE_REG		1
#define DIVISOR_LATCH_HIGH_REG		1

#define INTERRUPT_IDENTIFICATION_REG	2
#define FIFO_CONTROL_REG		2

#define LINE_CONTROL_REG		3

#define MODEM_CONTROL_REG		4

#define LINE_STATUS_REG			5

#define MODEM_STATUS_REG		6

#define SCRATCH_REG			7

PUBLIC int InitSerialDriver();

PUBLIC void SerialPutchar(unsigned char font);


#endif  /* _DRIVER_SERIAL_H */
