#Makefile for boot
#auther: Hu Zicheng
#time: 2019/1/24


#The tools name
NASM 		= nasm
CC			= gcc
LD 			= ld
DD 			= dd

#The file path
INCLUDE_DIR = ../../src/include/
H_USER_DIR = $(INCLUDE_DIR)user/
H_SHARE_DIR = $(INCLUDE_DIR)share/

USER_DIR = ../../src/user/
SHARE_DIR = ../../src/share/

BIN = a.out

BIN_OFF = 600 
BIN_CNTS = 50

#img dir
IMG_DIR = ../../img/

DISK_IMG = a.img

DISK_VHD = a.vhd

HD_IMG = $(IMG_DIR)$(DISK_IMG)
HD_VHD = $(IMG_DIR)$(DISK_VHD)

#flags
ASM_FLAGS	= -I $(INCLUDE_DIR) -I $(H_USER_DIR) -I $(H_SHARE_DIR) \
			-f elf
C_FLAGS	= -I $(INCLUDE_DIR) -I $(H_USER_DIR) -I $(H_SHARE_DIR) \
			-c -fno-builtin -Wall -Wunused -m32

#LDFLAGS		= --oformat binary -e _start -Ttext 0x00000000

LDFLAGS		= -m elf_i386 -e _start -Ttext 0x00000000

OBJS =  _start.o \
		main.o

LD_OBJS = 	$(USER_DIR)lib/log.o\
			$(USER_DIR)lib/mmap.o\
			$(USER_DIR)lib/fork.o\
			$(USER_DIR)lib/printf.o\
			$(USER_DIR)lib/getpid.o\
			$(USER_DIR)lib/execv.o\
			$(USER_DIR)lib/exit.o\
			$(USER_DIR)lib/_wait.o\
			$(SHARE_DIR)vsprintf.o\
			$(SHARE_DIR)string.o\
			
#First read here
.PHONY: all

all: compile link img

#Compile files
compile: $(OBJS)

link: $(BIN)
$(BIN): $(OBJS) $(LD_OBJS)
	$(LD) $(LDFLAGS) -o $(BIN) $(OBJS) $(LD_OBJS)

img:
	$(DD) if=$(BIN) of=$(HD_IMG) bs=512 seek=$(BIN_OFF) count=$(BIN_CNTS) conv=notrunc
	
vhd:
	$(DD) if=$(BIN) of=$(HD_VHD) bs=512 seek=$(BIN_OFF) count=$(BIN_CNTS) conv=notrunc
	
qemu: compile link img
	
vbox: compile link vhd

	
#Clean temporary files
clean:
	-rm *.o
	-rm $(BIN)

%.o: %.asm
	$(NASM) $(ASM_FLAGS) -o $@ $<
	
%.o: %.c
	$(CC) $(C_FLAGS) -o $@ $<
