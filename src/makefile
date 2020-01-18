#----
#File:		makefile
#Auther: 	Jason Hu, Yu Zhu
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
VBOX_MANAGE 	= VBoxManage

BOCHSDBG 	= bochsdbg

# bochsrc for windows
BXRC_WIN 		= $(BOCHS_DIR)bochsrc.win
# bochsrc for linux
BXRC_LINUX 		= $(BOCHS_DIR)bochsrc.linux

# virtual box 虚拟机名称
VBOX_NAME	= "Xbook"

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

# assume 256 kb 
KERNEL_CNTS = 512	

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

# System environment variable.
ifeq ($(OS),Windows_NT)
	HOSTOS		:= windows
else
	ifeq ($(shell uname),Darwin)
		HOSTOS		:= macos
	else
		ifeq ($(shell uname),Linux)
			HOSTOS		:= linux
		else
			HOSTOS		:= unix-like
		endif
	endif
endif

#objs
SRCDIRS	:= $(ARCH)/ init/ kernel/ hal/ share/ drivers/ mm/ fs/ user/ \
			net/ 

INCDIRS	:= include/ include/book/ $(ARCH)/include/ \
	$(ARCH)/include/bus/ \
	$(ARCH)/include/kernel/ \
	$(ARCH)/include/mm/

OBJS	:= $(patsubst %/, %/built-in.o, $(SRCDIRS))

INCLUDE_PATH	:= $(patsubst %, -I $(TOPDIR)/%, $(INCDIRS))

OPTFLAGS		:=
ifeq ("$(origin G)", "command line")
ifeq ($(G),on)
OPTFLAGS		:= -O0 -g -ggdb
endif
endif

CFLAGS	:=  -fno-builtin -Wall -Wunused -fno-PIE -m32 -fno-stack-protector $(OPTFLAGS) $(INCLUDE_PATH) -c
NASM_FLAGS	:= $(INCLUDE_PATH)

#kernel temp file	
KERNEL_ELF =  kernel.elf

PHONY	+= all run compile link disk bochs bochsdbg qemu qemudbg clean
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
ifeq ($(HOSTOS),windows)
	$(BOCHS) -q -f $(BXRC_WIN)
else
	$(BOCHS) -q -f $(BXRC_LINUX)
endif

bochsdbg: disk
ifeq ($(HOSTOS),windows)
	$(BOCHSDBG) -q -f $(BXRC_WIN)
else
	$(BOCHSDBG) -q -f $(BXRC_LINUX)
endif

#-hda $(HDA_IMG) -hdb $(HDB_IMG)
# 网卡配置: 
#	-net nic,vlan=0,model=rtl8139,macaddr=12:34:56:78:9a:be
# 网络模式：
#	1.User mode network(Slirp) :User网络
#		-net user
#	2.Tap/tun network : Tap网络
#		-net tap
# 		-net tap,vlan=0,ifname=tap0
# 音频配置：
# 	a.使用蜂鸣器：-soundhw pcspk
qemu: disk
	$(QEMU) -m 512M \
		-name "XBook Development Platform for x86" \
		-fda $(FLAPPY_IMG) -hda $(HDA_IMG) -hdb $(HDB_IMG) -boot a \
		-net nic,vlan=0,model=rtl8139,macaddr=00:57:20:01:15:02 \
		-net tap,vlan=0,ifname=tap0 \
		-serial stdio \
		-soundhw pcspk

qemudbg: disk
	$(QEMU) -m 512M \
		-name "XBook Development Platform for x86" \
		-S -gdb tcp::10001,ipv4 \
		-fda $(FLAPPY_IMG) -hda $(HDA_IMG) -hdb $(HDB_IMG) -boot a

# virtual box
vbox: disk
	$(VBOX_MANAGE) startvm $(VBOX_NAME) --type gui 

#clean temporary files
clean: $(SRCDIRS)
	$(RM) $(KERNEL_ELF)
.PHONY: $(PHONY)
