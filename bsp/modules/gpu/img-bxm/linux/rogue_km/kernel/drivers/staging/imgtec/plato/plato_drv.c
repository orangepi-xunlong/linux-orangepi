/*
 * @File        plato_drv.c
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

/*
 * This is a device driver for the plato PCI card. It creates
 * platform devices for the pdp, hdmi, and ext sub-devices, and
 * exports functions to manage the shared interrupt handling
 */

#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include "sysinfo.h"
#include "plato_drv.h"
#include "plato_aon_regs.h"
#include "plato_top_regs.h"
#include "plato_ddr_ctrl_regs.h"
#include "plato_ddr_publ_regs.h"

#include "pvrmodule.h"

#if defined(PLATO_MULTI_DEVICE)
#include <linux/idr.h>
#endif

#include "kernel_compatibility.h"

MODULE_DESCRIPTION("Plato PCI driver");

/* Clock speed module parameters */
static unsigned int mem_clock_speed  = PLATO_MEM_CLOCK_SPEED;
module_param(mem_clock_speed, uint, 0444);
MODULE_PARM_DESC(mem_clock_speed,
				"Plato memory clock speed in Hz (600000000 - 800000000)");

static unsigned int core_clock_speed = PLATO_RGX_CORE_CLOCK_SPEED;
module_param(core_clock_speed, uint, 0444);
MODULE_PARM_DESC(core_clock_speed,
				"Plato core clock speed in Hz (396000000 - 742500000)");

static int plato_init(struct pci_dev *pdev, const struct pci_device_id *id);
static void plato_exit(struct pci_dev *pdev);

struct pci_device_id plato_pci_tbl[] = {
	{ PCI_VDEVICE(PLATO, PCI_DEVICE_ID_PLATO) },
	{ },
};

static struct pci_driver plato_pci_driver = {
	.name		= PLATO_SYSTEM_NAME,
	.id_table	= plato_pci_tbl,
	.probe		= plato_init,
	.remove		= plato_exit,
};

module_pci_driver(plato_pci_driver);
MODULE_DEVICE_TABLE(pci, plato_pci_tbl);

static struct plato_debug_register plato_noc_regs[] = {
	{"NOC Offset 0x00", 0x00, 0},
	{"NOC Offset 0x04", 0x04, 0},
	{"NOC Offset 0x08", 0x08, 0},
	{"NOC Offset 0x0C", 0x0C, 0},
	{"NOC Offset 0x10", 0x10, 0},
	{"NOC Offset 0x14", 0x14, 0},
	{"NOC Offset 0x18", 0x18, 0},
	{"NOC Offset 0x1C", 0x1C, 0},
	{"NOC Offset 0x50", 0x50, 0},
	{"NOC Offset 0x54", 0x54, 0},
	{"NOC Offset 0x58", 0x58, 0},
	{"DDR A Ctrl", SYS_PLATO_REG_NOC_DBG_DDR_A_CTRL_OFFSET, 0},
	{"DDR A Data", SYS_PLATO_REG_NOC_DBG_DDR_A_DATA_OFFSET, 0},
	{"DDR A Publ", SYS_PLATO_REG_NOC_DBG_DDR_A_PUBL_OFFSET, 0},
	{"DDR B Ctrl", SYS_PLATO_REG_NOC_DBG_DDR_B_CTRL_OFFSET, 0},
	{"DDR B Data", SYS_PLATO_REG_NOC_DBG_DDR_B_DATA_OFFSET, 0},
	{"DDR B Publ", SYS_PLATO_REG_NOC_DBG_DDR_B_PUBL_OFFSET, 0},
	{"Display S", SYS_PLATO_REG_NOC_DBG_DISPLAY_S_OFFSET, 0},
	{"GPIO 0 S", SYS_PLATO_REG_NOC_DBG_GPIO_0_S_OFFSET, 0},
	{"GPIO 1 S", SYS_PLATO_REG_NOC_DBG_GPIO_1_S_OFFSET, 0},
	{"GPU S", SYS_PLATO_REG_NOC_DBG_GPU_S_OFFSET, 0},
	{"PCI PHY", SYS_PLATO_REG_NOC_DBG_PCI_PHY_OFFSET, 0},
	{"PCI Reg", SYS_PLATO_REG_NOC_DBG_PCI_REG_OFFSET, 0},
	{"PCI S", SYS_PLATO_REG_NOC_DBG_PCI_S_OFFSET, 0},
	{"Periph S", SYS_PLATO_REG_NOC_DBG_PERIPH_S_OFFSET, 0},
	{"Ret Reg", SYS_PLATO_REG_NOC_DBG_RET_REG_OFFSET, 0},
	{"Service", SYS_PLATO_REG_NOC_DBG_SERVICE_OFFSET, 0},
};

