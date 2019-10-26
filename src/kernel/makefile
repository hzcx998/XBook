#----
#File: 		kernel/makefile
#Auther: 	Jason Hu
#Time: 		2019/6/2
#copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
#----

#The tools name
CC			= gcc
NASM		= nasm
INCLUDE_PATH = ../include/

BLOCK_PATH = block/
CHAR_PATH = char/

#flags
C_FLAGS	= -c -I $(INCLUDE_PATH) \
	-fno-builtin -nostdlib \
	-Wall -Wunused \
	-m32 -fno-stack-protector

ASM_FLAGS = -I $(INCLUDE_PATH) -f elf

OBJS = bitmap.o \
		hal.o \
		debug.o \
		syscall.o \
		task.o \
		schedule.o \
		switch.o \
		sync_lock.o \
		sleep_wakeup.o \
		atomic.o \
		exit_wait.o \
		interrupt.o \
		softirq.o \
		workqueue.o \
		ioqueue.o \
		device.o \
		iostream.o \

#First read here
.PHONY: all

all: compile 

#Compile files
compile: $(OBJS)
	cd $(BLOCK_PATH) && make
	cd $(CHAR_PATH) && make

#Clean temporary files
clean:
	-rm *.o
	-rm *.elf
	cd $(BLOCK_PATH) && make clean
	cd $(CHAR_PATH) && make clean

%.o: %.c
	$(CC) $(C_FLAGS) -o $@ $<

%.o: %.asm
	$(NASM) $(ASM_FLAGS) -o $@ $<
		