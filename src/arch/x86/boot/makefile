BOOT_BIN = boot.bin
LOAD_BIN = loader.bin

ASM_FLAGS	= -I ../include/boot/

PREPARE	+= $(BOOT_BIN) $(LOAD_BIN)

$(BOOT_BIN): boot.asm
	@echo [NASM]  $(patsubst $(TOPDIR)/%,%,$(CURDIR))/$@
	@$(NASM) $(ASM_FLAGS) -o $@ $<
$(LOAD_BIN): loader.asm
	@echo [NASM]  $(patsubst $(TOPDIR)/%,%,$(CURDIR))/$@
	@$(NASM) $(ASM_FLAGS) -o $@ $<

xclean:
	$(RM) $(BOOT_BIN) $(LOAD_BIN)
