#!/bin/bash
set -e

localpath=$(cd $(dirname $0) && pwd)
build_system=lichee
[ -d $localpath/../../../device/config/chips ] && build_system=longan
[ -d $localpath/../../tools/pack/chips ] && build_system=lichee

echo "build_system: $build_system"

#Setup common variables
export ARCH=arm
export CROSS_COMPILE=arm-linux-gnueabi-
if [ -n "`echo ${LICHEE_CHIP} | grep "sun5[0-9]i"`" ] && \
        [ "x${LICHEE_ARCH}" = "xarm64" ]; then
    export ARCH=arm64
    export CROSS_COMPILE=aarch64-linux-gnu-
fi

if [ -n "${LICHEE_TOOLCHAIN_PATH}" \
        -a -d "${LICHEE_TOOLCHAIN_PATH}" ]; then
    GCC=$(find ${LICHEE_TOOLCHAIN_PATH} -perm /a+x -a -regex '.*-gcc');
    export CROSS_COMPILE="${GCC%-*}-";
elif [ -n "${LICHEE_CROSS_COMPILER}" ]; then
    export CROSS_COMPILE="${LICHEE_CROSS_COMPILER}-"
fi

MAKE="make"

if [ -n "$ANDROID_CLANG_PATH" ]; then
    MAKE="make CC=$LICHEE_TOP_DIR/../$ANDROID_CLANG_PATH/clang"
    ARCH_PREFIX=arm
    [ "x$ARCH" == "xarm64" ] && ARCH_PREFIX=aarch64
    if [ -n "$ANDROID_TOOLCHAIN_PATH" ]; then
        export CROSS_COMPILE=$LICHEE_TOP_DIR/../$ANDROID_TOOLCHAIN_PATH/$ARCH_PREFIX-linux-android-
        export CLANG_TRIPLE=$ARCH_PREFIX-linux-gnu-
    fi
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
if [ "x$CCACHE_DIR" != "x" ];then
    CCACHE_Y="ccache "
fi

KERNEL_VERSION=`${MAKE} -s kernelversion -C ./`
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

