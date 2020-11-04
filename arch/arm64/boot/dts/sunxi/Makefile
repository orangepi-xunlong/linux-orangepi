dtb-$(CONFIG_ARCH_SUN50IW3P1) += sun50iw3p1-fpga.dtb sun50iw3p1-soc.dtb \
				sun50iw3p1-perf1_v1_0.dtb sun50iw3p1-perf2_v1_0.dtb \
				sun50iw3p1-y1.dtb sun50iw3p1-t1.dtb

dtb-$(CONFIG_ARCH_SUN50IW1P1) += sun50iw1p1-fpga.dtb  sun50iw1p1-perf1_v1_0.dtb \
				sun50iw1p1-perf2_v1_0.dtb sun50iw1p1-t1.dtb \
				sun50iw1p1-sata.dtb sun50iw1p1-soc.dtb

dtb-$(CONFIG_ARCH_SUN50IW6P1) += sun50iw6p1-soc.dtb

dtb-$(CONFIG_ARCH_SUN50IW8P1) += sun50iw8p1-fpga.dtb sun50iw8p1-soc.dtb
dtb-$(CONFIG_ARCH_SUN50IW9P1) += sun50i-h616-orangepi-zero2.dtb
dtb-$(CONFIG_ARCH_SUN50IW10P1) += board.dtb
dtb-$(CONFIG_ARCH_SUN50IW5T) += sun50iw5t-fpga.dtb

DTC_FLAGS ?= -a 262144

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb
