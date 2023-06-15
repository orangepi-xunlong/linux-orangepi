/*
 * Copyright (C) 2019 Allwinner Technology Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <asm/io.h>
#include <linux/clk.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>

#include "platform.h"
#include <linux/clk-provider.h>
#include <linux/pm_runtime.h>
#include <linux/pm_opp.h>
#include <linux/regulator/consumer.h>

#ifdef CONFIG_DEVFREQ_THERMAL
#include <linux/devfreq_cooling.h>
#endif
#include <linux/debugfs.h>
#include <linux/thermal.h>
#include "rgxdevice.h"

#if defined(CONFIG_ARCH_SUN50IW10)
#define ID_POINTS			(0x03006200)
#define PPU_REG_BASE		(0x07001000)
#define GPU_SYS_REG_BASE		(0x01880000)
#define GPU_RSTN_REG		(0x0)
#define GPU_CLK_GATE_REG		(0x04)
#define GPU_POWEROFF_GATING_REG	(0x08)
#define GPU_PSWON_REG		(0x0c)
#define GPU_PSWOFF_REG		(0x10)
#define GPU_PD_STAT_REG		(0x20)
#define PPU_POWER_CTRL_REG	(0x24)
#define PPU_IRQ_MASK_REG		(0x2C)
#define GPU_PD_STAT_MASK		(0x03)
#define GPU_PD_STAT_ALLOFF	(0x02)
#define GPU_PD_STAT_3D_MODE	(0x00)
#define POWER_CTRL_MODE_MASK	(0x03)
#define POWER_CTRL_MODE_SW	(0x01)
#define POWER_CTRL_MODE_HW	(0x02)
#define POWER_IRQ_MASK_REQ	(0x01)
#define PPU_REG_SIZE		(0x34)

#define GPU_PD_STAT_MASK		(0x03)
#define GPU_PD_STAT_3D		(0x00)
#define GPU_PD_STAT_HOST		(0x01)
#define GPU_PD_STAT_ALLOFF	(0x02)


#define GPU_CLK_CTRL_REG		(GPU_SYS_REG_BASE + 0x20)
#define GPU_DFS_MAX		(16)
#define GPU_DFS_MIN		(13)
#endif /* defined(CONFIG_ARCH_SUN50IW10) */

#define POWER_DEBUG 0

struct sunxi_platform *sunxi_data;
#ifdef CONFIG_DEBUG_FS
static struct dentry *sunxi_debugfs;
#endif
static unsigned long dvfs_status;
static  wait_queue_head_t dvfs_wq;

static inline int get_clks_wrap(struct device *dev)
{
#if defined(CONFIG_OF)
	sunxi_data->clks.pll = of_clk_get(dev->of_node, 0);
	if (IS_ERR_OR_NULL(sunxi_data->clks.pll)) {
		dev_err(dev, "failed to get GPU pll clock\n");
		return -1;
	}

	sunxi_data->clks.core = of_clk_get(dev->of_node, 1);
	if (IS_ERR_OR_NULL(sunxi_data->clks.core)) {
		dev_err(dev, "failed to get GPU core clock\n");
		return -1;
	}

	sunxi_data->clks.bus = of_clk_get(dev->of_node, 2);
	if (IS_ERR_OR_NULL(sunxi_data->clks.bus)) {
		dev_err(dev, "failed to get GPU bus clock\n");
		return -1;
	}

	sunxi_data->clks.reset = devm_reset_control_get(dev, NULL);
	if (IS_ERR_OR_NULL(sunxi_data->clks.reset)) {
		dev_err(dev, "failed to get GPU reset handle\n");
		return -1;
	}
#endif /* defined(CONFIG_OF) */

	return 0;
}

static inline long ppu_mode_switch(int mode, bool irq_enable)
{
	long val;
	/* Mask the power_request irq */
	val = readl(sunxi_data->ppu_reg + PPU_IRQ_MASK_REG);
	if (!irq_enable)
		val = val & (~POWER_IRQ_MASK_REQ);
	else
		val = val | POWER_IRQ_MASK_REQ;
	writel(val, sunxi_data->ppu_reg + PPU_IRQ_MASK_REG);
	/* switch power mode control */
	val = readl(sunxi_data->ppu_reg + PPU_POWER_CTRL_REG);
	val = (val & ~POWER_CTRL_MODE_MASK) + mode;
	writel(val, sunxi_data->ppu_reg + PPU_POWER_CTRL_REG);

	return readl(sunxi_data->ppu_reg + PPU_POWER_CTRL_REG);
}