do_init_to_dts()
{
	local DTC_SRC_PATH=${LICHEE_KERN_DIR}/arch/${ARCH}/boot/dts
	if [ "x${ARCH}" == "xarm64" ]; then
		DTC_SRC_PATH=${DTC_SRC_PATH}/sunxi
	fi
	local DTC_DEP_BOARD1=${DTC_SRC_PATH}/.board.dtb.d.dtc.tmp
	local DTC_SRC_BOARD1=${DTC_SRC_PATH}/.board.dtb.dts.tmp
	local DTC_DEP_BOARD=${DTC_SRC_PATH}/.${LICHEE_CHIP}-${LICHEE_BOARD}.dtb.d.dtc.tmp
	local DTC_SRC_BOARD=${DTC_SRC_PATH}/.${LICHEE_CHIP}-${LICHEE_BOARD}.dtb.dts.tmp
	local DTC_DEP_BUSSINESS=${DTC_SRC_PATH}/.${LICHEE_CHIP}-${LICHEE_BUSSINESS}.dtb.d.dtc.tmp
	local DTC_SRC_BUSSINESS=${DTC_SRC_PATH}/.${LICHEE_CHIP}-${LICHEE_BUSSINESS}.dtb.dts.tmp
	local DTC_DEP_COMMON=${DTC_SRC_PATH}/.${LICHEE_CHIP}-soc.dtb.d.dtc.tmp
	local DTC_SRC_COMMON=${DTC_SRC_PATH}/.${LICHEE_CHIP}-soc.dtb.dts.tmp

	local DTC_COMPILER=${LICHEE_KERN_DIR}/scripts/dtc/dtc

	local DTC_INI_FILE_BASE=${LICHEE_BOARD_CONFIG_DIR}/sys_config.fex
	local DTC_INI_FILE=output/sys_config_fix.fex

	cp $DTC_INI_FILE_BASE $DTC_INI_FILE
	sed -i "s/\(\[dram\)_para\(\]\)/\1\2/g" $DTC_INI_FILE
	sed -i "s/\(\[nand[0-9]\)_para\(\]\)/\1\2/g" $DTC_INI_FILE

	if [ ! -f $DTC_COMPILER ]; then
		echo "Script_to_dts: Can not find dtc compiler.\n"
		exit 1
	fi

	if [ -f $DTC_DEP_BOARD1 ]; then
		printf "Script_to_dts: use board special dts file board.dts\n"
		DTC_DEP_FILE=${DTC_DEP_BOARD1}
		DTC_SRC_FILE=${DTC_SRC_BOARD1}
	elif [ -f $DTC_DEP_BOARD ]; then
		printf "Script_to_dts: use board special dts file ${LICHEE_CHIP}-${LICHEE_BOARD}.dts\n"
		DTC_DEP_FILE=${DTC_DEP_BOARD}
		DTC_SRC_FILE=${DTC_SRC_BOARD}
	elif [ -f $DTC_DEP_BUSSINESS ]; then
		printf "Script_to_dts: use bussiness special dts file ${LICHEE_CHIP}-${LICHEE_BUSSINESS}.dts\n"
		DTC_DEP_FILE=${DTC_DEP_BUSSINESS}
		DTC_SRC_FILE=${DTC_SRC_BUSSINESS}
	else
		printf "Script_to_dts: use common dts file ${LICHEE_CHIP}-soc.dts\n"
		DTC_DEP_FILE=${DTC_DEP_COMMON}
		DTC_SRC_FILE=${DTC_SRC_COMMON}
	fi

	local DTC_FLAGS="-W no-unit_address_vs_reg"

	if [ -d ${LICHEE_CHIP_CONFIG_DIR}/dtbo ];  then
		echo "sunxi_dtb create"
		$DTC_COMPILER ${DTC_FLAGS} -@ -O dtb -o ${LICHEE_KERN_DIR}/output/sunxi.dtb	\
			-b 0			\
			-i $DTC_SRC_PATH	\
			-F $DTC_INI_FILE	\
			-d $DTC_DEP_FILE $DTC_SRC_FILE
	else
		echo "sunxi_dtb create"
		$DTC_COMPILER -p 2048 ${DTC_FLAGS} -O dtb -o ${LICHEE_KERN_DIR}/output/sunxi.dtb	\
			-b 0			\
			-i $DTC_SRC_PATH	\
			-F $DTC_INI_FILE	\
			-d $DTC_DEP_FILE $DTC_SRC_FILE
	fi

	if [ $? -ne 0 ]; then
		echo "Conver script to dts failed"
		exit 1
	fi

	#restore the orignal dtsi
	if [ "x${LICHEE_LINUX_DEV}" = "xdragonboard" \
		-o "x${LICHEE_LINUX_DEV}" = "xdragonmat" ]; then
		local DTS_PATH=${LICHEE_KERN_DIR}/arch/${ARCH}/boot/dts
		[ "x${ARCH}" = "xarm64" ] && DTS_PATH=${LICHEE_KERN_DIR}/arch/${ARCH}/boot/dts/sunxi
		if [ -f ${DTS_PATH}/${LICHEE_CHIP}_bak.dtsi ];then
			rm -f ${DTS_PATH}/${LICHEE_CHIP}.dtsi
			mv  ${DTS_PATH}/${LICHEE_CHIP}_bak.dtsi  ${DTS_PATH}/${LICHEE_CHIP}.dtsi
		fi
	fi

	printf "Conver script to dts ok.\n"

	# It'is used for debug dtb
	$DTC_COMPILER ${DTC_FLAGS} -I dtb -O dts -o ${LICHEE_KERN_DIR}/output/.sunxi.dts ${LICHEE_KERN_DIR}/output/sunxi.dtb

	return
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

build_nand()
{
    NAND_ROOT=${LICHEE_KDIR}/modules/nand

    ${MAKE} -C modules/nand LICHEE_MOD_DIR=${LICHEE_MOD_DIR} \
        LICHEE_KDIR=${LICHEE_KDIR} \
        CONFIG_CHIP_ID=${CONFIG_CHIP_ID} install

}

gpu_message()
{
    echo -ne "\033[34;1m"
    echo "[GPU]: $1"
    echo -ne "\033[0m"
}

build_gpu()
{
    GPU_TYPE=`fgrep CONFIG_SUNXI_GPU_TYPE ${LICHEE_KDIR}/.config 2>/dev/null | cut -d \" -f 2`
    if [ "X$GPU_TYPE" = "XNone" -o "X$GPU_TYPE" = "X" ]; then
        gpu_message "No GPU type is configured in ${LICHEE_KDIR}/.config."
        return
    fi

    gpu_message "Building $GPU_TYPE device driver..."

    if [ "X${LICHEE_PLATFORM}" = "Xandroid" -o "X${LICHEE_PLATFORM}" = "Xsecureandroid" ] ; then
        TMP_OUT=$OUT
        TMP_TOP=$TOP
        unset OUT
        unset TOP
    fi

    ${MAKE} -C modules/gpu LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR}

    if [ "X${LICHEE_PLATFORM}" = "Xandroid" -o "X${LICHEE_PLATFORM}" = "Xsecureandroid" ] ; then
        export OUT=$TMP_OUT
        export TOP=$TMP_TOP
    fi

    gpu_message "$GPU_TYPE device driver has been built."
}

