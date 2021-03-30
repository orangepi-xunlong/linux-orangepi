ARCH=arm64
echo $PWD
CROSS_COMPILE=${PWD}/../../../../../out/sun50iw2p1/ubuntu/common/buildroot/external-toolchain/bin/aarch64-linux-gnu-
KERNELDIR=${PWD}/../../../../
make -C ${KERNELDIR} ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}  M=${PWD} -j4
