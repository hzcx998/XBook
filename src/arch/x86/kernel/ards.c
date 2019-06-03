/*
 * file:		arch/x86/kernel/ards.c
 * auther:		Jason Hu
 * time:		2019/6/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <ards.h>
#include <vga.h>

uint32_t init_ards()
{
	printk("> init ards start.\n");

	uint32_t total_size = 0;

	uint32_t ards_nr =  *((uint32_t *)ARDS_ADDR);	//ards 结构数
	
	if (ards_nr > MAX_ARDS_NR) {
		ards_nr = MAX_ARDS_NR;
	}
	struct ards_s *ards = (struct ards_s *) (ARDS_ADDR+4);	//ards 地址
	printk("ards nr %d ards address %x\n",ards_nr, ards);
	int i;
	for(i = 0; i < ards_nr; i++){
		//寻找可用最大内存
		if(ards->type == 1){
			//冒泡排序获得最大内存
			if(ards->base_low+ards->length_low > total_size){
				total_size = ards->base_low+ards->length_low;
			}
			printk("base %x length %x\n",ards->base_low, ards->length_low);
		}
		
		ards++;
	}
	printk("< init page done.\n");

	return total_size;
}