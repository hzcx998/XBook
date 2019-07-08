/*
 * file:		arch/x86/kernel/ards.c
 * auther:		Jason Hu
 * time:		2019/6/3
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <ards.h>
#include <book/debug.h>

/* 全局变量ards*/
struct Ards *ards;
uint64_t InitArds()
{
	PART_START("Ards");
	uint64_t totalSize = 0;

	unsigned int ardsNum =  *((uint64_t *)ARDS_ADDR);	//ards 结构数
	
	if (ardsNum > MAX_ARDS_NR) {
		ardsNum = MAX_ARDS_NR;
	}
	ards = (struct Ards *) (ARDS_ADDR+4);	//ards 地址
	//printk("\n |- ards nr %d ards address %x\n",ardsNum, ards);
	printk("\n");
	int i;
	for(i = 0; i < ardsNum; i++){
		//寻找可用最大内存
		if(ards->type == 1){
			//冒泡排序获得最大内存
			if(ards->baseLow+ards->lengthLow > totalSize){
				totalSize = ards->baseLow+ards->lengthLow;
			}
			
		}
		#ifdef CONFIG_ARDS_DEBUG
		printk(" |- base %x length %x type:%d\n",ards->baseLow, ards->lengthLow, ards->type);
		#endif
		ards++;
	}
	PART_END();
	return totalSize;
}