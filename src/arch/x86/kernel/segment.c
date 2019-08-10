/*
 * file:		arch/x86/kernel/segment.c
 * auther:		Jason Hu
 * time:		2019/6/23
 * copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
 */

#include <x86.h>
#include <segment.h>
#include <cpu.h>
#include <book/debug.h>

/* 
 * Global Descriptor Table
 */
struct SegmentDescriptor *gdt;

PUBLIC void InitSegmentDescriptor()
{
	PART_START("Segment descriptor");

	gdt = (struct SegmentDescriptor *) GDT_VADDR;

	int i;
	for (i = 0; i <= GDT_LIMIT/8; i++) {
		SetSegmentDescriptor(gdt + i, 0, 0, 0);
	}
	// 内核代码段和数据段
	SetSegmentDescriptor(gdt + INDEX_KERNEL_C, 0xffffffff,   0x00000000, DA_CR | DA_DPL0 | DA_32 | DA_G);
	SetSegmentDescriptor(gdt + INDEX_KERNEL_RW, 0xffffffff,   0x00000000, DA_DRW | DA_DPL0 | DA_32 | DA_G);
	// tss 段
	SetSegmentDescriptor(gdt + INDEX_TSS, sizeof(tss) - 1, (uint32_t )&tss, DA_386TSS);
	// 用户代码段和数据段
	SetSegmentDescriptor(gdt + INDEX_USER_C, 0xffffffff, 0x00000000, DA_CR | DA_DPL3 | DA_32 | DA_G);
	SetSegmentDescriptor(gdt + INDEX_USER_RW, 0xffffffff, 0x00000000, DA_DRW | DA_DPL3 | DA_32 | DA_G);

	LoadGDTR(GDT_LIMIT, GDT_VADDR);

	PART_END();
}

PUBLIC void SetSegmentDescriptor(struct SegmentDescriptor *descriptor, unsigned int limit, \
		unsigned int base, unsigned int attributes)
{
	descriptor->limitLow    = limit & 0xffff;
	descriptor->baseLow     = base & 0xffff;
	descriptor->baseMid     = (base >> 16) & 0xff;
	descriptor->accessRight = attributes & 0xff;
	descriptor->limitHigh   = ((limit >> 16) & 0x0f) | ((attributes >> 8) & 0xf0);
	descriptor->baseHigh    = (base >> 24) & 0xff;
}


