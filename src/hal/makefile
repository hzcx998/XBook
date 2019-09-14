#----
#File: hal/char/makefile
#Auther: Jason Hu
#Time: 2019/6/22
#copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
#----

#The tools name
CC			= gcc

#The file path
INCLUDE_PATH = ../include/

#flags
C_FLAGS	= -c -fno-builtin -nostdlib \
	-I $(INCLUDE_PATH) \
	-Wall -Wunused \
	-m32 -fno-stack-protector

OBJS =  display.o \
		clock.o \
		cpu.o \
		keyboard.o \
		ram.o \
		
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
	