clean_gpu()
{
    GPU_TYPE=`fgrep CONFIG_SUNXI_GPU_TYPE ${LICHEE_KDIR}/.config 2>/dev/null | cut -d \" -f 2`
    if [ "X$GPU_TYPE" = "XNone" -o "X$GPU_TYPE" = "X" ]; then
        gpu_message "No GPU type is configured in .config."
        return
    fi

    gpu_message "Cleaning $GPU_TYPE device driver..."
    ${MAKE} -C modules/gpu LICHEE_MOD_DIR=${LICHEE_MOD_DIR} LICHEE_KDIR=${LICHEE_KDIR} clean
    gpu_message "$GPU_TYPE device driver has been cleaned."
}

build_check()
{
    if [ "x$SUNXI_CHECK" = "x1" ];then
        SUNXI_SPARSE_CHECK=1
        SUNXI_SMATCH_CHECK=1
        SUNXI_STACK_CHECK=1
    fi

    if [ "x$SUNXI_SPARSE_CHECK" = "x1" ] && [ -f ../tools/codecheck/sparse/sparse ];then
        echo "\n\033[0;31;1mBuilding Round for sparse check...\033[0m\n\n"
        CHECK="../tools/codecheck/sparse/sparse"
        ${MAKE} CHECK=${CHECK} ARCH=${ARCH} CROSS_COMPILE="${CCACHE_Y}${CROSS_COMPILE}" -j${LICHEE_JLEVEL} \
                C=2 all modules 2>&1|tee output/build_sparse.txt
        cat output/build_sparse.txt|egrep -w '(warn|error|warning)' >output/warn_sparse.txt
    fi

    if [ "x$SUNXI_SMATCH_CHECK" = "x1" ]&& [ -f ../tools/codecheck/smatch/smatch ];then
        echo "\n\033[0;31;1mBuilding Round for smatch check...\033[0m\n\n"
        CHECK="../tools/codecheck/smatch/smatch --full-path --no-data -p=kkernel"
        ${MAKE} CHECK=${CHECK} ARCH=${ARCH} CROSS_COMPILE="${CCACHE_Y}${CROSS_COMPILE}" -j${LICHEE_JLEVEL} \
                C=2 all modules 2>&1|tee output/build_smatch.txt
        cat output/build_smatch.txt|egrep -w '(warn|error|warning)' >output/warn_smatch.txt
    fi

    if [ "x$SUNXI_STACK_CHECK" = "x1" ];then
        ${MAKE} ARCH=${ARCH} CROSS_COMPILE="${CCACHE_Y}${CROSS_COMPILE}" -j${LICHEE_JLEVEL} checkstack 2>&1 \
                |tee output/warn_stack.txt
    fi
}

