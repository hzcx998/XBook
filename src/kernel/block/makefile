#----
#File: 		kernel/block/makefile
#Auther: 	Jason Hu
#Time: 		2019/10/13
#copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
#----

#The tools name
CC			= gcc
NASM		= nasm

#The file path
INCLUDE_PATH = ../../include/

#flags
C_FLAGS	= -c -fno-builtin -nostdlib \
	-I $(INCLUDE_PATH) \
	-Wall -Wunused \
	-m32 -fno-stack-protector

ASM_FLAGS	= -I $(INCLUDE_PATH) -f elf

OBJS = 	block.o \
		blkdev.o \
		buffer.o \
		disk.o \
		request.o \
		elevator.o \

#First read here
.PHONY: all

all: compile

#Compile files
compile: $(OBJS)

#Clean temporary files
clean:
	-rm *.o
	
%.o: %.c
	$(CC) $(C_FLAGS) -o $@ $<

%.o: %.asm
	$(NASM) $(ASM_FLAGS) -o $@ $<
	