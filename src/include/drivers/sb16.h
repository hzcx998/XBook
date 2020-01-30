/*
 * file:		include/drivers/sb16.h
 * auther:		Jason Hu
 * time:		2020/1/18
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#ifndef _DRIVER_SB16_H
#define _DRIVER_SB16_H

#include <share/stdint.h>
#include <share/types.h>

//#define CONFIG_SB16

#ifdef CONFIG_SB16
PUBLIC int InitSoundBlaster16Driver();
#endif  /* CONFIG_SB16 */

#endif  /* _DRIVER_SB16_H */
