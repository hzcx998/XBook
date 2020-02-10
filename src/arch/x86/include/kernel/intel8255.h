/*
 * file:		include/arch/intel8255.h
 * auther:		Jason Hu
 * time:		2020/1/18
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _ARCH_INTEL8255_H
#define _ARCH_INTEL8255_H

#include <lib/stdint.h>
#include <lib/types.h>

/*
PPI （Programmable Peripheral Interface）

060-067  8255 Programmable Peripheral Interface  (PC,XT, PCjr)
060 8255 Port A keyboard input/output buffer (output PCjr)
061 8255 Port B output
062 8255 Port C input
063 8255 Command/Mode control register
*/

#define PPI_KEYBOARD        0X60
#define PPI_OUTPUT          0X61
#define PPI_INPUT           0X62
#define PPI_COMMAND_MODE    0X63

#endif   /* _ARCH_INTEL8255_H */
