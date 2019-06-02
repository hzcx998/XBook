/*
 * file:		arch/x86/kernel/page.c
 * auther:		Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <page.h>
#include <vga.h>
#include <share/stdint.h>
#include <book/bitmap.h>

/*
 * 功能: 初始化分页模式相关
 * 参数: 无
 * 返回: 成功返回0，失败返回-1
 * 说明: 对分页相关信息设置
 */
int init_page()
{
	printk("> init page start.\n");

	//把页目录表设置
	uint32_t *pdt = (uint32_t *)PAGE_DIR_VIR_ADDR;

	printk("pdt[0]=%x pdt[512]=%x pdt[1023]=%x\n", pdt[0], pdt[512], pdt[1023]);
	//把页目录表的第1项清空，这样，我们就只能通过高地址访问内核了
	pdt[0] = 0;
	printk("pdt[0]=%x pdt[512]=%x pdt[1023]=%x\n", pdt[0], pdt[512], pdt[1023]);

	//设置物理内存管理方式，以页框为单位
	printk("< init page done.\n");

	return 0;
}