static struct plato_debug_register plato_aon_regs[] = {
	{"AON Offset 0x0000", 0x0000, 0},
	{"AON Offset 0x0070", 0x0070, 0},
};

#if defined(PLATO_MULTI_DEVICE)
DEFINE_IDA(plato_ida);
#endif

static int poll_pr(struct device *dev, void *base, u32 reg, u32 val,
					u32 msk, u32 cnt, u32 intrvl)
{
	u32 polnum;

	for (polnum = 0; polnum < cnt; polnum++) {
		if ((plato_read_reg32(base, reg) & msk) == val)
			break;
		plato_sleep_ms(intrvl);
	}
	if (polnum == cnt) {
		dev_info(dev,
			"Poll failed for register: 0x%08X. Expected 0x%08X Received 0x%08X",
			(unsigned int)reg, val,
			plato_read_reg32(base, reg) & msk);
		return -ETIME;
	}

	return 0;
}

#define poll(dev, base, reg, val, msk) poll_pr(dev, base, reg, val, msk, 10, 10)

int request_pci_io_addr(struct pci_dev *pdev, u32 index,
	resource_size_t offset, resource_size_t length)
{
	resource_size_t start, end;

	start = pci_resource_start(pdev, index);
	end = pci_resource_end(pdev, index);

	if ((start + offset + length - 1) > end)
		return -EIO;
	if (pci_resource_flags(pdev, index) & IORESOURCE_IO) {
		if (request_region(start + offset, length, PVRSRV_MODNAME)
			== NULL)
			return -EIO;
	} else {
		if (request_mem_region(start + offset, length, PVRSRV_MODNAME)
			== NULL)
			return -EIO;
	}
	return PLATO_INIT_SUCCESS;
}

void release_pci_io_addr(struct pci_dev *pdev, u32 index,
	resource_size_t start, resource_size_t length)
{
	if (pci_resource_flags(pdev, index) & IORESOURCE_IO)
		release_region(start, length);
	else
		release_mem_region(start, length);
}

static void plato_devres_release(struct device *dev, void *res)
{
#if defined(PLATO_MULTI_DEVICE)
	struct plato_device *plato = res;

	if (plato->instance >= 0)
		ida_free(&plato_ida, plato->instance);
#endif
}

static irqreturn_t plato_irq_handler(int irq, void *data)
{
	u32 interrupt_status;
	unsigned long flags;
	irqreturn_t ret = IRQ_NONE;
	struct plato_device *plato = (struct plato_device *)data;
#if !defined(VIRTUAL_PLATFORM)
	void *perip_regs = plato->sys_io.registers + SYS_PLATO_REG_PERIP_OFFSET;
#endif

	spin_lock_irqsave(&plato->interrupt_handler_lock, flags);

#if defined(VIRTUAL_PLATFORM)
	/* On virtual platform all interrupt handlers need to be called */
	interrupt_status =
		(0x1 << PLATO_INT_SHIFT_GPU) | (0x1 << PLATO_INT_SHIFT_PDP);
#else
	interrupt_status =
		plato_read_reg32(perip_regs, PLATO_TOP_CR_INT_STATUS);
#endif

	if (interrupt_status & (1 << PLATO_INT_SHIFT_GPU)) {
		struct plato_interrupt_handler *rogue_int =
			&plato->interrupt_handlers[PLATO_INTERRUPT_GPU];

		if (rogue_int->enabled && rogue_int->handler_function)
			rogue_int->handler_function(rogue_int->handler_data);
		ret = IRQ_HANDLED;
	}
	if (interrupt_status & (1 << PLATO_INT_SHIFT_PDP)) {
		struct plato_interrupt_handler *pdp_int =
			&plato->interrupt_handlers[PLATO_INTERRUPT_PDP];

		if (pdp_int->enabled && pdp_int->handler_function)
			pdp_int->handler_function(pdp_int->handler_data);
		ret = IRQ_HANDLED;
	}
	if (interrupt_status & (1 << PLATO_INT_SHIFT_HDMI)) {
		struct plato_interrupt_handler *hdmi_int =
			&plato->interrupt_handlers[PLATO_INTERRUPT_HDMI];

		if (hdmi_int->enabled && hdmi_int->handler_function)
			hdmi_int->handler_function(hdmi_int->handler_data);
		ret = IRQ_HANDLED;
	}

	spin_unlock_irqrestore(&plato->interrupt_handler_lock, flags);

	return ret;
}

