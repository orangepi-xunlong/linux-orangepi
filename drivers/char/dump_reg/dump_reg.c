/*
 * dump registers sysfs driver
 *
 * Copyright(c) 2015-2018 Allwinnertech Co., Ltd.
 *      http://www.allwinnertech.com
 *
 * Author: Liugang <liugang@allwinnertech.com>
 *         Xiafeng <xiafeng@allwinnertech.com>
 *         Martin <wuyan@allwinnertech.com>
 *         Lewis  <liuyu@allwinnertech.com>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/kdev_t.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/seq_file.h>
#include <linux/platform_device.h>
#include <linux/sysfs.h>
#include <linux/mod_devicetable.h>
#include "dump_reg.h"

/* the register and vaule to be test by dump_reg */
static u32 test_addr;
static u32 test_size;
static struct class *dump_class;

/* Access in byte mode ? 1: byte-mode, 0: word-mode */
static unsigned int rw_byte_mode;

/* for dump_reg class */
static struct dump_addr dump_para;
static struct write_group *wt_group;
static struct compare_group *cmp_group;

static u32 _read(void __iomem *vaddr)
{
	if (rw_byte_mode)
		return (u32)readb(vaddr);
	else
		return readl(vaddr);
}

static void _write(u32 val, void __iomem *vaddr)
{
	if (rw_byte_mode)
		writeb((u8)val, vaddr);
	else
		writel(val, vaddr);
}

static void __iomem *_io_remap(unsigned long paddr, size_t size)
{
	return ioremap(paddr, size);
}

static void _io_unmap(void __iomem *vaddr)
{
	iounmap(vaddr);
}

static void __iomem *_mem_remap(unsigned long paddr, size_t size)
{
	return (void __iomem *)phys_to_virt(paddr);
}

/*
 * Convert a physical address (which is already mapped) to virtual address
 */
static void __iomem *_get_vaddr(struct dump_addr *dump_addr, unsigned long uaddr)
{
	unsigned long offset = uaddr - dump_addr->uaddr_start;
	return (void __iomem *)(dump_addr->vaddr_start + offset);
}

const struct dump_struct dump_table[] = {
	{
		.addr_start = SUNXI_IO_PHYS_START,
		.addr_end   = SUNXI_IO_PHYS_END,
		.remap = _io_remap,
		.unmap = _io_unmap,
		.get_vaddr = _get_vaddr,
		.read  = _read,
		.write = _write,
	},
	{
		.addr_start = SUNXI_PLAT_PHYS_START,
		.addr_end   = SUNXI_PLAT_PHYS_END,
		.remap = _mem_remap,
		.unmap = NULL,
		.get_vaddr = _get_vaddr,
		.read  = _read,
		.write = _write,
	},
#if defined(SUNXI_IOMEM_START)
	{
		.addr_start = SUNXI_IOMEM_START,
		.addr_end   = SUNXI_IOMEM_END,
		.remap = NULL,  /* .remap = NULL: uaddr is a virtual address */
		.unmap = NULL,
		.get_vaddr = _get_vaddr,
		.read  = _read,
		.write = _write,
	},
#endif
	{
		.addr_start = SUNXI_MEM_PHYS_START,
		.addr_end   = SUNXI_MEM_PHYS_END,
		.remap = NULL,  /* .remap = NULL: uaddr is a virtual address */
		.unmap = NULL,
		.get_vaddr = _get_vaddr,
		.read  = _read,
		.write = _write,
	},
};
EXPORT_SYMBOL(dump_table);

/**
 * __addr_valid - check if @uaddr is valid.
 * @uaddr: addr to judge.
 *
 * return index if @addr is valid, -ENXIO if not.
 */
int __addr_valid(unsigned long uaddr)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dump_table); i++)
		if (uaddr >= dump_table[i].addr_start &&
		    uaddr <= dump_table[i].addr_end)
			return i;
	return -ENXIO;
}
EXPORT_SYMBOL(__addr_valid);

