###############################################################################
#
# Common Variables that already set:
#     LICHEE_KDIR
#     LICHEE_MOD_DIR
#     CROSS_COMPILE
#     ARCH
#
#################################################################################
ifdef CONFIG_ARM64
ccflags-y += -g -c -w -nostdlib -march=armv8-a
endif

ifdef CONFIG_ARM
ccflags-y += -c -w -nostdlib -march=armv7-a -marm -mlittle-endian -O2 -msoft-float
endif

phy_obj=phy/build_nand_partition.o \
	phy-nand/phy/build_phy_partition.o \
	phy-nand/phy/nand_info_init.o \
	phy-nand/phy/phy_input.o    	\
	phy-nand/phy_v2/build_phy_partition_v2.o \
	phy-nand/phy_v2/nand_info_init_v2.o \
	phy-nand/physic_v2/nand_platform_data.o    	\
	phy-nand/physic_v2/nand_id2.o    	\
	phy-nand/physic_v2/version.o    	\
	phy-nand/physic_v2/nand_phyisc_interface_rawnand.o    	\
	phy-nand/physic_v2/uboot_tail_rawnand.o    	\
	phy-nand/physic_v2/nand_phy_init.o    	\
	phy-nand/physic_v2/chip/nand_chip_function.o    	\
	phy-nand/physic_v2/chip/nand_chip_interface.o    	\
	phy-nand/physic_v2/chip/nand_chip_common.o    	\
	phy-nand/physic_v2/chip/nand_super_chip_function.o    	\
	phy-nand/physic_v2/chip/nand_super_chip_interface.o    	\
	phy-nand/physic_v2/chip/nand_super_chip_common.o    	\
	phy-nand/physic_v2/controller/nand_controller_common.o    	\
	phy-nand/physic_v2/controller/nand_controller_op.o    	\
	phy-nand/physic_v2/controller/nand_controller.o    	\
	phy-nand/physic_v2/universal/nand0_chip_for_boot.o    	\
	phy-nand/physic_v2/universal/nand0_chip_op.o    	\
	phy-nand/physic_v2/universal/nand0_super_chip_op.o \
	phy-nand/physic_v2/hynix16nm/nand1_chip_for_boot.o    	\
	phy-nand/physic_v2/hynix16nm_4G/nand8_chip_for_boot.o    	\
	phy-nand/physic_v2/hynix20nm/nand2_chip_for_boot.o    	\
	phy-nand/physic_v2/hynix26nm/nand3_chip_for_boot.o    	\
	phy-nand/physic/nand_format.o \
	phy-nand/physic/nand_id.o \
	phy-nand/physic/nand_phy.o \
	phy-nand/physic/nand_physic_interface_spinand.o \
	phy-nand/physic/nand_scan.o \
	phy-nand/physic/spic_op.o \
	phy-nand/physic/spinand0/spi_nand0.o \
	phy-nand/physic/spinand1/spi_nand1.o \
	phy-nand/physic/spinand2/spi_nand2.o \
	phy-nand/physic/spinand3/spi_nand3.o \
	phy-nand/physic/uboot_tail_spinand.o \
	phy-nand/physic_common/nand_secure_storage.o    	\
	phy-nand/physic_common/nand_phyisc_interface.o    	\
	phy-nand/physic_common/uboot_tail.o	\

$(phy_obj) :  ./nand_osal.h
	$(CC) $(CFLAGS) -c $(@:%.o=%.c) -o $@

clean:
	-rm -rf $(phy_obj)