static inline void ppu_power_mode(int mode)
{
	unsigned int val;
	val = readl(sunxi_data->ppu_reg + GPU_PD_STAT_REG);
	val &= (~GPU_PD_STAT_MASK);
	val += mode;
	writel(val, sunxi_data->ppu_reg + GPU_PD_STAT_REG);
}

static inline void switch_interl_dfs(int val)
{
	unsigned int val2;
	writel(val, sunxi_data->gpu_reg);
	val2 = readl(sunxi_data->gpu_reg);
	WARN_ON(val != val2);
}

PVRSRV_ERROR sunxiPrePowerState(IMG_HANDLE hSysData,
					 PVRSRV_SYS_POWER_STATE eNewPowerState,
					 PVRSRV_SYS_POWER_STATE eCurrentPowerState,
					 IMG_BOOL bForced)
{
	struct sunxi_platform *platform = (struct sunxi_platform *)hSysData;

	if (eNewPowerState == PVRSRV_DEV_POWER_STATE_ON && !platform->power_on) {
		pm_runtime_get_sync(platform->dev);
		if (!sunxi_data->soft_mode) {
			ppu_mode_switch(POWER_CTRL_MODE_HW, false);
		}
		spin_lock(&sunxi_data->lock);
		if (!sunxi_data->man_ctrl)
			switch_interl_dfs(sunxi_data->current_clk->gpu_dfs);
		else
			switch_interl_dfs(0x00000001);
		platform->power_on = 1;
		spin_unlock(&sunxi_data->lock);
	}
	return PVRSRV_OK;

}

PVRSRV_ERROR sunxiPostPowerState(IMG_HANDLE hSysData,
					  PVRSRV_SYS_POWER_STATE eNewPowerState,
					  PVRSRV_SYS_POWER_STATE eCurrentPowerState,
					  IMG_BOOL bForced)
{
	struct sunxi_platform *platform = (struct sunxi_platform *)hSysData;

	if (eNewPowerState == PVRSRV_DEV_POWER_STATE_OFF
		&& platform->power_on && platform->power_idle) {
		spin_lock(&sunxi_data->lock);
		platform->power_on = 0;
		spin_unlock(&sunxi_data->lock);
		if (!sunxi_data->soft_mode) {
			ppu_mode_switch(POWER_CTRL_MODE_SW, false);
			ppu_power_mode(GPU_PD_STAT_HOST);
		}
		pm_runtime_put_sync(platform->dev);
	}
	return PVRSRV_OK;
}

static void sunxi_set_freq_safe(IMG_UINT32 ui32Frequency, bool force)
{
	int i = 0;
	struct sunxi_clks_table *find = NULL, *bak = NULL;
	long updata_core = 0;

	while (i < sunxi_data->table_num) {
		if (ui32Frequency == sunxi_data->clk_table[i].true_clk) {
			find = &sunxi_data->clk_table[i];
			break;
		}
		i++;
	}
	if (find == NULL) {
		dev_err(sunxi_data->dev, "%s:give us illlegal Frequency value =%u!",
			__func__, ui32Frequency);
		return;
	}
	if (find->true_clk == sunxi_data->current_clk->true_clk && !force)
		return;

	bak = sunxi_data->current_clk;
	updata_core = (long)(find->core_clk - sunxi_data->current_clk->core_clk);
loop:
	sunxi_data->current_clk = find;
	spin_lock(&sunxi_data->lock);
	if (sunxi_data->power_on)
		switch_interl_dfs(find->gpu_dfs);
	spin_unlock(&sunxi_data->lock);

	if (updata_core != 0 || force) {
		if (clk_set_rate(sunxi_data->clks.core, find->core_clk) < 0) {
			dev_err(sunxi_data->dev, "%s:clk_set_rate Frequency value =%u err!",
				__func__, ui32Frequency);
			find = bak;
			updata_core = 0;
			force = 0;
			goto loop;
		}
	}

}

