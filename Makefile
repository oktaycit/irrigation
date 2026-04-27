# Project-level convenience targets.

.PHONY: all clean flash debug-test debug-gdb stm32

all stm32:
	$(MAKE) -C stm32

clean flash debug-test debug-gdb:
	$(MAKE) -C stm32 $@
