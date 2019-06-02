#----
#File:	makefile
#Auther: Jason Hu
#Time: 2019/6/1
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

#----flags----
#out a map file: -Map kernel.map
#i386 link flags
LDFLAGS		= -m elf_i386 -e _start -Ttext 0x80000000

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


#sub dir
ARCH_BOOT_DIR = $(ARCH_DIR)boot/
ARCH_KERNEL_DIR = $(ARCH_DIR)kernel/


#----objs----
#arch kernel objs
ARCH_KERNEL_OBJS = $(ARCH_KERNEL_DIR)entry.o \
	$(ARCH_KERNEL_DIR)arch.o \
	$(ARCH_KERNEL_DIR)page.o \
	$(ARCH_KERNEL_DIR)x86.o \
	$(ARCH_KERNEL_DIR)vga.o \
	$(ARCH_KERNEL_DIR)ards.o \

#arch objs
ARCH_OBJS = $(ARCH_KERNEL_OBJS)

#init objs
INIT_OBJS = $(INIT_DIR)main.o \

#kernel objs
KERNEL_OBJS = $(KERNEL_DIR)bitmap.o \

#share objs
SHARE_OBJS = $(SHARE_DIR)string.o \
	$(SHARE_DIR)vsprintf.o

#objs
OBJS =  $(ARCH_OBJS) $(INIT_OBJS) $(KERNEL_OBJS) $(SHARE_OBJS)


#----bin and elf----
#boot temp file
BOOT_BIN =  $(ARCH_BOOT_DIR)boot.bin
LOADER_BIN =  $(ARCH_BOOT_DIR)loader.bin

#kernel temp file
KERNEL_ELF =  $(KERNEL_DIR)kernel.elf

#----cmd----	
.PHONY: default

default: compile link disk

all: compile link disk qemu

#compile file
compile:
	cd $(ARCH_DIR) && $(MAKE)
	cd $(INIT_DIR) && $(MAKE)
	cd $(KERNEL_DIR) && $(MAKE)
	cd $(SHARE_DIR) && $(MAKE)

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
	$(QEMU) -m 256 -fda $(FLAPPY_IMG)  \
	-boot a


#clean temporary files
clean:
	cd $(ARCH_DIR) && $(MAKE) clean
	cd $(KERNEL_DIR) && $(MAKE) clean
	cd $(INIT_DIR) && $(MAKE) clean
	cd $(SHARE_DIR) && $(MAKE) clean
