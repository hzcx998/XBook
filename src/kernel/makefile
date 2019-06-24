#----
#File: kernel/makefile
#Auther: Jason Hu
#Time: 2019/6/2
#copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
#----

#The tools name
CC			= gcc

INCLUDE_PATH = ../include/

#flags
C_FLAGS	= -c -I $(INCLUDE_PATH) \
	-fno-builtin -nostdlib \
	-Wall -Wunused \
	-m32 -fno-stack-protector

OBJS = 	bitmap.o \
		hal.o \
		debug.o \
		syscall.o \
		
#First read here
.PHONY: all

all: compile

#Compile files
compile: $(OBJS)

#Clean temporary files
clean:
	-rm *.o
	-rm *.elf
	
%.o: %.c
	$(CC) $(C_FLAGS) -o $@ $<
	