void sunxiSetFrequencyDVFS(IMG_UINT32 ui32Frequency)
{
	if (sunxi_data->man_ctrl || sunxi_data->scenectrl)
		goto dealed;
	if (!sunxi_data->dvfs)
		ui32Frequency = sunxi_data->clk_table[0].true_clk;

	sunxi_set_freq_safe(ui32Frequency, false);
dealed:
	dvfs_status++;
	if (dvfs_status%2 == 0)
		wake_up(&dvfs_wq);
}

void sunxiSetVoltageDVFS(IMG_UINT32 ui32Volt)
{
	if (!IS_ERR_OR_NULL(sunxi_data->regula)
		&& sunxi_data->independent_power
		&& !sunxi_data->man_ctrl
		&& !sunxi_data->scenectrl
		&& sunxi_data->dvfs) {
		if (regulator_set_voltage(sunxi_data->regula, ui32Volt, INT_MAX) != 0) {
			dev_err(sunxi_data->dev, "%s:Failed to set gpu power voltage=%d!",
				__func__, ui32Volt);
		}
	}
	dvfs_status++;
	if (dvfs_status%2 == 0)
		wake_up(&dvfs_wq);
}

static int sunxi_decide_pll(struct sunxi_platform *sunxi)
{
	long pll_rate = 504000000, core_rate = 504000000;
#if defined(CONFIG_PM_OPP) && defined(CONFIG_OF)
	const struct property *prop;
	__be32 *val;
	int i = 0, j = 0, nr = 0;
	unsigned long step = 0;
	unsigned long core_clk[4], true_core;
	u32 val2;
	int err;
	unsigned short *id_mark = NULL;
	unsigned short id_dts;

	sunxi->pll_clk_rate = 500000000;
	err = of_property_read_u32(sunxi->dev->of_node, "pll_rate", &val2);
	if (!err)
		sunxi->pll_clk_rate = val2 * 1000;
	while (i < 4) {
		core_clk[i] = sunxi->pll_clk_rate / (i + 1);
		core_clk[i] /= 1000;
		core_clk[i] *= 1000;
		i++;
	}
	true_core = sunxi->pll_clk_rate;
	id_mark = ioremap(ID_POINTS, 2);
	id_dts = *id_mark;

	prop = of_find_property(sunxi->dev->of_node, "markid-points", NULL);
	if (!prop)
		return -ENODEV;
	if (!prop->value)
		return -ENODATA;
	nr = prop->length / sizeof(u32);
	if (nr % 2) {
		dev_err(sunxi->dev, "%s: Invalid OPP table\n", __func__);
		return -EINVAL;
	}
	val = prop->value;
	while (nr) {
		unsigned short id = (short)be32_to_cpup(val++);
		if (id == id_dts) {
			true_core = be32_to_cpup(val) * 1000000;
		}
		val++;
		nr -= 2;
	}
	dev_info(sunxi->dev, "gpu core id:0x%04x core:%ld\n", id_dts, true_core);
	prop = of_find_property(sunxi->dev->of_node, "operating-points", NULL);
	if (!prop)
		return -ENODEV;
	if (!prop->value)
		return -ENODATA;

	nr = prop->length / sizeof(u32);
	if (nr % 2) {
		dev_err(sunxi->dev, "%s: Invalid OPP table\n", __func__);
		return -EINVAL;
	}
	sunxi->table_num = nr/2;
	if (nr < 2) {
		sunxi->table_num = 1;
	}
	sunxi->clk_table = (struct sunxi_clks_table *)kzalloc(sizeof(struct sunxi_clks_table) * sunxi->table_num, GFP_KERNEL);
	if (!sunxi->clk_table) {
		dev_err(sunxi->dev, "failed to get kzalloc sunxi_clks_table\n");
		return -1;

	}
	sunxi->clk_table[0].core_clk = true_core;
	sunxi->clk_table[0].true_clk = sunxi->pll_clk_rate;
	sunxi->clk_table[0].gpu_dfs = 0x08;

	i = 0;
	j = 0;
	val = prop->value;
	while (nr) {
		unsigned long freq2;
		unsigned long freq = be32_to_cpup(val++) * 1000;
		sunxi->clk_table[i].volt = be32_to_cpup(val++);
		while (freq < (core_clk[j] * GPU_DFS_MIN)/GPU_DFS_MAX && j < 4)
			j++;
		if (j >= 4) {
			dev_err(sunxi->dev, "%s: give us an OPP table\n", __func__);
		}
		step = core_clk[j]/16;
		sunxi->clk_table[i].gpu_dfs = 1<<(GPU_DFS_MAX-freq/step);
		sunxi->clk_table[i].core_clk =  true_core/(j+1);
		freq2 = freq/step * sunxi->clk_table[i].core_clk/16;
		sunxi->clk_table[i].true_clk = freq;
		dev_info(sunxi->dev, "set gpu core rate:%lu freq:%lu-%luuV dfs:0x%08x\n",
				sunxi->clk_table[i].core_clk, freq2, sunxi->clk_table[i].volt, sunxi->clk_table[i].gpu_dfs);
		if (freq % step)
			dev_err(sunxi->dev, "%s: give us an error freq:%lu\n", __func__, freq);
		nr -= 2;
		i++;
	}
	pll_rate = true_core;
	core_rate = sunxi->clk_table[0].core_clk;
	sunxi->current_clk = &sunxi->clk_table[0];
#else
	sunxi->table_num = 1;
	sunxi->clk_table = (struct sunxi_clks_table *)kzalloc(sizeof(struct sunxi_clks_table) * sunxi->table_num, GFP_KERNEL);
	sunxi->clk_table[0].core_clk = core_rate;
	sunxi->pll_clk_rate = pll_rate;
	sunxi->clk_table[0].gpu_dfs = 1;
	sunxi->clk_table[0].volt = 950000;
	sunxi->current_clk = &sunxi->clk_table[0];
#endif /* defined(CONFIG_OF) */

	clk_set_rate(sunxi->clks.pll, pll_rate);
	clk_set_rate(sunxi->clks.core, core_rate);

	return 0;
}

