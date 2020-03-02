/*
 * file:		kernel/pw/power.c
 * auther:		Jason Hu
 * time:		2020/3/2
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <book/arch.h>
#include <book/debug.h>
#include <book/power.h>

int SysReboot()
{
    int type = REBOOT_KBD;

    /* 进行环境保存 */

    
	//一直循环
	while(1){
		//先执行一次
		ArchDoReboot(type);
		//如果没有成功，就选择其它方式
		switch(type){
			case REBOOT_KBD:
				type = REBOOT_CF9;
				break;
			case REBOOT_CF9:
				type = REBOOT_BIOS;
				break;
			case REBOOT_BIOS:
				//回到第一个方法
				type = REBOOT_TRIPLE;
				break;
			default: 
				type = REBOOT_KBD;
				break;
		}
	}
}
