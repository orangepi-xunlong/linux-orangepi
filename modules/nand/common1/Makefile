ifeq ($(CC),clang)
ccflags-y += -fsanitize-blacklist=$(srctree)/$(src)/blacklist.txt
endif
obj-$(CONFIG_SUNXI_NAND) += nand.o

ifeq ($(CC),clang)

ifdef CONFIG_ARM64
nftl = nftl_v8_clang.S

ifeq ($(CONFIG_SUNXI_NAND),m)
nand-y += nftl_v8_clang.o
else
obj-$(CONFIG_SUNXI_NAND) += nftl_v8_clang.o
endif

endif

ifdef CONFIG_ARM
nftl = nftl_v7_clang.S

ifeq ($(CONFIG_SUNXI_NAND), m)
nand-y += nftl_v7_clang.o
else
obj-$(CONFIG_SUNXI_NAND) += nftl_v7_clang.o
endif

endif

else

ifdef CONFIG_ARM64
nftl = nftl_v8.S

ifeq ($(CONFIG_SUNXI_NAND),m)
nand-y += nftl_v8.o
else
obj-$(CONFIG_SUNXI_NAND) += nftl_v8.o
endif

endif

ifdef CONFIG_ARM
nftl = nftl_v7.S

ifeq ($(CONFIG_SUNXI_NAND), m)
nand-y += nftl_v7.o
else
obj-$(CONFIG_SUNXI_NAND) += nftl_v7.o
endif

endif

endif #clang


ifdef CONFIG_ARCH_SUN8IW18
	ndfc-v=phy-nand/rawnand/controller/ndfc_v2px.c
else
	ndfc-v=phy-nand/rawnand/controller/ndfc_v1px.c
endif

ifdef CONFIG_SUNXI_RAWNAND
	BUILD_RAWNAND=y
else
	BUILD_SPINAND=y
endif

$(srctree)/$(src)/$(nftl) : FORCE
ifneq ($(wildcard $(srctree)/$(src)/lib),)
	$(MAKE) -C $(srctree)/$(src)/.. build
endif

nand-y += nand_interface.o
nand-y += phy2nftl.o
nand-y += nfd/nand_ota_burn.o
nand-y += nfd/nand_base.o
nand-y += nfd/nand_blk.o
nand-y += nfd/nand_dev.o
nand-y += nfd/nand_class.o
nand-y += nfd/nand_test.o
nand-y += nfd/nand_osal_for_linux.o
ifneq ($(CONFIG_PSTORE_BLK),)
nand-y += nfd/nand_panic.o
endif
nand-y += phy-nand/nand_boot.o
#nand-y += phy-nand/nand-partition/build_nand_partition.o
#nand-y += phy-nand/nand-partition/build_phy_partition.o
#nand-y += phy-nand/nand-partition/nand_info_init.o
#nand-y += phy-nand/nand-partition2/build_phy_partition_v2.o
#nand-y += phy-nand/nand-partition2/nand_info_init_v2.o
nand-y += phy-nand/nand-partition3/nand_partitions.o
nand-y += phy-nand/nand_physic_interface.o
nand-y += phy-nand/nand_secure_storage.o
nand-y += phy-nand/version.o
nand-y += phy-nand/nand_weak.o
nand-y += phy-nand/nand_weak.o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/controller/ndfc_base.o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/controller/ndfc_timings.o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/controller/ndfc_v1px.o
nand-$(BUILD_RAWNAND) += $(basename $(ndfc-v)).o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/rawnand_base.o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/rawnand_boot1.o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/rawnand_boot0.o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/rawnand_cfg.o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/rawnand_ops.o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/rawnand_chip.o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/rawnand_debug.o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/rawnand_ids.o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/rawnand_readretry.o
nand-$(BUILD_RAWNAND) += phy-nand/rawnand/rawnand_timings.o
nand-$(BUILD_SPINAND) += phy-nand/spinand/spinand_boot.o
nand-$(BUILD_SPINAND) += phy-nand/spinand/spinand.o
#nand-y += $(libnand)
