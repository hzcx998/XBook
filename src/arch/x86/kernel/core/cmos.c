/*
 * file:		arch/x86/kernel/core/cmos.h
 * auther:		Jason Hu
 * time:		2019/9/6
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#include <kernel/x86.h>
#include <kernel/cmos.h>
#include <lib/types.h>

PRIVATE unsigned char ReadCMOS(unsigned char p)
{
	unsigned char data;
	Out8(CMOS_INDEX, p);
	data = In8(CMOS_DATA);
	Out8(CMOS_INDEX, 0x80);
	return data;
}

PUBLIC unsigned int CMOS_GetHourHex()
{
	return BCD_HEX(ReadCMOS(CMOS_CUR_HOUR));
}

PUBLIC unsigned int CMOS_GetMinHex()
{
	return BCD_HEX(ReadCMOS(CMOS_CUR_MIN));
}
PUBLIC unsigned char CMOS_GetMinHex8()
{
	return BCD_HEX(ReadCMOS(CMOS_CUR_MIN));
}
PUBLIC unsigned int CMOS_GetSecHex()
{
	return BCD_HEX(ReadCMOS(CMOS_CUR_SEC));
}
PUBLIC unsigned int CMOS_GetDayOfMonth()
{
	return BCD_HEX(ReadCMOS(CMOS_MON_DAY));
}
PUBLIC unsigned int CMOS_GetDayOfWeek()
{
	return BCD_HEX(ReadCMOS(CMOS_WEEK_DAY));
}
PUBLIC unsigned int CMOS_GetMonHex()
{
	return BCD_HEX(ReadCMOS(CMOS_CUR_MON));
}
PUBLIC unsigned int CMOS_GetYear()
{
	return (BCD_HEX(ReadCMOS(CMOS_CUR_CEN))*100) + \
		BCD_HEX(ReadCMOS(CMOS_CUR_YEAR))+1980;
}
