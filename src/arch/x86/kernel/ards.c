/*
 * file:		arch/x86/kernel/ards.c
 * auther:		Jason Hu
 * time:		2019/6/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */


#include <book/config.h>

#ifdef _CONFIG_ARCH_X86_

#include <ards.h>
#include <vga.h>


uint32_t init_ards()
{
	uint32_t total_size = 0;

	uint16_t ards_nr =  *((uint16_t *)ARDS_NR);	//ards 结构数
	struct ards_s *ards = (struct ards_s *) ARDS_ADDR;	//ards 地址
	int i;
	for(i = 0; i < ards_nr; i++){
		//寻找可用最大内存
		if(ards->type == 1){
			//冒泡排序获得最大内存
			if(ards->base_low+ards->length_low > total_size){
				total_size = ards->base_low+ards->length_low;
			}
			
		}
		ards++;
	}
	return total_size;
}

#endif //_CONFIG_ARCH_X86_


