/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Copyright (c) 2015-2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/iommu.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/dma-mapping.h>
#include <asm/cacheflush.h>
#include <asm/barrier.h>
#include <sunxi-iommu.h>
#include "sunxi-iommu.h"
#include <linux/iommu.h>
#include <asm/cacheflush.h>
#include <linux/module.h>
#include <linux/compiler.h>
#include <linux/delay.h>
#include <linux/dma-map-ops.h>

#if IS_ENABLED(CONFIG_AW_IOMMU_TESTS)

#ifdef CONFIG_64BIT

#define kstrtoux kstrtou64
#define kstrtox_from_user kstrtoll_from_user
#define kstrtosize_t kstrtoul

#else

#define kstrtoux kstrtou32
#define kstrtox_from_user kstrtoint_from_user
#define kstrtosize_t kstrtouint

#endif

#define ION_KERNEL_USER_ERR(str)	pr_err("%s failed!", #str)
struct ion_facade {
	struct ion_client *client;
	struct ion_handle *handle;
	dma_addr_t dma_address;
	void *virtual_address;
	size_t address_length;
	struct sg_table *sg_table;
};

static struct dentry *iommu_debugfs_top;

static LIST_HEAD(iommu_debug_devices);
static struct dentry *debugfs_tests_dir;

struct iommu_debug_device {
	struct device *dev;
	struct iommu_domain *domain;
	u64 iova;
	u64 phys;
	size_t len;
	struct list_head list;
};

static const char * const _size_to_string(unsigned long size)
{
	switch (size) {
	case SZ_4K:
		return "4K";
	case SZ_8K:
		return "8K";
	case SZ_16K:
		return "16K";
	case SZ_64K:
		return "64K";
	case SZ_2M:
		return "2M";
	case SZ_1M:
		return "1M";
	case SZ_1M * 4:
		return "4M";
	case SZ_1M * 8:
		return "8M";
	case SZ_1M * 16:
		return "16M";
	case SZ_1M * 32:
		return "32M";
	}
	return "unknown size, please add to _size_to_string";
}

static int iommu_debug_profiling_fast_dma_api_show(struct seq_file *s,
						 void *ignored)
{
	int i, experiment;
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	u64 map_elapsed_ns[10], unmap_elapsed_ns[10];
	dma_addr_t dma_addr;
	void *virt;
	const char * const extra_labels[] = {
		"not coherent",
		"coherent",
	};
	unsigned long extra_attrs[] = {
		0,
		DMA_ATTR_SKIP_CPU_SYNC,
	};

	virt = kmalloc(1518, GFP_KERNEL);
	if (!virt)
		goto out;

	for (experiment = 0; experiment < 2; ++experiment) {
		size_t map_avg = 0, unmap_avg = 0;

		for (i = 0; i < 10; ++i) {
			struct timespec64 tbefore, tafter, diff;
			u64 ns;

			ktime_get_ts64(&tbefore);
			dma_addr = dma_map_single_attrs(
				dev, virt, SZ_4K, DMA_TO_DEVICE,
				extra_attrs[experiment]);
			ktime_get_ts64(&tafter);
			diff.tv_sec = tafter.tv_sec - tbefore.tv_sec;
			diff.tv_nsec = tafter.tv_nsec - tbefore.tv_nsec;
			ns = diff.tv_sec * NSEC_PER_SEC + diff.tv_nsec;
			if (dma_mapping_error(dev, dma_addr)) {
				seq_puts(s, "dma_map_single failed\n");
				goto out_disable_config_clocks;
			}
			map_elapsed_ns[i] = ns;
			ktime_get_ts64(&tbefore);
			dma_unmap_single_attrs(
				dev, dma_addr, SZ_4K, DMA_TO_DEVICE,
				extra_attrs[experiment]);
			ktime_get_ts64(&tafter);
			diff.tv_sec = tafter.tv_sec - tbefore.tv_sec;
			diff.tv_nsec = tafter.tv_nsec - tbefore.tv_nsec;
			ns = diff.tv_sec * NSEC_PER_SEC + diff.tv_nsec;
			unmap_elapsed_ns[i] = ns;
		}
		seq_printf(s, "%13s %24s (ns): [", extra_labels[experiment],
			   "dma_map_single_attrs");
		for (i = 0; i < 10; ++i) {
			map_avg += map_elapsed_ns[i];
			seq_printf(s, "%5llu%s", map_elapsed_ns[i],
				   i < 9 ? ", " : "");
		}
		map_avg /= 10;
		seq_printf(s, "] (avg: %zu)\n", map_avg);

		seq_printf(s, "%13s %24s (ns): [", extra_labels[experiment],
			   "dma_unmap_single_attrs");
		for (i = 0; i < 10; ++i) {
			unmap_avg += unmap_elapsed_ns[i];
			seq_printf(s, "%5llu%s", unmap_elapsed_ns[i],
				   i < 9 ? ", " : "");
		}
		unmap_avg /= 10;
		seq_printf(s, "] (avg: %zu)\n", unmap_avg);
	}

out_disable_config_clocks:
	kfree(virt);
out:
	return 0;
}

static int iommu_debug_profiling_fast_dma_api_open(struct inode *inode,
						 struct file *file)
{
	return single_open(file, iommu_debug_profiling_fast_dma_api_show,
			   inode->i_private);
}

static const struct file_operations iommu_debug_profiling_fast_dma_api_fops = {
	.open	 = iommu_debug_profiling_fast_dma_api_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};


/* Creates a fresh fast mapping and applies @fn to it */
static int __apply_to_new_mapping(struct seq_file *s,
				    int (*fn)(struct device *dev,
					      struct seq_file *s,
					      struct iommu_domain *domain,
					      void *priv),
				    void *priv)
{
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	int ret = -EINVAL;

	ret = fn(dev, s, global_iommu_domain, priv);
	return ret;
}

#ifdef TLB_STRESS_SWEEP_TEST
static int __tlb_stress_sweep(struct device *dev, struct seq_file *s)
{
	int i, ret = 0;
	unsigned long iova;
	const unsigned long max = SZ_1G * 4UL;
	void *virt;
	phys_addr_t phys;
	dma_addr_t dma_addr;

	/*
	 * we'll be doing 4K and 8K mappings.  Need to own an entire 8K
	 * chunk that we can work with.
	 */
	virt = (void *)__get_free_pages(GFP_KERNEL, get_order(SZ_8K));
	phys = virt_to_phys(virt);

	/* fill the whole 4GB space */
	for (iova = 0, i = 0; iova < max; iova += SZ_8K, ++i) {
		dma_addr = dma_map_single(dev, virt, SZ_8K, DMA_TO_DEVICE);
		if (dma_addr == 0) {
			dev_err(dev, "Failed map on iter %d\n", i);
			ret = -EINVAL;
			goto out;
		}
	}

	/*
	 * free up 4K at the very beginning, then leave one 4K mapping,
	 * then free up 8K.  This will result in the next 8K map to skip
	 * over the 4K hole and take the 8K one.
	 */
	dma_unmap_single(dev, 0, SZ_4K, DMA_TO_DEVICE);
	dma_unmap_single(dev, SZ_8K, SZ_4K, DMA_TO_DEVICE);
	dma_unmap_single(dev, SZ_8K + SZ_4K, SZ_4K, DMA_TO_DEVICE);

	/* remap 8K */
	dma_addr = dma_map_single(dev, virt, SZ_8K, DMA_TO_DEVICE);

	/*
	 * now remap 4K.  We should get the first 4K chunk that was skipped
	 * over during the previous 8K map.  If we missed a TLB invalidate
	 * at that point this should explode.
	 */
	dma_addr = dma_map_single(dev, virt, SZ_4K, DMA_TO_DEVICE);


	/* we're all full again. unmap everything. */
	for (dma_addr = 0; dma_addr < max; dma_addr += SZ_8K)
		dma_unmap_single(dev, dma_addr, SZ_8K, DMA_TO_DEVICE);

out:
	free_pages((unsigned long)virt, get_order(SZ_8K));
	return ret;
}
#else
static int __tlb_stress_sweep(struct device *dev, struct seq_file *s)
{
	return 0;
}
#endif

