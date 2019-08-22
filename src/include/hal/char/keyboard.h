/*
 * file:		include/hal/char/keyboard.h
 * auther:		Jason Hu
 * time:		2019/8/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _HAL_CHAR_KEYBOARD_H
#define _HAL_CHAR_KEYBOARD_H

#include <share/stdint.h>
#include <share/types.h>
#include <book/hal.h>

/* I/O port for keyboard data
Read : Read Output Buffer
Write: Write Input Buffer(8042 Data&8048 Command) 
*/
#define PORT_KEYDAT		0x0060

/* I/O port for keyboard command
Read : Read Status Register
Write: Write Input Buffer(8042 Command) 
*/
#define PORT_KEYCMD		0x0064

#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

// 获取数据的操作
#define KEYBOARD_HAL_GET_DATA()   In8(PORT_KEYDAT)

//I/O 操作
#define KEYBOARD_HAL_IO_LED     1
#define KEYBOARD_HAL_IO_WAIT    2
#define KEYBOARD_HAL_IO_ACK     3

PUBLIC void KeyboardControllerWait();
PUBLIC void KeyboardControllerAck();


#endif  //_HAL_CHAR_KEYBOARD_H
