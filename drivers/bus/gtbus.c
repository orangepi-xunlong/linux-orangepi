/*
 * SUNXI GTBUS driver
 *
 * Copyright (C) 2014 AllWinnertech Ltd.
 * Author: xiafeng <xiafeng@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/io.h>
#include <linux/module.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/clk/sunxi_name.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/jiffies.h>
#include <linux/gtbus.h>

#include <mach/platform.h>

#include <asm/cacheflush.h>
#include <asm/irq_regs.h>
#include <asm/pmu.h>
#include <asm/smp_plat.h>

#define DRIVER_NAME             "GTBUS"
#define DRIVER_NAME_PMU         DRIVER_NAME"_PMU"

#define GT_MST_CFG_REG(n)               IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x0000 + (0x4 * n))) /* n = 0 ~ 35 */
#define GT_BW_WDW_CFG_REG               IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x0100))
#define GT_MST_READ_PROI_CFG_REG0       IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x0104))
#define GT_MST_READ_PROI_CFG_REG1       IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x0108))
#define GT_LVL2_MAST_CFG_REG            IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x010c))
#define GT_SW_CLK_ON_REG                IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x0110))
#define GT_SW_CLK_OFF_REG               IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x0114))
#define GT_PMU_MST_EN_REG               IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x0118))
#define GT_PMU_CFG_REG                  IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x011c))
#define GT_PMU_CNT_REG(n)               IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x0120 + (0x4 * n))) /* n = 0 ~ 18 */
/* generaly, the below registers are not needed to modify */
#define GT_CCI400_CONFIG_REG0           IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x0200))
#define GT_CCI400_CONFIG_REG1           IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x0204))
#define GT_CCI400_CONFIG_REG2           IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x0208))
#define GT_CCI400_STATUS_REG0           IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x020c))
#define GT_CCI400_STATUS_REG1           IO_ADDRESS((SUNXI_GTBUS_PBASE + 0x0210))
#define GT_RESOURCE_SIZE                (GT_CCI400_STATUS_REG1 - GT_MST_CFG_REG(0))

#define GT_MAX_PORTS            GT_PORT_CPUM0

/* Level 2 Arbiter Master Requeset Number */
#define GT_LVL2AB_MST7_RN       12 /* shirft, TS, DMA, CPUS, MP, NFC0~1, MSTG0~2 */
#define GT_LVL2AB_MST6_RN       8  /* shirft, USB0~2, HIS, SS, TH, GMAC, SATA, USB3 */
#define GT_LVL2AB_MST5_RN       4  /* shirft, VE, FD, CSI */
#define GT_LVL2AB_MST4_RN       0  /* shirft, FE0~2, BE0~2 */

#define GT_QOS_MAX    		0x03
#define GT_THD_MAX    		0x0fff
#define GT_BW_WDW_MAX    	0xffff

/* for register GT_MST_CFG_REG(n) */
#define GT_ENABLE_REQ   	(1<<31) /* mast clock on */
#define GT_DISABLE_REQ   	(1<<30) /* mast clock off */
#define GT_QOS_SHIFT    	28
#define GT_THD1_SHIFT    	16
#define GT_REQN_MAX    		0x0f /* max number master reques in one cycle */
#define GT_REQN_SHIFT    	12
#define GT_THD0_SHIFT    	0

/* for register GT_BW_WDW_CFG_REG */
#define GT_BW_QOSHPN_SHIFT      18
#define GT_BW_HPNEN_SHIFT       17
#define GT_BW_QOSEN_SHIFT       16

/* GTBUS PMU ids */
typedef enum gt_pmu{
	GT_PMU_CPU_DDR0 	= 0,  /* CPU bandwidth of DDR channel 0 */
	GT_PMU_GPU_DDR0 	= 1,  /* GPU bandwidth of DDR channel 0 */
	GT_PMU_DE_DDR0  	= 2,  /* FE & BE & IEP */
	GT_PMU_VCF_DDR0 	= 3,  /* VE & CSI & FD */
	GT_PMU_OTH_DDR0 	= 4,  /* other master */
	GT_PMU_CPU_DDR1 	= 5,  /* CPU bandwidth of DDR channel 1 */
	GT_PMU_GPU_DDR1 	= 6,  /* GPU bandwidth of DDR channel 1 */
	GT_PMU_DE_DDR1  	= 7,  /* FE & BE & IEP */
	GT_PMU_VCF_DDR1 	= 8,  /* VE & CSI & FD */
	GT_PMU_OTH_DDR1 	= 9,  /* other master */
	GT_PMU_CPUM1_LAT	= 10, /* CPU-M0 Latency(CCI-400_clk) */
	GT_PMU_CPUM2_LAT	= 11, /* CPU-M1 Latency(CCI-400_clk) */
	GT_PMU_GPUP0_LAT	= 12, /* GPU-Port 0 Latency */
	GT_PMU_GPUP1_LAT	= 13, /* GPU-Port 1 Latency */
	GT_PMU_CPUM1_CMN	= 14, /* CPU-M0 Command Number */
	GT_PMU_CPUM2_CMN	= 15, /* CPU-M1 Command Number */
	GT_PMU_GPUP0_CMN	= 16, /* GPU-Port 0 Command Number */
	GT_PMU_GPUP1_CMN	= 17, /* GPU-Port 1 Command Number */
	GT_PMU_GTB_CLKN 	= 18, /* gtb clock cycle number */
}gt_pmu_e;

