.PHONY = all
all $(MAKECMDGOALS):
	@$(MAKE) -s -C ./src $(MAKECMDGOALS)