#!/bin/bash
set -e

#Setup common variables
export ARCH=arm
export CROSS_COMPILE=${ARCH}-linux-gnueabi-
if [ -d "${LICHEE_TOOLCHAIN_PATH}" ]; then
	GCC=$(find ${LICHEE_TOOLCHAIN_PATH} -perm /a+x -a -regex '.*-gcc');
	export CROSS_COMPILE="${GCC%-*}-";
elif [ -n "${LICHEE_CROSS_COMPILER}" ]; then
	export CROSS_COMPILE="${LICHEE_CROSS_COMPILER}-"
fi

export AS=${CROSS_COMPILE}as
export LD=${CROSS_COMPILE}ld
export CC=${CROSS_COMPILE}gcc
export AR=${CROSS_COMPILE}ar
export NM=${CROSS_COMPILE}nm
export STRIP=${CROSS_COMPILE}strip
export OBJCOPY=${CROSS_COMPILE}objcopy
export OBJDUMP=${CROSS_COMPILE}objdump
export LOCALVERSION=""
export MKBOOTIMG=${LICHEE_TOOLS_DIR}/pack/pctools/linux/android/mkbootimg

KERNEL_VERSION=`make -s kernelversion -C ./`
LICHEE_KDIR=`pwd`
LICHEE_MOD_DIR=${LICHEE_KDIR}/output/lib/modules/${KERNEL_VERSION}
export LICHEE_KDIR

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

NAND_ROOT=${LICHEE_KDIR}/modules/nand

build_nand_lib()
{
	echo "build nand library ${NAND_ROOT}/${LICHEE_CHIP}/lib"
	if [ -d ${NAND_ROOT}/${LICHEE_CHIP}/lib ]; then
		echo "build nand library now"
		make -C modules/nand/${LICHEE_CHIP}/lib clean 2> /dev/null
		make -C modules/nand/${LICHEE_CHIP}/lib lib install
	else
		echo "build nand with existing library"
	fi
}

build_gpu_sun8i()
{
	export LANG=en_US.UTF-8
	unset LANGUAGE
	make -C modules/mali LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
	install
}

build_gpu_sun8iw6()
{
	if [ "x${LICHEE_PLATFORM}" = "xandroid" ] ; then
		unset OUT
		unset TOP
		make -j16 -C modules/eurasia_km/eurasiacon/build/linux2/sunxi_android	\
			LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR}
		for file in $(find  modules/eurasia_km -name "*.ko"); do
			cp $file ${LICHEE_MOD_DIR}
		done
	fi
}

build_gpu_sun9iw1()
{
	if [ "x${LICHEE_PLATFORM}" = "xandroid" ] ; then
		unset OUT
		unset TOP
		make -j16 -C modules/rogue_km/build/linux/sunxi_android \
			LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
			RGX_BVNC=1.75.2.30 \
			KERNELDIR=${LICHEE_KDIR}
		for file in $(find  modules/rogue_km -name "*.ko"); do
			cp $file ${LICHEE_MOD_DIR}
		done
	fi
}

build_gpu()
{
	chip_sw=`echo ${LICHEE_CHIP} | awk '{print substr($0,1,length($0)-2)}'`

	echo build gpu module for ${chip_sw} ${LICHEE_PLATFORM}

	if [ "${chip_sw}" = "sun9iw1" ]; then
		build_gpu_sun9iw1
	elif 
	[ "${chip_sw}" = "sun8iw3" ] || 
	[ "${chip_sw}" = "sun8iw5" ] || 
	[ "${chip_sw}" = "sun8iw7" ] || 
	[ "${chip_sw}" = "sun8iw9" ]; then
		build_gpu_sun8i
	elif [ "${chip_sw}" = "sun8iw6" ] ; then
		build_gpu_sun8iw6
	fi
}

clean_gpu_sun9iw1()
{
    unset OUT
    unset TOP
    make -C modules/rogue_km/build/linux/sunxi_android \
        LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
        RGX_BVNC=1.75.2.30 \
        KERNELDIR=${LICHEE_KDIR} clean
}

clean_gpu_sun8iw6()
{
	unset OUT
	unset TOP
	make -C modules/eurasia_km/eurasiacon/build/linux2/sunxi_android	\
		LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} clobber
}

clean_gpu()
{
	chip_sw=`echo $LICHEE_CHIP | awk '{print substr($0,1,length($0)-2)}'`
	echo
    echo clean gpu module ${chip_sw} $LICHEE_PLATFORM
	if [ "${chip_sw}" = "sun9iw1" ]; then
		clean_gpu_sun9iw1
	elif [ "${chip_sw}" = "sun8iw6" ]; then
		clean_gpu_sun8iw6
	fi
}