static u32 hash_noc_addr(u32 addr)
{
	u32 hash_bits = (addr & 0x4000 ? 1 : 0);

	if (addr & 0x2000)
		++hash_bits;
	if (addr & 0x1000)
		++hash_bits;
	if (addr & 0x0800)
		++hash_bits;
	if ((hash_bits == 1) || (hash_bits == 3))
		addr ^= 0x100;

	return addr;
}

int plato_debug_info(struct device *dev,
		     struct plato_debug_register *noc_dbg_regs,
		     struct plato_debug_register *aon_dbg_regs)
{
	int offset = 0;
	int i = 0;
	void *noc_regs;
	void *aon_regs;
	struct plato_device *plato = devres_find(dev, plato_devres_release,
		NULL, NULL);

	if (!plato) {
		dev_err(dev, "No plato device resources found\n");
		return -ENODEV;
	}

	noc_regs = plato->sys_io.registers + SYS_PLATO_REG_NOC_OFFSET;
	aon_regs = plato->aon_regs.registers;

	/* First set of NOC regs don't need to be hashed */
	for (i = noc_dbg_regs[0].offset;
		i < SYS_PLATO_REG_NOC_DBG_DDR_A_CTRL_OFFSET;
		i = noc_dbg_regs[offset].offset) {
		noc_dbg_regs[offset++].value = plato_read_reg32(noc_regs, i);
	}

	/* NOC regs that need to be hashed */
	for (i = offset;
		i < ARRAY_SIZE(plato_noc_regs);
		i++) {
		u32 hashed_offset = hash_noc_addr(noc_dbg_regs[i].offset + 0xC);

		noc_dbg_regs[offset++].value =
			plato_read_reg32(noc_regs, hashed_offset);
	}

	/* Fill in AON regs */
	for (i = 0;
		i < ARRAY_SIZE(plato_aon_regs);
		i++) {
		aon_dbg_regs[i].value =
			plato_read_reg32(aon_regs, aon_dbg_regs[i].offset);
	}

	return 0;
}
EXPORT_SYMBOL(plato_debug_info);

static int plato_enable_irq(struct plato_device *plato)
{
	int err = PLATO_INIT_SUCCESS;

#if !defined(VIRTUAL_PLATFORM)
	err = request_irq(plato->pdev->irq, plato_irq_handler,
		IRQF_SHARED, PLATO_SYSTEM_NAME, plato);
#endif

	return err;
}

static void plato_disable_irq(struct plato_device *plato)
{
	free_irq(plato->pdev->irq, plato);
}

static int register_rogue_device(struct plato_device *plato)
{
	int err = 0;
	struct resource rogue_resources[] = {
		DEFINE_RES_MEM_NAMED(pci_resource_start(plato->pdev,
			SYS_PLATO_REG_PCI_BASENUM) + SYS_PLATO_REG_RGX_OFFSET,
			SYS_PLATO_REG_RGX_SIZE, PLATO_ROGUE_RESOURCE_REGS),
	};

	struct plato_rogue_platform_data pdata = {
		.plato_memory_base = plato->rogue_mem.base,
		.has_nonmappable = plato->has_nonmappable,
		.rogue_heap_dev_addr = plato->dev_mem_base,
		.rogue_heap_mappable = plato->rogue_heap_mappable,
		.rogue_heap_nonmappable = plato->rogue_heap_nonmappable,
	#if defined(SUPPORT_PLATO_DISPLAY)
		.pdp_heap = plato->pdp_heap,
	#endif
	};
	char rogue_device_name[PLATO_MAX_DEVICE_NAME_LEN];
	struct platform_device_info rogue_device_info = {
		.parent = &plato->pdev->dev,
		.name = rogue_device_name,
		.id = plato->instance,
		.res = rogue_resources,
		.num_res = ARRAY_SIZE(rogue_resources),
		.data = &pdata,
		.size_data = sizeof(pdata),
		.dma_mask = DMA_BIT_MASK(40),
	};

	err = snprintf(rogue_device_name, sizeof(rogue_device_name),
		       PLATO_DEVICE_NAME_ROGUE_PRINTF_ARGS(plato->instance));
	if (WARN_ON(err >= sizeof(rogue_device_name)))
		return -ENOSPC;

	plato->rogue_dev = platform_device_register_full(&rogue_device_info);

	if (IS_ERR(plato->rogue_dev)) {
		err = PTR_ERR(plato->rogue_dev);
		dev_err(&plato->pdev->dev,
			"Failed to register rogue device (%d)\n", err);
		plato->rogue_dev = NULL;
	}
	return err;
}


