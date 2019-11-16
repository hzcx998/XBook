#----
#File:		makefile
#Auther: 	Jason Hu
#Time: 		2019/6/1
#copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
#----

#----tools----
#tool dir

PHONY		:=
all:

BOCHS_DIR = ../bochs/

NASM 		= nasm
CC			= gcc
LD 			= ld
DD 			= dd
QEMU 		= qemu-system-i386
BOCHS 		= bochs
BOCHSDBG 	= bochsdbg
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
LDFLAGS		= -m elf_i386

#system disk
LOADER_OFF = 2		
LOADER_CNTS = 8

KERNEL_OFF = 10
KERNEL_CNTS = 400

#----dir----
#arch
ARCH = arch/x86

ARCH_BOOT_DIR = $(ARCH)/boot/
ARCH_KERNEL_DIR = $(ARCH)/kernel/

#----bin and elf----
#boot temp file
BOOT_BIN =  $(ARCH_BOOT_DIR)boot.bin
LOADER_BIN =  $(ARCH_BOOT_DIR)loader.bin

TOPDIR	:= $(CURDIR)

#objs
SRCDIRS	:= $(ARCH)/ init/ kernel/ hal/ share/ driver/ mm/ fs/ user/

INCDIRS	:= include/ include/book/ $(ARCH)/include/ \
	$(ARCH)/include/bus/ \
	$(ARCH)/include/kernel/ \
	$(ARCH)/include/mm/

OBJS	:= $(patsubst %/, %/built-in.o, $(SRCDIRS))

INCLUDE_PATH	:= $(patsubst %, -I $(TOPDIR)/%, $(INCDIRS))

CFLAGS	:=  -fno-builtin -Wall -Wunused -fno-PIE -m32 -fno-stack-protector $(INCLUDE_PATH) -c
NASM_FLAGS	:= $(INCLUDE_PATH)

#kernel temp file	
KERNEL_ELF =  kernel.elf

PHONY	+= all run compile link disk bochs bochsdbg qemu clean
PHONY	+= $(SRCDIRS)

export NASM NASM_FLAGS CC LD CFLAGS LDFLAGS TOPDIR

#----cmd----
all: disk
run: qemu

#compile file
$(OBJS) : $(SRCDIRS) ;

$(SRCDIRS) :
ifneq ($(strip $(MAKECMDGOALS)),clean)
	$(MAKE) -C $@ -f $(TOPDIR)/Makefile.build
else
	$(MAKE) -C $@ -f $(TOPDIR)/Makefile.build clean
endif

$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS)  -e _start -Ttext 0x80100000 -o $(KERNEL_ELF) $(OBJS)

#write file into disk
disk: $(KERNEL_ELF)
	$(DD) if=$(BOOT_BIN) of=$(FLAPPY_IMG) bs=512 count=1 conv=notrunc
	$(DD) if=$(LOADER_BIN) of=$(FLAPPY_IMG) bs=512 seek=$(LOADER_OFF) count=$(LOADER_CNTS) conv=notrunc
	$(DD) if=$(KERNEL_ELF) of=$(FLAPPY_IMG) bs=512 seek=$(KERNEL_OFF) count=$(KERNEL_CNTS) conv=notrunc

#run kernel in vm
bochs: disk
	$(BOCHS) -q -f $(BXRC)

bochsdbg: disk
	$(BOCHSDBG) -q -f $(BXRC)

#-hda $(HDA_IMG) -hdb $(HDB_IMG)
qemu: disk
	$(QEMU) -m 512 -fda $(FLAPPY_IMG) \
	-hda $(HDA_IMG) -hdb $(HDB_IMG) \
	-boot a

#clean temporary files
clean: $(SRCDIRS)
	$(RM) $(KERNEL_ELF)
.PHONY: $(PHONY)