struct fib_state {
	unsigned long cur;
	unsigned long prev;
};

static __maybe_unused void fib_init(struct fib_state *f)
{
	f->cur = f->prev = 1;
}

static __maybe_unused unsigned long get_next_fib(struct fib_state *f)
{
	int next = f->cur + f->prev;

	f->prev = f->cur;
	f->cur = next;
	return next;
}

/*
 * Not actually random.  Just testing the fibs (and max - the fibs).
 */
#ifdef RAND_VA_SWEEP_TEST
static int __rand_va_sweep(struct device *dev, struct seq_file *s,
			   const size_t size)
{
	u64 iova;
	const unsigned long max = SZ_1G * 4UL;
	int i, remapped, unmapped, ret = 0;
	void *virt;
	dma_addr_t dma_addr, dma_addr2;
	struct fib_state fib;

	virt = (void *)__get_free_pages(GFP_KERNEL, get_order(size));
	if (!virt) {
		if (size > SZ_8K) {
			dev_err(dev,
				"Failed to allocate %s of memory, which is a lot. Skipping test for this size\n",
				_size_to_string(size));
			return 0;
		}
		return -ENOMEM;
	}

	/* fill the whole 4GB space */
	for (iova = 0, i = 0; iova < max; iova += size, ++i) {
		dma_addr = dma_map_single(dev, virt, size, DMA_TO_DEVICE);
		if (dma_addr == 0) {
			dev_err(dev, "Failed map on iter %d\n", i);
			ret = -EINVAL;
			goto out;
		}
	}

	/* now unmap "random" iovas */
	unmapped = 0;
	fib_init(&fib);
	for (iova = get_next_fib(&fib) * size;
	     iova < max - size;
	     iova = get_next_fib(&fib) * size) {
		dma_addr = iova;
		dma_addr2 = max - size - iova;
		if (dma_addr == dma_addr2) {
			WARN(1,
			"%s test needs update! The random number sequence is folding in on itself and should be changed.\n",
			__func__);
			return -EINVAL;
		}
		dma_unmap_single(dev, dma_addr, size, DMA_TO_DEVICE);
		dma_unmap_single(dev, dma_addr2, size, DMA_TO_DEVICE);
		unmapped += 2;
	}

	/* and map until everything fills back up */
	for (remapped = 0;; ++remapped) {
		dma_addr = dma_map_single(dev, virt, size, DMA_TO_DEVICE);
		if (dma_addr == 0)
			break;
	}

	if (unmapped != remapped) {
		dev_err(dev,
			"Unexpected random remap count! Unmapped %d but remapped %d\n",
			unmapped, remapped);
		ret = -EINVAL;
	}

	for (dma_addr = 0; dma_addr < max; dma_addr += size)
		dma_unmap_single(dev, dma_addr, size, DMA_TO_DEVICE);

out:
	free_pages((unsigned long)virt, get_order(size));
	return ret;
}
#else
static int __rand_va_sweep(struct device *dev, struct seq_file *s,
			   const size_t size)
{
	return 0;
}
#endif

struct dma_addr_list {
	dma_addr_t addr;
	struct dma_addr_list *next;
};

static int __full_va_sweep(struct device *dev, struct seq_file *s,
			   const size_t size, struct iommu_domain *domain)
{
	unsigned long iova;
	void *virt;
	phys_addr_t phys;
	const unsigned long max = SZ_128M;
	struct dma_addr_list *phead, *p, *ptmp;

	phead = kmalloc(sizeof(*phead), GFP_KERNEL);
	memset(phead, 0, sizeof(*phead));
	virt = (void *)__get_free_pages(GFP_KERNEL, get_order(size));
	if (!virt) {
		if (size > SZ_8K) {
			dev_err(dev,
				"Failed to allocate %s of memory, which is a lot. Skipping test for this size\n",
				_size_to_string(size));
			return 0;
		}
		return -ENOMEM;
	}
	phys = virt_to_phys(virt);

	for (p = phead, iova = 0; iova < max; iova += size, p = p->next) {
		p->addr = dma_map_single(dev, virt, size, DMA_TO_DEVICE);
		p->next = kmalloc(sizeof(*p->next), GFP_KERNEL);
		memset(p->next, 0, sizeof(*p->next));
		if (p->addr == 0 || p->addr == DMA_MAPPING_ERROR) {
			dev_err(dev, "Failed to map dma, out of iova space\n");
			return -ENOMEM;
		}
	}

	for (p = phead; p->addr != 0;) {
		dma_unmap_single(dev, p->addr, size, DMA_TO_DEVICE);
		ptmp = p;
		p = p->next;
		kfree(ptmp);
	}
	kfree(p);
	free_pages((unsigned long)virt, get_order(size));
	return 0;
}

#define ds_printf(d, s, fmt, ...) ({				\
			dev_err(d, fmt, ##__VA_ARGS__);		\
			seq_printf(s, fmt, ##__VA_ARGS__);	\
		})

static int __functional_dma_api_va_test(struct device *dev, struct seq_file *s,
				     struct iommu_domain *domain, void *priv)
{
	int i, j;
	int ret = 0;
	size_t *sz, *sizes = priv;

	for (j = 0; j < 1; ++j) {
		for (sz = sizes; *sz; ++sz) {
			for (i = 0; i < 2; ++i) {
				ds_printf(dev, s, "Full VA sweep @%s %d",
					       _size_to_string(*sz), i);
				if (__full_va_sweep(dev, s, *sz, domain)) {
					ds_printf(dev, s, "  -> FAILED\n");
					ret = -EINVAL;
					goto out;
				} else
					ds_printf(dev, s, "  -> SUCCEEDED\n");
			}
		}
	}

	ds_printf(dev, s, "bonus map:");
	if (__full_va_sweep(dev, s, SZ_4K, domain)) {
		ds_printf(dev, s, "  -> FAILED\n");
		ret = -EINVAL;
		goto out;
	} else
		ds_printf(dev, s, "  -> SUCCEEDED\n");