#if defined(SUPPORT_PLATO_DISPLAY)

static int register_pdp_device(struct plato_device *plato)
{
	int err = 0;

	struct resource pdp_resources[] = {
		DEFINE_RES_MEM_NAMED(pci_resource_start(plato->pdev,
			SYS_PLATO_REG_PCI_BASENUM) + SYS_PLATO_REG_PDP_OFFSET,
			SYS_PLATO_REG_PDP_SIZE, PLATO_PDP_RESOURCE_REGS),
		DEFINE_RES_MEM_NAMED(pci_resource_start(plato->pdev,
			SYS_PLATO_REG_PCI_BASENUM) + SYS_PLATO_REG_PDP_BIF_OFFSET,
			SYS_PLATO_REG_PDP_BIF_SIZE, PLATO_PDP_RESOURCE_BIF_REGS),
	};
	struct plato_pdp_platform_data pdata = {
		// .memory_base is translate factor from CPU (PCI) address to equivalent
		// GPU address. GEM PDP memory is allocated in GPU space as dev_addr.
		// The cpu_addr = dev_addr - memory_base
		.memory_base = plato->rogue_mem.base - plato->dev_mem_base,
		.pdp_heap_memory_base = plato->pdp_heap.base,
		.pdp_heap_memory_size = plato->pdp_heap.size,
	};
	char pdp_device_name[PLATO_MAX_DEVICE_NAME_LEN];
	struct platform_device_info pdp_device_info = {
		.parent = &plato->pdev->dev,
		.name = pdp_device_name,
		.id = plato->instance,
		.data = &pdata,
		.size_data = sizeof(pdata),
		.dma_mask = DMA_BIT_MASK(32),
	};

	err = snprintf(pdp_device_name, sizeof(pdp_device_name),
		       PLATO_DEVICE_NAME_PDP_PRINTF_ARGS(plato->instance));
	if (WARN_ON(err >= sizeof(pdp_device_name)))
		return -ENOSPC;

	pdp_device_info.res = pdp_resources;
	pdp_device_info.num_res = ARRAY_SIZE(pdp_resources);

	plato->pdp_dev = platform_device_register_full(&pdp_device_info);
	if (IS_ERR(plato->pdp_dev)) {
		err = PTR_ERR(plato->pdp_dev);
		dev_err(&plato->pdev->dev,
			"Failed to register PDP device (%d)\n", err);
		plato->pdp_dev = NULL;
		goto err;
	}
err:
	return err;
}

static int register_hdmi_device(struct plato_device *plato)
{
	int err = 0;

	struct resource hdmi_resources[] = {
		DEFINE_RES_MEM_NAMED(pci_resource_start(plato->pdev,
			SYS_PLATO_REG_PCI_BASENUM) + SYS_PLATO_REG_HDMI_OFFSET,
			SYS_PLATO_REG_HDMI_SIZE, PLATO_HDMI_RESOURCE_REGS),
	};
	struct plato_hdmi_platform_data pdata = {
		.plato_memory_base = plato->rogue_mem.base,
	};
	struct platform_device_info hdmi_device_info = {
		.parent = &plato->pdev->dev,
		.name = PLATO_DEVICE_NAME_HDMI,
		.id = plato->instance,
		.data = &pdata,
		.size_data = sizeof(pdata),
		.dma_mask = DMA_BIT_MASK(32),
	};

	hdmi_device_info.res = hdmi_resources;
	hdmi_device_info.num_res = ARRAY_SIZE(hdmi_resources);

	plato->hdmi_dev = platform_device_register_full(&hdmi_device_info);
	if (IS_ERR(plato->hdmi_dev)) {
		err = PTR_ERR(plato->hdmi_dev);
		dev_err(&plato->pdev->dev,
			"Failed to register HDMI device (%d)\n", err);
		plato->hdmi_dev = NULL;
		goto err;
	}
err:
	return err;
}

#endif

unsigned int plato_mem_clock_speed(struct device *dev)
{
	unsigned int ret = mem_clock_speed;

	(mem_clock_speed > PLATO_MAX_MEM_CLOCK_SPEED) ?
		ret = PLATO_MAX_MEM_CLOCK_SPEED : 0;
	(mem_clock_speed < PLATO_MIN_MEM_CLOCK_SPEED) ?
		ret = PLATO_MIN_MEM_CLOCK_SPEED : 0;

	return ret;
}
EXPORT_SYMBOL(plato_mem_clock_speed);

