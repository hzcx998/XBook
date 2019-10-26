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
	cd mm && make
	cd bus && make

#Clean temporary files
clean:
	cd boot && make clean
	cd kernel && make clean
	cd mm && make clean
	cd bus && make clean
