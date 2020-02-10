/*
 * file:		arch/x86/include/kernel/cmos.h
 * auther:		Jason Hu
 * time:		2019/9/6
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _X86_CMOS_H
#define _X86_CMOS_H

/**CMOS操作端口**/
#define CMOS_INDEX      0x70
#define CMOS_DATA       0x71
/**CMOS中相关信息偏移*/
#define CMOS_CUR_SEC	0x0			//CMOS中当前秒?(BCD)
#define CMOS_ALA_SEC	0x1			//CMOS中?警秒?(BCD)
#define CMOS_CUR_MIN	0x2			//CMOS中当前分?(BCD)
#define CMOS_ALA_MIN	0x3			//CMOS中?警分?(BCD)
#define CMOS_CUR_HOUR	0x4			//CMOS中当前小?(BCD)
#define CMOS_ALA_HOUR	0x5			//CMOS中?警小?(BCD)
#define CMOS_WEEK_DAY	0x6			//CMOS中一周中当前天(BCD)
#define CMOS_MON_DAY	0x7			//CMOS中一月中当前日(BCD)
#define CMOS_CUR_MON	0x8			//CMOS中当前月?(BCD)
#define CMOS_CUR_YEAR	0x9			//CMOS中当前年?(BCD)
#define CMOS_DEV_TYPE	0x12		//CMOS中??器格式
#define CMOS_CUR_CEN	0x32		//CMOS中当前世?(BCD)

#define BCD_HEX(n)	((n >> 4) * 10) + (n & 0xf)  //BCD?十六?制

#define BCD_ASCII_FIRST(n)	(((n<<4)>>4)+0x30)  //取BCD的个位并以字符?出,来自UdoOS
#define BCD_ASCII_S(n)	((n<<4)+0x30)  //取BCD的十位并以字符?出,来自UdoOS

/*
获得数据的端口
*/
unsigned int CMOS_GetHourHex();
unsigned int CMOS_GetMinHex();
unsigned char CMOS_GetMinHex8();
unsigned int CMOS_GetSecHex();
unsigned int CMOS_GetDayOfMonth();
unsigned int CMOS_GetDayOfWeek();
unsigned int CMOS_GetMonHex();
unsigned int CMOS_GetYear();

#endif  /* _X86_CMOS_H */

