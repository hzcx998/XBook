#----
#File:		libc/makefile
#Auther: 	Jason Hu
#Time: 		2020/3/7
#copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
#----

# tools
AR 			= ar
NASM 		= nasm
CC			= gcc
RM			= rm

# dir
DIR_ASM = 	./asm/
DIR_C = 	./c/
DIR_INCLUDE = 	./include/

# flags
ARFLAGS := -r
C_FLAGS	:=  -fno-builtin -nostdinc -Wall -Wunused -fno-PIE -m32 -fno-stack-protector -I $(DIR_INCLUDE) -c
NASM_FLAGS	:= -I $(DIR_INCLUDE)

# out file
LIBC_A = libc.a

# asm object files
OBJS_ASM := $(DIR_ASM)_brk.o \
			$(DIR_ASM)_wait.o \
			$(DIR_ASM)dir.o \
			$(DIR_ASM)execv.o \
			$(DIR_ASM)exit.o \
			$(DIR_ASM)file.o \
			$(DIR_ASM)fork.o \
			$(DIR_ASM)getmem.o \
			$(DIR_ASM)getpid.o \
			$(DIR_ASM)getver.o \
			$(DIR_ASM)kgcmsg.o \
			$(DIR_ASM)mmap.o \
			$(DIR_ASM)msleep.o \
			$(DIR_ASM)munmap.o \
			$(DIR_ASM)pipe.o \
			$(DIR_ASM)power.o \
			$(DIR_ASM)signal.o \
			$(DIR_ASM)sleep.o \
			$(DIR_ASM)task.o \
			$(DIR_ASM)time.o
			
# c object files
OBJS_C := 	$(DIR_C)asctime.o \
			$(DIR_C)assert.o \
			$(DIR_C)brk.o \
			$(DIR_C)conio.o \
			$(DIR_C)ctype.o \
			$(DIR_C)errno.o \
			$(DIR_C)inet.o \
			$(DIR_C)kgcgraph.o \
			$(DIR_C)longjmp.o \
			$(DIR_C)malloc.o \
			$(DIR_C)math.o \
			$(DIR_C)printf.o \
			$(DIR_C)qsort.o \
			$(DIR_C)setjmp.o \
			$(DIR_C)signal.o \
			$(DIR_C)stream.o \
			$(DIR_C)string.o \
			$(DIR_C)system.o \
			$(DIR_C)vsprintf.o	

# objects
OBJS = $(OBJS_ASM) $(OBJS_C) 

# all need to do
all: $(LIBC_A) 

# make a libary file
$(LIBC_A): $(OBJS) 
	@echo [AR] making libary file $(LIBC_A)
	@$(AR) $(ARFLAGS) $(LIBC_A) $(OBJS)

%.o : %.c
	@echo [CC] $@ $<
	@$(CC) $(C_FLAGS) -c -o $@ $<

%.o : %.asm
	@echo [NASM] $@ $<
	@$(NASM) $(NASM_FLAGS) -f elf -o $@ $<

# clean .o .a files
clean: 
	-$(RM) $(LIBC_A)
	-$(RM) $(DIR_ASM)/*.o
	-$(RM) $(DIR_C)/*.o