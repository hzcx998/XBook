/*
 * file:		include/book/bitops.h
 * auther:		Jason Hu
 * time:		2019/8/19
 * copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
 */

#ifndef _BOOK_BITOPS_H
#define _BOOK_BITOPS_H

#include <lib/types.h>
#include <book/arch.h>
#include <book/bitops.h>

/* 求位数值 */
#define LOW8(a) (unsigned char)(a & 0xff) 
#define HIGH8(a) (unsigned char)((a >> 8) & 0xff) 

#define LOW16(a) (unsigned short)(a & 0xffff) 
#define HIGH16(a) (unsigned short)((a >> 16) & 0xffff) 

#define LOW32(a) (unsigned int)(a & 0xffffffff) 
#define HIGH32(a) (unsigned int)((a >> 32) & 0xffffffff) 

/* 合并操作 */
#define MERGE64(a, b) (unsigned long)(((a & 0xffffffff) << 32) | (b & 0xffffffff)) 
#define MERGE32(a, b) (unsigned int)(((a & 0xffff) << 16) | (b & 0xffff)) 
#define MERGE16(a, b) (unsigned short)(((a & 0xff) << 8) | (b & 0xff)) 
#define MERGE8(a, b) (unsigned char)(((a & 0xf) << 4) | (b & 0xf)) 

/**
 * SetBit - 设置位为1
 * @nr: 要设置的位置
 * @addr: 要设置位的地址
 * 将addr的第nr(nr为0-31)位置值置为1， 
 * nr大于31时，把高27的值做为当前地址的偏移，低5位的值为要置为1的位数 
*/
PRIVATE INLINE unsigned int SetBit(int nr, unsigned int *addr)  
{  
   unsigned int mask, retval;  

   addr += nr >> 5;                 //nr大于31时，把高27的值做为当前地址的偏移，  
   mask = 1 << (nr & 0x1f);         //获取31范围内的值，并把1向左偏移该位数  
   DisableInterrupt();              //关所有中断  
   retval = (mask & *addr) != 0;    //位置置1  
   *addr |= mask;  
   EnableInterrupt();               //开所有中断  
   
   return retval;                   //返回置数值  
} 

/**
 * ClearBit - 把位置0
 * @nr: 要设置的位置
 * @addr: 要设置位的地址
 * 
 * 将addr的第nr(nr为0-31)位置值置为0;  
 * nr大于31时，把高27的值做为当前地址的偏移，低5位的值为要置为0的位数
 */
PRIVATE INLINE unsigned int ClearBit(int nr, unsigned int *addr)  
{  
   unsigned int mask, retval;  

   addr += nr >> 5;  
   mask = 1 << (nr & 0x1f);  
   DisableInterrupt();  
   retval = (mask & *addr) != 0;  
   *addr &= ~mask;
   EnableInterrupt();  
   return retval;  
}  
  
/**
 * TestBit - 测试位的值
 * @nr: 要测试的位置
 * @addr: 要设置位的地址
 * 
 * 判断addr的第nr(nr为0-31)位置的值是否为1;  
 * nr大于31时，把高27的值做为当前地址的偏移，低5位的值为要判断的位数；  
 */
PRIVATE INLINE unsigned int TestBit(int nr, unsigned int *addr)  
{  
   unsigned int mask;  

   addr += nr >> 5;
   mask = 1 << (nr & 0x1f);  
   return ((mask & *addr) != 0);  
}

/**
 * TestAndSetBit - 测试并置1
 * @nr: 要测试的位置
 * @addr: 要设置位的地址
 * 
 * 先测试，获取原来的值，再设置为1
 */
PRIVATE INLINE unsigned int TestAndSetBit(int nr, unsigned int *addr)  
{
   unsigned int old;  
   /* 先测试获得之前的值 */
   old = TestBit(nr, addr);
   /* 再值1 */
   SetBit(nr, addr);
   /* 返回之前的值 */
   return old;  
}

#endif   /*_BOOK_BITOPS_H*/
