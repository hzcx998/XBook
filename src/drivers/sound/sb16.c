/*
 * file:		drivers/sound/sb16.c
 * auther:		Jason Hu
 * time:		2020/1/18
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/config.h>
#include <book/debug.h>
#include <book/arch.h>

#include <arch/intel8253.h>
#include <arch/intel8255.h>

#include <share/string.h>

#include <drivers/sb16.h>

PRIVATE void SoundTest()
{
    
}

PUBLIC int InitSoundBlaster16Driver()
{
    SoundTest();

    while(1);
    return 0;
}
