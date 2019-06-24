/*
 * file:		arch/x86/kernel/pic.c
 * auther:		Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <pic.h>
#include <x86.h>

void InitPic(void)
{
	Out8(PIC0_IMR,  0xff  );
	Out8(PIC1_IMR,  0xff  );

	Out8(PIC0_ICW1, 0x11  );
	Out8(PIC0_ICW2, 0x20  );
	Out8(PIC0_ICW3, 1 << 2);
	Out8(PIC0_ICW4, 0x01  );

	Out8(PIC1_ICW1, 0x11  );
	Out8(PIC1_ICW2, 0x28  );
	Out8(PIC1_ICW3, 2     );
	Out8(PIC1_ICW4, 0x01  );

	//屏蔽所有中断
	Out8(PIC0_IMR,  0xff  );
	Out8(PIC1_IMR,  0xff  );
	
	return;
}
