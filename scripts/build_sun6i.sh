#!/bin/bash
set -e

#Setup common variables
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabi-
export AS=${CROSS_COMPILE}as
export LD=${CROSS_COMPILE}ld
export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export NM=${CROSS_COMPILE}nm
export STRIP=${CROSS_COMPILE}strip
export OBJCOPY=${CROSS_COMPILE}objcopy
export OBJDUMP=${CROSS_COMPILE}objdump
export LOCALVERSION=""

KERNEL_VERSION="3.8"
LICHEE_KDIR=`pwd`
LICHEE_MOD_DIR==${LICHEE_KDIR}/output/lib/modules/${KERNEL_VERSION}

#CONFIG_CHIP_ID=1633

update_kern_ver()
{
	if [ -r include/generated/utsrelease.h ]; then
		KERNEL_VERSION=`cat include/generated/utsrelease.h |awk -F\" '{print $2}'`
	fi
	LICHEE_MOD_DIR=${LICHEE_KDIR}/output/lib/modules/${KERNEL_VERSION}
}

show_help()
{
	printf "
Build script for Lichee platform

Invalid Options:

	help         - show this help
	kernel       - build kernel
	modules      - build kernel module in modules dir
	clean        - clean kernel and modules

"
}

build_standby()
{
	echo "build standby"
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} KDIR=${LICHEE_KDIR} \
		-C ${LICHEE_KDIR}/arch/arm/mach-sun6i/pm/standby all
}

build_mdfs()
{
	echo "build mdfs"
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} KDIR=${LICHEE_KDIR} \
		-C ${LICHEE_KDIR}/arch/arm/mach-sun6i/dram-freq/mdfs all
}

NAND_ROOT=${LICHEE_KDIR}/modules/nand

build_nand_lib()
{
	echo "build nand library ${NAND_ROOT}/lib"
	if [ -d ${NAND_ROOT}/lib ]; then
		echo "build nand library now"
	make -C modules/nand/lib clean	2>/dev/null
	make -C modules/nand/lib lib install
	else
		echo "build nand with existing library"
	fi
}

copy_nand_mod()
{

    cd $LICHEE_KDIR
    if [ -x "./scripts/build_rootfs.sh" ]; then
        ./scripts/build_rootfs.sh e rootfs.cpio.gz >/dev/null
    else
        echo "No such file: build_rootfs.sh"
        exit 1
    fi

    if [ ! -d "./skel/lib/modules/$KERNEL_VERSION" ]; then
        mkdir -p ./skel/lib/modules/$KERNEL_VERSION
    fi

    cp $LICHEE_MOD_DIR/nand.ko ./skel/lib/modules/$KERNEL_VERSION
    if [ $? -ne 0 ]; then
        echo "copy nand module error: $?"
        exit 1
    fi
    if [ -f "./rootfs.cpio.gz" ]; then
        rm rootfs.cpio.gz
    fi
    ./scripts/build_rootfs.sh c rootfs.cpio.gz >/dev/null
#    rm -rf skel

}

build_kernel()
{
	cd ${LICHEE_KDIR}
	if [ ! -e .config ]; then
		echo -e "\n\t\tUsing default config... ...!\n"
		cp arch/arm/configs/sun6ismp_defconfig .config
	fi

	#try to remove csi drivers
	#rm -rf ${LICHEE_KDIR}/drivers/media/video/sunxi_csi/device/*.ko
	#rm -rf ${LICHEE_KDIR}/drivers/media/video/sunxi_csi/device/*.o
	#rm -rf ${LICHEE_KDIR}/drivers/media/video/sunxi-vfe/device/*.ko
	#rm -rf ${LICHEE_KDIR}/drivers/media/video/sunxi-vfe/device/*.o

	#build_standby
	#build_mdfs
	make ARCH=arm CROSS_COMPILE=${CROSS_COMPILE} -j8 uImage modules

	# #*****Warnning*******#
	# Here, when we add OBJCOPYFLAGS_Image in Makefile.boot, don't use 
	# following command.

	#${OBJCOPY} -R .note.gnu.build-id -S -O binary vmlinux bImage

	update_kern_ver

	if [ -d output ]; then
		rm -rf output
	fi
	mkdir -p $LICHEE_MOD_DIR

	#The Image is origin binary from vmlinux.
	cp -vf arch/arm/boot/Image output/bImage
	cp -vf arch/arm/boot/[zu]Image output/
	cp .config output/

	for file in $(find drivers sound crypto block fs security net -name "*.ko"); do
		cp $file ${LICHEE_MOD_DIR}
	done
	cp -f Module.symvers ${LICHEE_MOD_DIR}

	#copy bcm4330 firmware and nvram.txt
#	cp drivers/net/wireless/bcm4330/firmware/bcm4330.bin ${LICHEE_MOD_DIR}
#	cp drivers/net/wireless/bcm4330/firmware/bcm4330.hcd ${LICHEE_MOD_DIR}
#	cp drivers/net/wireless/bcm4330/firmware/nvram.txt ${LICHEE_MOD_DIR}/bcm4330_nvram.txt
	echo "sun6i compile successful"
}

