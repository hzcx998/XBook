/*
 * file:		hal/char/keyboard.c
 * auther:		Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/stddef.h>
#include <hal/char/keyboard.h>
#include <book/arch.h>
#include <book/debug.h>

#define LED_CODE		0xED		//led操作
#define KBC_ACK			0xFA		//回复码

#define KEYSTA_SEND_NOTREADY	0x02

/**
 * KeyboardControllerWait - 等待 8042 的输入缓冲区空
 */ 
PUBLIC void KeyboardControllerWait()
{
	unsigned char stat;

	do {
		stat = In8(PORT_KEYCMD);
	} while (stat & KEYSTA_SEND_NOTREADY);
}

/**
 * KeyboardControllerAck - 等待键盘控制器回复
 */
PUBLIC void KeyboardControllerAck()
{
	unsigned char read;

	do {
		read = In8(PORT_KEYDAT);
	} while ((read =! KBC_ACK));
	
}

/**
 * SetLeds - 设置键盘led灯状态
 */
PRIVATE void SetLeds(unsigned char leds)
{
	/* 数据指向LED_CODE */
	KeyboardControllerWait();
	Out8(PORT_KEYDAT, LED_CODE);
	KeyboardControllerAck();

	/* 写入新的led值 */
	KeyboardControllerWait();
	Out8(PORT_KEYDAT, leds);
	KeyboardControllerAck();
}

PRIVATE int KeyboardHalRead(unsigned char *buffer, unsigned int count)
{
	*buffer = In8(PORT_KEYDAT);
	// printk("read");
	return 0;
}

PRIVATE void KeyboardHalIoctl(unsigned int cmd, unsigned int param)
{
	//根据类型设置不同的值
	switch (cmd)
	{
	case KEYBOARD_HAL_IO_LED:
		SetLeds(param);
		break;
	case KEYBOARD_HAL_IO_WAIT:
		KeyboardControllerWait();
		break;
	case KEYBOARD_HAL_IO_ACK:
		KeyboardControllerAck();
		break;
	default:
		break;
	}
}

PRIVATE void KeyboardHalInit()
{
	//printk("-> clock hal ");
	PART_START("Keyboard hal");

	/* 初始化键盘控制器 */
	KeyboardControllerWait();
	Out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	KeyboardControllerAck();

	KeyboardControllerWait();
	Out8(PORT_KEYDAT, KBC_MODE);
	KeyboardControllerAck();
	
	PART_END();
}

/* hal操作函数 */
PRIVATE struct HalOperate halOperate = {
	.Init = &KeyboardHalInit,
	.Read = &KeyboardHalRead,
	.Ioctl = &KeyboardHalIoctl,
};

/* hal对象，需要把它导入到hal系统中 */
PUBLIC HAL(halOfKeyboard, halOperate, "keyboard");