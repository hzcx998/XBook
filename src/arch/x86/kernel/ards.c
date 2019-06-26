/*
 * file:		arch/x86/kernel/ards.c
 * auther:		Jason Hu
 * time:		2019/6/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <ards.h>
#include <book/debug.h>

uint64_t InitArds()
{
	uint64_t totalSize = 0;

	unsigned int ardsNum =  *((uint64_t *)ARDS_ADDR);	//ards 结构数
	
	if (ardsNum > MAX_ARDS_NR) {
		ardsNum = MAX_ARDS_NR;
	}
	struct Ards *ards = (struct Ards *) (ARDS_ADDR+4);	//ards 地址
	//printk("ards nr %d ards address %x\n",ardsNum, ards);
	int i;
	for(i = 0; i < ardsNum; i++){
		//寻找可用最大内存
		if(ards->type == 1){
			//冒泡排序获得最大内存
			if(ards->baseLow+ards->lengthLow > totalSize){
				totalSize = ards->baseLow+ards->lengthLow;
			}
			//printk("base %x length %x\n",ards->baseLow, ards->lengthLow);
		}
		
		ards++;
	}
	return totalSize;
}