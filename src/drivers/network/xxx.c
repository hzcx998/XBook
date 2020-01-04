/*
 * file:		drivers/network/xxx.c
 * auther:		Jason Hu
 * time:		2020/1/4
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */


#include <book/config.h>
#include <book/debug.h>
#include <book/arch.h>
#include <book/interrupt.h>

#include <share/string.h>

#include <net/ethernet.h>

#include <drivers/xxx.h>

#define DEVNAME "xxx"

/**
 * XXXTransmit - 传输函数
 * @buf: 缓冲区
 * @len: 数据长度
 */
PUBLIC int XXXTransmit(char *buf, uint32 len)
{
    
    return 0;
}

/**
 * XXXReceive - 接收函数
 * @buf: 缓冲区
 * @len: 数据长度
 */
void XXXReceive(char *buf, uint32 len)
{
    
}

/**
 * XXXHandler - 中断处理函数
 * @irq: 中断号
 * @data: 中断的数据
 */
PRIVATE void XXXHandler(unsigned int irq, unsigned int data)
{
	printk("XXXHandler occur!\n");
    
}

/**
 * InitXXXDriver - 初始化驱动
 * 
 * 此函数在src/net/network.c中被调用
 * 
 */
PUBLIC int InitXXXDriver()
{
    PART_START("XXX Driver");


    int irq = 9;

    /* 注册并打开中断 */
	RegisterIRQ(irq, XXXHandler, IRQF_DISABLED, "IRQ-Network", DEVNAME, 0);
    EnableIRQ(irq);
    
    PART_END();
    return 0;
}