/**
 * __dump_regs_ex - dump a range of registers' value, copy to buf.
 * @dump_addr: start and end address of registers.
 * @buf: store the dump info.
 * @buf_size: buf size
 *
 * return bytes written to buf, <=0 indicate err
 */
ssize_t __dump_regs_ex(struct dump_addr *dump_addr, char *buf, ssize_t buf_size)
{
	int index;
	ssize_t cnt = 0;
	unsigned long uaddr;
	unsigned long remap_size;
	const struct dump_struct *dump;

	/* Make the address 4-bytes aligned */
	dump_addr->uaddr_start &= (~0x3UL);
	dump_addr->uaddr_end &= (~0x3UL);
	remap_size = dump_addr->uaddr_end - dump_addr->uaddr_start + 4;

	index = __addr_valid(dump_addr->uaddr_start);
	if ((index < 0) || (index != __addr_valid(dump_addr->uaddr_end)) ||
	    (buf == NULL)) {
		pr_err("%s(): Invalid para: index=%d, start=0x%lx, end=0x%lx, buf=0x%p\n",
		       __func__, index, dump_addr->uaddr_start, dump_addr->uaddr_end, buf);
		return -EIO;
	}

	dump = &dump_table[index];
	if (dump->remap) {
		dump_addr->vaddr_start = dump->remap(dump_addr->uaddr_start, remap_size);
		if (!dump_addr->vaddr_start) {
			pr_err("%s(): remap failed\n", __func__);
			return -EIO;
		}
	} else  /* if (dump->remap = NULL), then treat uaddr as a virtual address */
		dump_addr->vaddr_start = (void __iomem *)dump_addr->uaddr_start;

	if (dump_addr->uaddr_start == dump_addr->uaddr_end) {
		cnt = sprintf(buf, "0x%08x\n", dump->read(dump_addr->vaddr_start));
		goto out;
	}

	for (uaddr = (dump_addr->uaddr_start & ~0x0F); uaddr <= dump_addr->uaddr_end;
	     uaddr += 4) {
		if (!(uaddr & 0x0F))
			cnt += snprintf(buf + cnt, buf_size - cnt,
				     "\n" PRINT_ADDR_FMT ":", uaddr);

		if (cnt >= buf_size) {
			pr_warn("Range too large, strings buffer overflow\n");
			cnt = buf_size;
			goto out;
		}

		if (uaddr < dump_addr->uaddr_start)  /* Don't show unused uaddr */
			/* "0x12345678 ", 11 space */
			cnt += snprintf(buf + cnt, buf_size - cnt, "           ");
		else
			cnt += snprintf(buf + cnt, buf_size - cnt, " 0x%08x",
				dump->read(dump->get_vaddr(dump_addr, uaddr)));
	}
	cnt += snprintf(buf + cnt, buf_size - cnt, "\n");

	pr_debug("%s(): start=0x%lx, end=0x%lx, return=%zd\n", __func__,
		 dump_addr->uaddr_start, dump_addr->uaddr_end, cnt);

out:
	if (dump->unmap)
		dump->unmap(dump_addr->vaddr_start);

	return cnt;
}
EXPORT_SYMBOL(__dump_regs_ex);

/**
 * __parse_dump_str - parse the input string for dump attri.
 * @buf: the input string, eg: "0x01c20000,0x01c20300".
 * @size: buf size.
 * @start: store the start reg's addr parsed from buf, eg 0x01c20000.
 * @end: store the end reg's addr parsed from buf, eg 0x01c20300.
 *
 * return 0 if success, otherwise failed.
 */
