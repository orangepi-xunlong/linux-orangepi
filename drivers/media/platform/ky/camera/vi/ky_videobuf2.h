// SPDX-License-Identifier: GPL-2.0
/*
 * ky_videobuf2.h - definition of ky video buffer operations
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#ifndef _KY_VIDEOBUF2_H_
#define _KY_VIDEOBUF2_H_
#include <media/videobuf2-core.h>
#include <media/videobuf2-dma-contig.h>
#include <media/videobuf2-dma-sg.h>
#include "vdev.h"

//#define spm_vb2_buf_paddr					spm_vb2_usrptr_paddr
//#define spm_vb2_mem_ops						spm_vb2_get_usrptr_mem_ops()
//#define spm_vb2_destroy_alloc_ctx(ctx)
//#define KY_VB2_IO_MODE						VB2_USERPTR

#ifndef CONFIG_KY_X1_VI_IOMMU
static inline dma_addr_t spm_vb2_buf_paddr(struct vb2_buffer *vb, unsigned int plane_no)
{
	unsigned int offset = 0;
	dma_addr_t paddr = 0;
	struct spm_camera_vbuffer *sc_vb = vb2_buffer_to_spm_camera_vbuffer(vb);
	struct spm_camera_vnode *sc_vnode = sc_vb->sc_vnode;
	dma_addr_t *dma_addr = (dma_addr_t *) vb2_plane_cookie(vb, plane_no);

	BUG_ON(!sc_vnode);
	offset = sc_vnode->planes_offset[vb->index][plane_no];
	paddr = *dma_addr + offset;
	return paddr;
}

#define spm_vb2_mem_ops						(&vb2_dma_contig_memops)
#else
#define spm_vb2_mem_ops						(&vb2_dma_sg_memops)
#endif
#define spm_vb2_destroy_alloc_ctx(ctx)
#define KY_VB2_IO_MODE					VB2_DMABUF
#endif