	for (sz = sizes; *sz; ++sz) {
		for (i = 0; i < 2; ++i) {
			ds_printf(dev, s, "Rand VA sweep @%s %d",
				   _size_to_string(*sz), i);
			if (__rand_va_sweep(dev, s, *sz)) {
				ds_printf(dev, s, "  -> FAILED\n");
				ret = -EINVAL;
				goto out;
			} else
				ds_printf(dev, s, "  -> SUCCEEDED\n");
		}
	}

	ds_printf(dev, s, "TLB stress sweep");
	if (__tlb_stress_sweep(dev, s)) {
		ds_printf(dev, s, "  -> FAILED\n");
		ret = -EINVAL;
		goto out;
	} else
		ds_printf(dev, s, "  -> SUCCEEDED\n");

	ds_printf(dev, s, "second bonus map:");
	if (__full_va_sweep(dev, s, SZ_4K, domain)) {
		ds_printf(dev, s, "  -> FAILED\n");
		ret = -EINVAL;
		goto out;
	} else
		ds_printf(dev, s, "  -> SUCCEEDED\n");
out:
	return ret;
}

/* iova alloc strategy stress test */
static int iommu_iova_alloc_strategy_stress_show(struct seq_file *s,
						    void *ignored)
{
	size_t sizes[] = {SZ_4K, SZ_8K, SZ_16K, SZ_64K, 0};
	int ret = 0;

	ret = __apply_to_new_mapping(s, __functional_dma_api_va_test, sizes);
	return ret;
}

static int iommu_iova_alloc_strategy_stress_open(struct inode *inode,
						    struct file *file)
{
	return single_open(file, iommu_iova_alloc_strategy_stress_show,
			   inode->i_private);
}

static const struct file_operations iommu_iova_alloc_strategy_stress_fops = {
	.open	 = iommu_iova_alloc_strategy_stress_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};


static int __functional_dma_api_alloc_test(struct device *dev,
					   struct seq_file *s,
					   struct iommu_domain *domain,
					   void *ignored)
{
	size_t size = SZ_1K * 742;
	int ret = 0;
	u8 *data;
	dma_addr_t iova;

	/* Make sure we can allocate and use a buffer */
	ds_printf(dev, s, "Allocating coherent buffer");
	data = dma_alloc_coherent(dev, size, &iova, GFP_KERNEL);
	if (!data) {
		ds_printf(dev, s, "  -> FAILED\n");
		ret = -EINVAL;
	} else {
		int i;

		ds_printf(dev, s, "  -> SUCCEEDED\n");
		ds_printf(dev, s, "Using coherent buffer");
		for (i = 0; i < 742; ++i) {
			int ind = SZ_1K * i;
			u8 *p = data + ind;
			u8 val = i % 255;

			memset(data, 0xa5, size);
			*p = val;
			(*p)++;
			if ((*p) != val + 1) {
				ds_printf(dev, s,
					  "  -> FAILED on iter %d since %d != %d\n",
					  i, *p, val + 1);
				ret = -EINVAL;
				break;
			}
		}
		if (!ret)
			ds_printf(dev, s, "  -> SUCCEEDED\n");
		dma_free_coherent(dev, size, data, iova);
	}

	return ret;
}

/* iommu kernel virtual addr read/write */
static int iommu_kvirtual_addr_rdwr_show(struct seq_file *s,
						   void *ignored)
{
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	int ret = -EINVAL;

	ret = __functional_dma_api_alloc_test(dev, s,
				global_iommu_domain, NULL);
	return ret;
}

static int iommu_kvirtual_addr_rdwr_open(struct inode *inode,
						   struct file *file)
{
	return single_open(file, iommu_kvirtual_addr_rdwr_show,
			   inode->i_private);
}

