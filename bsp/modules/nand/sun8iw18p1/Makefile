ifndef CONFIG_MTD_UBI

obj-$(CONFIG_AW_NAND) += nand_interface.o \
	nfd/nand_ota_burn.o \
	nfd/nand_base.o \
	nfd/nand_blk.o \
	nfd/nand_dev.o \
	nfd/nand_class.o \
	nfd/nand_test.o \
	nfd/nand_osal_for_linux.o \
	nfd/nand_lib_cfg.o \
	spinand/spinand_ecc_op.o \
	spinand/spinand.o \
	spinand/spinand_ids.o

#obj-$(CONFIG_AW_NAND) += lib/

ifdef CONFIG_ARM
nftl = nftl_v7.S

obj-$(CONFIG_AW_NAND) += nftl_v7.o

endif

$(srctree)/$(src)/$(nftl) : FORCE
ifneq ($(wildcard $(srctree)/$(src)/lib),)
	$(MAKE) -C $(srctree)/$(src)/.. build
endif

endif