unsigned int plato_core_clock_speed(struct device *dev)
{
	unsigned int ret = core_clock_speed;

	(core_clock_speed > PLATO_RGX_MAX_CORE_CLOCK_SPEED) ?
		ret = PLATO_RGX_MAX_CORE_CLOCK_SPEED : 0;
	(core_clock_speed < PLATO_RGX_MIN_CORE_CLOCK_SPEED) ?
		ret = PLATO_RGX_MIN_CORE_CLOCK_SPEED : 0;

	return ret;
}
EXPORT_SYMBOL(plato_core_clock_speed);

unsigned int plato_pll_clock_speed(struct device *dev,
	unsigned int clock_speed)
{
	/*
	 * Force the lowest possible PLL clock in case when the requested
	 * clock speed is higher than the largest value known by the function.
	 */
	unsigned int pll_clock_speed = clock_speed;
	unsigned int acc_clock_speed = clock_speed;

	/* Tweak the values if the requested clock speed is a supported one */
	(clock_speed <= 742500000) ?
		(pll_clock_speed = 742500000,  acc_clock_speed = 742500000) : 0;
	(clock_speed <= 668250000) ?
		(pll_clock_speed = 1336500000, acc_clock_speed = 668250000) : 0;
	(clock_speed <= 594000000) ?
		(pll_clock_speed = 1188000000, acc_clock_speed = 594000000) : 0;
	(clock_speed <= 544500000) ?
		(pll_clock_speed = 1633500000, acc_clock_speed = 544500000) : 0;
	(clock_speed <= 519750000) ?
		(pll_clock_speed = 1039500000, acc_clock_speed = 519750000) : 0;
	(clock_speed <= 495000000) ?
		(pll_clock_speed = 1485000000, acc_clock_speed = 495000000) : 0;
	(clock_speed <= 445500000) ?
		(pll_clock_speed = 891000000,  acc_clock_speed = 445500000) : 0;
	(clock_speed <= 408375000) ?
		(pll_clock_speed = 1633500000, acc_clock_speed = 408375000) : 0;
	(clock_speed <= 396000000) ?
		(pll_clock_speed = 1188000000, acc_clock_speed = 396000000) : 0;

	/*
	 * Do fine grained adjustment if the requested clock speed
	 * is different than expected.
	 */
	return ((((unsigned long long int)pll_clock_speed << 32) /
		acc_clock_speed) >> 32) * clock_speed;
}
EXPORT_SYMBOL(plato_pll_clock_speed);

static int setup_io_region(struct pci_dev *pdev,
	struct plato_io_region *region, u32 index,
	resource_size_t offset,	resource_size_t size)
{
	int err;
	resource_size_t pci_phys_addr;

	err = request_pci_io_addr(pdev, index, offset, size);
	if (err != PLATO_INIT_SUCCESS) {
		dev_err(&pdev->dev,
			"Failed to request plato registers (err=%d)\n", err);
		return -EIO;
	}
	pci_phys_addr = pci_resource_start(pdev, index);
	region->region.base = pci_phys_addr + offset;
	region->region.size = size;

	region->registers = ioremap(region->region.base, region->region.size);

	if (!region->registers) {
		dev_err(&pdev->dev, "Failed to map plato registers\n");
		release_pci_io_addr(pdev, index,
			region->region.base, region->region.size);
		return -EIO;
	}
	return 0;
}

