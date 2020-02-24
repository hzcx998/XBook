#Makefile for boot
#auther: Hu Zicheng
#time: 2019/1/24


#The tools name
NASM 		= nasm
CC			= gcc
LD 			= ld
DD 			= dd

#The file path
LIB_H_DIR = ../../../src/include/lib/

LIB_A_DIR = ../../../src/
CORE_DIR = core/

BIN = a.out

BIN_OFF = 0
BIN_CNTS = 640

#img dir
DEVELOPMENTS_DIR = ../../../developments/

FLAPPY_IMG = $(DEVELOPMENTS_DIR)image/d.img


VBOX_IMG = $(DEVELOPMENTS_DIR)image/d.vhd


#flags
ASM_FLAGS	= -I $(LIB_H_DIR) -f elf
C_LIB_FLAGS	= -I $(LIB_H_DIR) -I $(CORE_DIR) -c -fno-builtin -Wall -Wunused -std=gnu99 -Wsequence-point

LDFLAGS		= -m elf_i386 -e _start -Ttext 0x00001000

OBJS =  _start.o\
		infoNES_System_Book.o

LD_OBJS = $(LIB_A_DIR)libc.a \

#First read here
.PHONY: all

all: compile link disk

#Compile files
compile: $(OBJS)

link: $(BIN)
$(BIN): $(OBJS) $(LD_OBJS)
	$(LD) $(LDFLAGS) -o $(BIN) $(OBJS) $(LD_OBJS)

disk:
	$(DD) if=$(BIN) of=$(FLAPPY_IMG) bs=512 seek=$(BIN_OFF) count=$(BIN_CNTS) conv=notrunc
	

vbox: compile link
	$(DD) if=$(BIN) of=$(VBOX_IMG) bs=512 seek=$(BIN_OFF) count=$(BIN_CNTS) conv=notrunc
	

	
#Clean temporary files
clean:
	-rm *.o
	-rm $(BIN)

%.o: %.asm
	$(NASM) $(ASM_FLAGS) -o $@ $<
	
%.o: %.c
	$(CC) $(C_LIB_FLAGS) -o $@ $<