static int parse_dts(struct device *dev, struct sunxi_platform *sunxi_data)
{
#ifdef CONFIG_OF
	u32 val;
	int err;
	err = of_property_read_u32(dev->of_node, "gpu_idle", &val);
	if (!err)
		sunxi_data->power_idle = val ? true : false;
	err = of_property_read_u32(dev->of_node, "independent_power", &val);
	if (!err)
		sunxi_data->independent_power = val ? true : false;
	err = of_property_read_u32(dev->of_node, "dvfs_status", &val);
	if (!err)
		sunxi_data->dvfs = val ? true : false;
#endif /* CONFIG_OF */

	return 0;
}

static ssize_t scene_ctrl_cmd_show(struct device *dev,
				struct device_attribute *attr,
				char *buf)
{
	return sprintf(buf, "%d\n", sunxi_data->scenectrl);
}

static ssize_t scene_ctrl_cmd_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t count)
{
	int err;
	unsigned long val = 504000000;
	unsigned long volt = 950000;
	bool first_vol;
	enum scene_ctrl_cmd cmd;
#if defined(CONFIG_PM_OPP)
	struct dev_pm_opp *opp;
#endif
	err = kstrtoul(buf, 10, &val);
	if (err) {
		dev_err(dev, "scene_ctrl_cmd_store gets a invalid parameter!\n");
		return count;
	}

	cmd = (enum scene_ctrl_cmd)val;
	switch (cmd) {
	case SCENE_CTRL_NORMAL_MODE:
		sunxi_data->scenectrl = 0;
		break;

	case SCENE_CTRL_PERFORMANCE_MODE:
		if (sunxi_data->man_ctrl) {
			dev_err(dev, "in man mode so not apply!\n");
			return count;
		}

		sunxi_data->scenectrl = 1;
#if defined(CONFIG_PM_OPP)
		val = ULONG_MAX;
		rcu_read_lock();
		opp = dev_pm_opp_find_freq_floor(dev, &val);
		if (!IS_ERR_OR_NULL(opp)) {
			volt = dev_pm_opp_get_voltage(opp);
		} else {
			val = sunxi_data->clk_table[0].true_clk;
			volt = sunxi_data->clk_table[0].volt;
		}
		rcu_read_unlock();
#endif
		if (dvfs_status%2)
			wait_event_timeout(dvfs_wq,
				(dvfs_status%2 == 0), msecs_to_jiffies(100));
		first_vol = (sunxi_data->current_clk->true_clk < val);
		if (first_vol && sunxi_data->independent_power) {
			regulator_set_voltage(sunxi_data->regula, volt, INT_MAX);
		}
		sunxi_set_freq_safe(val, false);
		if (!first_vol && sunxi_data->independent_power) {
			regulator_set_voltage(sunxi_data->regula, volt, INT_MAX);
		}
		break;

	default:
		dev_err(dev, "invalid scene control command %d!\n", cmd);
		return count;
	}

	return count;
}

