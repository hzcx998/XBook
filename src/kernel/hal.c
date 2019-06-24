/*
 * file:		kernel/hal.c
 * auther:	Jason Hu
 * time:		2019/6/22
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <book/hal.h>
#include <hal/hal.h>
#include <share/vsprintf.h>
#include <book/debug.h>

/*
   在这儿初始化所有的hal
 */
PUBLIC void InitHal()
{
   ClockHalInit();
   /* 
   HalInit(displayHal.hal);
   HalOpen(displayHal.hal);

   HalWrite(displayHal.hal,(unsigned char *)"hello", 5);

   //HalClose(displayHal.hal);
   HalWrite(displayHal.hal,(unsigned char *)"hello", 5);

   DebugLog("hello debug");
   */
   
}

PUBLIC void HalInit(struct Hal *hal)
{
   hal->halOperate->Init();
}

PUBLIC void HalOpen(struct Hal *hal)
{
   hal->halOperate->Open();
}

PUBLIC void HalClose(struct Hal *hal)
{
   hal->halOperate->Close();
}

PUBLIC void HalWrite(struct Hal *hal,unsigned char *buffer, unsigned int count)
{
   hal->halOperate->Write(buffer, count);
}

PUBLIC void HalRead(struct Hal *hal,unsigned char *buffer, unsigned int count)
{
   hal->halOperate->Read(buffer, count);
}

PUBLIC void HalIoctl(struct Hal *hal,unsigned int type, unsigned int value)
{
   hal->halOperate->Ioctl(type, value);
}
