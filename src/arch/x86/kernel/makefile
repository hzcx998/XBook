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
INCLUDE_PATH = ../include/kernel/

#flags
ASM_FLAGS	= -I $(INCLUDE_PATH) -f elf
C_FLAGS	= -c -I $(INCLUDE_PATH) -fno-builtin -nostdlib \
	-Wall -Wunused \
	-m32 -fno-stack-protector

OBJS =  arch.o \
		entry.o \
		

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
	