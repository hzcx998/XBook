/*
 * file:		arch/x86/include/mm/ioremap.h
 * auther:		Jason Hu
 * time:		2020/1/31
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _X86_MM_IOREMAP_H
#define _X86_MM_IOREMAP_H

#include <share/stddef.h>
#include <share/types.h>

PUBLIC int ArchIoRemap(unsigned long phyAddr, unsigned long virAddr, size_t size);
PUBLIC int ArchIoUnmap(unsigned int addr, size_t size);

#endif   /*_X86_MM_IOREMAP_H */
