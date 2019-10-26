#----
#file:		arch/x86/kernel/makefile
#auther: 	Jason Hu
#time: 		2019/6/2
#copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
#----

#The tools name
NASM 		= nasm
CC			= gcc

#The file path
ARCH_KERNEL_PATH = ../include/kernel/
ARCH_MM_PATH = ../include/mm/
ARCH_BUS_PATH = ../include/bus/

KERNEL_PATH = ../../../include/

#flags
ASM_FLAGS	= -I $(ARCH_KERNEL_PATH) -f elf
C_FLAGS	= -c -fno-builtin -nostdlib \
	-I $(ARCH_KERNEL_PATH) -I $(ARCH_MM_PATH) -I $(ARCH_BUS_PATH) -I $(KERNEL_PATH) \
	-Wall -Wunused \
	-m32 -fno-stack-protector

OBJS =  arch.o \
		entry.o \
		x86.o \
		segment.o \
		gate.o \
		interrupt.o \
		pic.o \
		cpu.o \
		cmos.o \
		
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
	