#define GT_MAX_PMU   GT_PMU_GTB_CLKN
#define GT_PORT_QOS  (GT_MAX_PMU + 1)
#define GT_PORT_THD0 (GT_MAX_PMU + 2)
#define GT_PORT_THD1 (GT_MAX_PMU + 3)
#define GT_PORT_REQN (GT_MAX_PMU + 4)
#define GT_PORT_PRI  (GT_MAX_PMU + 5)

/* number of master & slave interface */
struct gt_nb_ports {
	unsigned int nb_ace;
	unsigned int nb_ace_lite;
	unsigned int nb_axi;
	unsigned int mbus;
	unsigned int ahb;
};

enum gt_port_type {
	ACE_INVALID_PORT = 0x0,
	ACE_PORT,
	ACE_LITE_PORT,
	AXI_PORT,
	MBUS_PORT,
	AHB_PORT,
};

struct sunxi_gt_port {
	void __iomem *base; /* SUNXI_GTBUS_VBASE */
	unsigned long phys; /* SUNXI_GTBUS_PBASE */
	enum gt_port_type type;
	struct device_node *dn;
};

static DEFINE_MUTEX(gt_seting);
static DEFINE_MUTEX(gt_pmureading);

/*
 * gt_pmu_getstate() - function to get a GT pmu state
 *
 * @port: index of the port to setup
 * @return: if true the pmu is enable, else is disable
 */
static bool gt_pmu_getstate(gt_pmu_e port)
{
	unsigned int value;

	mutex_lock(&gt_seting);
	switch(port){
		case GT_PMU_CPU_DDR0:
		case GT_PMU_CPU_DDR1:
		case GT_PMU_CPUM1_LAT:
		case GT_PMU_CPUM2_LAT:
		case GT_PMU_CPUM1_CMN:
		case GT_PMU_CPUM2_CMN:
			value = readl_relaxed(GT_PMU_MST_EN_REG);
			value &= (0x03 << 0);
			break;
		case GT_PMU_GPU_DDR0:
		case GT_PMU_GPU_DDR1:
		case GT_PMU_GPUP0_LAT:
		case GT_PMU_GPUP1_LAT:
		case GT_PMU_GPUP0_CMN:
		case GT_PMU_GPUP1_CMN:
			value = readl_relaxed(GT_PMU_CFG_REG);
			value &= (0x03 << 4);
			break;
		case GT_PMU_DE_DDR0:
		case GT_PMU_DE_DDR1:
			value = readl_relaxed(GT_PMU_MST_EN_REG);
			value &= (0x0ff << 4);
			break;
		case GT_PMU_VCF_DDR0:
		case GT_PMU_VCF_DDR1:
			value = readl_relaxed(GT_PMU_MST_EN_REG);
			value &= (0x0f << 12);
			break;
		case GT_PMU_OTH_DDR0:
		case GT_PMU_OTH_DDR1:
			value = readl_relaxed(GT_PMU_MST_EN_REG);
			value &= ((0x03 << 2) | (0xffff << 16));
			value |= (readl_relaxed(GT_PMU_CFG_REG) & (0x03 << 6));
			break;
		case GT_PMU_GTB_CLKN:
			value = readl_relaxed(GT_PMU_CFG_REG);
			value &= (0x01 << 0);
			break;
		default:
			value = 0;
			break;
	}
	mutex_unlock(&gt_seting);

	if(value)
		return true;
	return false;
}

/*
 * gt_pmu_enable() - function to set a GT pmu enable
 *
 * @port: index of the port to setup
 * @enable: if true enables the pmu, if false disables it
 */
