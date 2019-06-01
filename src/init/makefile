#----
#File: init/makefile
#Auther: Jason Hu
#Time: 2019/6/2
#Copyright:	(C) 2018-2019 by BookOS developers. All rights reserved.
#----

#The tools name
CC			= gcc

INCLUDE_PATH = ../include/

#flags
C_FLAGS	= -I $(INCLUDE_PATH) -c -fno-builtin -Wall -Wunused -m32 -fno-stack-protector

OBJS =  main.o\

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
	