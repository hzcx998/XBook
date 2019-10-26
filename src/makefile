#----
#File:		makefile
#Auther: 	Jason Hu
#Time: 		2019/6/1
#copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
#----

#----tools----
#tool dir
BOCHS_DIR = ../bochs/

NASM 		= nasm
CC			= gcc
LD 			= ld
DD 			= dd
QEMU 		= qemu-system-i386
BOCHS 		= bochs
BOCHSDBG 		= bochsdbg
BXRC 		= $(BOCHS_DIR)bochsrc.bxrc

#----img----
#img dir
IMG_DIR = ../img/

FLAPPY_IMG 	= $(IMG_DIR)a.img
HDA_IMG		= $(IMG_DIR)c.img
HDB_IMG		= $(IMG_DIR)d.img
HDC_IMG		= $(IMG_DIR)e.img
HDD_IMG		= $(IMG_DIR)f.img

#----flags----
#out a map file: -Map kernel.map
#i386 link flags
LDFLAGS		= -m elf_i386 -e _start -Ttext 0x80100000

#system disk
LOADER_OFF = 2		
LOADER_CNTS = 8

KERNEL_OFF = 10
KERNEL_CNTS = 400

#----dir----
#arch
ARCH_X86 = arch/x86/
ARCH_IA64 = arch/ia64/


#current arch
ARCH_DIR = $(ARCH_X86)
INIT_DIR = init/
KERNEL_DIR = kernel/
SHARE_DIR = share/
HAL_DIR = hal/
DRIVER_DIR = driver/
MM_DIR = mm/
FS_DIR = fs/
USER_DIR = user/

#sub dir
ARCH_BOOT_DIR = $(ARCH_DIR)boot/
ARCH_KERNEL_DIR = $(ARCH_DIR)kernel/

#----objs----
#arch kernel objs
ARCH_KERNEL_OBJS = $(ARCH_KERNEL_DIR)entry.o \
	$(ARCH_KERNEL_DIR)arch.o \
	$(ARCH_KERNEL_DIR)x86.o \
	$(ARCH_KERNEL_DIR)segment.o \
	$(ARCH_KERNEL_DIR)gate.o \
	$(ARCH_KERNEL_DIR)pic.o \
	$(ARCH_KERNEL_DIR)interrupt.o \
	$(ARCH_KERNEL_DIR)cpu.o \
	$(ARCH_KERNEL_DIR)cmos.o \
	
#arch objs
ARCH_OBJS = $(ARCH_KERNEL_OBJS) \
			$(ARCH_DIR)mm/page.o \
			$(ARCH_DIR)mm/bootmem.o \
			$(ARCH_DIR)mm/ards.o \
			$(ARCH_DIR)mm/phymem.o \
			$(ARCH_DIR)bus/pci.o \

#init objs
INIT_OBJS = $(INIT_DIR)main.o \

#kernel objs
KERNEL_OBJS = 	$(KERNEL_DIR)bitmap.o \
				$(KERNEL_DIR)hal.o \
				$(KERNEL_DIR)debug.o \
				$(KERNEL_DIR)syscall.o \
				$(KERNEL_DIR)task.o \
				$(KERNEL_DIR)schedule.o \
				$(KERNEL_DIR)switch.o \
				$(KERNEL_DIR)sync_lock.o \
				$(KERNEL_DIR)sleep_wakeup.o \
				$(KERNEL_DIR)atomic.o \
				$(KERNEL_DIR)exit_wait.o \
				$(KERNEL_DIR)interrupt.o \
				$(KERNEL_DIR)softirq.o \
				$(KERNEL_DIR)workqueue.o \
				$(KERNEL_DIR)ioqueue.o \
				$(KERNEL_DIR)device.o \
				$(KERNEL_DIR)iostream.o \
				$(KERNEL_DIR)block/block.o \
				$(KERNEL_DIR)block/blkdev.o \
				$(KERNEL_DIR)block/buffer.o \
				$(KERNEL_DIR)block/disk.o \
				$(KERNEL_DIR)block/request.o \
				$(KERNEL_DIR)block/elevator.o \
				$(KERNEL_DIR)char/char.o \
				$(KERNEL_DIR)char/chrdev.o \
				
#hal objs
HAL_OBJS = 	$(HAL_DIR)cpu.o \
			$(HAL_DIR)ram.o \

#share objs
SHARE_OBJS = 	$(SHARE_DIR)string.o \
				$(SHARE_DIR)vsprintf.o \
				$(SHARE_DIR)math.o \
				$(SHARE_DIR)ctype.o