static int plato_dev_init(struct plato_device *plato, struct pci_dev *pdev)
{
	int err;

	plato->pdev = pdev;

	spin_lock_init(&plato->interrupt_handler_lock);
	spin_lock_init(&plato->interrupt_enable_lock);

	/* Reserve and map the plato sys registers up to the rogue regs */
	err = setup_io_region(pdev, &plato->sys_io,
		SYS_PLATO_REG_PCI_BASENUM,
		SYS_PLATO_REG_SYS_OFFSET, SYS_PLATO_REG_RGX_OFFSET);
	if (err)
		goto err_out;

	plato_dev_info(&pdev->dev,
		"Initialized system register region at base 0x%llx and size 0x%llx",
		(u64)plato->sys_io.region.base, (u64)plato->sys_io.region.size);

	/* Now map AON regs which come after rogue regs */
	err = setup_io_region(pdev, &plato->aon_regs,
		SYS_PLATO_REG_PCI_BASENUM,
		SYS_PLATO_REG_AON_OFFSET, SYS_PLATO_REG_AON_SIZE);
	if (err)
		goto err_unmap_sys;

	plato_dev_info(&pdev->dev,
		"Initialized AON register region at base 0x%llx and size 0x%llx",
		(u64)plato->aon_regs.region.base,
		(u64)plato->aon_regs.region.size);

	err = plato_memory_init(plato);
	if (err) {
		plato_dev_info(&plato->pdev->dev,
			"plato_memory_init failed (%d)", err);
		goto err_unmap_aon;
	}

err_out:
	return err;
err_unmap_aon:
	iounmap(plato->aon_regs.registers);
	release_pci_io_addr(pdev, SYS_PLATO_REG_PCI_BASENUM,
		plato->aon_regs.region.base, plato->aon_regs.region.size);
err_unmap_sys:
	iounmap(plato->sys_io.registers);
	release_pci_io_addr(pdev, SYS_PLATO_REG_PCI_BASENUM,
		plato->sys_io.region.base, plato->sys_io.region.size);
	goto err_out;
}

static void plato_dev_cleanup(struct plato_device *plato)
{
	plato_memory_deinit(plato);

	iounmap(plato->aon_regs.registers);
	release_pci_io_addr(plato->pdev, SYS_PLATO_REG_PCI_BASENUM,
		plato->aon_regs.region.base, plato->aon_regs.region.size);

	iounmap(plato->sys_io.registers);
	release_pci_io_addr(plato->pdev, SYS_PLATO_REG_PCI_BASENUM,
		plato->sys_io.region.base, plato->sys_io.region.size);
}


static int plato_init(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct plato_device *plato;
	int err = 0;

	if (!devres_open_group(&pdev->dev, NULL, GFP_KERNEL))
		return -ENOMEM;

	plato = devres_alloc(plato_devres_release,
		sizeof(*plato), GFP_KERNEL);
	if (!plato) {
		err = -ENOMEM;
		goto err_out;
	}

#if defined(PLATO_MULTI_DEVICE)
	plato->instance = ida_alloc_max(&plato_ida, PLATO_MAX_CARDS - 1,
					GFP_KERNEL);
	if (WARN_ON(plato->instance < 0)) {
		err = plato->instance;
		goto err_out;
	}
#else
	/*
	 * If IDA isn't available, fallback to automatically allocated
	 * instance IDs. Multi-GPU support may be restricted.
	 */
	plato->instance = -2;

	dev_warn_once(&pdev->dev,
		      "Old kernel version, multi-GPU support may be restricted\n");
#endif
	devres_add(&pdev->dev, plato);

	err = pci_enable_device(pdev);
	if (err) {
		dev_err(&pdev->dev,
			"error - pci_enable_device returned %d\n", err);
		goto err_out;
	}

	/* Check it's really a plato */
	if (pdev->vendor == PCI_VENDOR_ID_PLATO &&
		pdev->device == PCI_DEVICE_ID_PLATO) {

		err = plato_dev_init(plato, pdev);
		if (err)
			goto err_disable_device;

		err = plato_cfg_init(plato);
		if (err)
			goto err_dev_cleanup;

		err = plato_enable_irq(plato);
		if (err) {
			dev_err(&pdev->dev, "Failed to initialise IRQ\n");
			goto err_dev_cleanup;
		}
	} else {
		dev_err(&pdev->dev, "WARNING: vendor/device ID is not correct");
		goto err_disable_device;
	}

	/* Register rogue device */
	register_rogue_device(plato);

	/* Register display devices */
#if defined(SUPPORT_PLATO_DISPLAY)
	register_pdp_device(plato);
	register_hdmi_device(plato);
#endif

	devres_remove_group(&pdev->dev, NULL);

	goto plato_init_return;

err_dev_cleanup:
	plato_dev_cleanup(plato);
err_disable_device:
	pci_disable_device(pdev);
err_out:
	devres_release_group(&pdev->dev, NULL);

plato_init_return:
	return err;
}

