dtb-$(CONFIG_ARCH_SUNXI) += sun50i-h616-orangepi-zero3.dtb

always		:= $(dtb-y)
subdir-y	:= $(dts-dirs)
clean-files	:= *.dtb
DTC_FLAGS_board += -@
DTC_FLAGS ?= -a 262144

subdir-y       := $(dts-dirs) overlay
