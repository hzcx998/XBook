/***************************************************
*		 Copyright (c) 2018 MINE 田宇
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of version 2 of the GNU General Public
* License as published by the Free Software Foundation.
*
***************************************************/

/*
 * file:		drivers/tty.c
 * auther:		Jason Hu
 * time:		2019/8/8
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <book/arch.h>
#include <book/debug.h>
#include <drivers/serial.h>

PRIVATE void SerialSend(unsigned char font)
{
	int timeout = 100000;
	while((In8(COM1 + LINE_STATUS_REG) & 0x20) != 0x20)
		if(!timeout--)
			return;

	Out8(COM1 + TRANSMITTER_HOLDING_REG,font);
}

PUBLIC void SerialPutchar(unsigned char font)
{
	if(font == '\n')
		SerialSend('\r');
	SerialSend(font);
}


PUBLIC int InitSerialDriver()
{
	//unsigned char value = 0;
/*
	Out8(COM1 + INTERRUPT_ENABLE_REG,0x02);
	value = In8(COM1 + INTERRUPT_ENABLE_REG);
	color_printk(WHITE,BLACK,"Serial send 1:%x\n",value);

	Out8(COM2 + INTERRUPT_ENABLE_REG,0x02);
	value = In8(COM2 + INTERRUPT_ENABLE_REG);
	color_printk(WHITE,BLACK,"Serial send 2:%x\n",value);

	Out8(COM3 + INTERRUPT_ENABLE_REG,0x02);
	value = In8(COM3 + INTERRUPT_ENABLE_REG);
	color_printk(WHITE,BLACK,"Serial send 3:%x\n",value);

	Out8(COM4 + INTERRUPT_ENABLE_REG,0x02);
	value = In8(COM4 + INTERRUPT_ENABLE_REG);
	color_printk(WHITE,BLACK,"Serial send 4:%x\n",value);
*/
	
	///8bit,1stop,No Parity,Break signal Disabled
	///DLAB=1
	Out8(COM1 + LINE_CONTROL_REG,0x83);

	///set Baud rate 115200
	Out8(COM1 + DIVISOR_LATCH_HIGH_REG,0x00);
	Out8(COM1 + DIVISOR_LATCH_LOW_REG,0x01);

	///8bit,1stop,No Parity,Break signal Disabled
	///DLAB=0
	Out8(COM1 + LINE_CONTROL_REG,0x03);

	///disable ALL interrupt
	Out8(COM1 + INTERRUPT_ENABLE_REG,0x00);

	///enable FIFO,clear receive FIFO,Clear transmit FIFO
	///enable 64Byte FIFO,receive FIFO interrupt trigger level
	Out8(COM1 + FIFO_CONTROL_REG,0xe7);

	Out8(COM1 + MODEM_CONTROL_REG,0x00);
	Out8(COM1 + SCRATCH_REG,0x00);

    return 0;
}

