// SPDX-License-Identifier: GPL-2.0
/*
 * lizhirong <zhirong.li@ky.com>
 *
 * Copyright (C) 2023 KY Micro Limited
 */
//#define DEBUG

#include <linux/device.h>
#include <linux/dma-direction.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/gfp.h>
#include <linux/scatterlist.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/dma-buf.h>
#include <media/x1/x1_cpp_uapi.h>
#include "x1_cpp.h"
#include "cpp_iommu.h"
#include "cpp_dmabuf.h"

int cpp_dma_alloc_iommu_channels(struct cpp_device *cpp_dev,
				 struct cpp_dma_port_info *dma_info)
{
	struct cpp_iommu_device *mmu_dev = cpp_dev->mmu_dev;
	struct cpp_dma_chnl_info *dma_chnl;
	unsigned int tid, chnl;
	int rc;

	for (chnl = 0; chnl < MAX_DMA_CHNLS; ++chnl) {
		dma_chnl = &dma_info->dma_chnls[chnl];
		tid = dma_chnl->tid;
		if (dma_chnl->dbuf_mapped) {
			rc = mmu_dev->ops->acquire_channel(mmu_dev, tid);
			if (rc)
				continue;
			dma_info->dma_chnls[chnl].mmu_attached = 1;
		}
	}

	return 0;
}

int cpp_dma_fill_iommu_channels(struct cpp_device *cpp_dev,
				struct cpp_dma_port_info *dma_info)
{
	struct cpp_iommu_device *mmu_dev = cpp_dev->mmu_dev;
	struct cpp_dma_chnl_info *dma_chnl;
	unsigned int tid, chnl;

	for (chnl = 0; chnl < MAX_DMA_CHNLS; ++chnl) {
		dma_chnl = &dma_info->dma_chnls[chnl];
		tid = dma_chnl->tid;
		if (dma_chnl->mmu_attached) {
			mmu_dev->ops->setup_sglist(mmu_dev, tid, dma_chnl->fd,
						   dma_chnl->offset, dma_chnl->length);
			mmu_dev->ops->config_channel(mmu_dev, tid, NULL, 0);
			mmu_dev->ops->enable_channel(mmu_dev, tid);
			dma_chnl->phy_addr =
			    mmu_dev->ops->get_iova(mmu_dev, tid, dma_chnl->offset);

			pr_debug
			    ("channel tid-%x: dma addr 0x%llx, tt base %p, tt size %d\n",
			     tid, dma_chnl->phy_addr, dma_chnl->tt_base,
			     dma_chnl->tt_size);
		}
	}

	return 0;
}

int cpp_dma_free_iommu_channels(struct cpp_device *cpp_dev,
				struct cpp_dma_port_info *dma_info)
{
	struct cpp_iommu_device *mmu_dev = cpp_dev->mmu_dev;
	struct cpp_dma_chnl_info *dma_chnl;
	unsigned int tid, chnl;

	for (chnl = 0; chnl < MAX_DMA_CHNLS; ++chnl) {
		dma_chnl = &dma_info->dma_chnls[chnl];
		tid = dma_chnl->tid;
		if (dma_chnl->mmu_attached) {
			mmu_dev->ops->disable_channel(mmu_dev, tid);
			mmu_dev->ops->release_channel(mmu_dev, tid);
			dma_chnl->mmu_attached = 0;
		}
	}

	return 0;
}

static int get_dma_channel_id(unsigned int layer_idx, unsigned int plane_idx,
			      unsigned int is_kgain, enum cpp_pix_format format)
{
	int chnl_id;

	if (layer_idx >= CPP_MAX_LAYERS || plane_idx >= CPP_MAX_PLANAR) {
		pr_err("invalid layer %d or plane %d", layer_idx, plane_idx);
		return -EINVAL;
	}

	if (is_kgain) {
		chnl_id = MAC_DMA_CHNL_KGAIN_L0;
		chnl_id += layer_idx;
	} else {
		chnl_id = (format == PIXFMT_FBC_DWT && layer_idx == 0) ?
		    MAC_DMA_CHNL_FBC_HEADER : MAC_DMA_CHNL_DWT_Y_L0;
		chnl_id += (layer_idx * 2 + plane_idx);
	}

	return chnl_id;
}