build_kernel()
{
	echo "Building kernel"

	cd ${LICHEE_KDIR}

	rm -rf output/
	echo "${LICHEE_MOD_DIR}"
	mkdir -p ${LICHEE_MOD_DIR}

	# We need to copy rootfs files to compile kernel for linux image
	cp -f rootfs.cpio.gz output/

    if [ ! -f .config ] ; then
        printf "\n\033[0;31;1mUsing default config ${LICHEE_KERN_DEFCONF} ...\033[0m\n\n"
        cp arch/arm/configs/${LICHEE_KERN_DEFCONF} .config
    fi

    if [ "x$SUNXI_CHECK" = "x1" ];then
        SUNXI_SPARSE_CHECK=1
        SUNXI_SMATCH_CHECK=1
        SUNXI_STACK_CHECK=1
    fi
    if [ "x$SUNXI_SPARSE_CHECK" = "x" ] && [ "x$SUNXI_SMATCH_CHECK" = "x" ];then
        make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j${LICHEE_JLEVEL} uImage modules
    else
        if [ "x$SUNXI_SPARSE_CHECK" = "x1" ] && [ -f ../tools/codecheck/sparse/sparse ];then
            echo "\n\033[0;31;1mBuilding Round for sparse check ${KERNEL_CFG}...\033[0m\n\n"
            make clean
            make CHECK="../tools/codecheck/sparse/sparse" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j${LICHEE_JLEVEL} C=2 uImage modules 2>&1|tee output/build_sparse.txt
            cat output/build_sparse.txt|egrep -w '(warn|error|warning)' >output/warn_sparse.txt
        fi
        if [ "x$SUNXI_SMATCH_CHECK" = "x1" ]&& [ -f ../tools/codecheck/smatch/smatch ];then
            echo "\n\033[0;31;1mBuilding Round for smatch check ${KERNEL_CFG}...\033[0m\n\n"
            make clean
            make CHECK="../tools/codecheck/smatch/smatch --full-path --no-data -p=kkernel" ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j${LICHEE_JLEVEL} C=2 uImage modules 2>&1|tee output/build_smatch.txt
            cat output/build_smatch.txt|egrep -w '(warn|error|warning)' >output/warn_smatch.txt
        fi
    fi
    if [ "x$SUNXI_STACK_CHECK" = "x1" ];then
        make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j${LICHEE_JLEVEL} checkstack 2>&1|tee output/warn_stack.txt
    fi
	update_kern_ver

	#The Image is origin binary from vmlinux.
	cp -vf arch/arm/boot/Image output/bImage
	cp -vf arch/arm/boot/[zu]Image output/

	cp .config output/

	tar -jcf output/vmlinux.tar.bz2 vmlinux

        if [ ! -f ./drivers/arisc/binary/arisc ]; then
                echo "arisc" > ./drivers/arisc/binary/arisc
        fi
        cp ./drivers/arisc/binary/arisc output/

    #for file in $(find drivers sound crypto block fs security net -name "*.ko"); do
    #   cp $file ${LICHEE_MOD_DIR}
    #done
    #cp -f Module.symvers ${LICHEE_MOD_DIR}
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} -j${LICHEE_JLEVEL} INSTALL_MOD_PATH=output modules_install

}

build_modules()
{
	echo "Building modules"

	if [ ! -f include/generated/utsrelease.h ]; then
		printf "Please build kernel first!\n"
		exit 1
	fi

	update_kern_ver

	# build_nand_lib
	# make -C modules/nand LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} \
	# 	CONFIG_CHIP_ID=${CONFIG_CHIP_ID} install

	build_gpu
}

regen_rootfs_cpio()
{
	echo "regenerate rootfs cpio"

	cd ${LICHEE_KDIR}/output
	if [ -x "../scripts/build_rootfs.sh" ]; then
		../scripts/build_rootfs.sh e ./rootfs.cpio.gz > /dev/null
	else
		echo "No such file: scripts/build_rootfs.sh"
		exit 1
	fi

	mkdir -p ./skel/lib/modules/${KERNEL_VERSION}

	if [ -e ${LICHEE_MOD_DIR}/nand.ko ]; then
		cp ${LICHEE_MOD_DIR}/nand.ko ./skel/lib/modules/${KERNEL_VERSION}
		if [ $? -ne 0 ]; then
			echo "copy nand module error: $?"
			exit 1
		fi
	fi

	ko_file=`find ./skel/lib/modules/$KERNEL_VERSION/ -name *.ko`
	if [ ! -z "$ko_file" ]; then
	        ${STRIP} -d ./skel/lib/modules/$KERNEL_VERSION/*.ko
	fi

	rm -f rootfs.cpio.gz

	CHIP=`echo ${LICHEE_CHIP} | sed -n -e 's/.*\(sun8iw8\).*/\1/pg'`;
	if [ "${CHIP}" == "sun8iw8" ]; then
		if [ -d skel ]; then
			(cd skel; find . | fakeroot  cpio -o -Hnewc | lzma -9 > ../rootfs.cpio.gz)
		else
			echo "Warning: Not found skel dir"
		fi
	else
		../scripts/build_rootfs.sh c rootfs.cpio.gz > /dev/null
	fi

	rm -rf skel

	cd - > /dev/null
}

