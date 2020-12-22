EXTRA_LDFLAGS += --strip-debug
ifneq ($(strip $(CONFIG_ARCH_SUN8IW8)),)

obj-$(CONFIG_OV2710_MIPI)	+= ov2710_mipi.o
obj-$(CONFIG_OV4689)		+= ov4689.o
obj-$(CONFIG_OV4689_60FPS)	+= ov4689_60fps.o
obj-$(CONFIG_AR0330_MIPI)	+= ar0330_mipi.o
obj-$(CONFIG_OV4689_SDV)	+= ov4689_sdv.o
obj-$(CONFIG_GC1004_MIPI)	+= gc1004_mipi.o
obj-$(CONFIG_H22_MIPI)		+= h22_mipi.o

else
obj-m			+= ov5640.o
obj-m			+= ov2640.o
obj-m			+= ov7736.o
obj-m			+= s5k4ec.o
obj-m			+= s5k4ec_mipi.o
obj-m			+= gc2035.o
obj-m			+= gt2005.o
obj-m			+= gc0307.o
obj-m			+= gc0308.o
obj-m			+= gc0328.o
obj-m			+= gc0329.o
obj-m			+= gc0311.o
obj-m			+= hi253.o
obj-m			+= sp2518.o
obj-m			+= sp2519.o
obj-m			+= sp0718.o
obj-m			+= sp0838.o
obj-m			+= ov16825.o
obj-m			+= ov5650.o
obj-m			+= ov5647.o
obj-m			+= ov5647_mipi.o
obj-m			+= t8et5.o
obj-m			+= s5k4e1.o
obj-m			+= s5k4e1_mipi.o
obj-m			+= sp2518.o
obj-m			+= sp0718.o
obj-m			+= gc5004.o
obj-m			+= gc5004_mipi.o
obj-m			+= ov5648.o
obj-m			+= ar0330.o
obj-m			+= ov5648.o
obj-m			+= sp5408.o
obj-m			+= ov12830.o
obj-m			+= ov8825.o
obj-m			+= ov8850.o
obj-m			+= gc2155.o
obj-m			+= ov8858.o
obj-m			+= ov13850.o
obj-m			+= imx214.o
obj-m			+= ov8858_4lane.o
obj-m			+= sp5409.o
obj-m			+= s5k5e2yx.o
obj-m			+= ov2710_mipi.o

endif

