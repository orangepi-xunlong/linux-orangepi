Mainline linux kernel for Orange Pi PC/PC2/PC3/One, TBS A711, PinePhone, PocketBook Touch Lux 3
-----------------------------------------------------------------------------------------------

This kernel tree is meant for:

- Orange Pi One
- Orange Pi PC
- Orange Pi PC 2
- Orange Pi 3
- PinePhone 1.0, 1.1 and 1.2(a/b)
- TBS A711 Tablet
- PocketBook Touch Lux 3
- Pinebook Pro

Features in addition to mainline:

- [Orange Pi One/PC/PC2] More aggressive OPPs for CPU
- [All] Mark one of DRM planes as a cursor plane, speeding up Xorg based desktop with modesetting driver
- [Orange Pi One/PC/PC2] Configure on-board micro-switches to perform system power off function
- [Orange Pi One/PC/PC2/3] HDMI audio
- [Orange Pi 3] Ethernet
- [TBS A711] HM5065 (back camera)
- [PinePhone] WiFi, Bluetooth, Audio, Modem power, HDMI out over USB-C, USB-C support, cameras, PMIC improvements, power management, fixes here and there
- [PocketBook Touch Lux 3] Display and Touchscreen support

Pre-built u-boot and kernels are available at https://xff.cz/kernels/

You may need some firmware files for some part of the functionality. Those are
available at: https://megous.com/git/linux-firmware

If you want to reproduce my pre-built kernels exactly, you'll need to uncomment
CONFIG_EXTRA_FIRMWARE_DIR and CONFIG_EXTRA_FIRMWARE in the defconfigs, and
point CONFIG_EXTRA_FIRMWARE_DIR to a directory on your computer where the
clone of https://megous.com/git/linux-firmware resides.

You can also leave those two config options commented out, and copy the contents
of https://megous.com/git/linux-firmware to /lib/firmware/ on the target device.

You can use this kernel to run a desktop environment on Orange Pi SBCs,
Arch Linux on your Pinephone, or to have a completely opensource OS on
a Pocketbook e-ink book reader.

Have fun!


Build instructions
------------------

These are rudimentary instructions and you need to understand what you're doing.
These are just core steps required to build the ATF/u-boot/kernel. Downloading,
verifying, renaming to correct directories is not described or mentioned. You
should be able to infer missing necessary steps yourself for your particular needs.

Get necessary toolchains from:

- https://releases.linaro.org/components/toolchain/binaries/latest/aarch64-linux-gnu/ for 64bit Orange Pi PC2 and Orange Pi 3, PinePhone
- https://releases.linaro.org/components/toolchain/binaries/latest/arm-linux-gnueabihf/ for 32bit Orange Pis, Pocketbook, TBS tablet

Extract toolchains and prepare the environment:

    CWD=`pwd`
    OUT=$CWD/builds
    SRC=$CWD/u-boot
    export PATH="$PATH:$CWD/Toolchains/arm/bin:$CWD/Toolchains/aarch64/bin"

For Orange Pi PC2, Orange Pi 3 or PinePhone:

    export CROSS_COMPILE=aarch64-linux-gnu-
    export KBUILD_OUTPUT=$OUT/.tmp/uboot-pc2
    rm -rf "$KBUILD_OUTPUT"
    mkdir -p $KBUILD_OUTPUT $OUT/pc2

Get and build ATF from https://github.com/ARM-software/arm-trusted-firmware:

    make -C "$CWD/arm-trusted-firmware" PLAT=sun50i_a64 DEBUG=1 bl31
    cp "$CWD/arm-trusted-firmware/build/sun50i_a64/debug/bl31.bin" "$KBUILD_OUTPUT"

Use sun50i_a64 for Orange Pi PC2 or PinePhone and sun50i_h6 for Orange Pi 3.

Build u-boot from https://megous.com/git/u-boot/ (opi-v2020.04 branch) with appropriate
defconfig (orangepi_one_defconfig, orangepi_pc2_defconfig, orangepi_pc_defconfig, orangepi_3_defconfig, tbs_a711_defconfig, pinephone_defconfig).

My u-boot branch already has all the necessary patches integrated and is configured for quick u-boot/kernel startup.

    make -C u-boot orangepi_pc2_defconfig
    make -C u-boot -j5
    
    cp $KBUILD_OUTPUT/.config $OUT/pc2/uboot.config
    cat $KBUILD_OUTPUT/{spl/sunxi-spl.bin,u-boot.itb} > $OUT/pc2/uboot.bin

Get kernel from this repository and checkout the latest orange-pi-5.13 branch.

Build the kernel for 64-bit boards:

    export ARCH=arm64
    export CROSS_COMPILE=aarch64-linux-gnu-
    export KBUILD_OUTPUT=$OUT/.tmp/linux-arm64
    mkdir -p $KBUILD_OUTPUT $OUT/pc2

    make -C linux orangepi_defconfig
    # or make -C linux pocketbook_touch_lux_3_defconfig
    # or make -C linux tbs_a711_defconfig
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

    make orangepi_defconfig
    # or make pinephone_defconfig
    make -C linux orangepi_defconfig
    make -C linux -j5 clean
    make -C linux -j5 zImage dtbs
    
    cp -f $KBUILD_OUTPUT/arch/arm/boot/zImage $OUT/pc/
    cp -f $KBUILD_OUTPUT/.config $OUT/pc/linux.config
    cp -f $KBUILD_OUTPUT/arch/arm/boot/dts/sun8i-h3-orangepi-pc.dtb $OUT/pc/board.dtb
    # Or use sun8i-h3-orangepi-one.dtb for Orange Pi One


PinePhone
---------

I don't run u-boot on PinePhone, so my pre-built kernel packages don't come
with u-boot built for PinePhone.


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
