/*
 * Copyright (C) 2023 Allwinner Technology Limited. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * Author: zhengwanyu<zhengwanyu@allwinnertech.com>
 */
#include <linux/of.h>
#include <linux/interrupt.h>
#include <asm-generic/io.h>
#include <linux/delay.h>

#include "pvr_dvfs.h"
#include "sunxi_dvfs_ctrl.h"

union gpu_dvfs_ctrl_reg {
	u32 dwval;
	struct {
		u32 dvfs_ctrl_enable:1;
		u32 vol_ctrl_enable:1;
		u32 reserved1:6;
		u32 freq_ctrl_delay:8;
		u32 reserved2:8;
		u32 gpu_cur_vol:8;
	} bits;
};

union gpu_dvfs_err_reg {
	u32 dwval;
	struct {
		u32 volt_err:1;
		u32 reserved1:7;
		u32 vf_level:8;
		u32 reserved2:16;
	} bits;
};

union gpu_dvfs_dbg_reg {
	u32 dwval;
	struct {
		u32 dbg_en:1;
		u32 time_out:1;
		u32 reserved1:6;
		u32 vf_level:8;
		u32 dvfs_state:3;
		u32 reserved2:13;
	} bits;
};

union gpu_timeout_dbg_reg {
	u32 dwval;
	struct {
		u32 t_num:32;
	} bits;
};

union gpu_irq_ctrl_reg {
	u32 dwval;
	struct {
		u32 defs_exe:1;
		u32 reserved1:3;
		u32 dvfs_err:1;
		u32 reserved2:3;
		u32 dvfs_time_out:1;
		u32 reserved3:23;
	} bits;
};

union gpu_drm_ctrl_reg {
	u32 dwval;
	struct {
		u32 drm_bypass:1;
		u32 reserved1:7;
		u32 tb_bypass:1;
		u32 reserved2:23;
	} bits;
};

union gpu_mult_core_cnt_reg {
	u32 dwval;
	struct {
		u32 gpu_core_cnt:2;
		u32 reserved:30;
	} bits;
};

union gpu_mult_core_id_reg {
	u32 dwval;
	struct {
		u32 gpu_prm_core_id:4;
		u32 gpu_sec_core_id:4;
		u32 reserved1:24;
	} bits;
};

union gpu_vf_level_reg {
	u32 dwval;
	struct {
		u32 volt_value:8;
		u32 clk_div:8;
		u32 clk_src_sel:8;
		u32 fre_level:8;
	} bits;
};

union gpu_pwr_cmp_dly_reg {
	u32 dwval;
	struct {
		u32 pwr_dly_up:16;
		u32 pwr_dly_down:16;
	} bits;
};

union gpu_pwr_qchan_cfg_reg {
	u32 dwval;
	struct {
		u32 core0_sw_cfg_en:1;
		u32 core0_q_active:1;
		u32 core0_q_accept:1;
		u32 core0_q_deny:1;
		u32 core1_sw_cfg_en:1;
		u32 core1_q_active:1;
		u32 core1_q_accept:1;
		u32 core1_q_deny:1;
		u32 core0_clk_en:1;
		u32 reserved1:23;
	} bits;
};

struct gpu_glb_reg {
	union gpu_dvfs_ctrl_reg     dvfs_ctrl;        //0x00
	union gpu_dvfs_err_reg      dvfs_err;         //0x04
	union gpu_dvfs_dbg_reg      dvfs_dbg;         //0x08
	union gpu_timeout_dbg_reg   timeout_dbg;      //0x0c
	union gpu_irq_ctrl_reg      irq_ctrl;         //0x10
	union gpu_drm_ctrl_reg      drm_ctrl;         //0x14
	union gpu_mult_core_cnt_reg mult_core_cnt;    //0x18
	union gpu_mult_core_id_reg  mult_core_id;     //0x1c
	union gpu_vf_level_reg      vf_level[16];     //0x20~0x5c
	unsigned char 		    reserved[0x100 - 0x60];
	union gpu_pwr_cmp_dly_reg   pwr_cmp_dly;      //0x100
	union gpu_pwr_qchan_cfg_reg pwr_qchan_cfg;    //0x104
};

