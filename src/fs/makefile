#----
#File: kernel/makefile
#Auther: Jason Hu
#Time: 2019/6/2
#copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
#----

#The tools name
CC			= gcc
NASM		= nasm 

INCLUDE_PATH = ../include/

ABSURD_PATH = absurd/
BOFS_PATH = bofs/


#flags
C_FLAGS	= -c -I $(INCLUDE_PATH) \
	-fno-builtin -nostdlib \
	-Wall -Wunused \
	-m32 -fno-stack-protector

ASM_FLAGS	= -I $(INCLUDE_PATH) -f elf

OBJS = 	interface.o \
		partition.o \
		directory.o \
		file.o \
		flat.o \
		bitmap.o \
		node.o \
		
#First read here
.PHONY: all

all: compile

#Compile files
compile: $(OBJS)
	cd $(BOFS_PATH) && make

#Clean temporary files
clean:
	-rm *.o
	cd $(BOFS_PATH) && make clean

%.o: %.c
	$(CC) $(C_FLAGS) -o $@ $<

%.o: %.asm
	$(NASM) $(ASM_FLAGS) -o $@ $<
		