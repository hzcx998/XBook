#Makefile for test
#auther: Hu Zicheng
#time: 2019/1/24

#The tools name
NASM 		= nasm
CC			= gcc
LD 			= ld
DD 			= dd

#The file path
LIB_H_DIR = ../../libary/libc/include
LIB_A_DIR = ../../libary/libc/

BIN = a.out

BIN_OFF = 0 
BIN_CNTS = 100

#img dir
IMG_DIR = ../../developments/image/

DISK_IMG = d.img
DISK_VHD = d.vhd
DISK_VMDK = d-flat.vmdk

HD_IMG = $(IMG_DIR)$(DISK_IMG)
HD_VHD = $(IMG_DIR)$(DISK_VHD)
HD_VMDK = $(IMG_DIR)$(DISK_VMDK)

#flags
ASM_FLAGS	= -I $(LIB_H_DIR) \
			-f elf
C_FLAGS	= -I $(LIB_H_DIR) \
			-c -fno-builtin -Wall -Wunused -m32

LDFLAGS		= -m elf_i386 -e _start -Ttext 0x00001000

OBJS =  _start.o \
		main.o

LD_OBJS = 	$(LIB_A_DIR)libc.a \

#First read here
.PHONY: all

all: compile link disk

#Compile files
compile: $(OBJS)

link: $(BIN)
$(BIN): $(OBJS) $(LD_OBJS)
	$(LD) $(LDFLAGS) -o $(BIN) $(OBJS) $(LD_OBJS)


disk: img vhd vmdk

img:
	$(DD) if=$(BIN) of=$(HD_IMG) bs=512 seek=$(BIN_OFF) count=$(BIN_CNTS) conv=notrunc
	
vhd:
	$(DD) if=$(BIN) of=$(HD_VHD) bs=512 seek=$(BIN_OFF) count=$(BIN_CNTS) conv=notrunc
	
vmdk:
	$(DD) if=$(BIN) of=$(HD_VMDK) bs=512 seek=$(BIN_OFF) count=$(BIN_CNTS) conv=notrunc
	
qemu: compile link img
	
vbox: compile link vhd

vmware: compile link vmdk
	
#Clean temporary files
clean:
	-rm *.o
	-rm $(BIN)

%.o: %.asm
	$(NASM) $(ASM_FLAGS) -o $@ $<
	
%.o: %.c
	$(CC) $(C_FLAGS) -o $@ $<
