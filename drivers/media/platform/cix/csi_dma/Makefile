ccflags-y += -I$(srctree)/drivers/media/platform/cadence
ccflags-y += -I$(srctree)/drivers/phy/cadence
ccflags-y += -DCSIDMA_DEBUG
ifdef CONFIG_PLAT_BBOX
#ccflags-y += -DCONFIG_VI_BBOX
endif
ifneq ($(KERNELRELEASE),)
	obj-$(CONFIG_CSI_DMA) += csi_dma.o
	csi_dma-objs = csi_dma_core.o csi_bridge_hw.o csi_dma_cap.o
	obj-$(CONFIG_CSI_DMA) += csi_mipi_csi2.o
	obj-$(CONFIG_CSI_DMA) += csi_mipi_dphy_rx.o
	obj-$(CONFIG_CSI_DMA) += csi_mipi_dphy_hw.o
	obj-$(CONFIG_CSI_DMA) += csi_rcsu_hw.o
else
	PWD := $(shell pwd)
ifeq ($(ARCH), )
	ARCH := arm64
endif

# if not defined, replace with your path here
ifeq ($(CROSS_COMPILE), )
	CROSS_COMPILE := aarch64-none-linux-gnu-
endif

# if not defined, replace with your path here
ifeq ($(KSRC), )
	KSRC := /$(PWD)/../../../../../
endif

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(PWD) modules

clean:
	find . -name '*.o' -type f -print -exec rm -rf {} \;
	find . -name '*.mod.c' -type f -print -exec rm -rf {} \;
	find . -name '*.ko' -type f -print -exec rm -rf {} \;
	find . -name '*.symvers' -type f -print -exec rm -rf {} \;
	find . -name '*.order' -type f -print -exec rm -rf {} \;
	find . -name '*.a' -type f -print -exec rm -rf {} \;
	find . -name '*.mod' -type f -print -exec rm -rf {} \;
	find . -name '*.cmd' -type f -print -exec rm -rf {} \;
	find . -name '*.o.d' -type f -print -exec rm -rf {} \;
endif