#driver objs
DRIVER_OBJS = 	$(DRIVER_DIR)clock.o \
				$(DRIVER_DIR)console.o \
				$(DRIVER_DIR)timer.o \
				$(DRIVER_DIR)keyboard.o \
				$(DRIVER_DIR)ide.o \
				$(DRIVER_DIR)ramdisk.o \

#mm objs
MM_OBJS = 	$(MM_DIR)mmu.o \
			$(MM_DIR)vmspace.o \
			$(MM_DIR)fork.o \
			$(MM_DIR)exec.o \
			$(MM_DIR)vmarea.o \
			$(MM_DIR)memcache.o \
			$(MM_DIR)dma.o \

#fs objs
FS_OBJS = 	$(FS_DIR)interface.o \
			$(FS_DIR)partition.o \
			$(FS_DIR)bofs/main.o \
			$(FS_DIR)bofs/super_block.o \
			$(FS_DIR)bofs/inode.o \
			$(FS_DIR)bofs/dir_entry.o \
			$(FS_DIR)bofs/bitmap.o \
			$(FS_DIR)bofs/dir.o \
			$(FS_DIR)bofs/file.o \
			$(FS_DIR)bofs/device.o \
						
#user objs
USER_OBJS = $(USER_DIR)lib/log.o \
			$(USER_DIR)lib/fork.o \
			$(USER_DIR)lib/mmap.o \
			$(USER_DIR)lib/munmap.o \
			$(USER_DIR)lib/getpid.o \
			$(USER_DIR)lib/printf.o \
			$(USER_DIR)lib/execv.o \
			$(USER_DIR)lib/sleep.o \
			$(USER_DIR)lib/msleep.o \
			$(USER_DIR)lib/exit.o \
			$(USER_DIR)lib/_wait.o \
			$(USER_DIR)lib/_brk.o \
			$(USER_DIR)lib/brk.o \
			$(USER_DIR)lib/malloc.o \
			$(USER_DIR)lib/file.o \
			$(USER_DIR)lib/dir.o \

#objs
OBJS =  $(ARCH_OBJS) $(INIT_OBJS) $(KERNEL_OBJS) $(HAL_OBJS) $(SHARE_OBJS) \
		$(DRIVER_OBJS) $(MM_OBJS) $(FS_OBJS) $(USER_OBJS)


#----bin and elf----
#boot temp file
BOOT_BIN =  $(ARCH_BOOT_DIR)boot.bin
LOADER_BIN =  $(ARCH_BOOT_DIR)loader.bin

#kernel temp file	
KERNEL_ELF =  $(KERNEL_DIR)kernel.elf

#----cmd----	
.PHONY: all

all: compile link disk

run: compile link disk qemu

#compile file
compile:
	cd $(ARCH_DIR) && $(MAKE)
	cd $(INIT_DIR) && $(MAKE)
	cd $(KERNEL_DIR) && $(MAKE)
	cd $(SHARE_DIR) && $(MAKE)
	cd $(HAL_DIR) && $(MAKE)
	cd $(DRIVER_DIR) && $(MAKE)
	cd $(MM_DIR) && $(MAKE)
	cd $(FS_DIR) && $(MAKE)
	cd $(USER_DIR) && $(MAKE)


link: $(KERNEL_ELF)

$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) $(OBJS)
	
#write file into disk
disk:
	$(DD) if=$(BOOT_BIN) of=$(FLAPPY_IMG) bs=512 count=1 conv=notrunc
	$(DD) if=$(LOADER_BIN) of=$(FLAPPY_IMG) bs=512 seek=$(LOADER_OFF) count=$(LOADER_CNTS) conv=notrunc
	$(DD) if=$(KERNEL_ELF) of=$(FLAPPY_IMG) bs=512 seek=$(KERNEL_OFF) count=$(KERNEL_CNTS) conv=notrunc

#run kernel in vm
bochs:
	$(BOCHS) -q -f $(BXRC)
	
bochsdbg:
	$(BOCHSDBG) -q -f $(BXRC)

#-hda $(HDA_IMG) -hdb $(HDB_IMG)
qemu:
	$(QEMU) -m 512 -fda $(FLAPPY_IMG) \
	-hda $(HDA_IMG) -hdb $(HDB_IMG)\
	-boot a

#clean temporary files
clean:
	cd $(ARCH_DIR) && $(MAKE) clean
	cd $(KERNEL_DIR) && $(MAKE) clean
	cd $(INIT_DIR) && $(MAKE) clean
	cd $(SHARE_DIR) && $(MAKE) clean
	cd $(HAL_DIR) && $(MAKE) clean
	cd $(DRIVER_DIR) && $(MAKE) clean
	cd $(MM_DIR) && $(MAKE) clean
	cd $(FS_DIR) && $(MAKE) clean
	cd $(USER_DIR) && $(MAKE) clean