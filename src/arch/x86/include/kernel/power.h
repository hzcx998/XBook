/*
 * file:		arch/x86/include/kernel/power.h
 * auther:		Jason Hu
 * time:		2020/3/2
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _X86_POWER_H
#define _X86_POWER_H

#include <lib/stdint.h>
#include <lib/types.h>

void ArchDoReboot(int type);

#endif	/* _X86_POWER_H */