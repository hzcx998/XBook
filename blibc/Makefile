#----
#File:blibc/Makefile
#Auther:Suote127
#Time:2019/11/29
#Copyright(C) 2019 by Book OS developers.All rights reserved.
#----

#Some tools
AS			=as
LIBTOOL 	=ar
NASM		=nasm

#Source File
SRC_DIR		=./src

#Output
OUTPUT		=blibc.a
SYSCALLOUT	=_brk.o	_wait.o	dir.o	execv.o	exit.o \
			 fork.o	getpid.o	log.o	mmap.o	msleep.o \
			 munmap.o	sleep.o	
OBJ_FILE	=$(SYSCALLOUT)

#Include
INC_DIR		=./include
NASM_INC	=$(INC_DIR)

#Flags
ASFLAG		= --32 -o
SYSCALLFLAG =$(SYSCALL) $(ASFLAG) $(SYSCALLOUT)
LIBFLAG		=-r $(OUTPUT) $(OBJ_FILE) 
NASM_FLAG	=-I $(NASM_INC)

blibc:$(OBJ_FILE)
	$(LIBTOOL) $(LIBFLAG)

cleanall:clean
	rm -rf $(OUTPUT)

clean:
	rm -rf $(OBJ_FILE)
	
%.o:$(SRC_DIR)/%.asm
	@$(NASM) $(NASM_FLAG)/ -f elf -o $*.o $(SRC_DIR)/$*.asm