build_dts(){
    echo "---build dts for ${LICHEE_CHIP} ${LICHEE_BOARD}-----"
    #make ARCH=${ARCH} ${LICHEE_CHIP}-perf1.dtb
    dtb_file="${LICHEE_CHIP}-${LICHEE_BOARD}.dtb"
    dts_file="${LICHEE_CHIP}-${LICHEE_BOARD}.dts"
    if [ "x${ARCH}" = "xarm" ];then
        dts_path="arch/arm/boot/dts"
    else
        dts_path="arch/arm64/boot/dts/sunxi"
    fi
    dts_file=
    if [ ! -f $dts_path/$dts_file ];then
        dts_file="${LICHEE_CHIP}-soc.dts"
        dtb_file="${LICHEE_CHIP}-soc.dtb"
    fi
    if [ ! -f $dts_path/$dts_file ];then
        dtb_file=board.dtb
    fi
    make ARCH=${ARCH} ${dtb_file}
}

build_kernel()
{
    echo "Building kernel"

    cd ${LICHEE_KDIR}
    rm -rf output/
    echo "${LICHEE_MOD_DIR}"
    mkdir -p ${LICHEE_MOD_DIR}

    # uImage is arm architecture specific target
    local arch_target=""
    if [ "${ARCH}" = "arm" ]; then
        arch_target="uImage dtbs"
    else
        arch_target="all"
    fi

    #exchange sdc0 and sdc2 for dragonBoard card boot
    local check_cardboot=false
    local SYS_CONFIG_FILE=""
    if [ "x$build_system" == "xlichee" ]; then
        [ "x${LICHEE_PLATFORM}" == "xdragonboard" -o "x${LICHEE_PLATFORM}" == "xdragonmat" ] && check_cardboot=true
        SYS_CONFIG_FILE=$localpath/../../tools/pack/chips/${LICHEE_CHIP}/configs/${LICHEE_BOARD}/sys_config.fex
    else
        [ "x${LICHEE_PLATFORM}" == "xlinux" ] && [ "x${LICHEE_LINUX_DEV}" == "xdragonboard" -o "x${LICHEE_LINUX_DEV}" == "xdragonmat" ] && check_cardboot=true
        SYS_CONFIG_FILE=${LICHEE_BOARD_CONFIG_DIR}/sys_config.fex
    fi

    if [ "x$check_cardboot" == "xtrue" ]; then
        local DTS_PATH=./arch/${ARCH}/boot/dts
        [ "x$ARCH" == "xarm64" ] && DTS_PATH=$DTS_PATH/sunxi

        if [ -f ${DTS_PATH}/${LICHEE_CHIP}_bak.dtsi ];then
            mv ${DTS_PATH}/${LICHEE_CHIP}_bak.dtsi ${DTS_PATH}/${LICHEE_CHIP}.dtsi
        fi

        # if find dragonboard_test=1 in sys_config.fex ,then will exchange sdc0 and sdc2
        local card_boot=$(awk -F= '/^dragonboard_test[[:space:]]*=/{gsub(/[[:blank:]]*/,"",$2);print $2}' $SYS_CONFIG_FILE)
        if [ "x$card_boot" == "x1" ]; then
            echo "exchange sdc0 and sdc2 for dragonboard card boot"
            ./scripts/exchange-sdc0-sdc2-for-dragonboard.sh ${DTS_PATH}/${LICHEE_CHIP}.dtsi
        fi
    fi

    if [ ! -f .config ] ; then
        printf "\n\033[0;31;1mUsing default config ${LICHEE_KERN_DEFCONF_ABSOLUTE} ...\033[0m\n\n"
        ${MAKE} ARCH=${ARCH} defconfig KBUILD_DEFCONFIG=${LICHEE_KERN_DEFCONF_RELATIVE}
    fi

    if [ "x$PACK_TINY_ANDROID" = "xtrue" ]; then
        ARCH=${ARCH} scripts/kconfig/merge_config.sh .config \
            linaro/configs/sunxi-tinyandroid.conf
    fi

    if [ "x$PACK_BSPTEST" != "x" -o "x$BUILD_SATA" != "x" -o "x$LICHEE_LINUX_DEV" = "xsata" ]; then
	    if [ -f linaro/configs/sunxi-common.conf ];then
	        ARCH=${ARCH} scripts/kconfig/merge_config.sh .config \
                linaro/configs/sunxi-common.conf
	    fi
	    if [ -f linaro/configs/sunxi-sata.conf ];then
	        ARCH=${ARCH} scripts/kconfig/merge_config.sh .config \
                linaro/configs/sunxi-sata.conf
        fi
        if [ -f linaro/configs/sunxi-sata-${LICHEE_CHIP}.conf ];then
            ARCH=${ARCH} scripts/kconfig/merge_config.sh .config \
                linaro/configs/sunxi-sata-${LICHEE_CHIP}.conf
        fi
        if [ -f linaro/configs/sunxi-sata-${ARCH}.conf ];then
            ARCH=${ARCH} scripts/kconfig/merge_config.sh .config \
                linaro/configs/sunxi-sata-${ARCH}.conf
        fi
    fi

    ${MAKE} ARCH=${ARCH} CROSS_COMPILE="${CCACHE_Y}${CROSS_COMPILE}" -j${LICHEE_JLEVEL} ${arch_target} modules
    build_check
    update_kern_ver
    do_init_to_dts
    #The Image is origin binary from vmlinux.
    if [ -f arch/${ARCH}/boot/Image ]; then
        cp -vf arch/${ARCH}/boot/Image output/bImage
    fi

    if [ -f arch/${ARCH}/boot/zImage ] || [ -f arch/${ARCH}/boot/uImage ]; then
        cp -vf arch/${ARCH}/boot/[zu]Image output/
    fi

    if [ -f arch/${ARCH}/boot/Image.gz ]; then
        cp -vf arch/${ARCH}/boot/Image.gz output/
    fi

    echo "Copy rootfs.cpio.gz for ${ARCH}"
    if [ "${ARCH}" = "arm" ]; then
        cp -f rootfs_32bit.cpio.gz output/rootfs.cpio.gz
    else
        cp -f rootfs.cpio.gz output/rootfs.cpio.gz
    fi

    cp .config output/

    tar -jcf output/vmlinux.tar.bz2 vmlinux
    for file in $(find drivers sound crypto block fs security net -name "*.ko"); do
        cp $file ${LICHEE_MOD_DIR}
    done
    cp -f Module.symvers ${LICHEE_MOD_DIR}

}

