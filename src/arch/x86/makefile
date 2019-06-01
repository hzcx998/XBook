#----
#Makefile for x86
#Auther: Jason Hu
#Time: 2019/6/2
#Copyright:	(C) 2018-2019 by Retal developers. All rights reserved.
#E-mail: 2323168280@qq.com
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