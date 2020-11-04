obj-$(CONFIG_SUNXI_NAND) += nand.o

libnand = libnand_sun8iw15p1
libnand-3d = libnand_sun8iw15p1-3d

$(obj)/$(libnand) : FORCE
ifneq ($(wildcard $(obj)/phy-nand/physic_v2/rr_lib),)
	$(MAKE) -C $(obj)/.. build_rr
endif
ifneq ($(wildcard $(obj)/lib),)
	$(MAKE) -C $(obj)/.. build
endif


$(obj)/$(libnand-3d) : FORCE
ifneq ($(wildcard $(obj)/lib-3d),)
	$(MAKE) -C $(obj)/.. build_3d
endif
include $(obj)/phy-nand/Makefile
nand-objs += nand_interface.o \
	nfd/nand_ota_burn.o \
	nfd/nand_base.o \
	nfd/nand_blk.o \
	nfd/nand_dev.o \
	nfd/nand_class.o \
	nfd/nand_test.o \
	nfd/nand_osal_for_linux.o \
	nfd/nand_lib_cfg.o \
	phy-nand/${phy_obj}	\
	$(libnand) \
	$(libnand-3d)

ifneq ($(CONFIG_SUNXI_PANICPART),)
nand-objs += nfd/nand_panic.o
endif