int __parse_dump_str(const char *buf, size_t size,
			    unsigned long *start, unsigned long *end)
{
	char *ptr = NULL;
	char *ptr2 = (char *)buf;
	int ret = 0, times = 0;

	/* Support single address mode, some time it haven't ',' */
next:
	/*
	 * Default dump only one register(*start =*end).
	 * If ptr is not NULL, we will cover the default value of end.
	 */
	if (times == 1)
		*start = *end;

	if (!ptr2 || (ptr2 - buf) >= size)
		goto out;

	ptr = ptr2;
	ptr2 = strnchr(ptr, size - (ptr - buf), ',');
	if (ptr2) {
		*ptr2 = '\0';
		ptr2++;
	}

	ptr = strim(ptr);
	if (!strlen(ptr))
		goto next;

	ret = kstrtoul(ptr, 16, end);
	if (!ret) {
		times++;
		goto next;
	} else
		pr_warn("String syntax errors: \"%s\"\n", ptr);

out:
	return ret;
}
EXPORT_SYMBOL(__parse_dump_str);

/**
 * __write_show - dump a register's value, copy to buf.
 * @pgroup: the addresses to read.
 * @buf: store the dump info.
 *
 * return bytes written to buf, <=0 indicate err.
 */
ssize_t __write_show(struct write_group *pgroup, char *buf, ssize_t len)
{
#define WR_DATA_FMT PRINT_ADDR_FMT"  0x%08x  %s"

	int i = 0;
	ssize_t cnt = 0;
	unsigned long reg = 0;
	u32 val;
	u8 rval_buf[16];
	struct dump_addr dump_addr;

	if (!pgroup) {
		pr_err("%s,%d err, pgroup is NULL!\n", __func__, __LINE__);
		goto end;
	}

	cnt += snprintf(buf, len - cnt, WR_PRINT_FMT);
	if (cnt > len) {
		cnt = -EINVAL;
		goto end;
	}

	for (i = 0; i < pgroup->num; i++) {
		reg = pgroup->pitem[i].reg_addr;
		val = pgroup->pitem[i].val;
		dump_addr.uaddr_start = reg;
		dump_addr.uaddr_end = reg;
		if (__dump_regs_ex(&dump_addr, rval_buf, sizeof(rval_buf)) < 0)
			return -EINVAL;

		cnt +=
		    snprintf(buf + cnt, len - cnt, WR_DATA_FMT, reg, val,
			     rval_buf);
		if (cnt > len) {
			cnt = len;
			goto end;
		}
	}

end:
	return cnt;
}
EXPORT_SYMBOL(__write_show);

/**
 * __parse_write_str - parse the input string for write attri.
 * @str: string to be parsed, eg: "0x01c20818 0x55555555".
 * @reg_addr: store the reg address. eg: 0x01c20818.
 * @val: store the expect value. eg: 0x55555555.
 *
 * return 0 if success, otherwise failed.
 */
static int __parse_write_str(char *str, unsigned long *reg_addr, u32 *val)
{
	char *ptr = str;
	char *tstr = NULL;
	int ret = 0;

	/*
	 * Skip the leading whitespace, find the true split symbol.
	 * And it must be 'address value'.
	 */
	tstr = strim(str);
	ptr = strchr(tstr, ' ');
	if (!ptr)
		return -EINVAL;

	/*
	 * Replaced split symbol with a %NUL-terminator temporary.
	 * Will be fixed at end.
	 */
	*ptr = '\0';
	ret = kstrtoul(tstr, 16, reg_addr);
	if (ret)
		goto out;

	ret = kstrtou32(skip_spaces(ptr + 1), 16, val);

out:
	return ret;
}

/**
 * __write_item_init - init for write attri. parse input string,
 *                     and construct write struct.
 * @ppgroup: store the struct allocated, the struct contains items parsed from
 *           input buf.
 * @buf: input string, eg: "0x01c20800 0x00000031,0x01c20818 0x55555555,...".
 * @size: buf size.
 *
 * return 0 if success, otherwise failed.
 */
