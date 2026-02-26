#
# Makefile for phy-nand
#


obj-$(CONFIG_AW_NAND) += version.o
obj-$(CONFIG_AW_NAND) += nand_physic_interface.o
obj-$(CONFIG_AW_NAND) += nand_secure_storage.o
obj-$(CONFIG_AW_NAND) += nand_boot.o
obj-$(CONFIG_AW_NAND) += nand_weak.o

obj-y += nand-partition3/
#obj-y += nand-partition/
#obj-y += nand-partition2/
obj-$(CONFIG_AW_RAWNAND) += rawnand/
obj-$(CONFIG_AW_SPINAND) += spinand/
