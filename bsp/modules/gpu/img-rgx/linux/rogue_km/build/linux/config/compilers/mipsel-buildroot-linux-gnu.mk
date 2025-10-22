# 32-bit MIPS compiler
TARGET_PRIMARY_ARCH := target_mips32r6el
SYSROOT ?= $(shell $(CROSS_COMPILE)gcc -print-sysroot)
