#----
#File: user/lib/makefile
#Auther: Jason Hu
#Time: 2019/7/29
#copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
#----

#The tools name
CC			= gcc
NASM		= nasm

#The file path
INCLUDE_PATH = ../../include/

SYS_PATH = $(INCLUDE_PATH)book/

#flags
C_FLAGS	= -c -fno-builtin -nostdlib \
	-I $(INCLUDE_PATH) -I $(SYS_PATH) \
	-Wall -Wunused \
	-m32 -fno-stack-protector

ASM_FLAGS	= -I $(INCLUDE_PATH) -I $(SYS_PATH) -f elf

OBJS =  log.o \
		fork.o \
		mmap.o \
		munmap.o \
		getpid.o \
		printf.o \
		execv.o \
		sleep.o \
		msleep.o \
		exit.o \
		_wait.o \
		_brk.o \
		brk.o \
		malloc.o \
		file.o \
		
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
	