build_ramfs()
{
	local bss_sz=0;
	local CHIP="";

	local BIMAGE="output/bImage";
	local RAMDISK="output/rootfs.cpio.gz";
	local BASE="";
	local OFFSET="";

	# update rootfs.cpio.gz with new module files
	regen_rootfs_cpio

	CHIP=`echo ${LICHEE_CHIP} | sed -e 's/.*\(sun[0-9x]i\).*/\1/g'`;

	if [ "${CHIP}" = "sun9i" ]; then
		BASE="0x20000000";
	else
		BASE="0x40000000";
	fi

	if [ -n "`echo ${LICHEE_CHIP} | sed -n -e 's/.*\(sun8iw8\).*/\1/pg'`" ]; then
		BIMAGE="output/zImage";
	fi

	if [ -f vmlinux ]; then
		bss_sz=`${CROSS_COMPILE}readelf -S vmlinux | \
			awk  '/\.bss/ {print strtonum("0x"$5)+strtonum("0x"$6)}'`;
	fi
	#bss_sz=`du -sb ${BIMAGE} | awk '{printf("%u", $1)}'`;
	#
	# If the size of bImage larger than 16M, will offset 0x02000000
	#
	if [ ${bss_sz} -gt $((16#1000000)) ]; then
		OFFSET="0x02000000";
	else
		OFFSET="0x01000000";
	fi

	${MKBOOTIMG} --kernel ${BIMAGE} \
		--ramdisk ${RAMDISK} \
		--board ${CHIP} \
		--base ${BASE} \
		--ramdisk_offset ${OFFSET} \
		-o output/boot.img
	
	# If uboot use *bootm* to boot kernel, we should use uImage.
	echo build_ramfs
    	echo "Copy boot.img to output directory ..."
    	cp output/boot.img ${LICHEE_PLAT_OUT}
    	cp output/uImage ${LICHEE_PLAT_OUT}
    	cp output/zImage ${LICHEE_PLAT_OUT}
	cp output/vmlinux.tar.bz2 ${LICHEE_PLAT_OUT}

        if [ ! -f output/arisc ]; then
        	echo "arisc" > output/arisc
        fi
        cp output/arisc    ${LICHEE_PLAT_OUT}
}

gen_output()
{
    if [ "x${LICHEE_PLATFORM}" = "xandroid" ] ; then
        echo "Copy modules to target ..."
        rm -rf ${LICHEE_PLAT_OUT}/lib
        cp -rf ${LICHEE_KDIR}/output/* ${LICHEE_PLAT_OUT}
        return
    fi

    if [ -d ${LICHEE_BR_OUT}/target ] ; then
        echo "Copy modules to target ..."
        local module_dir="${LICHEE_BR_OUT}/target/lib/modules"
        rm -rf ${module_dir}
        mkdir -p ${module_dir}
        cp -rf ${LICHEE_MOD_DIR} ${module_dir}
    fi
}

clean_kernel()
{
    echo "Cleaning kernel"
    if [ -f .config ] ; then
        make -C modules/mali LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} clean
    fi
    make distclean
    rm -rf output/*

    (
    export LANG=en_US.UTF-8
    unset LANGUAGE
    )
}

clean_modules()
{
	echo "Cleaning modules"
	clean_gpu
}

#####################################################################
#
#                      Main Runtine
#
#####################################################################

#LICHEE_ROOT=`(cd ${LICHEE_KDIR}/..; pwd)`
#export PATH=${LICHEE_ROOT}/buildroot/output/external-toolchain/bin:${LICHEE_ROOT}/tools/pack/pctools/linux/android:$PATH
#if [ x$2 = x ];then
#	echo Error! you show pass chip name as param 2
#	exit 1
#else
#	chip_name=$2
#	platform_name=${chip_name:0:5}
#fi

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
*)
	build_kernel
	build_modules
	build_ramfs
	gen_output
	echo -e "\n\033[0;31;1m${LICHEE_CHIP} compile Kernel successful\033[0m\n\n"
	;;
esac

