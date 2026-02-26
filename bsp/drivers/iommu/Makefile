# SPDX-License-Identifier: GPL-2.0
obj-$(CONFIG_AW_IOMMU_DEBUG) += sunxi-iommu-debug.o

obj-$(CONFIG_AW_IOMMU) += sunxi-iommu.o
sunxi-iommu-objs :=  sunxi-iommu-common.o sunxi-iommu-pgtable.o
# wo need to pass tristate from aw_iommu to driver compilation
# so use AW_IOMMU as obj-suffix instead of the version config,
# say, CONFIG_AW_IOMMU_V1, itself
ifeq ($(CONFIG_AW_IOMMU_V2),y)
sunxi-iommu-objs += sunxi-iommu-v2.o
else ifeq ($(CONFIG_AW_IOMMU_V1),y)
sunxi-iommu-objs += sunxi-iommu-v1.o
else ifeq ($(CONFIG_AW_IOMMU_V3),y)
sunxi-iommu-objs += sunxi-iommu-v3.o
endif