int __write_item_init(struct write_group **ppgroup, const char *buf,
			     size_t size)
{
	char *ptr, *ptr2;
	unsigned long addr = 0;
	u32 val;
	struct write_group *pgroup;

	/* alloc item buffer */
	pgroup = kmalloc(sizeof(struct write_group), GFP_KERNEL);
	if (!pgroup)
		return -ENOMEM;

	pgroup->pitem = kmalloc(sizeof(struct write_item) * MAX_WRITE_ITEM,
				GFP_KERNEL);
	if (!pgroup->pitem) {
		kfree(pgroup);
		return -ENOMEM;
	}

	pgroup->num = 0;
	ptr = (char *)buf;
	do {
		ptr2 = strchr(ptr, ',');
		if (ptr2)
			*ptr2 = '\0';

		if (!__parse_write_str(ptr, &addr, &val)) {
			pgroup->pitem[pgroup->num].reg_addr = addr;
			pgroup->pitem[pgroup->num].val = val;
			pgroup->num++;
		} else
			pr_err("%s: Failed to parse string: %s\n", __func__,
			       ptr);

		if (!ptr2)
			break;

		ptr = ptr2 + 1;
		*ptr2 = ',';

	} while (pgroup->num <= MAX_WRITE_ITEM);

	/* free buffer if no valid item */
	if (pgroup->num == 0) {
		kfree(pgroup->pitem);
		kfree(pgroup);
		return -EINVAL;
	}

	*ppgroup = pgroup;
	return 0;
}
EXPORT_SYMBOL(__write_item_init);

/**
 * __write_item_deinit - reled_addrse memory that cred_addrted by
 *                       __write_item_init.
 * @pgroup: the write struct allocated in __write_item_init.
 */
void __write_item_deinit(struct write_group *pgroup)
{
	if (pgroup != NULL) {
		if (pgroup->pitem != NULL)
			kfree(pgroup->pitem);
		kfree(pgroup);
	}
}
EXPORT_SYMBOL(__write_item_deinit);

/**
 * __compare_regs_ex - dump a range of registers' value, copy to buf.
 * @pgroup: addresses of registers.
 * @buf: store the dump info.
 *
 * return bytes written to buf, <= 0 indicate err.
 */
ssize_t __compare_regs_ex(struct compare_group *pgroup, char *buf,
				 ssize_t len)
{
#define CMP_DATAO_FMT PRINT_ADDR_FMT"  0x%08x  0x%08x  0x%08x  OK\n"
#define CMP_DATAE_FMT PRINT_ADDR_FMT"  0x%08x  0x%08x  0x%08x  ERR\n"

	int i;
	ssize_t cnt = 0;
	unsigned long reg;
	u32 expect, actual, mask;
	u8 actualb[16];
	struct dump_addr dump_addr;

	if (!pgroup) {
		pr_err("%s,%d err, pgroup is NULL!\n", __func__, __LINE__);
		goto end;
	}

	cnt += snprintf(buf, len - cnt, CMP_PRINT_FMT);
	if (cnt > len) {
		cnt = -EINVAL;
		goto end;
	}

	for (i = 0; i < pgroup->num; i++) {
		reg = pgroup->pitem[i].reg_addr;
		expect = pgroup->pitem[i].val_expect;
		dump_addr.uaddr_start = reg;
		dump_addr.uaddr_end = reg;
		if (__dump_regs_ex(&dump_addr, actualb, sizeof(actualb)) < 0)
			return -EINVAL;

		if (kstrtou32(actualb, 16, &actual))
			return -EINVAL;

		mask = pgroup->pitem[i].val_mask;
		if ((actual & mask) == (expect & mask))
			cnt +=
			    snprintf(buf + cnt, len - cnt, CMP_DATAO_FMT, reg,
				     expect, actual, mask);
		else
			cnt +=
			    snprintf(buf + cnt, len - cnt, CMP_DATAE_FMT, reg,
				     expect, actual, mask);
		if (cnt > len) {
			cnt = -EINVAL;
			goto end;
		}
	}

end:
	return cnt;
}
EXPORT_SYMBOL(__compare_regs_ex);

/**
 * __parse_compare_str - parse the input string for compare attri.
 * @str: string to be parsed, eg: "0x01c20000 0x80000011 0x00000011".
 * @reg_addr: store the reg address. eg: 0x01c20000.
 * @val_expect: store the expect value. eg: 0x80000011.
 * @val_mask: store the mask value. eg: 0x00000011.
 *
 * return 0 if success, otherwise failed.
 */