static struct gpu_glb_reg *dvfs_reg;
static u32 gpu_clk_rate;
static IMG_OPP *opp_table;
static u32 table_num;
static struct device *device;

static int sunxi_dvfs_ctrl_set_level(IMG_OPP *table, u32 size)
{
	u32 i = 0;
	u32 hwval = 0;


	for (i = 0; i < size; i++) {
		hwval = 0;
		hwval |= (i + 1) << 24;
		hwval |= (u32)(((u64)table[i].ui32Freq * 16) / gpu_clk_rate) << 8;
		hwval |= (table[i].ui32Volt - 500000) / 10000;
		dev_info(device, "logical hw value:0x%x\n", hwval);
		dvfs_reg->vf_level[i].dwval = hwval;
		msleep(1);
		dev_info(device, "logical hw value:0x%x after set\n", dvfs_reg->vf_level[i].dwval);



		/*dvfs_reg->vf_level[i].bits.fre_level = i + 1;
		dvfs_reg->vf_level[i].bits.clk_src_sel = 0;
		if (gpu_clk_rate >= table[i].ui32Freq) {
			dvfs_reg->vf_level[i].bits.clk_div
				= (u32)(((u64)table[i].ui32Freq * 16) / gpu_clk_rate);
			dev_info(device, "set clk_div:%d\n", dvfs_reg->vf_level[i].bits.clk_div);
		} else {
			dev_err(device, "[ERROR] OPP freq[%d] is bigger than gpu_clk_rate[%d]\n",
				table[i].ui32Freq, gpu_clk_rate);
			return -1;
		}
		dvfs_reg->vf_level[i].bits.volt_value = (table[i].ui32Volt - 500000) / 10000;*/

		dev_info(device, "level:%d gpu_clk_rate:%d freq:%d vol:%d   clk_div:%d volt:%d\n", i + 1, gpu_clk_rate, table[i].ui32Freq, table[i].ui32Volt, (u32)(((u64)table[i].ui32Freq * 16) / gpu_clk_rate),
			(table[i].ui32Volt - 500000) / 10000);

	}

	return 0;
}

static irqreturn_t sunxi_ctrl_irq_handler(int irq, void *data)
{
	int i = 0;

	dev_info(device, "%s\n", __func__);

	dvfs_reg->irq_ctrl.dwval = 0x0;
	dev_info(device, "dvfs_ctrl:0x%x\n", dvfs_reg->dvfs_ctrl.dwval);
	dev_info(device, "dvfs_err:0x%x\n", dvfs_reg->dvfs_err.dwval);

	dev_info(device, "dvfs_dbg:0x%x\n", dvfs_reg->dvfs_dbg.dwval);
	dev_info(device, "timeout_dbg:0x%x\n", dvfs_reg->timeout_dbg.dwval);
	dev_info(device, "irq_ctrl:0x%x\n", dvfs_reg->irq_ctrl.dwval);
	dev_info(device, "drm_ctrl:0x%x\n", dvfs_reg->drm_ctrl.dwval);
	dev_info(device, "mult_core_cnt:0x%x\n", dvfs_reg->mult_core_cnt.dwval);
	dev_info(device, "mult_core_id:0x%x\n", dvfs_reg->mult_core_id.dwval);


	for (i = 0; i < 16; i++) {
		dev_info(device, "vf_level[%d]:0x%x\n", i, dvfs_reg->vf_level[i].dwval);
	}
	dev_info(device, "pwr_cmp_dly:0x%x ", dvfs_reg->pwr_cmp_dly.dwval);
	dev_info(device, "pwr_qchan_cfg:0x%x ", dvfs_reg->pwr_qchan_cfg.dwval);


	dev_info(device, "current voltage:%d\n", dvfs_reg->dvfs_ctrl.bits.gpu_cur_vol);
	dev_info(device, "current vf level:%d\n", dvfs_reg->dvfs_err.bits.vf_level);
	dev_info(device, "voltage change:%s\n", dvfs_reg->dvfs_err.bits.volt_err ? "failed" : "success");

	//dbg
	dev_info(device, "dvfs state:%d\n", dvfs_reg->dvfs_dbg.bits.dvfs_state);
	dev_info(device, "time out vf_level:%d\n", dvfs_reg->dvfs_dbg.bits.vf_level);
	dev_info(device, "dbg dvfs timeout:%d\n", dvfs_reg->dvfs_dbg.bits.time_out);

	return IRQ_HANDLED;
}

int sunxi_dvfs_ctrl_init(struct device *dev,
		struct sunxi_dvfs_init_params *para)
{
	const struct property *prop;
	u32 nr = 0;
	int i = 0, ret = 0;
	const __be32 *prop_value;

	device = dev;
	gpu_clk_rate = para->clk_rate;
	dev_err(dev, "%s gpu_clk_rate:%d Hz\n", __func__, gpu_clk_rate);

	dvfs_reg = (struct gpu_glb_reg *)ioremap(para->reg_base, 0x110);

#if defined(SUNXI_DVFS_CTRL_ENABLE)
/*parse OPP and set opp_table*/
	prop = of_find_property(dev->of_node, "operating-points", NULL);
	if (!prop) {
		dev_err(dev, "failed to find of_node:operating-points");
		return -ENODEV;
	}

	if (!prop->value) {
		dev_err(dev, "of_node:operating-points has no value\n");
		return -ENODATA;
	}

	nr = prop->length / sizeof(u32);
	if (nr % 2) {
		dev_err(dev, "Invalid OPP table\n");
		return -EINVAL;
	}

	table_num = nr / 2;
	if (table_num == 0) {
		dev_err(dev, "NO OPP table numbers\n");
		return -EINVAL;
	}
	dev_info(dev, "OPP table has %d value, e.g. %d members\n", nr, table_num);

	opp_table = kzalloc(sizeof(IMG_OPP) * table_num, GFP_KERNEL);
	if (!opp_table) {
		dev_err(dev, "kzalloc for OPP failed!\n");
		return -EINVAL;
	}

	dev_info(dev, "opp table:\n");
	prop_value = prop->value;
	for (i = 0; i < table_num; i++) {
		opp_table[i].ui32Freq = be32_to_cpup(prop_value++) * 1000;
		opp_table[i].ui32Volt = be32_to_cpup(prop_value++);

		dev_info(dev, "freq:%u vol:%u\n", opp_table[i].ui32Freq, opp_table[i].ui32Volt);
	}

	/*if (opp_table[0].freq != gpu_clk_rate)
		clk_set_rate(); */
	ret = sunxi_dvfs_ctrl_set_level(opp_table, table_num);
	if (ret < 0) {
		dev_err(dev, "sunxi_dvfs_ctrl_set_level failed\n");
		return -1;
	}
	dev_info(dev, "sunxi_dvfs_ctrl_set_level\n");
/*end of parsing OPP*/
#endif
	ret  = request_irq(para->irq_no, sunxi_ctrl_irq_handler,
			IRQF_TRIGGER_HIGH, "gpu-dvfs", NULL);

	dev_info(dev, "request_irq\n");

	dvfs_reg->timeout_dbg.dwval = 0x1ff;
	//dvfs_reg->dvfs_dbg.bits.dbg_en = 1;
	dvfs_reg->dvfs_dbg.dwval = 0x1;

	dvfs_reg->irq_ctrl.dwval = 0x0;

	//dvfs_reg->pwr_cmp_dly.dwval = 0x00640064;

//enable devfs ctrl
	/* us / 31.25 */
/*	dvfs_reg->dvfs_ctrl.bits.freq_ctrl_delay = 0x10;
    msleep(1);
	dvfs_reg->dvfs_ctrl.bits.dvfs_ctrl_enable = 1;
    msleep(1);
	//dvfs_reg->dvfs_ctrl.bits.vol_ctrl_enable = 0;*/
	dvfs_reg->dvfs_ctrl.dwval = 0x1003;

	return 0;
}

/*void sunxi_dvfs_ctrl_exit(void)
{
	free_irq(para->irq_no, NULL);
	kfree(opp_table);
	opp_table = NULL;
	table_num = 0;
}
*/

int sunxi_dvfs_ctrl_get_opp_table(IMG_OPP **opp, u32 *num)
{
	if (opp_table && table_num) {
		*opp = opp_table;
		*num = table_num;
		return 0;
	}

	return -1;
}
