#----
#file:		arch/x86/makefile
#auther: 	Jason Hu
#time: 		2019/6/2
#copyright:	(C) 2018-2019 by Book OS developers. All rights reserved.
#----


#First read here
.PHONY: all

all: compile

#Compile files
compile: 
	cd boot && make
	cd kernel && make

#Clean temporary files
clean:
	cd boot && make clean
	cd kernel && make clean