#----
#File:		makefile
#Auther: 	Jason Hu, Yu Zhu
#Time: 		2019/6/1
#copyright:	(C) 2018-2020 by Book OS developers. All rights reserved.
#----

.PHONY: all $(MAKECMDGOALS)
all $(MAKECMDGOALS):
	@$(MAKE) -s -C ./src $(MAKECMDGOALS)
