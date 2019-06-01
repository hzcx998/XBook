#----
#File:	arch/x86/kernel/makefile
#Auther: Jason Hu
#Time: 2019/6/2
#Copyright:	(C) 2018-2019 by Retal developers. All rights reserved.
#E-mail: 2323168280@qq.com
#----

#The tools name
NASM 		= nasm
CC			= gcc

#The file path
INCLUDE_PATH = ../include/

#flags
ASM_FLAGS	= -I $(INCLUDE_PATH) -f elf
C_FLAGS	= -I $(INCLUDE_PATH) -c -fno-builtin -Wall -Wunused -m32 -fno-stack-protector

OBJS =  entry.o\
	
#First read here
.PHONY: all

all: compile

#Compile files
compile: $(OBJS)

#Clean temporary files
clean:
	-rm *.o
	
%.o: %.asm
	$(NASM) $(ASM_FLAGS) -o $@ $<
	
.o: %.c
	$(CC) $(C_FLAGS) -o $@ $<