static int __parse_compare_str(char *str, unsigned long *reg_addr,
			       u32 *val_expect, u32 *val_mask)
{
	unsigned long result_addr[3] = { 0 };
	char *ptr = str;
	char *ptr2 = NULL;
	int i, ret = 0;

	for (i = 0; i < ARRAY_SIZE(result_addr); i++) {
		ptr = skip_spaces(ptr);
		ptr2 = strchr(ptr, ' ');
		if (ptr2)
			*ptr2 = '\0';

		ret = kstrtoul(ptr, 16, &result_addr[i]);
		if (!ptr2)
			break;

		*ptr2 = ' ';

		if (ret)
			break;

		ptr = ptr2 + 1;
	}

	*reg_addr = result_addr[0];
	*val_expect = (u32) result_addr[1];
	*val_mask = (u32) result_addr[2];

	return ret;
}

/**
 * __compare_item_init - init for compare attri. parse input string,
 *                       and construct compare struct.
 * @ppgroup: store the struct allocated, the struct contains items parsed from
 *           input buf.
 * @buf: input string,
 *  eg: "0x01c20000 0x80000011 0x00000011,0x01c20004 0x0000c0a4 0x0000c0a0,...".
 * @size: buf size.
 *
 * return 0 if success, otherwise failed.
 */
int __compare_item_init(struct compare_group **ppgroup,
			       const char *buf, size_t size)
{
	char *ptr, *ptr2;
	unsigned long addr = 0;
	u32 val_expect = 0, val_mask = 0;
	struct compare_group *pgroup = NULL;

	/* alloc item buffer */
	pgroup = kmalloc(sizeof(struct compare_group), GFP_KERNEL);
	if (pgroup == NULL)
		return -EINVAL;

	pgroup->pitem = kmalloc(sizeof(struct compare_item) * MAX_COMPARE_ITEM,
				GFP_KERNEL);
	if (pgroup->pitem == NULL) {
		kfree(pgroup);
		return -EINVAL;
	}

	pgroup->num = 0;

	/* get item from buf */
	ptr = (char *)buf;
	do {
		ptr2 = strchr(ptr, ',');
		if (ptr2)
			*ptr2 = '\0';

		if (!__parse_compare_str(ptr, &addr, &val_expect, &val_mask)) {
			pgroup->pitem[pgroup->num].reg_addr = addr;
			pgroup->pitem[pgroup->num].val_expect = val_expect;
			pgroup->pitem[pgroup->num].val_mask = val_mask;
			pgroup->num++;
		} else
			pr_err("%s: Failed to parse string: %s\n", __func__,
			       ptr);

		if (!ptr2)
			break;

		*ptr2 = ',';
		ptr = ptr2 + 1;

	} while (pgroup->num <= MAX_COMPARE_ITEM);

	/* free buffer if no valid item */
	if (pgroup->num == 0) {
		kfree(pgroup->pitem);
		kfree(pgroup);
		return -EINVAL;
	}
	*ppgroup = pgroup;

	return 0;
}
EXPORT_SYMBOL(__compare_item_init);

/**
 * __compare_item_deinit - reled_addrse memory that cred_addrted by
 *                         __compare_item_init.
 * @pgroup: the compare struct allocated in __compare_item_init.
 */
void __compare_item_deinit(struct compare_group *pgroup)
{
	if (pgroup) {
		kfree(pgroup->pitem);
		kfree(pgroup);
	}
}
EXPORT_SYMBOL(__compare_item_deinit);

/**
 * dump_show - show func of dump attribute.
 * @dev: class ptr.
 * @attr: attribute ptr.
 * @buf: the input buf which contain the start and end reg.
 *       eg: "0x01c20000,0x01c20100\n".
 *
 * return size written to the buf, otherwise failed.
 */
static ssize_t
dump_show(struct class *class, struct class_attribute *attr, char *buf)
{
	return __dump_regs_ex(&dump_para, buf, PAGE_SIZE);
}