struct cpp_dma_port_info *cpp_dmabuf_prepare(struct cpp_device *cpp_dev,
					     struct cpp_buffer_info *buf_info,
					     uint8_t port_id)
{
	struct cpp_dma_port_info *port_info;
	struct cpp_plane_info *plane_info;
	struct cpp_dma_chnl_info *chnl_info;
	int layer, plane, chnl;
	uint32_t map_flags = 0;
	int rc;

	port_info = kzalloc(sizeof(*port_info), GFP_KERNEL);
	if (!port_info) {
		pr_err("%s: alloc dma port info failed\n", __func__);
		return NULL;
	}
	port_info->port_id = port_id;

	if (port_id == MAC_DMA_PORT_W0)
		map_flags |= IOMMU_MAP_FLAG_WRITE_ONLY;
	else
		map_flags |= IOMMU_MAP_FLAG_READ_ONLY;

	if (buf_info->index > 1)
		map_flags |= IOMMU_MAP_FLAG_NOSYNC;

	for (layer = 0; layer < buf_info->num_layers; ++layer) {
		for (plane = 0; plane < CPP_MAX_PLANAR; ++plane) {
			chnl = get_dma_channel_id(layer, plane, 0, buf_info->format);
			if (chnl < 0) {
				pr_err
				    ("%s: port%d, dwt layer%d, plane%d failed to get channel id\n",
				     __func__, port_id, layer, plane);
				goto err;
			}
			chnl_info = &port_info->dma_chnls[chnl];
			plane_info = &buf_info->dwt_planes[layer][plane];

			rc = cpp_iommu_map_dmabuf(cpp_dev->mmu_dev,
						  plane_info->m.fd, map_flags,
						  &chnl_info->phy_addr);
			if (rc) {
				pr_err("%s: port%d, dwt layer%d, plane%d map failed\n",
				       __func__, port_id, layer, plane);
				goto err;
			}

			chnl_info->fd = plane_info->m.fd;
			chnl_info->dbuf_mapped = 1;
			chnl_info->chnl_id = chnl;
			chnl_info->tid = MMU_TID(port_id, chnl);
			chnl_info->length = plane_info->length;
			chnl_info->offset = plane_info->data_offset;
			if (chnl_info->offset)
				chnl_info->phy_addr += chnl_info->offset;
		}
	}

	if (buf_info->format == PIXFMT_FBC_DWT) {
		/* workaround: suppose header and payload exist the same dmabuf fd */
		port_info->dma_chnls[MAC_DMA_CHNL_FBC_HEADER].length +=
		    port_info->dma_chnls[MAC_DMA_CHNL_FBC_PAYLOAD].length;
		port_info->fbc_enabled = true;
	}

	for (layer = 0; buf_info->kgain_used && (layer < buf_info->num_layers); ++layer) {
		chnl = get_dma_channel_id(layer, 0, 1, 0);
		if (chnl < 0) {
			pr_err("%s: port%d, kgain layer%d failed to get channel id\n",
			       __func__, port_id, layer);
			goto err;
		}
		chnl_info = &port_info->dma_chnls[chnl];
		plane_info = &buf_info->kgain_planes[layer];

		rc = cpp_iommu_map_dmabuf(cpp_dev->mmu_dev, plane_info->m.fd,
					  map_flags, &chnl_info->phy_addr);
		if (rc) {
			pr_err("%s: port%d, kgain layer%d map failed\n",
			       __func__, port_id, layer);
			goto err;
		}

		chnl_info->fd = plane_info->m.fd;
		chnl_info->dbuf_mapped = 1;
		chnl_info->chnl_id = chnl;
		chnl_info->tid = MMU_TID(port_id, chnl);
		chnl_info->length = plane_info->length;
		chnl_info->offset = plane_info->data_offset;
		if (chnl_info->offset)
			chnl_info->phy_addr += chnl_info->offset;
	}

	return port_info;

err:
	cpp_dmabuf_cleanup(cpp_dev, port_info);
	return NULL;
}

void cpp_dmabuf_cleanup(struct cpp_device *cpp_dev, struct cpp_dma_port_info *port_info)
{
	struct cpp_dma_chnl_info *chnl_info;
	unsigned int chnl;

	if (IS_ERR_OR_NULL(port_info))
		return;

	for (chnl = 0; chnl < MAX_DMA_CHNLS; ++chnl) {
		chnl_info = &port_info->dma_chnls[chnl];
		if (!chnl_info->dbuf_mapped)
			continue;
		cpp_iommu_unmap_dmabuf(cpp_dev->mmu_dev, chnl_info->fd);
		chnl_info->dbuf_mapped = 0;
	}

	kfree(port_info);
}

void cpp_dmabuf_debug_dump(struct cpp_dma_port_info *port_info)
{
	unsigned int chnl;

	for (chnl = 0; chnl < MAX_DMA_CHNLS; ++chnl) {
		if (port_info->dma_chnls[chnl].dbuf_mapped == 0)
			continue;
		pr_info
		    ("P%d-CHNL%d: fd=%d, phy_addr=0x%llx, offset=0x%x, length=0x%x, chnl_id=0x%x, tid=0x%x\n",
		     port_info->port_id, chnl, port_info->dma_chnls[chnl].fd,
		     port_info->dma_chnls[chnl].phy_addr,
		     port_info->dma_chnls[chnl].offset,
		     port_info->dma_chnls[chnl].length,
		     port_info->dma_chnls[chnl].chnl_id,
		     port_info->dma_chnls[chnl].tid);
	}
}
