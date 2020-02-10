#----
#File:		makefile
#Auther: 	Jason Hu, Yu Zhu
#Time: 		2019/6/1
#copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
#----

QEMU 		= qemu-system-i386

FLAPPY_IMG 	= ./a.img
HDA_IMG		= ./c.img
HDB_IMG		= ./d.img

.PHONY: qemu

qemu:
	$(QEMU) -m 512M \
		-name "XBook Development Platform for x86" \
		-fda $(FLAPPY_IMG) -hda $(HDA_IMG) -hdb $(HDB_IMG) -boot a \
		-serial stdio \
		-soundhw pcspk