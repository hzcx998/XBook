/*
 * file:		kernel/bitmap.c
 * auther:	Jason Hu
 * time:		2019/6/2
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <share/string.h>
#include <book/bitmap.h>

/*
 * 功能: 初始化一个位图结构
 * 参数: btmp 要初始化的位图结构的地址
 * 返回: 无
 * 说明: 初始化一个位图结构，其实就是把位图数据指针指向的地址清0
 */
void bitmap_init(struct bitmap_s* btmp) 
{
   memset(btmp->bits, 0, btmp->btmp_bytes_len);   
}

/*
 * 功能: 检测位图某位是否为1
 * 参数: btmp 要检测的位图
 *       bit_idx 要检测哪一位
 * 返回: true 说明该位是1
 *       false 说明该位是0
 * 说明: 可以通过检测寻找哪些是已经使用的，哪些还没有使用
 */
bool bitmap_scanTest(struct bitmap_s* btmp, uint32_t bit_idx) 
{
   uint32_t byte_idx = bit_idx / 8;   
   uint32_t bit_odd  = bit_idx % 8;  
   return (btmp->bits[byte_idx] & (BITMAP_MASK << bit_odd));
}

/*
 * 功能: 扫描n个位
 * 参数: btmp 要检测的位图
 *       cnt 要扫描多少位
 * 返回: -1 失败
 *       大于0 返回扫描到的首个位图地址
 * 说明: 可以通过检测寻找哪些是已经使用的，哪些还没有使用
 */
int bitmap_scan(struct bitmap_s* btmp, uint32_t cnt) {
   uint32_t idx_byte = 0;

   while (( 0xff == btmp->bits[idx_byte]) && (idx_byte < btmp->btmp_bytes_len)) {
      idx_byte++;
   }

   
   if (idx_byte == btmp->btmp_bytes_len) {  
      return -1;
   }

   int idx_bit = 0;

   while ((uint8_t)(BITMAP_MASK << idx_bit) & btmp->bits[idx_byte]) { 
	 idx_bit++;
   }
	 
   int bit_idx_start = idx_byte * 8 + idx_bit;  
   if (cnt == 1) {
      return bit_idx_start;
   }

   uint32_t bit_left = (btmp->btmp_bytes_len * 8 - bit_idx_start);
   uint32_t next_bit = bit_idx_start + 1;
   uint32_t count = 1;

   bit_idx_start = -1;
   while (bit_left-- > 0) {
      if (!(bitmap_scanTest(btmp, next_bit))) {
	 count++;
      } else {
	 count = 0;
      }
      if (count == cnt) {
	 bit_idx_start = next_bit - cnt + 1;
	 break;
      }
      next_bit++;          
   }
   return bit_idx_start;
}

/*
 * 功能: 把某一个位设置成value值
 * 参数: btmp 要检测的位图
 *       bit_idx 哪个地址
 *       value 要设置的值（0或1）
 * 返回: 无
 * 说明: 可以通过设置位图某位的值来表达某一个事物是否使用
 */
void bitmap_set(struct bitmap_s* btmp, uint32_t bit_idx, int8_t value)
{
   uint32_t byte_idx = bit_idx / 8;
   uint32_t bit_odd  = bit_idx % 8;

   if (value) {
      btmp->bits[byte_idx] |= (BITMAP_MASK << bit_odd);
   } else {
      btmp->bits[byte_idx] &= ~(BITMAP_MASK << bit_odd);
   }
}

