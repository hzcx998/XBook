/*
 * file:		include/book/arch.h
 * auther:		Jason Hu
 * time:		2019/6/3
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_ARCH_H
#define _BOOK_ARCH_H

#include <book/config.h>

#ifdef CONFIG_ARCH_X86
   //把平台的相关头文件导入
   #include "../arch/x86/include/kernel/arch.h"
   #include "../arch/x86/include/kernel/const.h"
   #include "../arch/x86/include/kernel/x86.h"
   #include "../arch/x86/include/kernel/ards.h"
   #include "../arch/x86/include/kernel/interrupt.h"
   #include "../arch/x86/include/kernel/tss.h"
   #include "../arch/x86/include/kernel/segment.h"
   #include "../arch/x86/include/kernel/cmos.h"
   #include "../arch/x86/include/kernel/intel8253.h"
   #include "../arch/x86/include/kernel/intel8255.h"
   #include "../arch/x86/include/kernel/dma.h"
   #include "../arch/x86/include/kernel/power.h"
   #include "../arch/x86/include/mm/page.h"
   #include "../arch/x86/include/mm/phymem.h"
   #include "../arch/x86/include/mm/ioremap.h"
#endif   /*_ARCH_X86_*/

#endif   /*_BOOK_ARCH_H*/