build_modules()
{

    echo "Building modules"

    if [ ! -f include/generated/utsrelease.h ]; then
        printf "Please build kernel first!\n"
        exit 1
    fi

    update_kern_ver
    build_nand
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
    ../scripts/build_rootfs.sh c rootfs.cpio.gz > /dev/null
    rm -rf skel
    cd - > /dev/null
}

build_ramfs()
{
    local bss_sz=0;
    local bss_offset=0;
    local bss_size=0;
    local CHIP="";

    local BIMAGE="output/bImage";
    local RAMDISK="output/rootfs.cpio.gz";
    local DTB="output/sunxi.dtb";
    local BASE="";
    local RAMDISK_OFFSET="";
    local KERNEL_OFFSET="";

    # update rootfs.cpio.gz with new module files
    regen_rootfs_cpio

    CHIP=`echo ${LICHEE_CHIP} | sed -e 's/.*\(sun[0-9x]*i\).*/\1/g'`;

    if [ "${CHIP}" = "sun9i" ]; then
        BASE="0x20000000";
    else
        BASE="0x40000000";
    fi

    if [ "${ARCH}" = "arm" ]; then
        KERNEL_OFFSET="0x8000";
    elif [ "${ARCH}" = "arm64" ]; then
        KERNEL_OFFSET="0x80000";
    fi

    if [ -f vmlinux ]; then
        bss_offset=`${CROSS_COMPILE}readelf -S vmlinux | \
            awk  '/\.bss/ {print $5}'`;
        bss_size=`${CROSS_COMPILE}readelf -S vmlinux | \
            awk  '/\.bss/ {print $6}'`;

        # If linux-4.9 the bss_size will be in next line
        if [ -z "$bss_size" ]; then
            bss_size=`${CROSS_COMPILE}readelf -S vmlinux | \
                 awk  '/\.bss/ {getline var; print var}' | awk '{print $1}'`;
        fi

        bss_sz=$[ $((16#$bss_offset))+$((16#$bss_size))]
    fi

    # If the size of bImage larger than 16M, will offset 0x02000000
    if [ ${bss_sz} -gt $((16#1000000)) ]; then
        RAMDISK_OFFSET="0x02000000";
    else
        RAMDISK_OFFSET="0x01000000";
    fi
    DTB_OFFSET="0x04000000";

    ${MKBOOTIMG} --kernel ${BIMAGE} \
        --ramdisk ${RAMDISK} \
        --dtb ${DTB} \
        --board ${CHIP}_${ARCH} \
        --base ${BASE} \
        --kernel_offset ${KERNEL_OFFSET} \
        --ramdisk_offset ${RAMDISK_OFFSET} \
        --dtb_offset ${DTB_OFFSET} \
        --header_version 0x2 \
        -o output/boot.img

    # If uboot use *bootm* to boot kernel, we should use uImage.
    echo build_ramfs
    echo "Copy boot.img to output directory ..."
    cp output/boot.img ${LICHEE_PLAT_OUT}
    cp output/vmlinux.tar.bz2 ${LICHEE_PLAT_OUT}

    if [ -f output/zImage ] || [ -f output/uImage ]; then
        cp output/[zu]Image ${LICHEE_PLAT_OUT}
    fi

    if [ ! -f output/arisc ]; then
        echo "arisc" > output/arisc
    fi
    cp output/arisc ${LICHEE_PLAT_OUT}

    cp output/.sunxi.dts ${LICHEE_PLAT_OUT}
    cp output/sunxi.dtb ${LICHEE_PLAT_OUT}
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

    echo "$0"
}

clean_kernel()
{
    clarg="clean"
    if [ "x$1" == "xdistclean" ]; then
        clarg="distclean"
    fi
    echo "Cleaning kernel, arg: $clarg ..."
    ${MAKE} ARCH=${ARCH} "$clarg"
    rm -rf output/*
    echo
}

clean_modules()
{
    echo "Cleaning modules ..."
    clean_gpu
    echo
}

#####################################################################
#
#                      Main Runtine
#
#####################################################################

case "$1" in
    kernel)
        build_kernel
        ;;
    modules)
        build_modules
        ;;
    clean|distclean)
        clean_modules
        clean_kernel "$1"
        ;;
    dts)
        build_dts
        ;;
    *)
        build_kernel
        build_modules
        build_ramfs
        gen_output
        echo -e "\n\033[0;31;1m${LICHEE_CHIP} compile Kernel successful\033[0m\n\n"
        ;;
esac