build_modules()
{
	echo "Building modules"

	if [ ! -f include/generated/utsrelease.h ]; then
		printf "Please build kernel first!\n"
		exit 1
	fi

	update_kern_ver

	make -C modules/example LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
		CONFIG_CHIP_ID=${CONFIG_CHIP_ID} install

	build_nand_lib
	make -C modules/nand LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
		CONFIG_CHIP_ID=${CONFIG_CHIP_ID} install
	copy_nand_mod
#	(
#	export LANG=en_US.UTF-8
#	unset LANGUAGE
#	make -C modules/mali LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
#		CONFIG_CHIP_ID=${CONFIG_CHIP_ID} install
#	)

#	#build ar6302 sdio wifi module
#	make -C modules/wifi/ar6302/AR6K_SDK_ISC.build_3.1_RC.329/host CROSS_COMPILE=${CROSS_COMPILE} \
#	        ARCH=arm KERNEL_DIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} INSTALL_DIR=${LICHEE_MOD_DIR} \
#	        all install
#	#build ar6003 sdio wifi module
#	make -C modules/wifi/ar6003/AR6kSDK.build_3.1_RC.514/host CROSS_COMPILE=${CROSS_COMPILE} \
#	        ARCH=arm KERNEL_DIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} INSTALL_DIR=${LICHEE_MOD_DIR} \
#	        all
#	#build usi-bmc4329 sdio wifi module
#	make -C modules/wifi/usi-bcm4329/v4.218.248.15/open-src/src/dhd/linux \
#		CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
#		LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} \
#		INSTALL_DIR=${LICHEE_MOD_DIR} dhd-cdc-sdmmc-gpl
#	#build bcm40181 sdio wifi module 5.90.125.69.2
#	make -C modules/wifi/bcm40181/5.90.125.69.2/open-src/src/dhd/linux \
#		CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
#		LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} \
#		INSTALL_DIR=${LICHEE_MOD_DIR} OEM_ANDROID=1 dhd-cdc-sdmmc-gpl
#	#build bcm40183 sdio wifi module
#	make -C modules/wifi/bcm40183/5.90.125.95.3/open-src/src/dhd/linux \
#		CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
#		LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} \
#		INSTALL_DIR=${LICHEE_MOD_DIR} OEM_ANDROID=1 dhd-cdc-sdmmc-gpl
}

build_ramfs()
{
	# If uboot use *boota* to boot kernel, we should use bImage
	cp rootfs.cpio.gz output/
	mkbootimg --kernel output/bImage \
		--ramdisk output/rootfs.cpio.gz \
		--board 'a31' \
		--base 0x40000000 \
		-o output/boot.img
	
	# If uboot use *bootm* to boot kernel, we should use uImage.
	#cp output/uImage output/boot.img

	echo build_ramfs
}


clean_kernel()
{
	make clean
	rm -rf output/*

#        (
#	export LANG=en_US.UTF-8
#	unset LANGUAGE
#	make -C modules/mali LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} clean
#	)

}

clean_modules()
{
	echo "Cleaning modules"
	make -C modules/example LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} clean

#	#build ar6302 sdio wifi module
#	make -C modules/wifi/ar6302/AR6K_SDK_ISC.build_3.1_RC.329/host CROSS_COMPILE=${CROSS_COMPILE} \
#	        ARCH=arm KERNEL_DIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} INSTALL_DIR=${LICHEE_MOD_DIR} \
#	        clean
#	#build ar6003 sdio wifi module
#	make -C modules/wifi/ar6003/AR6kSDK.build_3.1_RC.514/host CROSS_COMPILE=${CROSS_COMPILE} \
#	        ARCH=arm KERNEL_DIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} INSTALL_DIR=${LICHEE_MOD_DIR} \
#	        clean
#	#build usi-bmc4329 sdio wifi module
#	make -C modules/wifi/usi-bcm4329/v4.218.248.15/open-src/src/dhd/linux \
#		CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
#		LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} \
#		INSTALL_DIR=${LICHEE_MOD_DIR} clean
#	#build bcm40181 sdio wifi module 5.90.125.69.2
#	make -C modules/wifi/bcm40181/5.90.125.69.2/open-src/src/dhd/linux \
#		CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
#		LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} \
#		INSTALL_DIR=${LICHEE_MOD_DIR} clean
#	#build bcm40183 sdio wifi module
#	make -C modules/wifi/bcm40183/5.90.125.95.3/open-src/src/dhd/linux \
#		CROSS_COMPILE=${CROSS_COMPILE} ARCH=arm LINUXVER=${KERNEL_VERSION} \
#		LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LINUXDIR=${LICHEE_KDIR} CONFIG_CHIP_ID=${CONFIG_CHIP_ID} \
#		INSTALL_DIR=${LICHEE_MOD_DIR} OEM_ANDROID=1 clean
}

#####################################################################
#
#                      Main Runtine
#
#####################################################################

LICHEE_ROOT=`(cd ${LICHEE_KDIR}/..; pwd)`
export PATH=${LICHEE_ROOT}/buildroot/output/external-toolchain/bin:${LICHEE_ROOT}/tools/pack/pctools/linux/android:$PATH

case "$1" in
kernel)
	build_kernel
	;;
modules)
	build_modules
	;;
clean)
	clean_kernel
	clean_modules
	;;
all)
	build_kernel
	#build_modules
	build_ramfs
	;;
*)
	show_help
	;;
esac

