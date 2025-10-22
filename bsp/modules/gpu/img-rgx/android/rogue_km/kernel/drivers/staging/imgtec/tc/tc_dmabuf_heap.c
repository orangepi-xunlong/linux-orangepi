/*
 * @Codingstyle LinuxKernel
 * @Copyright   Copyright (c) Imagination Technologies Ltd. All Rights Reserved
 * @License     Dual MIT/GPLv2
 *
 * The contents of this file are subject to the MIT license as set out below.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Alternatively, the contents of this file may be used under the terms of
 * the GNU General Public License Version 2 ("GPL") in which case the provisions
 * of GPL are applicable instead of those above.
 *
 * If you wish to allow use of your version of this file only under the terms of
 * GPL, and not to allow others to use your version of this file under the terms
 * of the MIT license, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by GPL as set
 * out in the file called "GPL-COPYING" included in this distribution. If you do
 * not delete the provisions above, a recipient may use your version of this file
 * under the terms of either the MIT license or GPL.
 *
 * This License is also included in this distribution in the file called
 * "MIT-COPYING".
 *
 * EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
 * PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <linux/version.h>

#include "tc_drv_internal.h"

#include "dma_lma_heap.h"
#include "tc_dmabuf_heap.h"

int tc_dmabuf_heap_init(struct tc_device *tc, int mem_bar)
{
	struct tc_dma_heap_info dma_heap_data[TC_DMA_HEAP_COUNT] = {
		{
			.name          = "tc-pdp",
			.base          = tc->pdp_heap_mem_base,
			.size          = tc->pdp_heap_mem_size,
			.allow_cpu_map = true,
			.uncached      = true,
			.priv          = (void *)tc->tc_mem.base, /* offset */
		},
		{
			.name          = "tc-rogue",
			.base          = tc->ext_heap_mem_base,
			.size          = tc->ext_heap_mem_size,
			.allow_cpu_map = true,
			.uncached      = true,
		},
#if defined(SUPPORT_FAKE_SECURE_DMA_HEAP)
		{
			.name          = "tc-secure",
			.base          = tc->secure_heap_mem_base,
			.size          = tc->secure_heap_mem_size,
			.allow_cpu_map = false,
			.uncached      = true,
			.priv          = (void *)tc->tc_mem.base, /* offset */
		},
#endif /* defined(SUPPORT_FAKE_SECURE_DMA_HEAP) */
	};
	int i, err;

	err = request_pci_io_addr(tc->pdev, mem_bar, 0,
		tc->tc_mem.size);
	if (err) {
		dev_err(&tc->pdev->dev,
			"Failed to request tc memory (%d)\n", err);
		goto err_out;
	}

	for (i = 0; i < ARRAY_SIZE(dma_heap_data); i++) {
		tc->dma_heaps[i] = dma_lma_heap_create(&dma_heap_data[i]);
		if (IS_ERR_OR_NULL(tc->dma_heaps[i])) {
			err = PTR_ERR(tc->dma_heaps[i]);
			tc->dma_heaps[i] = NULL;
			goto err_destroy_heaps;
		}
	}

	return 0;

err_destroy_heaps:
	for (i = 0; i < ARRAY_SIZE(dma_heap_data); i++) {
		if (!tc->dma_heaps[i])
			break;
		dma_lma_heap_destroy(tc->dma_heaps[i]);
	}
	release_pci_io_addr(tc->pdev, mem_bar,
		tc->tc_mem.base, tc->tc_mem.size);
err_out:
	return err;
}

void tc_dmabuf_heap_deinit(struct tc_device *tc, int mem_bar)
{
	int i;

	for (i = 0; i < TC_DMA_HEAP_COUNT; i++)
		dma_lma_heap_destroy(tc->dma_heaps[i]);

	release_pci_io_addr(tc->pdev, mem_bar,
		tc->tc_mem.base, tc->tc_mem.size);
}