static DEVICE_ATTR(command, 0660,
		scene_ctrl_cmd_show, scene_ctrl_cmd_store);

static struct attribute *scene_ctrl_attributes[] = {
	&dev_attr_command.attr,
	NULL
};

static struct attribute_group scene_ctrl_attribute_group = {
	.name = "scenectrl",
	.attrs = scene_ctrl_attributes
};

#ifdef CONFIG_DEBUG_FS
static ssize_t write_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *offp)
{
	int i, err;
	unsigned long val, val2;
	bool semicolon = false;
	char buffer[50], data[32];
	int head_size = 0, data_size = 0;

	if (count >= sizeof(buffer))
		goto err_out;

	if (copy_from_user(buffer, buf, count))
		return -EFAULT;

	/* The command format is '<head>:<data>' or '<head>:<data>;' */
	for (i = 0; i < count; i++) {
		if (*(buffer+i) == ':')
			head_size = i;
		if (*(buffer+i) == ';' && head_size) {
			data_size = count - head_size - 3;
			semicolon = true;
			break;
		}
	}

	if (!head_size)
		goto err_out;

	if (!semicolon)
		data_size = count - head_size - 2;

	if (data_size > 32)
		goto err_out;

	memcpy(data, buffer + head_size + 1, data_size);
	data[data_size] = '\0';

	err = kstrtoul(data, 10, &val);
	if (err)
		goto err_out;

	if (!strncmp("frequency", buffer, head_size)) {
		if (!sunxi_data->man_ctrl) {
			printk("you must first set man mode\n");
			goto err_out;
		}
		val *= 1000000;
		val2 = val;
		while (val <= 200000000)
			val *= 2;
		PVRSRVDevicePreClockSpeedChange(sunxi_data->config->psDevNode, true, NULL);
		clk_set_rate(sunxi_data->clks.pll, val);
		clk_set_rate(sunxi_data->clks.core, val2);
		spin_lock(&sunxi_data->lock);
		if (sunxi_data->power_on)
			switch_interl_dfs(0x00000001);
		spin_unlock(&sunxi_data->lock);
		PVRSRVDevicePostClockSpeedChange(sunxi_data->config->psDevNode, true, NULL);

	} else if (!strncmp("voltage", buffer, head_size)) {
		if (!sunxi_data->man_ctrl) {
			printk("you must first set man mode\n");
			goto err_out;
		}
		val *= 1000;
		if (sunxi_data->regula) {
			regulator_set_voltage(sunxi_data->regula, val, val);
			printk("set gpu regulator to %lu uV\n", val);
		}
	} else if (!strncmp("idle", buffer, head_size)) {
		sunxi_data->power_idle = !!val;
	} else if (!strncmp("dvfs", buffer, head_size)) {
		sunxi_data->dvfs = !!val;
	} else if (!strncmp("man", buffer, head_size)) {
		sunxi_data->man_ctrl = !!val;
		if (sunxi_data->man_ctrl) {
			wait_event_timeout(dvfs_wq,
				(dvfs_status%2 == 0), msecs_to_jiffies(100));
		} else {
			PVRSRVDevicePreClockSpeedChange(sunxi_data->config->psDevNode, true, NULL);
			if (sunxi_data->regula) {
				regulator_set_voltage(sunxi_data->regula,
					sunxi_data->current_clk->volt, sunxi_data->current_clk->volt);
				printk("set gpu regulator to %lu uV\n", sunxi_data->current_clk->volt);
			}
			clk_set_rate(sunxi_data->clks.pll, sunxi_data->pll_clk_rate);
			sunxi_set_freq_safe(sunxi_data->current_clk->true_clk, true);
			PVRSRVDevicePostClockSpeedChange(sunxi_data->config->psDevNode, true, NULL);
		}
	} else {
		goto err_out;
	}

	return count;

err_out:
	dev_err(sunxi_data->dev, "sunxi gpu invalid parameter:%s!\n", buffer);
	return -EINVAL;
}

