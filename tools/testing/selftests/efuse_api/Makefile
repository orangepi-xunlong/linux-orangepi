# Makefile for vm tools
#

buildconfig = ../../../../.buildconfig
ifeq ($(buildconfig), $(wildcard $(buildconfig)))
        LICHEE_TOOLCHAIN_PATH=$(shell cat $(buildconfig) | grep -w "LICHEE_TOOLCHAIN_PATH" | awk -F= '{printf $$2}')
        LICHEE_PLAT_OUT=$(shell cat $(buildconfig) | grep -w "LICHEE_PLAT_OUT" | awk -F= '{printf $$2}')
        export LICHEE_BUSSINESS LICHEE_CHIP_CONFIG_DIR LICHEE_IC
endif
CC=$(LICHEE_TOOLCHAIN_PATH)/bin/aarch64-linux-gnu-gcc

all:
	@$(CC)  efuse.c main.c
	@-mv a.out $(LICHEE_PLAT_OUT)/rootfs_def/bin/test_efuse
clean:
	@rm -rf *.out *.o
