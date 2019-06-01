#----
#File:	arch/x86/boot/makefile
#Auther: Jason Hu
#Time: 2019/6/2
#Copyright:	(C) 2018-2019 by Retal developers. All rights reserved.
#----

#The tools name
NASM 		= nasm

INCLUDE_PATH = ../include/boot/

#The file path
BOOT_BIN = boot.bin
LOAD_BIN = loader.bin

ASM_FLAGS	= -I $(INCLUDE_PATH)
#First read here
.PHONY: all

all: compile

#Compile files
compile: $(BOOT_BIN) $(LOAD_BIN) 

#Clean temporary files
clean:
	-rm boot.bin
	-rm loader.bin

$(BOOT_BIN): *.asm
	$(NASM) $(ASM_FLAGS) -o $@ $<
	
$(LOAD_BIN): loader.asm
	$(NASM) $(ASM_FLAGS) -o $@ $<