static void plato_exit(struct pci_dev *pdev)
{
	int i;
	struct plato_device *plato;

	plato_dev_info(&pdev->dev, "%s entry\n", __func__);

	plato = devres_find(&pdev->dev, plato_devres_release, NULL, NULL);

	if (!plato)
		return;

	if (plato->rogue_dev) {
		plato_dev_info(&pdev->dev,
				"%s: platform_device_unregister rogue_dev\n",
				__func__);
		platform_device_unregister(plato->rogue_dev);
	}

#if defined(SUPPORT_PLATO_DISPLAY)
	if (plato->pdp_dev) {
		plato_dev_info(&pdev->dev, "platform_device_unregister pdp_dev\n");
		platform_device_unregister(plato->pdp_dev);
	}

	if (plato->hdmi_dev) {
		plato_dev_info(&pdev->dev, "platform_device_unregister hdmi_dev\n");
		platform_device_unregister(plato->hdmi_dev);
	}
#endif

	for (i = 0; i < PLATO_INTERRUPT_MAX; i++)
		plato_disable_interrupt(&pdev->dev, i);

	plato_disable_irq(plato);
	plato_dev_cleanup(plato);

	plato_dev_info(&pdev->dev, "pci_disable_device\n");

	pci_disable_device(pdev);

	plato_dev_info(&pdev->dev, "%s exit\n");
}

static u32 plato_interrupt_id_to_flag(enum PLATO_INTERRUPT interrupt_id)
{
	switch (interrupt_id) {
	case PLATO_INTERRUPT_GPU:
		return (1 << PLATO_INT_SHIFT_GPU);
	case PLATO_INTERRUPT_PDP:
		return (1 << PLATO_INT_SHIFT_PDP);
	case PLATO_INTERRUPT_HDMI:
		return (1 << PLATO_INT_SHIFT_HDMI);
	default:
		BUG();
	}
}

int plato_enable(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	int err;

	err = pci_enable_device(pdev);
	if (!err)
		pci_set_master(pdev);

	return err;
}
EXPORT_SYMBOL(plato_enable);

void plato_disable(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);

	pci_disable_device(pdev);
}
EXPORT_SYMBOL(plato_disable);

int plato_set_interrupt_handler(struct device *dev,
	enum PLATO_INTERRUPT interrupt_id,
	void (*handler_function)(void *),
	void *data)
{
	int err = 0;
	unsigned long flags;
	struct plato_device *plato = devres_find(dev, plato_devres_release,
		NULL, NULL);

	if (!plato) {
		dev_err(dev, "No plato device resources found\n");
		err = -ENODEV;
		goto err_out;
	}

	if (interrupt_id < 0 || interrupt_id >= PLATO_INTERRUPT_MAX) {
		dev_err(dev, "Invalid interrupt ID (%d)\n", interrupt_id);
		err = -EINVAL;
		goto err_out;
	}

	spin_lock_irqsave(&plato->interrupt_handler_lock, flags);

	plato->interrupt_handlers[interrupt_id].handler_function =
		handler_function;
	plato->interrupt_handlers[interrupt_id].handler_data = data;

	spin_unlock_irqrestore(&plato->interrupt_handler_lock, flags);

err_out:
	return err;
}
EXPORT_SYMBOL(plato_set_interrupt_handler);

int plato_enable_interrupt(struct device *dev, enum PLATO_INTERRUPT interrupt_id)
{
	int err = 0;
	unsigned long flags;
	void *perip_regs;
	int value;
	struct plato_device *plato = devres_find(dev, plato_devres_release,
		NULL, NULL);

	if (!plato) {
		dev_err(dev, "No plato device resources found\n");
		err = -ENODEV;
		goto err_out;
	}
	if (interrupt_id < 0 || interrupt_id >= PLATO_INTERRUPT_MAX) {
		dev_err(dev, "Invalid interrupt ID (%d)\n", interrupt_id);
		err = -EINVAL;
		goto err_out;
	}
	spin_lock_irqsave(&plato->interrupt_enable_lock, flags);

	if (plato->interrupt_handlers[interrupt_id].enabled) {
		plato_dev_warn(dev, "Interrupt ID %d already enabled\n",
			interrupt_id);
		err = -EEXIST;
		goto err_unlock;
	}
	plato->interrupt_handlers[interrupt_id].enabled = true;

#if !defined(VIRTUAL_PLATFORM)
	perip_regs = plato->sys_io.registers + SYS_PLATO_REG_PERIP_OFFSET;

	value = plato_read_reg32(perip_regs, PLATO_TOP_CR_PCI_INT_MASK);
	value |= plato_interrupt_id_to_flag(interrupt_id);
	plato_write_reg32(perip_regs, PLATO_TOP_CR_PCI_INT_MASK, value);

	(void) plato_read_reg32(perip_regs, PLATO_TOP_CR_PCI_INT_MASK);
#endif

err_unlock:
	spin_unlock_irqrestore(&plato->interrupt_enable_lock, flags);
err_out:
	return err;
}
EXPORT_SYMBOL(plato_enable_interrupt);

