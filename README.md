Mainline linux kernel for Orange Pi PC/PC2/PC3/One
--------------------------------------------------

This kernel tree is meant for:

- Orange Pi One
- Orange Pi PC
- Orange Pi PC 2
- Orange Pi 3
- TBS A711 Tablet

Features in addition to mainline:

- [All] Thermal regulation/CPU DVFS (if CPU heats above certain temperature, it will try to cool itself down by reducing CPU frequency)
- [All] Mark one of DRM planes as a cursor plane, speeding up Xorg based desktop with modesetting driver
- [All] Wireguard (https://www.wireguard.com/)
- [Orange Pi One/PC/PC2] Configure on-board micro-switches to perform system power off function
- [Orange Pi 3] Ethernet, Bluetooth, USB 3.0
- [TBS A711] HM5065 (back camera), Touch screen, ...

Pre-built u-boot and kernels are available at https://xff.cz/kernels/

You may need some firmware files for some part of the functionality. Those are
available at: https://megous.com/git/linux-firmware

You may want to configure CONFIG_EXTRA_FIRMWARE and CONFIG_EXTRA_FIRMWARE_DIR
to your liking. Default is to build the firmware files into the kernel.

You can use this kernel to run a desktop environment on Orange Pi SBCs.

Have fun!


Build instructions
------------------

These are rudimentary instructions and you need to understand what you're doing.
These are just core steps required to build the ATF/u-boot/kernel. Downloading,
verifying, renaming to correct directories is not described or mentioned. You
should be able to infer missing necessary steps yourself for your particular needs.

Get necessary toolchains from:

- https://releases.linaro.org/components/toolchain/binaries/latest/aarch64-linux-gnu/ for 64bit Orange Pi PC2 and Orange Pi 3
- https://releases.linaro.org/components/toolchain/binaries/latest/arm-linux-gnueabihf/ for 32bit Orange Pis

Extract toolchains and prepare the environment:

    CWD=`pwd`
    OUT=$CWD/builds
    SRC=$CWD/u-boot
    export PATH="$PATH:$CWD/Toolchains/arm/bin:$CWD/Toolchains/aarch64/bin"

For Orange Pi PC2:

    export CROSS_COMPILE=aarch64-linux-gnu-
    export KBUILD_OUTPUT=$OUT/.tmp/uboot-pc2
    rm -rf "$KBUILD_OUTPUT"
    mkdir -p $KBUILD_OUTPUT $OUT/pc2

Get and build ATF from https://github.com/ARM-software/arm-trusted-firmware:

    make -C "$CWD/arm-trusted-firmware" PLAT=sun50i_a64 DEBUG=1 bl31
    cp "$CWD/arm-trusted-firmware/build/sun50i_a64/debug/bl31.bin" "$KBUILD_OUTPUT"

Use sun50i_a64 for Orange Pi PC2 and sun50i_h6 for Orange Pi 3.

Build u-boot from https://megous.com/git/u-boot/ (opi-v2019.10 branch) with appropriate
defconfig (orangepi_one_defconfig, orangepi_pc2_defconfig, orangepi_pc_defconfig, orangepi_3_defconfig, tbs_a711_defconfig).

This u-boot branch already has all the necessary patches integrated and is configured for quick u-boot/kernel startup.

    make -C u-boot orangepi_pc2_defconfig
    make -C u-boot -j5
    
    cp $KBUILD_OUTPUT/.config $OUT/pc2/uboot.config
    cat $KBUILD_OUTPUT/{spl/sunxi-spl.bin,u-boot.itb} > $OUT/pc2/uboot.bin

Get kernel from this repository and checkout the latest orange-pi-5.3 branch.

Build the kernel for 64-bit boards:

    export ARCH=arm64
    export CROSS_COMPILE=aarch64-linux-gnu-
    export KBUILD_OUTPUT=$OUT/.tmp/linux-arm64
    mkdir -p $KBUILD_OUTPUT $OUT/pc2

    make -C linux orangepi_defconfig
    make -C linux -j5 clean
    make -C linux -j5 Image dtbs

    cp -f $KBUILD_OUTPUT/arch/arm64/boot/Image $OUT/pc2/
    cp -f $KBUILD_OUTPUT/.config $OUT/pc2/linux.config
    cp -f $KBUILD_OUTPUT/arch/arm64/boot/dts/allwinner/sun50i-h5-orangepi-pc2.dtb $OUT/pc2/board.dtb

Build the kernel for 32-bit boards:

    export ARCH=arm
    export CROSS_COMPILE=arm-linux-gnueabihf-
    export KBUILD_OUTPUT=$OUT/.tmp/linux-arm
    mkdir -p $KBUILD_OUTPUT $OUT/pc

    make -C linux orangepi_defconfig
    make -C linux -j5 clean
    make -C linux -j5 zImage dtbs
    
    cp -f $KBUILD_OUTPUT/arch/arm/boot/zImage $OUT/pc/
    cp -f $KBUILD_OUTPUT/.config $OUT/pc/linux.config
    cp -f $KBUILD_OUTPUT/arch/arm/boot/dts/sun8i-h3-orangepi-pc.dtb $OUT/pc/board.dtb
    # Or use sun8i-h3-orangepi-one.dtb for Orange Pi One


Kernel lockup issues
--------------------

*If you're getting lockups on boot or later during thermal regulation,
you're missing an u-boot patch.*

This patch is necessary to run this kernel!

These lockups are caused by improper NKMP clock factors selection
in u-boot for PLL_CPUX. (M divider should not be used. P divider
should be used only for frequencies below 240MHz.)

This patch for u-boot fixes it:

  0001-sunxi-h3-Fix-PLL1-setup-to-never-use-dividers.patch

Kernel side is already fixed in this kernel tree.
