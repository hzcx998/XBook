;----
;file:      arch/x86/include/boot/const.inc
;auther:    Jason Hu
;time:      2019/6/2
;copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
;----

;----boot const----
;we read loader.bin from disk sector 2
LOADER_OFF EQU 2	

;we assume loader.bin size is 4kb(8 sectors)	
LOADER_CNTS EQU 8		

;we read loader.bin to memory 0x90000
;so the segment is 0x9000, we are in real mode now.
LOADER_SEG EQU 0x9000	

;---loader const----
;we read kernel.elf from disk sector 10 
KERNEL_OFF EQU 10	

;----we assume that kernel max is 200 kb (0x32000 byte)----

;we read kernel.elf to memory 0x10000
;so the segment is 0x1000, we are in real mode now.
KERNEL_SEG EQU 0x1000

;kernel.elf phy addr is KERNEL_SEG*16, 
;0x1000*16 = 0x10000
KERNEL_PHY_ADDR EQU KERNEL_SEG*16

;----we assume that file max is 312 kb (0x4e000 byte)----
;we read init.bin from disk sector 500 
FILE_OFF EQU 500

FILE_SEG EQU 0x4200

FILE2_OFF EQU 600

FILE2_SEG EQU 0x5000

FILE3_OFF EQU 700

FILE3_SEG EQU 0x6000

FILE4_OFF EQU 800

FILE4_SEG EQU 0x7000


;we will read kernel from kerne.elf to 1M after we jmp to protect mode,
;so this is our kernel finally addr
KERNEL_START_ADDR EQU 0x80100000

;loader stack top is 0x90000
LOADER_STACK_TOP EQU 0x90000

;we can read sector less than 128 sector. so I set block size,
;every time we load 128 sectors
BLOCK_SIZE EQU 128


ARDS_SEG	EQU		0x100	;ards start segment

;分页机制
;内核页目录表地址
PAGE_DIR_PHY_ADDR EQU   0x201000
;内核页表地址
PAGE_TBL_PHY_ADDR EQU   0x202000

;显存页表地址
VRAM_PT_PHY_ADDR    EQU 0x203000

PAGE_P_1	EQU  	1	; 0001 exist in memory
PAGE_P_0	EQU  	0	; 0000 not exist in memory
PAGE_RW_R  	EQU     0	; 0000 R/W read/execute
PAGE_RW_W  	EQU     2	; 0010 R/W read/write/execute
PAGE_US_S  	EQU     0	; 0000 U/S system level, cpl0,1,2
PAGE_US_U  	EQU     4	; 0100 U/S user level, cpl3

KERNEL_PAGE_ATTR    EQU (PAGE_US_S | PAGE_RW_W | PAGE_P_1)