int plato_disable_interrupt(struct device *dev, enum PLATO_INTERRUPT interrupt_id)
{
	int err = 0;
	unsigned long flags;
	void *perip_regs;
	int value;
	struct plato_device *plato = devres_find(dev, plato_devres_release,
		NULL, NULL);

	if (!plato) {
		dev_err(dev, "No plato device resources found\n");
		err = -ENODEV;
		goto err_out;
	}
	if (interrupt_id < 0 || interrupt_id >= PLATO_INTERRUPT_MAX) {
		dev_err(dev, "Invalid interrupt ID (%d)\n", interrupt_id);
		err = -EINVAL;
		goto err_out;
	}
	spin_lock_irqsave(&plato->interrupt_enable_lock, flags);

	if (!plato->interrupt_handlers[interrupt_id].enabled) {
		plato_dev_warn(dev, "Interrupt ID %d already disabled\n",
			interrupt_id);
	}
	plato->interrupt_handlers[interrupt_id].enabled = false;

#if !defined(VIRTUAL_PLATFORM)
	perip_regs = plato->sys_io.registers + SYS_PLATO_REG_PERIP_OFFSET;

	value = plato_read_reg32(perip_regs, PLATO_TOP_CR_PCI_INT_MASK);
	value &= ~(plato_interrupt_id_to_flag(interrupt_id));
	plato_write_reg32(perip_regs, PLATO_TOP_CR_PCI_INT_MASK, value);

	(void) plato_read_reg32(perip_regs, PLATO_TOP_CR_PCI_INT_MASK);
#endif

	spin_unlock_irqrestore(&plato->interrupt_enable_lock, flags);
err_out:
	return err;
}
EXPORT_SYMBOL(plato_disable_interrupt);

void plato_enable_pdp_clock(struct device *dev)
{
	void *perip_regs;
	struct plato_device *plato = devres_find(dev, plato_devres_release,
		NULL, NULL);

	perip_regs = plato->sys_io.registers + SYS_PLATO_REG_PERIP_OFFSET;

	/* Enabling PDP gated clock output - 198 MHz
	 * 0x1210 sets the dividers to (1+1)*(2+1) = 6,
	 * and GPU_PLL defaults to 1188MHz
	 */
	plato_write_reg32(perip_regs, PLATO_TOP_CR_PDP_CLK_CTRL, 0x00001210);
	poll(dev, perip_regs, PLATO_TOP_CR_PDP_CLK_CTRL,
				0x00001210, 0x00001210);
	udelay(100);
}
EXPORT_SYMBOL(plato_enable_pdp_clock);

/* Pixel Clock setup */
void plato_enable_pixel_clock(struct device *dev, u32 pixel_clock)
{
	u32 div0;
	u32 div1;
	u32 reg;
	u32 pclock_mhz = pixel_clock / 1000;
	u32 core_clock = plato_core_clock_speed(dev);
	u32 pll_mhz = (plato_pll_clock_speed(dev, core_clock)) / 1000000;
	void *top_regs;
	struct plato_device *plato = devres_find(dev, plato_devres_release,
		NULL, NULL);

	top_regs = plato->sys_io.registers + SYS_PLATO_REG_CHIP_LEVEL_OFFSET;

	/*
	 * Obtain divisor, round to nearest.
	 */
	div1 = (pll_mhz + pclock_mhz / 2) / pclock_mhz;

	if (div1 <= 32) {
		if (div1 < 17) {
			div0 = 0;
			div1--;
		} else {
			div0 = 1;
			div1 /= 2;
			div1--;
		}
	} else {
		dev_warn(dev,
			 "- %s: Can't find dividers for requested pixel clock (using max values)",
			 __func__);
		div0 = 1;
		div1 = 15;
	}

	reg = (PLATO_CR_HDMIG_GATE_EN_MASK | (div1 << PLATO_CR_HDMIV1_DIV_0_SHIFT) |
			(div0 << PLATO_CR_HDMIV0_DIV_0_SHIFT));

	plato_write_reg32(top_regs, PLATO_TOP_CR_HDMI_CLK_CTRL, reg);
	poll(dev, top_regs, PLATO_TOP_CR_HDMI_CLK_CTRL, reg, reg);

	plato_dev_info(dev, "- %s: Set up Pixel Clock dividers for %d: Div0=%d(+1) and Div1=%d(+1)",
		       __func__, pixel_clock, div0, div1);

}
EXPORT_SYMBOL(plato_enable_pixel_clock);
