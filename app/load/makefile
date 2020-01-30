#Makefile for load
#auther: Jason Hu
#time: 2020/1/26

DD = dd

FILE = ./baby.wav

FILE_OFF = 600
FILE_CNTS = 120

#img dir
IMG_DIR = ../../img/

DISK_IMG = a.img

DISK_VHD = a.vhd

HD_IMG = $(IMG_DIR)$(DISK_IMG)
HD_VHD = $(IMG_DIR)$(DISK_VHD)

#First read here
.PHONY: all

all: img

img:
	$(DD) if=$(FILE) of=$(HD_IMG) bs=512 seek=$(FILE_OFF) count=$(FILE_CNTS) conv=notrunc
	
vhd:
	$(DD) if=$(FILE) of=$(HD_VHD) bs=512 seek=$(FILE_OFF) count=$(FILE_CNTS) conv=notrunc
