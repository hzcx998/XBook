#Makefile for boot
#auther: Hu Zicheng
#time: 2019/1/24


#The tools name
NASM 		= nasm
CC			= gcc
LD 			= ld
DD 			= dd

#The file path
USER_DIR = ../../src/include/user/
SHARE_DIR = ../../src/include/share/

LIB_DIR = ../../src/
CORE_DIR = core/

BIN = a.out

BIN_OFF = 0
BIN_CNTS = 620

#img dir
IMG_DIR = ../../img/

FLAPPY_IMG = $(IMG_DIR)d.img

#flags
ASM_FLAGS	= -I $(USER_DIR) -I $(SHARE_DIR) -f elf
C_LIB_FLAGS	= -I $(USER_DIR) -I $(SHARE_DIR) -I $(CORE_DIR) -c -fno-builtin -Wall -Wunused -std=gnu99 -Wsequence-point

LDFLAGS		= -m elf_i386 -e _start -Ttext 0x00001000

OBJS =  _start.o\
		infoNES_System_Book.o

LD_OBJS = $(LIB_DIR)libc.a \

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
	

	
#Clean temporary files
clean:
	-rm *.o
	-rm $(BIN)

%.o: %.asm
	$(NASM) $(ASM_FLAGS) -o $@ $<
	
%.o: %.c
	$(CC) $(C_LIB_FLAGS) -o $@ $<