static const struct file_operations write_fops = {
	.owner = THIS_MODULE,
	.write = write_write,
};

static int dump_debugfs_show(struct seq_file *s, void *data)
{
	int set = 0;
	int div = sunxi_data->current_clk->gpu_dfs;
#ifdef CONFIG_REGULATOR
	if (!IS_ERR_OR_NULL(sunxi_data->regula)) {
		int vol = regulator_get_voltage(sunxi_data->regula);
		seq_printf(s, "voltage:%dmV;\n", vol);
	}
#endif /* CONFIG_REGULATOR */
	while (!(div&0x01<<set) && set < 5) {
		set++;
	}
	set = GPU_DFS_MAX-set;
	seq_printf(s, "man:%s;\n", sunxi_data->man_ctrl ? "on" : "off");
	seq_printf(s, "idle:%s;\n", sunxi_data->power_idle ? "on" : "off");
	seq_printf(s, "scenectrl:%s;\n",
			sunxi_data->scenectrl ? "on" : "off");
	seq_printf(s, "dvfs:%s;\n",
			sunxi_data->dvfs ? "on" : "off");
	seq_printf(s, "independent_power:%s;\n",
			sunxi_data->independent_power ? "Yes" : "No");
	seq_printf(s, "Frequency:%luMHz;\n", sunxi_data->current_clk->core_clk/GPU_DFS_MAX*set/1000/1000);

	seq_puts(s, "\n");

	return 0;
}

static int dump_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, dump_debugfs_show, inode->i_private);
}

