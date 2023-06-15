################################################################################
#
# Common Variables that already set:
#     LICHEE_KDIR
#     LICHEE_MOD_DIR
#     CROSS_COMPILE
#     ARCH
#
#################################################################################

# Do not use allwinner private variables. But for compatible solution of old build
# scritp, load it as default values. Please remove it in the furture.
# All need setup:
#
# KERNEL_SRC_DIR  (KERNEL_SRC)
# KERNEL_OUT_DIR  (O)
# MODULE_INS_DIR  (INSTALL_MOD_PATH)

# Init all variables
KERNEL_SRC_DIR := $(LICHEE_KDIR)
MODULE_INS_DIR := $(LICHEE_MOD_DIR)
KERNEL_OUT_DIR := $(O)

# Re-setup it
KERNEL_SRC_DIR := $(word 1, $(KERNEL_SRC_DIR) $(KERNEL_SRC))
KERNEL_OUT_DIR := $(word 1, $(KERNEL_OUT_DIR) $(KERNEL_SRC_DIR))

ifeq ($(KERNEL_SRC_DIR),)
  $(error KERNEL_SRC_DIR not setup)
endif

KERNEL_VERSION := $(shell awk -F\" '/UTS_RELEASE/{print $$2}' $(KERNEL_OUT_DIR)/include/generated/utsrelease.h 2>/dev/null)

ifneq ($(INSTALL_MOD_PATH),)
  MODULE_INS_DIR := $(INSTALL_MOD_PATH)/lib/modules/$(KERNEL_VERSION)
  MODULE_INS_DIR := $(patsubst %/, %, $(MODULE_INS_DIR))
endif

SOC := $(shell sed -n 's/^CONFIG_ARCH_\(SUN.*W.[0-9]\)=y/\1p1/p' $(KERNEL_OUT_DIR)/.config 2>/dev/null | tr '[:upper:]' '[:lower:]')


#$(warning $(SOC))
ifeq ($(SOC),)
SOC := $(shell sed -n 's/^CONFIG_ARCH_\(SUN.*W.*\)=y/\1p1/p' $(KERNEL_OUT_DIR)/.config 2>/dev/null | tr '[:upper:]' '[:lower:]')
endif
#$(warning $(SOC))



# The nand driver of sun8iw15p1 is differnet between Tina and Android.
# The Android is using driver on directory A50 for large capacity while
# Tina uses driver on directory MR133 for small capacity.
ifeq ($(TARGET_BUILD_VARIANT), tina)
ifeq ($(SOC), sun8iw15p1)
SOC := $(SOC)-small
endif
endif

obj-$(CONFIG_SUNXI_NAND) += $(SOC)/

.PHONY: install build clean build_rr

NAND_MOD_OUT_DIR  := $(patsubst $(KERNEL_SRC_DIR)/%,$(KERNEL_OUT_DIR)/%,$(shell pwd)/$(SOC))
NAND_BUILD_CONFIG := $(shell grep "CONFIG_SUNXI_NAND=m" $(KERNEL_OUT_DIR)/.config 2>/dev/null)

modules_install:
	@if [ -n "$(NAND_BUILD_CONFIG)" ]; then \
		if [ -z "$(MODULE_INS_DIR)" ]; then \
			echo "MODULE_INS_DIR not configured, nothing to do"; \
		else \
			mkdir -p $(MODULE_INS_DIR); \
			cp $(NAND_MOD_OUT_DIR)/nand.ko $(MODULE_INS_DIR)/; \
		fi; \
	else \
		echo "[NAND] NOT buid as ko"; \
	fi

clean:
	@rm -rf *.o *.ko .*.cmd *.mod.c *.order *.symvers .tmp_versions *~

build:
	@make -C ${SOC}/lib clean 2>/dev/null
	@make -C ${SOC}/lib install

build_rr:
	@make -C ${SOC}/phy-nand/physic_v2/rr_lib clean 2>/dev/null
	@make -C ${SOC}/phy-nand/physic_v2/rr_lib

build_3d:
	@make -C ${SOC}/lib-3d clean 2>/dev/null
	@make -C ${SOC}/lib-3d install