static int gt_pmu_enable(gt_pmu_e port)
{
	unsigned int value;

	mutex_lock(&gt_seting);
	switch(port){
		case GT_PMU_CPU_DDR0:
		case GT_PMU_CPU_DDR1:
		case GT_PMU_CPUM1_LAT:
		case GT_PMU_CPUM2_LAT:
		case GT_PMU_CPUM1_CMN:
		case GT_PMU_CPUM2_CMN:
			value = readl_relaxed(GT_PMU_MST_EN_REG);
			value |= (0x03 << 0);
			writel_relaxed(value, GT_PMU_MST_EN_REG);
			break;
		case GT_PMU_GPU_DDR0:
		case GT_PMU_GPU_DDR1:
		case GT_PMU_GPUP0_LAT:
		case GT_PMU_GPUP1_LAT:
		case GT_PMU_GPUP0_CMN:
		case GT_PMU_GPUP1_CMN:
			value = readl_relaxed(GT_PMU_CFG_REG);
			value |= (0x03 << 4);
			writel_relaxed(value, GT_PMU_CFG_REG);
			break;
		case GT_PMU_DE_DDR0:
		case GT_PMU_DE_DDR1:
			value = readl_relaxed(GT_PMU_MST_EN_REG);
			value |= (0x0ff << 4);
			writel_relaxed(value, GT_PMU_MST_EN_REG);
			break;
		case GT_PMU_VCF_DDR0:
		case GT_PMU_VCF_DDR1:
			value = readl_relaxed(GT_PMU_MST_EN_REG);
			value |= (0x0f << 12);
			writel_relaxed(value, GT_PMU_MST_EN_REG);
			break;
		case GT_PMU_OTH_DDR0:
		case GT_PMU_OTH_DDR1:
			value = readl_relaxed(GT_PMU_MST_EN_REG);
			value |= ((0x03 << 2) | (0xffff << 16));
			writel_relaxed(value, GT_PMU_MST_EN_REG);
			value = readl_relaxed(GT_PMU_CFG_REG);
			value |= (0x03 << 6);
			writel_relaxed(value, GT_PMU_CFG_REG);
			break;
		case GT_PMU_GTB_CLKN:
			value = readl_relaxed(GT_PMU_CFG_REG);
			value |= (0x01 << 0);
			writel_relaxed(value, GT_PMU_CFG_REG);
			break;
		default:
			value = 0;
			break;
	}

	mutex_unlock(&gt_seting);

	return 0;
}

/*
 * gt_port_setreqn() - function to set a GT port reqst number
 *
 * @port: index of the port to setup
 * @reqn: the request number will be set
 */