static const struct file_operations dump_fops = {
	.owner = THIS_MODULE,
	.open = dump_debugfs_open,
	.read  = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int sunxi_create_debugfs(void)
{
	struct dentry *node;

	sunxi_debugfs = debugfs_create_dir("sunxi_gpu", NULL);
	if (IS_ERR_OR_NULL(sunxi_debugfs))
		return -ENOMEM;

	node = debugfs_create_file("write", 0644,
				sunxi_debugfs, NULL, &write_fops);
	if (IS_ERR_OR_NULL(node))
		return -ENOMEM;

	node = debugfs_create_file("dump", 0644,
				sunxi_debugfs, NULL, &dump_fops);
	if (IS_ERR_OR_NULL(node))
		return -ENOMEM;

	return 0;
}
#endif /* CONFIG_DEBUG_FS */

static inline int sunxi_get_ic_version(struct device *dev, char *version)
{
#define SYS_CFG_BASE 0x03000000
#define VER_REG_OFFS 0x00000024
	void __iomem *io = NULL;
	static char ver = 0xff;
	/* IC version:
	 * A/B/C: 0
	 *     D: 3
	 *     E: 4
	 *     F: 5
	 *     G: 6
	 *        7/1/2 to be used in future.
	 */
	*version = 0;
	if (ver == 0xff) {
		io = ioremap(SYS_CFG_BASE, 0x100);
		if (io == NULL) {
			dev_err(dev, "ioremap of sys_cfg register failed!\n");
			return -1;
		}
		*version = (char)(readl(io + VER_REG_OFFS) & 0x7);
		iounmap(io);
		ver = *version;
	} else {
		*version = ver;
	}
	return 0;
}

bool sunxi_ic_version_ctrl(struct device *dev)
{
	char ic_version = 0;
	sunxi_get_ic_version(dev, &ic_version);
	/*
	 * The flow of jtag reset before gpu reset will cause MIPS crash, so we
	 * will put the domainA, domainB and gpu reset operations to boot0 stage.
	 * And in kernel stage, we will always keep domainA poweron.
	 */
	if (ic_version == 0 || ic_version == 3 || ic_version == 4
		|| ic_version == 5 || ic_version == 6 || ic_version == 7) {
		return false;
	}
	return true;
}

int sunxi_platform_init(struct device *dev)
{
#if defined(CONFIG_OF)
	struct resource *reg_res;
	struct platform_device *pdev = to_platform_device(dev);
#endif /* defined(CONFIG_OF) */
	unsigned int val, volt_val = 0;
	char ic_version = 0;

	sunxi_data = (struct sunxi_platform *)kzalloc(sizeof(struct sunxi_platform), GFP_KERNEL);
	if (!sunxi_data) {
		dev_err(dev, "failed to get kzalloc sunxi_platform");
		return -1;
	}

	dev->platform_data = sunxi_data;

#if defined(CONFIG_OF)
	reg_res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (reg_res == NULL) {
		dev_err(dev, "failed to get register data from device tree");
		return -1;
	}
	sunxi_data->reg_base = reg_res->start;
	sunxi_data->reg_size = reg_res->end - reg_res->start + 1;
	sunxi_data->dev = dev;

	sunxi_data->irq_num = platform_get_irq_byname(pdev, "IRQGPU");
	if (sunxi_data->irq_num < 0) {
		dev_err(dev, "failed to get irq number from device tree");
		return -1;
	}
#endif /* defined(CONFIG_OF) */
	sunxi_data->regula = regulator_get_optional(dev, "gpu");
	if (!IS_ERR_OR_NULL(sunxi_data->regula))
		volt_val = regulator_get_voltage(sunxi_data->regula);

	if (get_clks_wrap(dev))
		return -1;

	sunxi_data->power_idle = 1;
	sunxi_data->dvfs = 1;
	sunxi_data->independent_power = 0;
	sunxi_data->soft_mode = 1;

	parse_dts(dev, sunxi_data);
	if (!sunxi_data->independent_power)
		sunxi_data->dvfs = 0;

	sunxi_get_ic_version(dev, &ic_version);
	dev_info(dev, "IC version: 0x%08x \n", ic_version);

	if (!sunxi_ic_version_ctrl(dev)) {
		sunxi_data->power_idle = 0;
	}

	sunxi_decide_pll(sunxi_data);
	spin_lock_init(&sunxi_data->lock);
	pm_runtime_enable(dev);

	sunxi_data->ppu_reg = ioremap(PPU_REG_BASE, PPU_REG_SIZE);
	sunxi_data->gpu_reg = ioremap(GPU_CLK_CTRL_REG, 4);

	reset_control_deassert(sunxi_data->clks.reset);
	clk_prepare_enable(sunxi_data->clks.bus);
	clk_prepare_enable(sunxi_data->clks.pll);
	clk_prepare_enable(sunxi_data->clks.core);
	sunxi_data->power_on = 0;

	val = readl(sunxi_data->ppu_reg + GPU_PD_STAT_REG);
	WARN_ON((val & GPU_PD_STAT_MASK) != GPU_PD_STAT_3D_MODE);
	if (!sunxi_data->soft_mode) {
		val = ppu_mode_switch(POWER_CTRL_MODE_HW, false);
		WARN_ON((val & POWER_CTRL_MODE_MASK) != POWER_CTRL_MODE_HW);
	} else {
		val = ppu_mode_switch(POWER_CTRL_MODE_SW, true);
		WARN_ON((val & POWER_CTRL_MODE_MASK) != POWER_CTRL_MODE_SW);
	}
#ifdef CONFIG_DEBUG_FS
	sunxi_create_debugfs();
#endif
	if (sysfs_create_group(&dev->kobj, &scene_ctrl_attribute_group)) {
		dev_err(dev, "sunxi sysfs group creation failed!\n");
	}
#if POWER_DEBUG
	sunxi_data->power_idle = 0;
	regulator_set_voltage(sunxi_data->regula, 900000, 900000);
#endif
	init_waitqueue_head(&dvfs_wq);

	dev_info(dev, "idle:%d dvfs:%d power:%d %s mode:%d volt:%u core:%lu\n",
		sunxi_data->power_idle, sunxi_data->dvfs, sunxi_data->independent_power,
		IS_ERR_OR_NULL(sunxi_data->regula)?"No":"Yes", sunxi_data->soft_mode,
		volt_val, clk_get_rate(sunxi_data->clks.core));

	return 0;
}

void sunxi_platform_term(void)
{
	pm_runtime_disable(sunxi_data->dev);
	kfree(sunxi_data);

	sunxi_data = NULL;
}
