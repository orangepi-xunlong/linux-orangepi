#!/bin/bash -e

SCRIPT="./scripts/build_kernel.sh"

CLEAN=false
BUILD_DEB=false

while getopts "cdh" opt
do
    case $opt in
        c)
            CLEAN=true
            ;;
        d)
            BUILD_DEB=true
            ;;
        ?)
            echo "Usage: Run on the top of linux directory"
            echo "$SCRIPT [-c] [-d]"
            exit 1
            ;;
    esac
done

if [ $0 != $SCRIPT ]
then
    echo "Run $(basename $SCRIPT) on the top of linux directory"
    exit 1
fi

command -v riscv64-unknown-linux-gnu-gcc > /dev/null || \
    (echo "Install cross compile and add to PATH" && exit 1)

export ARCH=riscv
export CROSS_COMPILE=riscv64-unknown-linux-gnu-
export KERNELRELEASE=6.1.15
export LOCALVERSION=""

if [ $CLEAN = true ]
then
    make distclean
    if [ $BUILD_DEB = true ]
    then
        rm -f ../linux-headers-$KERNELRELEASE_*_riscv64.deb
        rm -f ../linux-image-$KERNELRELEASE_*_riscv64.deb
        rm -f ../linux-image-$KERNELRELEASE-dbg_*_riscv64.deb
        rm -f ../linux-libc-dev_*_riscv64.deb
        rm -f ../linux-riscv-ky_*_riscv64.*
    fi
fi

make x1_defconfig

if [ $BUILD_DEB = true ]
then
    KDEB_SOURCENAME=linux-riscv-ky \
    KDEB_PKGVERSION=6.1.15-$(TZ=Asia/Shanghai date +"%Y%m%d%H%M%S") \
    KDEB_CHANGELOG_DIST=mantic-porting \
    make -j$(nproc) bindeb-pkg
else
    make -j$(nproc)
fi