static ssize_t
dump_store(struct class *class, struct class_attribute *attr,
	   const char *buf, size_t count)
{
	int index;
	unsigned long start_reg = 0;
	unsigned long end_reg = 0;

	if (__parse_dump_str(buf, count, &start_reg, &end_reg)) {
		pr_err("%s,%d err, invalid para!\n", __func__, __LINE__);
		goto err;
	}

	index = __addr_valid(start_reg);
	if ((index < 0) || (index != __addr_valid(end_reg))) {
		pr_err("%s,%d err, invalid para!\n", __func__, __LINE__);
		goto err;
	}

	dump_para.uaddr_start = start_reg;
	dump_para.uaddr_end = end_reg;
	pr_debug("%s,%d, start_reg:" PRINT_ADDR_FMT ", end_reg:" PRINT_ADDR_FMT
		 "\n", __func__, __LINE__, start_reg, end_reg);

	return count;

err:
	dump_para.uaddr_start = 0;
	dump_para.uaddr_end = 0;

	return -EINVAL;
}

static ssize_t
write_show(struct class *class, struct class_attribute *attr, char *buf)
{
	/* display write result */
	return __write_show(wt_group, buf, PAGE_SIZE);
}

static ssize_t
write_store(struct class *class, struct class_attribute *attr,
	    const char *buf, size_t count)
{
	int i;
	int index;
	unsigned long reg;
	u32 val;
	const struct dump_struct *dump;
	struct dump_addr dump_addr;

	/* free if not NULL */
	if (wt_group) {
		__write_item_deinit(wt_group);
		wt_group = NULL;
	}

	/* parse input buf for items that will be dumped */
	if (__write_item_init(&wt_group, buf, count) < 0)
		return -EINVAL;

	/**
	 * write reg
	 * it is better if the regs been remaped and unmaped only once,
	 * but we map everytime for the range between min and max address
	 * maybe too large.
	 */
	for (i = 0; i < wt_group->num; i++) {
		reg = wt_group->pitem[i].reg_addr;
		dump_addr.uaddr_start = reg;
		val = wt_group->pitem[i].val;
		index = __addr_valid(reg);
		dump = &dump_table[index];
		if (dump->remap)
			dump_addr.vaddr_start = dump->remap(reg, 4);
		else
			dump_addr.vaddr_start = (void __iomem *)reg;
		dump->write(val, dump->get_vaddr(&dump_addr, reg));
		if (dump->unmap)
			dump->unmap(dump_addr.vaddr_start);
	}

	return count;
}

static ssize_t
compare_show(struct class *class, struct class_attribute *attr, char *buf)
{
	/* dump the items */
	return __compare_regs_ex(cmp_group, buf, PAGE_SIZE);
}

static ssize_t
compare_store(struct class *class, struct class_attribute *attr,
	      const char *buf, size_t count)
{
	/* free if struct not null */
	if (cmp_group) {
		__compare_item_deinit(cmp_group);
		cmp_group = NULL;
	}

	/* parse input buf for items that will be dumped */
	if (__compare_item_init(&cmp_group, buf, count) < 0)
		return -EINVAL;

	return count;
}

static ssize_t
rw_byte_show(struct class *class, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "read/write mode: %u(%s)\n", rw_byte_mode,
		       rw_byte_mode ? "byte" : "word");
}

static ssize_t
rw_byte_store(struct class *class, struct class_attribute *attr,
	      const char *buf, size_t count)
{
	unsigned long value;
	int ret;

	ret = kstrtoul(buf, 10, &value);
	if (!ret && (value > 1)) {
		pr_err("%s,%d err, invalid para!\n", __func__, __LINE__);
		goto out;
	}
	rw_byte_mode = value;
out:
	return count;
}

static ssize_t
test_show(struct class *class, struct class_attribute *attr, char *buf)
{
	return sprintf(buf, "addr:0x%08x\nsize:0x%08x\n", test_addr, test_size);
}