int notrace gt_port_setreqn(gt_port_e port, unsigned int reqn)
{
	unsigned int value;

	if (port > GT_MAX_PORTS)
		return -ENODEV;

	if (reqn > GT_REQN_MAX)
		return -EPERM;

	mutex_lock(&gt_seting);
	value = readl_relaxed(GT_MST_CFG_REG(port));

	value &= ~(GT_REQN_MAX << GT_REQN_SHIFT);
	writel_relaxed(value | (reqn << GT_REQN_SHIFT), GT_MST_CFG_REG(port));
	mutex_unlock(&gt_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(gt_port_setreqn);

/*
 * gt_port_setthd() - function to set a GT port threshold 0 & 1
 *
 * @port: index of the port to setup
 * @thd0, thd1: thd1 should bigger than thd0
 */
int notrace gt_port_setthd(gt_port_e port, unsigned int thd0, unsigned int thd1)
{
	unsigned int value;

	if (port > GT_MAX_PORTS)
		return -ENODEV;

	if (thd0 < thd1) {
		dev_err(NULL,  "thd0 should bigger than thd1!\n");
		return -EPERM;
	}

	if ((thd0 > GT_THD_MAX) || (thd1 > GT_THD_MAX))
		return -EPERM;

	mutex_lock(&gt_seting);
	value = readl_relaxed(GT_MST_CFG_REG(port));

	value &= ~((GT_THD_MAX << GT_THD0_SHIFT) | (GT_THD_MAX << GT_THD1_SHIFT));
	writel_relaxed(value | (thd0 << GT_THD0_SHIFT) | (thd1 << GT_THD1_SHIFT), GT_MST_CFG_REG(port));
	mutex_unlock(&gt_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(gt_port_setthd);

/*
 * gt_port_setqos() - function to set a GT port QOS
 *
 * @port: index of the port to setup
 * @qos: the qos value want to set
 */
int notrace gt_port_setqos(gt_port_e port, unsigned int qos)
{
	unsigned int value;

	if (port > GT_MAX_PORTS)
		return -ENODEV;

	if (qos > GT_QOS_MAX)
		return -EPERM;
	mutex_lock(&gt_seting);
	value = readl_relaxed(GT_MST_CFG_REG(port));
	value &= ~(GT_QOS_MAX << GT_QOS_SHIFT);
	writel_relaxed(value | (qos << GT_QOS_SHIFT), GT_MST_CFG_REG(port));

	/* set QOS_OF_HPR for GPU specially if hpn used */
	if ((port == GT_PORT_GPU0) || (port == GT_PORT_GPU1)) {
		value = readl_relaxed(GT_BW_WDW_CFG_REG);
		if (value & GT_BW_HPNEN_SHIFT) {
			value &= ~(GT_QOS_MAX << GT_BW_QOSHPN_SHIFT);
			writel_relaxed(value | (qos << GT_BW_QOSHPN_SHIFT), GT_BW_WDW_CFG_REG);;
		}
	}
	mutex_unlock(&gt_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(gt_port_setqos);

/*
 * gt_bw_setbw() - function to set a GT BandWidth
 *
 * @bw: the bandwidth want to set
 */
int notrace gt_bw_setbw(unsigned int bw)
{
	unsigned int value;

	if (bw > GT_BW_WDW_MAX)
		return -EPERM;

	mutex_lock(&gt_seting);
	value = readl_relaxed(GT_BW_WDW_CFG_REG);

	value &= ~(GT_BW_WDW_MAX << 0);
	writel_relaxed(value | (bw << 0), GT_BW_WDW_CFG_REG);
	mutex_unlock(&gt_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(gt_bw_setbw);

/*
 * gt_bw_control() - function to set a GT BandWidth enable or disable
 *
 * @enable: if true enables the port, if false disables it
 */
int notrace gt_bw_enable(bool enable)
{
	unsigned int value;

	mutex_lock(&gt_seting);
	value = readl_relaxed(GT_BW_WDW_CFG_REG);

	writel_relaxed(enable ? (value | (1 << GT_BW_QOSEN_SHIFT)) :\
		 (value & ~(1 << GT_BW_QOSEN_SHIFT)), GT_BW_WDW_CFG_REG);
	mutex_unlock(&gt_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(gt_bw_enable);

/*
 * gt_priority_set() - function to set a GT master's priority
 *
 * @port: index of the port to setup
 * @pri: 0 : low, 1 : high
 */
int notrace gt_priority_set(gt_port_e port, unsigned int pri)
{
	unsigned int value;

	if (port > GT_MAX_PORTS)
		return -ENODEV;

	if (pri > 1)
		return -EPERM;

	mutex_lock(&gt_seting);
	if (port < 32) {
		value = readl_relaxed(GT_MST_READ_PROI_CFG_REG0);
		value &= ~(1 << port);
		writel_relaxed(value | (pri << port), GT_MST_READ_PROI_CFG_REG0);
	} else {
		port -= 32;
		value = readl_relaxed(GT_MST_READ_PROI_CFG_REG1);
		value &= ~(1 << port);
		writel_relaxed(value | (pri << port), GT_MST_READ_PROI_CFG_REG1);
	}
	mutex_unlock(&gt_seting);

	return 0;
}
EXPORT_SYMBOL_GPL(gt_priority_set);

/*
 * They are called by low-level power management code to disable slave
 * interfaces snoops and DVM broadcast.
 * Since they may execute with cache data allocation disabled and
 * after the caches have been cleaned and invalidated the functions provide
 * no explicit locking since they may run with D-cache disabled, so normal
 * cacheable kernel locks based on ldrex/strex may not work.
 * Locking has to be provided by BSP implementations to ensure proper
 * operations.
 */

/*
 * gt_port_control() - function to control a GT port,
 * generaly not be used.
 *
 * @port: index of the port to setup
 * @enable: if true enables the port, if false disables it
 */
static void notrace gt_port_control(gt_port_e port, bool enable)
{
	unsigned int value;

	/*
	 * This function is called from power down procedures
	 * and must not execute any instruction that might
	 * cause the processor to be put in a quiescent state
	 * (eg wfi). Hence, cpu_relax() can not be added to this
	 * read loop to optimize power, since it might hide possibly
	 * disruptive operations.
	 */
	mutex_lock(&gt_seting);
	value = readl_relaxed(GT_MST_CFG_REG(port));
	if (enable) {
		value &= ~GT_DISABLE_REQ;
	} else {
		if (GT_PORT_CPUM0 == port) {
			value |= GT_ENABLE_REQ; /* default is open */
		} else {
			value &= ~GT_ENABLE_REQ;
		}
		value |= GT_DISABLE_REQ;
	}
	writel_relaxed(value, GT_MST_CFG_REG(port));
	mutex_unlock(&gt_seting);
}

/*
 * gt_control_port_by_index() - function to control a GTBUS port by port index
 *
 * @port: port index previously retrieved with gt_ace_get_port()
 * @enable: if true enables the port, if false disables it
 *
 * Return:
 *	0 on success
 *	-ENODEV on port index out of range
 *	-EPERM if operation carried out on an ACE PORT
 */
int notrace gt_control_port_by_index(gt_port_e port, bool enable)
{
	if (port > GT_MAX_PORTS)
		return -ENODEV;
	/*
	 * GT control for ports connected to CPUS is extremely fragile
	 * and must be made to go through a specific and controlled
	 * interface (ie gt_disable_port_by_cpu(); control by general purpose
	 * indexing is therefore disabled for ACE ports.
	 */

	gt_port_control(port, enable);
	return 0;
}
EXPORT_SYMBOL_GPL(gt_control_port_by_index);


static int gt_probe(void)
{
	//int i;

	/*
	 * the purpose freq of GTBUS is 400M, and the default freq is 24M,
	 * PLL_PERIPH1 is needed for GTBUS, before PLL_PERIPH1 is set
	 * to 1200M, and the division of GTBUS is set to 3.
	 * at this time, clk source managemnet is not ok.
	 */
#if 0
	/* GTBUS clock has been set in uboot */
	writel_relaxed(0x00003200, IO_ADDRESS(SUNXI_CCM_PLL_PBASE + 0x2c));
	writel_relaxed((1U << 31) | readl(IO_ADDRESS(SUNXI_CCM_PLL_PBASE + 0x2c)), \
					IO_ADDRESS(SUNXI_CCM_PLL_PBASE + 0x2c)); /* set PLL_PERIPH1 to 1200M */
	mdelay(10); /* 10ms, waiting PLL_PERIPH1 become stable */

	writel_relaxed(0x00000002, IO_ADDRESS(SUNXI_CCM_PLL_PBASE + 0x5c)); /* set divide to 3 */
	udelay(100); //waiting GTBUS divide become valid
	writel_relaxed(0x02000002, IO_ADDRESS(SUNXI_CCM_PLL_PBASE + 0x5c)); /* switch clock source */
	udelay(100); //waiting GTBUS divide become stable
#endif

	/* all the port is default opened */

	/* set default bandwidth */
	//gt_bw_setbw(0x400); /* default value */
	gt_bw_enable(true);

	/* set default QOS */
	//for(i = 0; i <= GT_MAX_PORTS; i++)
		//gt_port_setqos(i, 2);

	/* set masters' request number */

	/* set masters' threshold0/1 */

	return 0;
}

static int gt_init_status = -EAGAIN;
static DEFINE_MUTEX(gt_proing);

static int gt_init(void)
{
	if (gt_init_status != -EAGAIN)
		return gt_init_status;

	mutex_lock(&gt_proing);
	if (gt_init_status == -EAGAIN)
		gt_init_status = gt_probe();
	mutex_unlock(&gt_proing);
	return gt_init_status;
}

/*
 * To sort out early init calls ordering a helper function is provided to
 * check if the GT driver has beed initialized. Function check if the driver
 * has been initialized, if not it calls the init function that probes
 * the driver and updates the return value.
 */
bool gt_probed(void)
{
	return gt_init() == 0;
}
EXPORT_SYMBOL_GPL(gt_probed);


static const struct gt_nb_ports gt_ports = {
	.nb_ace = 2,
	.nb_ace_lite = 3,
	.nb_axi = 2,
	.mbus = 2,
	.ahb = 2,
};

static const struct of_device_id arm_gt_pmu_matches[] = {
	{
		.compatible = "arm,gt-pmu",
	},
	{},
};

struct gt_data {
	struct device *hwmon_dev;
	struct mutex update_lock;
	bool valid;
	unsigned long last_updated;
	int kind;
};

static struct gt_data hw_gt_pmu;

static unsigned int gt_update_device(struct gt_data *data, gt_pmu_e port)
{
	unsigned int value = 0;

	mutex_lock(&data->update_lock);

	/* confirm the pmu is enabled */
	if(!gt_pmu_getstate(port))
		gt_pmu_enable(port);

	/* read pmu conter */
	value = readl_relaxed(GT_PMU_CNT_REG(port));
	data->last_updated = jiffies;
	data->valid = 1;

	mutex_unlock(&data->update_lock);

	return value;
}

static unsigned int gt_get_value(struct gt_data *data, unsigned int index, char *buf)
{
	unsigned int i, size = 0;
	unsigned int value;

	mutex_lock(&data->update_lock);
	switch (index) {
	case GT_PORT_QOS:
		for (i = 0; i <= GT_MAX_PORTS; i++) {
			value = readl_relaxed(GT_MST_CFG_REG(i));
			value >>= GT_QOS_SHIFT;
			value &= GT_QOS_MAX;
			size += sprintf(buf + size, "master%2d qos:%1d\n", i, value);
		}
		break;
	case GT_PORT_THD0:
		for (i = 0; i <= GT_MAX_PORTS; i++) {
			value = readl_relaxed(GT_MST_CFG_REG(i));
			value >>= GT_THD0_SHIFT;
			value &= GT_THD_MAX;
			size += sprintf(buf + size, \
					"master%2d threshold0:%4d\n", i, value);
		}
		break;
	case GT_PORT_THD1:
		for (i = 0; i <= GT_MAX_PORTS; i++) {
			value = readl_relaxed(GT_MST_CFG_REG(i));
			value >>= GT_THD1_SHIFT;
			value &= GT_THD_MAX;
			size += sprintf(buf + size, \
					"master%2d threshold1:%4d\n", i, value);
		}
		break;
	case GT_PORT_REQN:
		 for (i = 0; i <= GT_MAX_PORTS; i++) {
			value = readl_relaxed(GT_MST_CFG_REG(i));
			value >>= GT_REQN_SHIFT;
			value &= GT_REQN_MAX;
			size += sprintf(buf + size, "master%2d request number:%2d\n", i, value);
		}
		break;
	case GT_PORT_PRI:
		for (i = 0; i < 32; i++) {
			value = readl_relaxed(GT_MST_READ_PROI_CFG_REG0);
			value >>= i;
			value &= 1;
			size += sprintf(buf + size, "master%2d priority:%1d\n", i, value);
		}
		for(i = 32; i <= GT_MAX_PORTS; i++) {
			value = readl_relaxed(GT_MST_READ_PROI_CFG_REG1);
			value >>= (i - 32);
			value &= 1;
			size += sprintf(buf + size, "master%2d priority:%1d\n", i, value);
		}
		break;
	default:
		/* programmer goofed */
		WARN_ON_ONCE(1);
		value = 0;
		break;
	}
	mutex_unlock(&data->update_lock);

	return size;
}

static ssize_t gt_show_value(struct device *dev, struct device_attribute *da, char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	unsigned int len;

#if 0
	struct gt_data *data = &hw_gt_pmu;

	gt_update_device(&hw_gt_pmu, attr->index);
	return snprintf(buf, PAGE_SIZE, "%d\n", gt_get_value(data, attr->index));
#endif
	if (attr->index > GT_MAX_PMU) {
		len = gt_get_value(&hw_gt_pmu, attr->index, buf);
		len = (len < PAGE_SIZE) ? len : PAGE_SIZE;
		return len;
	}

	return snprintf(buf, PAGE_SIZE, "%d\n", gt_update_device(&hw_gt_pmu, attr->index));
}

static unsigned int gt_set_value(struct gt_data *data, unsigned int index,\
								gt_port_e port, unsigned int val)
{
	unsigned int value;

	mutex_lock(&data->update_lock);
	switch (index) {
	case GT_PORT_QOS:
		gt_port_setqos(port, val);
		break;
	case GT_PORT_THD0:
		value = readl_relaxed(GT_MST_CFG_REG(port));
		value &= ~(GT_THD_MAX << GT_THD0_SHIFT);
		writel_relaxed(value | (val << GT_THD0_SHIFT), GT_MST_CFG_REG(port));
		break;
	case GT_PORT_THD1:
		value = readl_relaxed(GT_MST_CFG_REG(port));
		value &= ~(GT_THD_MAX << GT_THD1_SHIFT);
		writel_relaxed(value | (val << GT_THD1_SHIFT), GT_MST_CFG_REG(port));
		break;
	case GT_PORT_REQN:
		 gt_port_setreqn(port, val);
		break;
	case GT_PORT_PRI:
		gt_priority_set(port, val);
		break;
	default:
		/* programmer goofed */
		WARN_ON_ONCE(1);
		value = 0;
		break;
	}
	mutex_unlock(&data->update_lock);

	return 0;
}

static ssize_t gt_store_value(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	int nr = to_sensor_dev_attr(attr)->index;
	unsigned long port, val;
	unsigned char buffer[64];
	unsigned char *pbuf, *pbufi;
	int err;

	if (strlen(buf) >= 64) {
		dev_err(dev, "arguments out of range!\n");
		return -EINVAL;
	}

	while(*buf == ' ') /* find the first unblank character */
		buf++;
	strncpy(buffer, buf, strlen(buf));

	pbufi = buffer;
	while(*pbufi != ' ') /* find the first argument */
		pbufi++;
	*pbufi = 0x0;
	pbuf = (unsigned char *)buffer;
	err = kstrtoul(pbuf, 10, &port);
	if (err < 0)
		return err;
	if (port > GT_MAX_PORTS) {
		dev_err(dev, "master is illegal\n");
		return -EINVAL;
	}

	pbuf = ++pbufi;
	while(*pbuf == ' ') /* remove extra space character */
		pbuf++;
	pbufi = pbuf;
	while((*pbufi != ' ') && (*pbufi != '\n'))
		pbufi++;
	*pbufi = 0x0;

	err = kstrtoul(pbuf, 10, &val);
	if (err < 0)
		return err;

	gt_set_value(&hw_gt_pmu, nr, (gt_port_e)port, (unsigned int)val);

	return count;
}

/* CPU bandwidth of DDR channel 0 */
static SENSOR_DEVICE_ATTR(pmu_cpuddr0, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_CPU_DDR0);

/* GPU bandwidth of DDR channel 0 */
static SENSOR_DEVICE_ATTR(pmu_gpuddr0, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_GPU_DDR0);

/* DE bandwidth of DDR channel 0 */
static SENSOR_DEVICE_ATTR(pmu_de_ddr0, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_DE_DDR0);

/* VE & CSI & FD bandwidth of DDR channel 0 */
static SENSOR_DEVICE_ATTR(pmu_vcfddr0, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_VCF_DDR0);

/* other master bandwidth of DDR channel 0 */
static SENSOR_DEVICE_ATTR(pmu_othddr0, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_OTH_DDR0);

/* CPU bandwidth of DDR channel 1 */
static SENSOR_DEVICE_ATTR(pmu_cpuddr1, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_CPU_DDR1);

/* GPU bandwidth of DDR channel 1 */
static SENSOR_DEVICE_ATTR(pmu_gpuddr1, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_GPU_DDR1);

/* DE bandwidth of DDR channel 1 */
static SENSOR_DEVICE_ATTR(pmu_de_ddr1, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_DE_DDR1);

/* VE & CSI & FD bandwidth of DDR channel 1 */
static SENSOR_DEVICE_ATTR(pmu_vcfddr1, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_VCF_DDR1);

/* other master bandwidth of DDR channel 1 */
static SENSOR_DEVICE_ATTR(pmu_othddr1, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_OTH_DDR1);

/* CPU-cluster 0 Latency(CCI-400_clk) */
static SENSOR_DEVICE_ATTR(pmu_cpum1lat, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_CPUM1_LAT);

/* CPU-cluster 1 Latency(CCI-400_clk) */
static SENSOR_DEVICE_ATTR(pmu_cpum2lat, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_CPUM2_LAT);

/* GPU-Port 0 Latency */
static SENSOR_DEVICE_ATTR(pmu_gpup0lat, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_GPUP0_LAT);

/* GPU-Port 1 Latency */
static SENSOR_DEVICE_ATTR(pmu_gpup1lat, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_GPUP1_LAT);

/* CPU-Cluster 0 Command Number */
static SENSOR_DEVICE_ATTR(pmu_cpum1cmn, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_CPUM1_CMN);

/* CPU-Cluster 1 Command Number */
static SENSOR_DEVICE_ATTR(pmu_cpum2cmn, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_CPUM2_CMN);

/* GPU-Port 0 Command Number */
static SENSOR_DEVICE_ATTR(pmu_gpup0cmn, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_GPUP0_CMN);

/* GPU-Port 1 Command Number */
static SENSOR_DEVICE_ATTR(pmu_gpup1cmn, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_GPUP1_CMN);

/* gtb clock cycle number */
static SENSOR_DEVICE_ATTR(pmu_gtclknum, S_IRUGO, gt_show_value, NULL,
			  GT_PMU_GTB_CLKN);

/* get all masterss' qos or set a master's qos */
static SENSOR_DEVICE_ATTR(port_qos, S_IRUGO | S_IWUSR, gt_show_value, gt_store_value,
			  GT_PORT_QOS);

/* get all masterss' threshold or set a master's threshold */
static SENSOR_DEVICE_ATTR(port_thd0, S_IRUGO | S_IWUSR, gt_show_value, gt_store_value,
			  GT_PORT_THD0);

/* get all masterss' threshold or set a master's threshold */
static SENSOR_DEVICE_ATTR(port_thd1, S_IRUGO | S_IWUSR, gt_show_value, gt_store_value,
			  GT_PORT_THD1);

/* get all masters' requeset number or set a master's number */
static SENSOR_DEVICE_ATTR(port_reqn, S_IRUGO | S_IWUSR, gt_show_value, gt_store_value,
			  GT_PORT_REQN);

/* get all masters' priority or set a master's priority */
static SENSOR_DEVICE_ATTR(port_prio, S_IRUGO | S_IWUSR, gt_show_value, gt_store_value,
			  GT_PORT_PRI);

/* pointers to created device attributes */
static struct attribute *gt_attributes[] = {
	&sensor_dev_attr_pmu_cpuddr0.dev_attr.attr,
	&sensor_dev_attr_pmu_gpuddr0.dev_attr.attr,
	&sensor_dev_attr_pmu_de_ddr0.dev_attr.attr,
	&sensor_dev_attr_pmu_vcfddr0.dev_attr.attr,
	&sensor_dev_attr_pmu_othddr0.dev_attr.attr,
	&sensor_dev_attr_pmu_cpuddr1.dev_attr.attr,
	&sensor_dev_attr_pmu_gpuddr1.dev_attr.attr,
	&sensor_dev_attr_pmu_de_ddr1.dev_attr.attr,
	&sensor_dev_attr_pmu_vcfddr1.dev_attr.attr,
	&sensor_dev_attr_pmu_othddr1.dev_attr.attr,
	&sensor_dev_attr_pmu_cpum1lat.dev_attr.attr,
	&sensor_dev_attr_pmu_cpum2lat.dev_attr.attr,
	&sensor_dev_attr_pmu_gpup0lat.dev_attr.attr,
	&sensor_dev_attr_pmu_gpup1lat.dev_attr.attr,
	&sensor_dev_attr_pmu_cpum1cmn.dev_attr.attr,
	&sensor_dev_attr_pmu_cpum2cmn.dev_attr.attr,
	&sensor_dev_attr_pmu_gpup0cmn.dev_attr.attr,
	&sensor_dev_attr_pmu_gpup1cmn.dev_attr.attr,
	&sensor_dev_attr_pmu_gtclknum.dev_attr.attr,
	&sensor_dev_attr_port_qos.dev_attr.attr,
	&sensor_dev_attr_port_thd0.dev_attr.attr,
	&sensor_dev_attr_port_thd1.dev_attr.attr,
	&sensor_dev_attr_port_reqn.dev_attr.attr,
	&sensor_dev_attr_port_prio.dev_attr.attr,
	NULL,
};

static const struct attribute_group gt_group = {
	.attrs = gt_attributes,
};


static int gt_pmu_probe(struct platform_device *pdev)
{
	int ret;
	unsigned int value;
	struct resource *res;

	/* dram type has been config by boot */
	value =  readl_relaxed(GT_PMU_CFG_REG);
	value |= 1; /* enable pmu */
	writel_relaxed(value, GT_PMU_CFG_REG);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		return -ENODEV;
	}

	if (!request_mem_region(res->start, resource_size(res), res->name)) {
		return -ENOMEM;
	}

	ret = sysfs_create_group(&pdev->dev.kobj, &gt_group);
	if (ret)
		return ret;

	hw_gt_pmu.hwmon_dev= hwmon_device_register_attr(&pdev->dev, "gtbus_pmu");
	if (IS_ERR(hw_gt_pmu.hwmon_dev)) {
		ret = PTR_ERR(hw_gt_pmu.hwmon_dev);
		goto out_err_gt;
	}

	hw_gt_pmu.last_updated = 0;
	hw_gt_pmu.valid = 0;
	mutex_init(&hw_gt_pmu.update_lock);

	dev_info(&(pdev->dev), "probed\n");

	return 0;

out_err_gt:
	dev_err(&(pdev->dev), "probed faild\n");
	sysfs_remove_group(&pdev->dev.kobj, &gt_group);

	return ret;
}

static int gt_pmu_remove(struct platform_device *pdev)
{
	hwmon_device_unregister(hw_gt_pmu.hwmon_dev);
	sysfs_remove_group(&pdev->dev.kobj, &gt_group);

	return 0;
}

#ifdef CONFIG_PM
static int sunxi_gtbus_suspend(struct device *dev)
{
	dev_info(dev, "gtbus suspend okay\n");

	return 0;
}

static int sunxi_gtbus_resume(struct device *dev)
{
	dev_info(dev, "gtbus resume okay\n");

	return 0;
}

static const struct dev_pm_ops sunxi_gtbus_pm_ops = {
	.suspend = sunxi_gtbus_suspend,
	.resume = sunxi_gtbus_resume,
};

#define SUNXI_GTBUS_PM_OPS (&sunxi_gtbus_pm_ops)
#else
#define SUNXI_GTBUS_PM_OPS NULL
#endif

/* GTBUS MEM resouces */
static struct resource gt_resource[] = {
	[0] = {
		.start = SUNXI_GTBUS_PBASE,
		.end   = SUNXI_GTBUS_PBASE + GT_RESOURCE_SIZE,
		.flags = IORESOURCE_MEM,
	},
};

static struct platform_device gt_pmu_device = {
	.name		    = DRIVER_NAME_PMU,
	.id		        = PLATFORM_DEVID_NONE,
	.num_resources	= ARRAY_SIZE(gt_resource),
	.resource	    = gt_resource,
};

static struct platform_driver gt_pmu_driver = {
	.driver = {
		   .name = DRIVER_NAME_PMU,
		   .owner = THIS_MODULE,
		   //.of_match_table = arm_gt_pmu_matches,
		   .pm = SUNXI_GTBUS_PM_OPS,
		  },
	.probe = gt_pmu_probe,
	.remove = gt_pmu_remove,
};


static int __init gt_platform_init(void)
{
	int ret;

	ret = platform_device_register(&gt_pmu_device);
	if (IS_ERR_VALUE(ret)) {
		dev_err(&gt_pmu_device.dev, "register sunxi gtbus platform device failed\n");
		goto dev_err;
	}

	ret = platform_driver_register(&gt_pmu_driver);
	if (IS_ERR_VALUE(ret)) {
		dev_err(&gt_pmu_device.dev, "register sunxi gtbus platform driver failed\n");
		goto drv_err;
	}

	return ret;

drv_err:
	platform_driver_unregister(&gt_pmu_driver);

dev_err:
	platform_device_unregister(&gt_pmu_device);

	return -EINVAL;
}

early_initcall(gt_init);
device_initcall(gt_platform_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SUNXI GTBUS support");
MODULE_AUTHOR("xiafeng");
