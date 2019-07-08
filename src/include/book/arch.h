/*
 * file:		   include/book/arch.h
 * auther:		Jason Hu
 * time:		2019/6/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_ARCH_H
#define _BOOK_ARCH_H

#include <book/config.h>

#ifdef CONFIG_ARCH_X86
   //把平台的相关头文件导入
   #include "../arch/x86/include/kernel/arch.h"
   #include "../arch/x86/include/kernel/vga.h"
   #include "../arch/x86/include/kernel/x86.h"
   #include "../arch/x86/include/kernel/ards.h"
   #include "../arch/x86/include/kernel/interrupt.h"
   #include "../arch/x86/include/mm/page.h"
   #include "../arch/x86/include/mm/zone.h"
   #include "../arch/x86/include/mm/area.h"
   
#endif   /*_ARCH_X86_*/

#endif   /*_BOOK_ARCH_H*/