static const struct file_operations iommu_kvirtul_addr_rdwr_fops = {
	.open	 = iommu_kvirtual_addr_rdwr_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static int __functional_dma_api_ion_test(struct device *dev,
					   struct seq_file *s,
					   struct iommu_domain *domain,
					   void *ignored)
{
	return 0;
}

/* iommu ion interface test */
static int iommu_ion_interface_test_show(struct seq_file *s,
						   void *ignored)
{
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	int ret = -EINVAL;

	ret = __functional_dma_api_ion_test(dev, s,
				global_iommu_domain, NULL);
	return ret;
}

static int iommu_ion_interface_test_open(struct inode *inode,
						   struct file *file)
{
	return single_open(file, iommu_ion_interface_test_show,
			   inode->i_private);
}

static const struct file_operations iommu_ion_interface_test_fops = {
	.open	 = iommu_ion_interface_test_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

static bool __testing_largeiova;
__maybe_unused static int __extra_test(struct device *dev, struct seq_file *s,
				       struct iommu_domain *domain,
				       void *ignored)
{
	u32 *coherent, *noncoherent;
	dma_addr_t dma_coherent, dma_noncoherent;
	int ret;
	const size_t size = SZ_4K * 1024;
	u32 *tmp_buffer;
	const int rount = 100;

	struct timespec64 tbefore, tafter, diff;
	u64 ns;

	/* alloc coherent */
	coherent = dma_alloc_coherent(dev, size, &dma_coherent, GFP_KERNEL);
	/* alloc noncoherent */
	noncoherent = dma_alloc_noncoherent(dev, size, &dma_noncoherent,
					    DMA_BIDIRECTIONAL, GFP_KERNEL);
	/* cpu va write */
	memset(coherent, 0xa5, size);
	memset(noncoherent, 0x3c, size);
	wmb();
	/* iova write */
	sunxi_iova_test_write((dma_addr_t)dma_coherent, 0xdead);
	sunxi_iova_test_write((dma_addr_t)dma_noncoherent, 0xdead);
	rmb();
	/* (non)coherent read */
	ds_printf(dev, s, "coherent: %x, noncoherent: %x", coherent[0],
		  noncoherent[0]);
	/* perfermence test */
	tmp_buffer = kmalloc(size, GFP_KERNEL);
	ktime_get_ts64(&tbefore);
	for (ret = 0; ret < rount; ret++) {
		memcpy(tmp_buffer, coherent, size);
	}
	ktime_get_ts64(&tafter);
	diff.tv_sec = tafter.tv_sec - tbefore.tv_sec;
	diff.tv_nsec = tafter.tv_nsec - tbefore.tv_nsec;
	ns = diff.tv_sec * NSEC_PER_SEC + diff.tv_nsec;
	ds_printf(dev, s, "coherent time for %d round %ld bytes %lld", rount,
		  size, ns);

	ktime_get_ts64(&tbefore);
	for (ret = 0; ret < rount; ret++) {
		memcpy(tmp_buffer, noncoherent, size);
	}
	ktime_get_ts64(&tafter);
	diff.tv_sec = tafter.tv_sec - tbefore.tv_sec;
	diff.tv_nsec = tafter.tv_nsec - tbefore.tv_nsec;
	ns = diff.tv_sec * NSEC_PER_SEC + diff.tv_nsec;
	ds_printf(dev, s,
		  "noncoherent(non flush) time for %d round %ld bytes %lld",
		  rount, size, ns);

	ktime_get_ts64(&tbefore);
	for (ret = 0; ret < rount; ret++) {
		dma_sync_single_for_cpu(dev, dma_noncoherent, size,
					DMA_FROM_DEVICE);
		memcpy(tmp_buffer, noncoherent, size);
	}
	ktime_get_ts64(&tafter);
	diff.tv_sec = tafter.tv_sec - tbefore.tv_sec;
	diff.tv_nsec = tafter.tv_nsec - tbefore.tv_nsec;
	ns = diff.tv_sec * NSEC_PER_SEC + diff.tv_nsec;
	ds_printf(dev, s, "noncoherent(flush) time for %d round %ld bytes %lld",
		  rount, size, ns);
	dma_free_coherent(dev, size, coherent, dma_coherent);
	dma_free_noncoherent(dev, size, noncoherent, dma_noncoherent,
			     DMA_FROM_DEVICE);
	kfree(tmp_buffer);
	return 0;
}

static bool __testing_largeiova;
static int __functional_dma_api_iova_test(struct device *dev,
					   struct seq_file *s,
					   struct iommu_domain *domain,
					   void *ignored)
{
	size_t size = SZ_4K * 512;
	int ret = 0;
	u32 *data;
	dma_addr_t iova;

	sunxi_set_debug_mode();
	//__extra_test(dev,s,domain,ignored);

	/* Make sure we can allocate and use a buffer */
	ds_printf(dev, s, "Allocating coherent iova buffer");
	/*
	 * same size will resulting in reusing freed iova
	 * not what we want when testing large iova and normal
	 * iova, use different size(in order) insted
	 */
	if (__testing_largeiova)
		size *= 2;
	data = dma_alloc_coherent(dev, size, &iova, GFP_KERNEL);
	if (!data) {
		ds_printf(dev, s, "  -> FAILED\n");
		ret = -EINVAL;
	} else {
		int i;

		if (__testing_largeiova) {
			/* we are testing large iova, iova should be large enough */
			if (iova < 0x100000000) {
				ds_printf(
					dev, s,
					"iova %pad too small for a large iova test\n",
					&iova);
				goto out;
			}
		}
		ds_printf(dev, s, "  -> SUCCEEDED\n");
#if 1
		{
			char *buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
			sunxi_iommu_dump_pgtable(buf, PAGE_SIZE, false);
			pr_err("%s", buf);
			kfree(buf);
		}
#endif
		ds_printf(dev, s, "Using coherent buffer");
		for (i = 0; i < 128; ++i) {
			int ind = (SZ_4K * i) / sizeof(u32);
			u32 *p = data + ind;
			u32 *p1 = (u32 *)iova + ind;
			u32 read_data;

			memset(data, 0xa5, size);
			*p = 0x5a5a5a5a;
			/**
			 * make sure that *p is written before
			 * the write operation of the debug mode of iommu
			 */
			wmb();
			sunxi_iova_test_write((dma_addr_t)p1, 0xdead);
			/**
			 * do the write operation of debug mode of iommu
			 * in order
			 */
			rmb();
			if ((*p) != 0xdead) {
				ds_printf(dev, s,
				 "-> FAILED on iova0 iter %x  %x\n", i, *p);
				ret = -EINVAL;
				goto out;
			}

			*p = 0xffffaaaa;
			/**
			 * make sure that *p is written before
			 * the read operation of the debug mode of iommu
			 */
			wmb();
			read_data = sunxi_iova_test_read((dma_addr_t)p1);
			if (read_data != 0xffffaaaa) {
				ds_printf(dev, s,
					"-> FAILED on iova1 iter %x  %x\n",
					i, read_data);
				ret = -EINVAL;
				goto out;
			}

		}
		if (!ret)
			ds_printf(dev, s, "  -> SUCCEEDED\n");
	}
out:
	dma_free_coherent(dev, size, data, iova);
	sunxi_set_prefetch_mode();
	return ret;
}

/* iommu test use debug interface */
static int iommu_vir_devio_addr_rdwr_show(struct seq_file *s, void *ignored)
{
	int ret = 0;
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	u64 old_dma_mask;
	const char *idx_prompt[] = { "1st", "2nd", "3rd", "th" };
	int i, j;

	for (j = 0; j < 2; j++) {
		if (j == 1) {
			old_dma_mask = dma_get_mask(dev);
			dma_set_mask(dev, DMA_BIT_MASK(64));
			__testing_largeiova = 1;
		}
		for (i = 0; i < 3; i++) {
			ret = __apply_to_new_mapping(
				s, __functional_dma_api_iova_test, NULL);
			if (ret) {
				if (i < 3) {
					pr_err("the %s iova test failed\n",
					       idx_prompt[i]);
				} else {
					pr_err("the %d%s iova test failed\n",
					       i + 1, idx_prompt[3]);
				}
			}
		}
		if (j == 1) {
			dma_set_mask(dev, old_dma_mask);
			__testing_largeiova = 0;
		}
	}
	return 0;
}

static int iommu_vir_devio_addr_rdwr_open(struct inode *inode,
						    struct file *file)
{
	return single_open(file, iommu_vir_devio_addr_rdwr_show,
			   inode->i_private);
}

static const struct file_operations iommu_vir_devio_addr_rdwr_fops = {
	.open	 = iommu_vir_devio_addr_rdwr_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};


static int __functional_dma_api_basic_test(struct device *dev,
					   struct seq_file *s,
					   struct iommu_domain *domain,
					   void *ignored)
{
	size_t size = 1518;
	int i, j, ret = 0;
	u8 *data;
	dma_addr_t iova;
	phys_addr_t pa, pa2;

	ds_printf(dev, s, "Basic DMA API test");
	/* Make sure we can allocate and use a buffer */
	for (i = 0; i < 1000; ++i) {
		data = kmalloc(size, GFP_KERNEL);
		if (!data) {
			ds_printf(dev, s, "  -> FAILED\n");
			ret = -EINVAL;
			goto out;
		}
		memset(data, 0xa5, size);
		iova = dma_map_single(dev, data, size, DMA_TO_DEVICE);
#if IS_ENABLED(DEBUG)
		{
			char *buf = kmalloc(PAGE_SIZE, GFP_KERNEL);
			sunxi_iommu_dump_pgtable(buf, PAGE_SIZE, false);
			pr_err("%s", buf);
			kfree(buf);
		}
#endif
		pa = iommu_iova_to_phys(domain, iova);
		pa2 = virt_to_phys(data);
		if (pa != pa2) {
			dev_err(dev,
				"iova_to_phys doesn't match virt_to_phys: %pa != %pa\n",
				&pa, &pa2);
			ret = -EINVAL;
			kfree(data);
			goto out;
		}
		dma_unmap_single(dev, iova, size, DMA_TO_DEVICE);
		for (j = 0; j < size; ++j) {
			if (data[j] != 0xa5) {
				dev_err(dev, "data[%d] != 0xa5\n", data[j]);
				ret = -EINVAL;
				kfree(data);
				goto out;
			}
		}
		kfree(data);
	}

out:
	if (ret)
		ds_printf(dev, s, "  -> FAILED\n");
	else
		ds_printf(dev, s, "  -> SUCCEEDED\n");

	return ret;
}

/* iommu basic test */
static int iommu_debug_basic_test_show(struct seq_file *s,
						   void *ignored)
{
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	int ret = -EINVAL;

	ret = __functional_dma_api_basic_test(dev, s,
				global_iommu_domain, NULL);
	return ret;
}

static int iommu_debug_basic_test_open(struct inode *inode,
						   struct file *file)
{
	return single_open(file, iommu_debug_basic_test_show,
			   inode->i_private);
}

static const struct file_operations iommu_debug_basic_test_fops = {
	.open	 = iommu_debug_basic_test_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release,
};

#ifdef AW_IOMMU_PGTABLE_V2
/*
 * only page table v2 can handle iova/phys > 4G,
 * we use it to determ whether we should run larget iova/phys test
 */
#define LARGE_IOVA_PHY_TEST
#endif

#ifdef LARGE_IOVA_PHY_TEST
/* iommu basic test */
static int iommu_debug_basic_large_iova_test_show(struct seq_file *s,
						  void *ignored)
{
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	int ret = -EINVAL;
	u64 old_dma_mask;
	ds_printf(dev, s, "large iova(>8G)");

	old_dma_mask = dma_get_mask(dev);

	dma_set_mask(dev, DMA_BIT_MASK(64));
	ret = __functional_dma_api_basic_test(dev, s, global_iommu_domain,
					      NULL);
	dma_set_mask(dev, old_dma_mask);
	return ret;
}

static int iommu_debug_basic_large_iova_test_open(struct inode *inode,
						  struct file *file)
{
	return single_open(file, iommu_debug_basic_large_iova_test_show,
			   inode->i_private);
}

static const struct file_operations iommu_debug_basic_large_iova_test_fops = {
	.open = iommu_debug_basic_large_iova_test_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __tested_phy_is_rollbacked(dma_addr_t phys, uint32_t test_val)
{
	/*
	 * if addressed phys is rollbacked to a lower address
	 * then *phys should be same as *(phys & mask(bits(phys)-1))
	 */
	unsigned long width = __ffs64(phys);
	dma_addr_t mask = (1 << width) - 1;
	uint32_t *test_ptr;
	int ret = 0;
	uint32_t offset = phys - PAGE_ALIGN(phys) / sizeof(uint32_t *);
	phys = PAGE_ALIGN(phys);
	while (mask > 0x40000000 && !ret) {
		phys = phys & mask;
		if (!phys)
			break; /* no more valid candidate */
		test_ptr = ioremap(phys, 4096);
		if (test_ptr[offset] == test_val)
			ret = 1; /* mask addr got same val, possible rollback detected */
		iounmap(test_ptr);
		mask >>= 1;
	}
	return ret;
}

static int __functional_large_phys_test(struct device *dev, struct seq_file *s,
					struct iommu_domain *domain,
					void *ignored)
{
	size_t size = SZ_4K * 128;
	char *dump_buffer;
	int ret = 0;
	int i;
	u32 *data;
	dma_addr_t iova;
	u64 test_param[4];
	struct page *premap_pages[128];
	struct of_phandle_args args;
	const char *name;
	dump_buffer = kmalloc(4096, GFP_KERNEL);
	if (!dump_buffer)
		return -ENOMEM;
	/* test only for g2d , pass in other device */
	of_parse_phandle_with_args(dev->of_node, "iommus", "#iommu-cells", 0,
				   &args);
	of_property_read_string_index(to_of_node(dev->iommu->iommu_dev->fwnode),
				      "masters", args.args[0], &name);
	if (of_property_read_string_index(
		    to_of_node(dev->iommu->iommu_dev->fwnode), "masters",
		    args.args[0], &name) ||
	    (strcasecmp("g2d", name) != 0)) {
		pr_err("search iommmu name for maseter %s failed\n",
		       dev_name(dev));
		return 0;
	}

	memset(test_param, 0, sizeof(test_param));
	/* premap large iova(to identical large pa)*/
	if (!of_find_property(dev->of_node, "sunxi-iova-premap", NULL)) {
		return -ENOMEM;
	}
	if (of_property_read_variable_u64_array(
		    dev->of_node, "sunxi-iova-premap", test_param, 0, 4) < 0)
		pr_err("read_premap_failed");
	iova = test_param[0];

	/* map premap large pa for cpu access */
	for (i = 0; i < size / SZ_4K; i++)
		premap_pages[i] = i ? premap_pages[i - 1] + 1 :
				      phys_to_page(test_param[0]);
	data = vmap(premap_pages, size / SZ_4K, VM_MAP,
		    pgprot_writecombine(PAGE_KERNEL));
	size = test_param[1] > size ? size : test_param[1];

	sunxi_iommu_dump_pgtable(dump_buffer, 4096, false);
	ds_printf(dev, s, "%s", dump_buffer);

	ds_printf(dev, s, "  -> SUCCEEDED\n");

	/* make sure we are not in a rollbacked access */
	data[0] = 0xA5A5A5A5;
	data[1] = 0x3C3C3C3C;
	if (__tested_phy_is_rollbacked(test_param[0], data[0]) &&
	    __tested_phy_is_rollbacked(test_param[0] + sizeof(data), data[1])) {
		ds_printf(dev, s,
			  "%llu is rollbacked and not a valid phys addr",
			  test_param[0]);
		ret = -EINVAL;
		goto out;
	}

	ds_printf(dev, s, "Using coherent buffer");
	for (i = 0; i < 128; ++i) {
		int ind = (SZ_4K * i) / sizeof(u32);
		u32 *p = data + ind;
		u32 *p1 = (u32 *)iova + ind;
		u32 read_data;

		memset(data, 0xa5, size);
		*p = 0x5a5a5a5a;
		/**
		 * make sure that *p is written before
		 * the write operation of the debug mode of iommu
		 */
		wmb();
		sunxi_iova_test_write((dma_addr_t)p1, 0xdead);
		/**
		 * do the write operation of debug mode of iommu
		 * in order
		 */
		rmb();
		if ((*p) != 0xdead) {
			ds_printf(dev, s, "-> FAILED on iova0 iter %x  %x\n", i,
				  *p);
			ret = -EINVAL;
			goto out;
		}

		*p = 0xffffaaaa;
		/**
		 * make sure that *p is written before
		 * the read operation of the debug mode of iommu
		 */
		wmb();
		read_data = sunxi_iova_test_read((dma_addr_t)p1);
		if (read_data != 0xffffaaaa) {
			ds_printf(dev, s, "-> FAILED on iova1 iter %x  %x\n", i,
				  read_data);
			ret = -EINVAL;
			goto out;
		}
	}

out:
	vunmap(data);
	kfree(dump_buffer);
	return ret;
}

/* iommu basic test */
static int iommu_debug_large_phys_show(struct seq_file *s, void *ignored)
{
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	int ret = -EINVAL;

	ret = __functional_large_phys_test(dev, s, global_iommu_domain, NULL);
	return ret;
}

static int iommu_debug_large_phys_open(struct inode *inode, struct file *file)
{
	return single_open(file, iommu_debug_large_phys_show, inode->i_private);
}

static const struct file_operations iommu_debug_large_phys_fops = {
	.open = iommu_debug_large_phys_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif
#if defined(CONFIG_AW_IOMMU_V2) || defined(CONFIG_AW_IOMMU_V3)
#define BYPASS_TEST
/* we may use other master for test later */
#define BYPASS_TEST_USE_G2D
#ifndef TEST_USE_G2D
#define TEST_USE_G2D
#endif
#endif

#if defined(CONFIG_AW_IOMMU_V2)
#define PREVENT_HANG_TEST
/* we may use other master for test later */
#define PREVENT_HANG_TEST_USE_G2D
#ifndef TEST_USE_G2D
#define TEST_USE_G2D
#endif
#endif

#ifdef TEST_USE_G2D
#include <sunxi-g2d.h>

static int __prepare_rotate0_param(dma_addr_t src, dma_addr_t dst, size_t size,
				   g2d_blt_h *blit_para)
{
	if (size < 512 * 512 * 4) {
		WARN_ON(size < 512 * 512 * 4);
		return -EINVAL;
	}
	/* to output size for preparing */
	if (blit_para == NULL) {
		return 512 * 512 * 4;
	}
	memset(blit_para, 0, sizeof(*blit_para));

	blit_para->src_image_h.laddr[0] = src;
	blit_para->dst_image_h.laddr[0] = dst;
	pr_err("src_paddr: 0x%x\n", blit_para->src_image_h.laddr[0]);
	pr_err("dst_paddr: 0x%x\n", blit_para->dst_image_h.laddr[0]);

	blit_para->flag_h = G2D_ROT_0;
	/* configure src image */
	blit_para->src_image_h.bbuff = 1;
	blit_para->src_image_h.format = G2D_FORMAT_ARGB8888;
	blit_para->src_image_h.laddr[0] =
		(int)(blit_para->src_image_h.laddr[0]);
	blit_para->src_image_h.use_phy_addr = 1;
	blit_para->src_image_h.width = 512;
	blit_para->src_image_h.height = 512;
	blit_para->src_image_h.align[0] = 0;
	blit_para->src_image_h.align[1] = 0;
	blit_para->src_image_h.align[2] = 0;
	blit_para->src_image_h.clip_rect.x = 0;
	blit_para->src_image_h.clip_rect.y = 0;
	blit_para->src_image_h.clip_rect.w = 512;
	blit_para->src_image_h.clip_rect.h = 512;
	blit_para->src_image_h.coor.x = 0;
	blit_para->src_image_h.coor.y = 0;
	blit_para->src_image_h.gamut = G2D_BT601;

	/* configure dst image */
	blit_para->dst_image_h.bbuff = 1;
	blit_para->dst_image_h.format = G2D_FORMAT_ARGB8888;
	blit_para->dst_image_h.laddr[0] =
		(int)(blit_para->dst_image_h.laddr[0]);
	blit_para->dst_image_h.use_phy_addr = 1;
	blit_para->dst_image_h.width = 512;
	blit_para->dst_image_h.height = 512;
	blit_para->dst_image_h.align[0] = 0;
	blit_para->dst_image_h.align[1] = 0;
	blit_para->dst_image_h.align[2] = 0;
	blit_para->dst_image_h.clip_rect.x = 0;
	blit_para->dst_image_h.clip_rect.y = 0;
	blit_para->dst_image_h.clip_rect.w = 512;
	blit_para->dst_image_h.clip_rect.h = 512;
	blit_para->src_image_h.coor.x = 0;
	blit_para->src_image_h.coor.y = 0;
	blit_para->dst_image_h.gamut = G2D_BT601;
	return 512 * 512 * 4;
}

extern int g2d_blit_h(g2d_blt_h *para);
extern int g2d_open(struct inode *inode, struct file *file);
extern int g2d_release(struct inode *inode, struct file *file);
#define ROTATE_FLUSH_SRC (0x1)
#define ROTATE_FLUSH_DST (0x2)
static int __perform_one_rotate_0(struct device *dev, dma_addr_t src,
				  dma_addr_t dst, size_t size,
				  uint32_t flush_flag)
{
	g2d_blt_h g2d_param;
	struct file g2d_file;
	int ret;
	g2d_open(0, &g2d_file);
	__prepare_rotate0_param(src, dst, size, &g2d_param);
	if (flush_flag & 0x1)
		dma_sync_single_range_for_device(dev, src, 0, size,
						 DMA_TO_DEVICE);
	if (flush_flag & 0x2)
		dma_sync_single_range_for_device(dev, dst, 0, size,
						 DMA_TO_DEVICE);
	ret = g2d_blit_h(&g2d_param);
	if (flush_flag & 0x1)
		dma_sync_single_range_for_cpu(dev, src, 0, size,
					      DMA_FROM_DEVICE);
	if (flush_flag & 0x2)
		dma_sync_single_range_for_cpu(dev, dst, 0, size,
					      DMA_FROM_DEVICE);
	g2d_release(0, &g2d_file);

	return ret;
}
#endif

#ifdef PREVENT_HANG_TEST
/* iommu basic test */
static int iommu_debug_prevent_hang_show(struct seq_file *s, void *ignored)
{
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	int ret = -EINVAL;
	union {
		uint64_t buf[4];
		struct {
			uint64_t buffer_invalid_src_addr;
			uint64_t buffer_invalid_src_size;
			uint64_t buffer_invalid_dst_addr;
			uint64_t buffer_invalid_dst_size;
		};
	} test_params;
	dma_addr_t valid_src_phys, valid_dst_phys;
	void *valid_src_virt, *valid_dst_virt;
	uint32_t size;

	/*
	 * 3 cases
			SRC(read)	DST(write)
	 * 1:	valid		valid
	 * 2:	invalid		valid
	 * 3:	valid		invalid
	 * we used g2d rotate 0 degree as tester as
	 * it act like a memcpy and easy to understand in/out put
	 */
	/* buffer preparation */
	/* get reserved iova for later test */
	if (of_property_count_u64_elems(dev->of_node, "sunxi-iova-reserve") !=
	    ARRAY_SIZE(test_params.buf)) {
		return ret;
	}

	of_property_read_u64_array(dev->of_node, "sunxi-iova-reserve",
				   test_params.buf,
				   ARRAY_SIZE(test_params.buf));
	/* make sure input buffer unmapped */
	if (iommu_iova_to_phys(iommu_get_domain_for_dev(dev),
			       test_params.buffer_invalid_src_addr) ||
	    iommu_iova_to_phys(iommu_get_domain_for_dev(dev),
			       test_params.buffer_invalid_dst_addr)) {
		return -EINVAL;
	}
	size = __prepare_rotate0_param(0, 0, test_params.buffer_invalid_src_size, NULL);
	if (size > test_params.buffer_invalid_src_size ||
	    size > test_params.buffer_invalid_dst_size) {
		pr_err("reserve buffer too small (%llu/%llu < %u) for prevent hang test",
		       test_params.buffer_invalid_src_size,
		       test_params.buffer_invalid_dst_size, size);
	}

	/* valid src & dst buffer, just alloc once */
	valid_src_virt =
		dma_alloc_coherent(dev, size, &valid_src_phys, GFP_KERNEL);
	valid_dst_virt =
		dma_alloc_coherent(dev, size, &valid_dst_phys, GFP_KERNEL);

	sunxi_iommu_enable_interrupt(0);
	sunxi_iommu_prevent_hang_enable(1);

	/* case 1 copy from valid src to valid dst */
	memset(valid_src_virt, 2, size);
	memset(valid_dst_virt, 3, size);
	__perform_one_rotate_0(dev, valid_src_phys, valid_dst_phys, size,
			       ROTATE_FLUSH_DST | ROTATE_FLUSH_SRC);
	if (memcmp(valid_src_virt, valid_dst_virt, size) != 0) {
		ds_printf(dev, s, "prevent hang test failed at case 1");
		print_hex_dump(KERN_ERR, "src", DUMP_PREFIX_OFFSET, 16, 1,
			       valid_src_virt, 64, false);
		print_hex_dump(KERN_ERR, "dst", DUMP_PREFIX_OFFSET, 16, 1,
			       valid_dst_virt, 64, false);
	} else {
		ds_printf(dev, s, "prevent hang test passed at case 1");
	}
	sunxi_iommu_enable_interrupt(1);
	mdelay(200);
	sunxi_iommu_enable_interrupt(0);

	/* case 2 copy from invlalid src to valid dst */
	memset(valid_src_virt, 2, size);
	memset(valid_dst_virt, 3, size);
	__perform_one_rotate_0(dev, test_params.buffer_invalid_src_addr,
			       valid_dst_phys, size, ROTATE_FLUSH_DST);
	/* g2d read from invalid src get 0, so dst buffer should be all 0 */
	if (memcmp(valid_dst_virt, memset(valid_src_virt, 0, size), size) !=
	    0) {
		ds_printf(dev, s, "prevent hang test failed at case 2");
		print_hex_dump(KERN_ERR, "src", DUMP_PREFIX_OFFSET, 16, 1,
			       valid_src_virt, 64, false);
		print_hex_dump(KERN_ERR, "dst", DUMP_PREFIX_OFFSET, 16, 1,
			       valid_dst_virt, 64, false);
	} else {
		ds_printf(dev, s, "prevent hang test passed at case 2");
	}
	sunxi_iommu_enable_interrupt(1);
	mdelay(200);
	sunxi_iommu_enable_interrupt(0);

	/* case 2 copy from vlalid src to invalid dst */
	memset(valid_src_virt, 2, size);
	memset(valid_dst_virt, 3, size);
	__perform_one_rotate_0(dev, valid_src_phys,
			       test_params.buffer_invalid_dst_addr, size,
			       ROTATE_FLUSH_SRC);
	/* g2d write to invalid dst do nothing , so dst buffer should be untouch */
	if (memcmp(valid_dst_virt, memset(valid_src_virt, 3, size), size) !=
	    0) {
		ds_printf(dev, s, "prevent hang test failed at case 3");
		print_hex_dump(KERN_ERR, "src", DUMP_PREFIX_OFFSET, 16, 1,
			       valid_src_virt, 64, false);
		print_hex_dump(KERN_ERR, "dst", DUMP_PREFIX_OFFSET, 16, 1,
			       valid_dst_virt, 64, false);
	} else {
		ds_printf(dev, s, "prevent hang test passed at case 3");
	}
	sunxi_iommu_prevent_hang_enable(0);
	sunxi_iommu_enable_interrupt(1);

	return 0;
}

static int iommu_debug_prevent_hang_open(struct inode *inode, struct file *file)
{
	return single_open(file, iommu_debug_prevent_hang_show,
			   inode->i_private);
}

static const struct file_operations iommu_debug_prevent_hang_fops = {
	.open = iommu_debug_prevent_hang_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

#ifdef BYPASS_TEST
#ifdef BYPASS_TEST_USE_G2D
/* iommu basic test */
static int iommu_debug_bypass_show(struct seq_file *s, void *ignored)
{
	struct iommu_debug_device *ddev = s->private;
	struct device *dev = ddev->dev;
	int ret = -EINVAL;
	union {
		uint64_t buf[4];
		struct {
			uint64_t premap_1_addr;
			uint64_t premap_1_size;
			uint64_t premap_2_addr;
			uint64_t premap_2_size;
		};
	} test_params;
	dma_addr_t valid_src_phys;
	void *valid_src_virt;
	void *premap_1, *premap_2;
	uint32_t size;
	struct of_phandle_args args;
	const char *name;

	/* test only for g2d , pass in other device */
	of_parse_phandle_with_args(dev->of_node, "iommus", "#iommu-cells", 0,
				   &args);
	if (of_property_read_string_index(
		    to_of_node(dev->iommu->iommu_dev->fwnode), "masters",
		    args.args[0], &name) ||
	    (strcasecmp("g2d", name) != 0))
		pr_err("search iommmu name for maseter %s failed\n",
		       dev_name(dev));
	{
		return 0;
	}
	/*
	 * premap range A & B
	 * then we iommu_map iova A to phy B and set g2d to write addr A.
	 * if bypass works, g2d write to phy A
	 * if bypass not work, g2d write to phy B since iova A
	 *   is mapped to phy B
	 */
	/* buffer preparation */
	/* get reserved iova for later test */
	if (of_property_count_u64_elems(dev->of_node, "sunxi-iova-premap") !=
	    ARRAY_SIZE(test_params.buf)) {
		return ret;
	}

	of_property_read_u64_array(dev->of_node, "sunxi-iova-premap",
				   test_params.buf,
				   ARRAY_SIZE(test_params.buf));
	size = __prepare_rotate0_param(0, 0, test_params.premap_1_size, NULL);
	if (size > test_params.premap_1_size ||
	    size > test_params.premap_2_size) {
		pr_err("reserve buffer too small (%llu/%llu < %u) for prevent hang test",
		       test_params.premap_1_size, test_params.premap_2_size,
		       size);
	}
	/* now remap iova A to phy B */
	iommu_unmap(iommu_get_domain_for_dev(dev), test_params.premap_1_addr,
		    test_params.premap_1_size);
	iommu_map(iommu_get_domain_for_dev(dev), test_params.premap_1_addr,
		  test_params.premap_2_addr, test_params.premap_2_size,
		  IOMMU_READ | IOMMU_WRITE, GFP_KERNEL);
	/* also phy A to B for cpu to check result later */
	premap_1 =
		ioremap(test_params.premap_1_addr, test_params.premap_1_size);
	premap_2 =
		ioremap(test_params.premap_2_addr, test_params.premap_2_size);

	/* valid src & dst buffer, just alloc once */
	valid_src_virt =
		dma_alloc_coherent(dev, size, &valid_src_phys, GFP_KERNEL);

	/*
	 * actual test, check if g2d output to phy B with bypass enabled
	 * bypass disabled is default state and checked in basic test
	 * do not test here
	 */
	sunxi_enable_device_iommu(args.args[0], 0);

	memset(valid_src_virt, 3, size);
	memset(premap_1, 0xc, size);
	memset(premap_2, 0xa, size);

	__perform_one_rotate_0(dev, valid_src_phys, test_params.premap_1_addr,
			       size, ROTATE_FLUSH_DST | ROTATE_FLUSH_SRC);
	if (memcmp(valid_src_virt, premap_2, size) != 0 ||
	    memcpy(valid_src_virt, premap_1, size) == 0) {
		ds_printf(dev, s, "bypass test failed at case 1");
	} else {
		ds_printf(dev, s, "bypass test passed at case 1");
	}
	sunxi_enable_device_iommu(args.args[0], 1);
	ret = 0;

	iounmap(premap_1);
	iounmap(premap_2);
	dma_free_coherent(dev, size, valid_src_virt, valid_src_phys);
	/* incase we want to repeat test later */
	iommu_unmap(iommu_get_domain_for_dev(dev), test_params.premap_1_addr,
		    test_params.premap_1_size);
	iommu_map(iommu_get_domain_for_dev(dev), test_params.premap_1_addr,
		  test_params.premap_1_addr, test_params.premap_1_size,
		  IOMMU_READ | IOMMU_WRITE, GFP_KERNEL);
	return ret;
}
#endif
static int iommu_debug_bypass_open(struct inode *inode, struct file *file)
{
	return single_open(file, iommu_debug_bypass_show,
			   inode->i_private);
}

static const struct file_operations iommu_debug_bypass_fops = {
	.open = iommu_debug_bypass_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};
#endif

/*
 * The following will only work for drivers that implement the generic
 * device tree bindings described in
 * Documentation/devicetree/bindings/iommu/iommu.txt
 */
static int snarf_iommu_devices(struct device *dev, const char *name)
{
	struct iommu_debug_device *ddev;
	struct dentry *dir;

	if (IS_ERR_OR_NULL(dev))
		return -EINVAL;

	ddev = kzalloc(sizeof(*ddev), GFP_KERNEL);
	if (!ddev)
		return -ENODEV;
	ddev->dev = dev;
	ddev->domain = global_iommu_domain;
	dir = debugfs_create_dir(name, debugfs_tests_dir);
	if (!dir) {
		pr_err("Couldn't create iommu/devices/%s debugfs dir\n",
		       name);
		goto err;
	}

	if (!debugfs_create_file("profiling_fast_dma_api", 0400, dir, ddev,
				 &iommu_debug_profiling_fast_dma_api_fops)) {
		pr_err("Couldn't create iommu/devices/%s/profiling_fast_dma_api debugfs file\n",
		       name);
		goto err_rmdir;
	}

	if (!debugfs_create_file("iommu_basic_test", 0400, dir, ddev,
				 &iommu_debug_basic_test_fops)) {
		pr_err("Couldn't create iommu/devices/%s/iommu_basic_test debugfs file\n",
		       name);
		goto err_rmdir;
	}

	if (!debugfs_create_file("ion_interface_test", 0400, dir, ddev,
				 &iommu_ion_interface_test_fops)) {
		pr_err("Couldn't create iommu/devices/%s/ion_interface_test debugfs file\n",
		       name);
		goto err_rmdir;
	}

	if (!debugfs_create_file("iova_alloc_strategy_stress_test",
		0200, dir, ddev,
		&iommu_iova_alloc_strategy_stress_fops)) {
		pr_err("Couldn't create iommu/devices/%s/iova_alloc_strategy_stress_test debugfs file\n",
		       name);
		goto err_rmdir;
	}

	if (!debugfs_create_file("kvirtual_addr_rdwr_test", 0200, dir, ddev,
				 &iommu_kvirtul_addr_rdwr_fops)) {
		pr_err("Couldn't create iommu/devices/%s/kvirtual_addr_rdwr_test debugfs file\n",
		       name);
		goto err_rmdir;
	}

	if (!debugfs_create_file("vir_devio_addr_rdwr_test", 0200, dir, ddev,
				 &iommu_vir_devio_addr_rdwr_fops)) {
		pr_err("Couldn't create iommu/devices/%s/vir_devio_addr_rdwr_test debugfs file\n",
		       name);
		goto err_rmdir;
	}

#ifdef LARGE_IOVA_PHY_TEST
	if (!debugfs_create_file("iommu_large_phys", 0400, dir, ddev,
				 &iommu_debug_large_phys_fops)) {
		pr_err("Couldn't create iommu/devices/%s/iommu_large_phys debugfs file\n",
		       name);
		goto err_rmdir;
	}

	if (!debugfs_create_file("iommu_basic_large_iova_test", 0400, dir, ddev,
				 &iommu_debug_basic_large_iova_test_fops)) {
		pr_err("Couldn't create iommu/devices/%s/iommu_basic_large_iova_test debugfs file\n",
		       name);
		goto err_rmdir;
	}
#endif

#ifdef PREVENT_HANG_TEST
	if (!debugfs_create_file("iommu_basic_prevent_hang_test", 0400, dir,
				 ddev, &iommu_debug_prevent_hang_fops)) {
		pr_err("Couldn't create iommu/devices/%s/iommu_basic_prevent_hang_test debugfs file\n",
		       name);
		goto err_rmdir;
	}
#endif

#ifdef BYPASS_TEST
	if (!debugfs_create_file("iommu_bypass_test", 0400, dir,
				 ddev, &iommu_debug_bypass_fops)) {
		pr_err("Couldn't create iommu/devices/%s/iommu_basic_test debugfs file\n",
		       name);
		goto err_rmdir;
	}
#endif

	list_add(&ddev->list, &iommu_debug_devices);
	return 0;

err_rmdir:
	debugfs_remove_recursive(dir);
err:
	kfree(ddev);
	return 0;
}

static int pass_iommu_devices(struct device *dev, void *ignored)
{
	if (!of_find_property(dev->of_node, "iommus", NULL))
		return 0;

	return snarf_iommu_devices(dev, dev_name(dev));
}

static int iommu_debug_populate_devices(void)
{
	return bus_for_each_dev(&platform_bus_type, NULL, NULL,
			pass_iommu_devices);
}

static int iommu_debug_init_tests(void)
{
	iommu_debugfs_top = debugfs_create_dir("iommu", NULL);
	if (!iommu_debugfs_top) {
		pr_err("Couldn't create iommu debugfs directory\n");
		return -ENODEV;
	}
	debugfs_tests_dir = debugfs_create_dir("tests",
					       iommu_debugfs_top);
	if (!debugfs_tests_dir) {
		pr_err("Couldn't create iommu/tests debugfs directory\n");
		return -ENODEV;
	}

	return iommu_debug_populate_devices();
}

static void iommu_debug_destroy_tests(void)
{
	debugfs_remove_recursive(debugfs_tests_dir);
}
#else
static inline int iommu_debug_init_tests(void) { return 0; }
static inline void iommu_debug_destroy_tests(void) { }
#endif

static int __init iommu_debug_init(void)
{
	if (iommu_debug_init_tests())
		return -ENODEV;

	return 0;
}

static void __exit iommu_debug_exit(void)
{
	iommu_debug_destroy_tests();
}

module_init(iommu_debug_init);
module_exit(iommu_debug_exit);

MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.1.0");
MODULE_AUTHOR("huangshuosheng<huangshuosheng@allwinnertech.com>");