static ssize_t
help_show(struct class *class, struct class_attribute *attr, char *buf)
{
	const char *info =
		"dump single register:          echo {addr} > dump; cat dump\n"
		"dump multi  registers:         echo {start-addr},{end-addr} > dump; cat dump\n"
		"write single register:         echo {addr} {val} > write; cat write\n"
		"write multi  registers:        echo {addr1} {val1},{addr2} {val2},... > write; cat write\n"
		"compare single register:       echo {addr} {expect-val} {mask} > compare; cat compare\n"
		"compare multi  registers:      echo {addr1} {expect-val1} {mask1},{addr2} {expect-val2} {mask2},... > compare; cat compare\n"
		"byte-access mode:              echo 1 > rw_byte\n"
		"word-access mode (default):    echo 0 > rw_byte\n"
		"show test address info:        cat test\n";
	return sprintf(buf, info);
}

static struct class_attribute dump_class_attrs[] = {
	__ATTR(dump,     S_IWUSR | S_IRUGO, dump_show,     dump_store),
	__ATTR(write,    S_IWUSR | S_IRUGO, write_show,    write_store),
	__ATTR(compare,  S_IWUSR | S_IRUGO, compare_show,  compare_store),
	__ATTR(rw_byte,  S_IWUSR | S_IRUGO, rw_byte_show,  rw_byte_store),
	__ATTR(test,     S_IRUGO,           test_show, NULL),
	__ATTR(help,     S_IRUGO,           help_show, NULL),
};

static const struct of_device_id sunxi_dump_reg_match[] = {
	{.compatible = "allwinner,sunxi-dump-reg", },
	{}
};
MODULE_DEVICE_TABLE(of, sunxi_dump_reg_match);

static int sunxi_dump_reg_probe(struct platform_device *pdev)
{
	struct resource *res;
	struct device *dev = &pdev->dev;

	int err;
	int i;

	/* sys/class/sunxi_dump */
	dump_class = class_create(THIS_MODULE, "sunxi_dump");
	if (IS_ERR(dump_class)) {
		pr_err("%s:%u class_create() failed\n", __func__, __LINE__);
		return PTR_ERR(dump_class);
	}

	/* sys/class/sunxi_dump/xxx */
	for (i = 0; i < ARRAY_SIZE(dump_class_attrs); i++) {
		err = class_create_file(dump_class, &dump_class_attrs[i]);
		if (err) {
			pr_err("%s:%u class_create_file() failed. err=%d\n", __func__, __LINE__, err);
			while (i--) {
				class_remove_file(dump_class, &dump_class_attrs[i]);
			}
			class_destroy(dump_class);
			dump_class = NULL;
			return err;
		}
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "Fail to get IORESOURCE_MEM \n");
		goto error;
	}

	test_addr = res->start;
	test_size = resource_size(res);

	return 0;
error:
	dev_err(dev, "sunxi_dump_reg probe error\n");
	return -1;
}

static int sunxi_dump_reg_remove(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(dump_class_attrs); i++) {
		class_remove_file(dump_class, &dump_class_attrs[i]);
	}

	class_destroy(dump_class);
	return 0;
}

static struct platform_driver sunxi_dump_reg_driver = {
	.probe  = sunxi_dump_reg_probe,
	.remove = sunxi_dump_reg_remove,
	.driver = {
		.name   = "dump_reg",
		.owner  = THIS_MODULE,
		.of_match_table = sunxi_dump_reg_match,
	},
};

module_platform_driver(sunxi_dump_reg_driver);

MODULE_ALIAS("dump reg driver");
MODULE_ALIAS("platform:dump reg");
MODULE_LICENSE("GPL v2");
MODULE_VERSION("1.0.3");
MODULE_AUTHOR("xiafeng <xiafeng@allwinnertech.com>");
MODULE_AUTHOR("Martin <wuyan@allwinnertech.com>");
MODULE_AUTHOR("liuyu <SWCliuyus@allwinnertech.com>");
MODULE_DESCRIPTION("dump registers driver");
