/*
 * file:		kernel/bitmap.c
 * auther:	Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/string.h>
#include <book/bitmap.h>

/*
 * Bitmapinit - 初始化位图
 * btmp: 要初始化的位图结构的地址
 * 
 * 初始化一个位图结构，其实就是把位图数据指针指向的地址清0
 */
PUBLIC void BitmapInit(struct Bitmap *btmp) 
{
   memset(btmp->bits, 0, btmp->btmpBytesLen);   
}

/*
 * BitmapScanTest - 检测位图某位是否为1
 * @btmp: 要检测的位图
 * @bitIdx: 要检测哪一位
 * 
 * 可以通过检测寻找哪些是已经使用的，哪些还没有使用
 */
PUBLIC bool BitmapScanTest(struct Bitmap *btmp, unsigned int  bitIdx) 
{
   unsigned int byteIdx = bitIdx / 8;   
   unsigned int bitOdd  = bitIdx % 8;  
   return (btmp->bits[byteIdx] & (BITMAP_MASK << bitOdd));
}

/*
 * Bitmapscan - 扫描n个位
 * @btmp: 要检测的位图
 * @cnt: 要扫描多少位
 * 
 * 可以通过检测寻找哪些是已经使用的，哪些还没有使用
 */
PUBLIC int BitmapScan(struct Bitmap *btmp, unsigned int cnt) {
   unsigned int idxByte = 0;

   while (( 0xff == btmp->bits[idxByte]) && (idxByte < btmp->btmpBytesLen)) {
      idxByte++;
   }

   
   if (idxByte == btmp->btmpBytesLen) {  
      return -1;
   }

   int idxBit = 0;

   while ((unsigned char)(BITMAP_MASK << idxBit) & btmp->bits[idxByte]) { 
	 idxBit++;
   }
	 
   int bitIdxStart = idxByte * 8 + idxBit;  
   if (cnt == 1) {
      return bitIdxStart;
   }

   unsigned int bit_left = (btmp->btmpBytesLen * 8 - bitIdxStart);
   unsigned int nextBit = bitIdxStart + 1;
   unsigned int count = 1;

   bitIdxStart = -1;
   while (bit_left-- > 0) {
      if (!(BitmapScanTest(btmp, nextBit))) {
	 count++;
      } else {
	 count = 0;
      }
      if (count == cnt) {
	 bitIdxStart = nextBit - cnt + 1;
	 break;
      }
      nextBit++;          
   }
   return bitIdxStart;
}

/*
 * BitmapSet - 把某一个位设置成value值
 * @btmp: 要检测的位图
 * @bitIdx: 哪个地址
 * @value: 要设置的值（0或1）
 * 
 * 可以通过设置位图某位的值来表达某一个事物是否使用
 */
PUBLIC void BitmapSet(struct Bitmap *btmp, unsigned int bitIdx, char value)
{
   unsigned int byteIdx = bitIdx / 8;
   unsigned int bitOdd  = bitIdx % 8;

   if (value) {
      btmp->bits[byteIdx] |= (BITMAP_MASK << bitOdd);
   } else {
      btmp->bits[byteIdx] &= ~(BITMAP_MASK << bitOdd);
   }
}

/*
 * BitmapChange - 把一位取反
 * @btmp: 要检测的位图
 * @bitIdx: 哪个地址
 */
PUBLIC int BitmapChange(struct Bitmap *btmp, unsigned int bitIdx)
{
   unsigned int byteIdx = bitIdx / 8;
   unsigned int bitOdd  = bitIdx % 8;
   //进行异或
   btmp->bits[byteIdx] ^= (BITMAP_MASK << bitOdd);
   //返回该位
   return (btmp->bits[byteIdx] & (BITMAP_MASK << bitOdd));
}

/*
 * BitmapTestAndChange - 测试并改变该位
 * @btmp: 要检测的位图
 * @bitIdx: 哪个地址
 * 
 * 返回之前的状态
 */
PUBLIC int BitmapTestAndChange(struct Bitmap *btmp, unsigned int bitIdx)
{
   unsigned int byteIdx = bitIdx / 8;
   unsigned int bitOdd  = bitIdx % 8;

   //获取该位
   int ret = btmp->bits[byteIdx] & (BITMAP_MASK << bitOdd);

   //进行异或取反
   btmp->bits[byteIdx] ^= (BITMAP_MASK << bitOdd);
   //返回之前的状态
   return ret;
}
