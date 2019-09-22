#Makefile for load
#auther: Jason Hu
#time: 2019/8/31


DD = dd
#The file path

FILE = code.sp

FILE_OFF = 1000 
FILE_CNTS = 1

#img dir
IMG_DIR = ../../img/

DISK_IMG = c.img

DISK_VHD = c.vhd

HD_IMG = $(IMG_DIR)$(DISK_IMG)
HD_VHD = $(IMG_DIR)$(DISK_VHD)

#First read here
.PHONY: all

all: img

img:
	$(DD) if=$(FILE) of=$(HD_IMG) bs=512 seek=$(FILE_OFF) count=$(FILE_CNTS) conv=notrunc
	
vhd:
	$(DD) if=$(FILE) of=$(HD_VHD) bs=512 seek=$(FILE_OFF) count=$(FILE_CNTS) conv=notrunc
#Clean temporary files
clean:
	-rm $(FILE)
