/****************************************************************
*
*VERSION 1.0 Inital Version
*note: when swith to real ic,undef FPGA_SIM_CONFIG, vice versa.
*****************************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/keyboard.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/kernel.h>
#include <linux/pm.h>
#include <linux/init-input.h>
#include <linux/arisc/arisc.h>
#include <asm/irq.h>
#include <asm/io.h>
#include <mach/irqs.h>
#include <mach/hardware.h>
#include <mach/sunxi-smc.h>
#include "ir-keymap.h"
#include "sunxi-ir-rx.h"
#undef CONFIG_HAS_EARLYSUSPEND
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_SUNXI_ANYIR_SUPPORT
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include<linux/vmalloc.h>
#include "multi-ir.h"

static int sunxi_multi_ir_remap_code(unsigned int rawcode);

#define MAX_MAPPING_TABLE	(16)

struct multi_ir_info_t {

	struct mutex table_mutex; 	/* protect mapping_table */
	int mapping_cnt;
	struct mapping_table_t *mapping_table[MAX_MAPPING_TABLE];
};

struct cpus_wakeup_config_t
{
	struct work_struct work;
	int address;
	int powerkey;
};

static struct multi_ir_info_t multi_ir_info;
static struct cpus_wakeup_config_t cpus_wakeup;
#endif

#define REPORT_REPEAT_KEY_VALUE	(1)
#define SUNXI_REP_DELAY			(280)	/* in millisecond */
#define SUNXI_REP_PERIOD		(40)	/* in millisecond */

static struct clk *ir_clk;
static struct clk *ir_clk_source;

struct sunxi_ir_data {
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#else
#ifdef CONFIG_PM
	struct dev_pm_domain ir_pm_domain;
#endif
#endif
};
#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
static struct sunxi_ir_data *ir_data;
#endif

static struct ir_config_info ir_info = {
	.input_type = IR_TYPE,
};

struct ir_raw_buffer {
	unsigned long dcnt;                  		/*Packet Count*/
	#define	IR_RAW_BUF_SIZE		128
	unsigned char buf[IR_RAW_BUF_SIZE];
};

static unsigned int ir_cnt = 0;
static struct input_dev *ir_dev;
static struct timer_list *s_timer;
static unsigned long ir_code=0;
static int timer_used=0;
static struct ir_raw_buffer	ir_rawbuf;
static char ir_dev_name[] = "s_cir0";

static u32 debug_mask = 0x00;
#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	printk(fmt , ## arg)

static inline void ir_reset_rawbuffer(void)
{
	ir_rawbuf.dcnt = 0;
}

static inline void ir_write_rawbuffer(unsigned char data)
{
	if (ir_rawbuf.dcnt < IR_RAW_BUF_SIZE)
		ir_rawbuf.buf[ir_rawbuf.dcnt++] = data;
	else
		pr_err("ir_write_rawbuffer: IR Rx Buffer Full!!\n");
}

static inline unsigned char ir_read_rawbuffer(void)
{
	unsigned char data = 0x00;

	if (ir_rawbuf.dcnt > 0)
		data = ir_rawbuf.buf[--ir_rawbuf.dcnt];

	return data;
}

static inline int ir_rawbuffer_empty(void)
{
	return (ir_rawbuf.dcnt == 0);
}

static inline int ir_rawbuffer_full(void)
{
	return (ir_rawbuf.dcnt >= IR_RAW_BUF_SIZE);
}

static void ir_clk_cfg(void)
{

	unsigned long rate = 0; /* 3Mhz */

	ir_clk_source = clk_get(NULL, HOSC_CLK);
	if (!ir_clk_source || IS_ERR(ir_clk_source)) {
		pr_err("try to get ir_clk_source clock failed!\n");
		return;
	}

	rate = clk_get_rate(ir_clk_source);
	dprintk(DEBUG_INIT, "%s: get ir_clk_source rate %dHZ\n", __func__, (__u32)rate);

	ir_clk = clk_get(NULL, "cpurcir");
	if (!ir_clk || IS_ERR(ir_clk)) {
		pr_err("try to get ir clock failed!\n");
		return;
	}

	if(clk_set_parent(ir_clk, ir_clk_source))
		pr_err("%s: set ir_clk parent to ir_clk_source failed!\n", __func__);

	if (clk_set_rate(ir_clk, 3000000)) {
		pr_err("set ir clock freq to 3M failed!\n");
	}
	rate = clk_get_rate(ir_clk);
	dprintk(DEBUG_INIT, "%s: get ir_clk rate %dHZ\n", __func__, (__u32)rate);

	if (clk_prepare_enable(ir_clk)) {
			pr_err("try to enable ir_clk failed!\n");
	}

	return;
}

static void ir_clk_uncfg(void)
{

	if(NULL == ir_clk || IS_ERR(ir_clk)) {
		pr_err("ir_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(ir_clk);
		clk_put(ir_clk);
		ir_clk = NULL;
	}

	if(NULL == ir_clk_source || IS_ERR(ir_clk_source)) {
		pr_err("ir_clk_source handle is invalid, just return!\n");
		return;
	} else {
		clk_put(ir_clk_source);
		ir_clk_source = NULL;
	}
	return;
}
static int ir_sys_cfg(void)
{
	if (input_fetch_sysconfig_para(&(ir_info.input_type))) {
		pr_err("%s: ir_rx_fetch_sysconfig_para err.\n", __func__);
		return -1;
	}
	if (ir_info.ir_used == 0) {
		pr_err("*** ir_used set to 0 !\n");
		return -1;
	}

	return 0;
}

static void ir_setup_destroy(void)
{
	ir_clk_uncfg();
	return;
}

static void ir_mode_set(enum ir_mode set_mode)
{
	u32 ctrl_reg = 0;

	switch (set_mode) {
	case CIR_MODE_ENABLE:
		ctrl_reg = sunxi_smc_readl(IR_BASE+IR_CTRL_REG);
		ctrl_reg |= IR_CIR_MODE;
		break;
	case IR_MODULE_ENABLE:
		ctrl_reg = sunxi_smc_readl(IR_BASE+IR_CTRL_REG);
		ctrl_reg |= IR_ENTIRE_ENABLE;
		break;
	default:
		return;
	}
	sunxi_smc_writel(ctrl_reg, IR_BASE+IR_CTRL_REG);
}

static void ir_sample_config(enum ir_sample_config set_sample)
{
	u32 sample_reg = 0;

	sample_reg = sunxi_smc_readl(IR_BASE+IR_SPLCFG_REG);

	switch (set_sample) {
	case IR_SAMPLE_REG_CLEAR:
		sample_reg = 0;
		break;
	case IR_CLK_SAMPLE:
#ifdef FPGA_SIM_CONFIG
		sample_reg |= 0x3<<0;  /* Fsample = 24MHz/512 = 46875Hz (21.33us) */
#else
		sample_reg |= IR_SAMPLE_128;
#endif
		break;
	case IR_FILTER_TH:
		sample_reg |= IR_RXFILT_VAL;
		break;
	case IR_IDLE_TH:
		sample_reg |= IR_RXIDLE_VAL;
		break;
	case IR_ACTIVE_TH:
		sample_reg |= IR_ACTIVE_T;
		sample_reg |= IR_ACTIVE_T_C;
		break;
	default:
		return;
	}
	sunxi_smc_writel(sample_reg, IR_BASE+IR_SPLCFG_REG);
}

static void ir_signal_invert(void)
{
	u32 reg_value;
	reg_value = 0x1<<2;
	sunxi_smc_writel(reg_value, IR_BASE+IR_RXCFG_REG);
}

static void ir_irq_config(enum ir_irq_config set_irq)
{
	u32 irq_reg = 0;

	switch (set_irq) {
	case IR_IRQ_STATUS_CLEAR:
		sunxi_smc_writel(0xef, IR_BASE+IR_RXINTS_REG);
		return;
	case IR_IRQ_ENABLE:
		irq_reg = sunxi_smc_readl(IR_BASE+IR_RXINTE_REG);
		irq_reg |= IR_IRQ_STATUS;
		break;
	case IR_IRQ_FIFO_SIZE:
		irq_reg = sunxi_smc_readl(IR_BASE+IR_RXINTE_REG);
		irq_reg |= IR_FIFO_32;
		break;
	default:
		return;
	}
	sunxi_smc_writel(irq_reg, IR_BASE+IR_RXINTE_REG);
}

static void ir_reg_cfg(void)
{
	/* Enable IR Mode */
	ir_mode_set(CIR_MODE_ENABLE);

	/* Config IR Smaple Register */
	ir_sample_config(IR_SAMPLE_REG_CLEAR);
	ir_sample_config(IR_CLK_SAMPLE);
	ir_sample_config(IR_FILTER_TH);		/* Set Filter Threshold */
	ir_sample_config(IR_IDLE_TH); 		/* Set Idle Threshold */
	ir_sample_config(IR_ACTIVE_TH);         /* Set Active Threshold */

	/* Invert Input Signal */
	ir_signal_invert();

	/* Clear All Rx Interrupt Status */
	ir_irq_config(IR_IRQ_STATUS_CLEAR);

	/* Set Rx Interrupt Enable */
	ir_irq_config(IR_IRQ_ENABLE);
	ir_irq_config(IR_IRQ_FIFO_SIZE);	/* Rx FIFO Threshold = FIFOsz/2; */

	/* Enable IR Module */
	ir_mode_set(IR_MODULE_ENABLE);

	return;
}

static void ir_setup(void)
{
	dprintk(DEBUG_INIT, "ir_rx_setup: ir setup start!!\n");

	ir_code = 0;
	timer_used = 0;
	ir_reset_rawbuffer();
	ir_clk_cfg();
	ir_reg_cfg();

	dprintk(DEBUG_INIT, "ir_rx_setup: ir setup end!!\n");
	return;
}

static inline unsigned char ir_get_data(void)
{
	return (unsigned char)(sunxi_smc_readl(IR_BASE + IR_RXDAT_REG));
}

static inline unsigned long ir_get_intsta(void)
{
	return (sunxi_smc_readl(IR_BASE + IR_RXINTS_REG));
}

static inline void ir_clr_intsta(unsigned long bitmap)
{
	unsigned long tmp = sunxi_smc_readl(IR_BASE + IR_RXINTS_REG);

	tmp &= ~0xff;
	tmp |= bitmap&0xff;
	sunxi_smc_writel(tmp, IR_BASE + IR_RXINTS_REG);
}

static void print_err_code(unsigned char *buf, unsigned long dcnt)
{
	unsigned long i=0;
	printk("error code:\n");
	for(i = 0; i < dcnt; i++){
		printk("%d:%d  ",(buf[i]&0x80)>>7, buf[i]&0x7f);
		if((i+1)%6 == 0)
			printk("\n");
	}
}

static unsigned long ir_packet_handler(unsigned char *buf, unsigned long dcnt)
{
	unsigned long len;
	unsigned char val = 0x00;
	unsigned char last = 0x00;
	unsigned long code = 0;
	int bitCnt = 0;
	unsigned long i=0;
	unsigned int active_delay = 0;

	//print_hex_dump_bytes("--- ", DUMP_PREFIX_NONE, buf, dcnt);

	dprintk(DEBUG_DATA_INFO, "dcnt = %d \n", (int)dcnt);

	/* Find Lead '1' */
	active_delay = (IR_ACTIVE_T+1)*(IR_ACTIVE_T_C ? 128:1);
	dprintk(DEBUG_DATA_INFO, "%d active_delay = %d\n", __LINE__, active_delay);
	len = 0;
	len += (active_delay>>1);
	for (i=0; i<dcnt; i++) {
		val = buf[i];
		if (val & 0x80) {
			len += val & 0x7f;
		} else {
			if (len > IR_L1_MIN)
				break;

			len = 0;
		}
	}

	dprintk(DEBUG_DATA_INFO, "%d len = %ld\n", __LINE__, len);

	if ((val&0x80) || (len<=IR_L1_MIN)){
		dprintk(DEBUG_DATA_INFO, "start 1 error code\n" );
		goto error_code; /* Invalid Code */
	}

	/* Find Lead '0' */
	len = 0;
	for (; i<dcnt; i++) {
		val = buf[i];
		if (val & 0x80) {
			if(len > IR_L0_MIN)
				break;

			len = 0;
		} else {
			len += val & 0x7f;
		}
	}

	if ((!(val&0x80)) || (len<=IR_L0_MIN)){
		dprintk(DEBUG_DATA_INFO, "start 0 error code\n");
		goto error_code; /* Invalid Code */
	}


	/* go decoding */
	code = 0;  /* 0 for Repeat Code */
	bitCnt = 0;
	last = 1;
	len = 0;
	for (; i<dcnt; i++) {
		val = buf[i];
		if (last) {
			if (val & 0x80) {
				len += val & 0x7f;
			} else {
				if (len > IR_PMAX) {		/* Error Pulse */
					dprintk(DEBUG_DATA_INFO, "len > IR_PMAX\n");
					goto error_code;
				}
				last = 0;
				len = val & 0x7f;
			}
		} else {
			if (val & 0x80) {
				if (len > IR_DMAX){		/* Error Distant */
					dprintk(DEBUG_DATA_INFO, "len > IR_DMAX\n");
					goto error_code;
				} else {
					if (len > IR_DMID)  {
						/* data '1'*/
						code |= 1<<bitCnt;
					}
					bitCnt ++;
					if (bitCnt == 32)
						break;  /* decode over */
				}
				last = 1;
				len = val & 0x7f;
			} else {
				len += val & 0x7f;
			}
		}
	}
	return code;
error_code:
	if (unlikely(debug_mask & DEBUG_ERR))
		print_err_code(buf,dcnt);
	return IR_ERROR_CODE;
}

static int ir_code_valid(unsigned long code)
{
	unsigned long tmp1, tmp2;

#ifdef IR_CHECK_ADDR_CODE
	/* Check Address Value */
	if ((code&0xffff) != (IR_ADDR_CODE&0xffff))
		return 0; /* Address Error */

	tmp1 = code & 0x00ff0000;
	tmp2 = (code & 0xff000000)>>8;

	return ((tmp1^tmp2)==0x00ff0000);  /* Check User Code */
#else
	/* Do Not Check Address Value */
	tmp1 = code & 0x00ff00ff;
	tmp2 = (code & 0xff00ff00)>>8;

	//return ((tmp1^tmp2)==0x00ff00ff);
	return (((tmp1^tmp2) & 0x00ff0000)==0x00ff0000 );
#endif /* #ifdef IR_CHECK_ADDR_CODE */
}

static irqreturn_t ir_irq_service(int irqno, void *dev_id)
{
	unsigned long intsta = ir_get_intsta();

	dprintk(DEBUG_INT, "IR RX IRQ Serve\n");

	ir_clr_intsta(intsta);

	/*Read Data Every Time Enter this Routine*/
	{
		//unsigned long dcnt =  (ir_get_intsta()>>8) & 0x1f;
		unsigned long dcnt =  (ir_get_intsta()>>8) & 0x7f;
		unsigned long i = 0;

		/* Read FIFO */
		for (i=0; i<dcnt; i++) {
			if (ir_rawbuffer_full()) {

				ir_get_data();

			} else {
				ir_write_rawbuffer(ir_get_data());
			}
		}
	}

	if (intsta & IR_RXINTS_RXPE) {	 /* Packet End */
		unsigned long code;
		int code_valid;

		if (ir_rawbuffer_full()) {
			dprintk(DEBUG_INT, "ir_rx_irq_service: Raw Buffer Full!!\n");
			ir_rawbuf.dcnt = 0;
			return IRQ_HANDLED;
		}

		code = ir_packet_handler(ir_rawbuf.buf, ir_rawbuf.dcnt);
		ir_rawbuf.dcnt = 0;
		code_valid = ir_code_valid(code);

		dprintk(DEBUG_INT, "IR code = 0x%lx\n", code);

		if (timer_used) {
			if (code_valid) {  /* the pre-key is released */
				#ifdef CONFIG_SUNXI_ANYIR_SUPPORT
				input_report_key(ir_dev, sunxi_multi_ir_remap_code(ir_code), 0);
				#else
				input_report_key(ir_dev, ir_keycodes[(ir_code>>16)&0xff], 0);
				#endif
				input_sync(ir_dev);

				dprintk(DEBUG_INT, "IR KEY UP\n");

				ir_cnt = 0;
			}
			if ((code==IR_REPEAT_CODE)||(code_valid)) {	/* Error, may interfere from other sources */
				mod_timer(s_timer, jiffies + (HZ/5));
			}
		} else {
			if (code_valid) {
				mod_timer(s_timer, jiffies + (HZ/5));
				timer_used = 1;
			}
		}

		if (timer_used) {
			ir_cnt++;
			if (ir_cnt == 1) {
				if (code_valid)
					ir_code = code;  /* update saved code with a new valid code */

				dprintk(DEBUG_INT, "IR RAW CODE : %lu\n",(ir_code>>16)&0xff);
				#ifdef CONFIG_SUNXI_ANYIR_SUPPORT
				input_report_key(ir_dev, sunxi_multi_ir_remap_code(ir_code), 1);
				#else
				input_report_key(ir_dev, ir_keycodes[(ir_code>>16)&0xff], 1);
				#endif
				dprintk(DEBUG_INT, "IR CODE : %d\n",ir_keycodes[(ir_code>>16)&0xff]);

				input_sync(ir_dev);

				dprintk(DEBUG_INT, "IR KEY VALE %d\n",ir_keycodes[(ir_code>>16)&0xff]);

			}
		}
		dprintk(DEBUG_INT, "ir_rx_irq_service: Rx Packet End, code=0x%x, ir_code=0x%x, timer_used=%d \n", (int)code, (int)ir_code, timer_used);
	}
	if (intsta & IR_RXINTS_RXOF) {  /* FIFO Overflow */
		/* flush raw buffer */
		ir_reset_rawbuffer();

		dprintk(DEBUG_INT, "ir_rx_irq_service: Rx FIFO Overflow!!\n");
	}
	return IRQ_HANDLED;
}

static void ir_timer_handle(unsigned long arg)
{
	timer_used = 0;
	/* Time Out, means that the key is up */
	#ifdef CONFIG_SUNXI_ANYIR_SUPPORT
	input_report_key(ir_dev, sunxi_multi_ir_remap_code(ir_code), 0);
	#else
	input_report_key(ir_dev, ir_keycodes[(ir_code>>16)&0xff], 0);
	#endif

	input_sync(ir_dev);

	dprintk(DEBUG_INT, "IR KEY TIMER OUT UP\n");

	ir_cnt = 0;

	dprintk(DEBUG_INT, "ir_timer_handle: timeout \n");
}

/* ͣ���豸 */
#ifdef CONFIG_HAS_EARLYSUSPEND
static void sunxi_ir_early_suspend(struct early_suspend *h)
{
	dprintk(DEBUG_SUSPEND, "enter earlysuspend: sunxi_ir_rx_suspend. \n");

	disable_irq_nosync(IR_IRQNO);

	if(NULL == ir_clk || IS_ERR(ir_clk)) {
		printk("ir_clk handle is invalid, just return!\n");
		return;
	} else {
		clk_disable_unprepare(ir_clk);
	}
	return ;
}

/* ���»��� */
static void sunxi_ir_late_resume(struct early_suspend *h)
{
	unsigned int ir_event = 0;
#ifdef CONFIG_SUNXI_ANYIR_SUPPORT
	int code = sunxi_multi_ir_remap_code((cpus_wakeup.powerkey<<16)|cpus_wakeup.address);
#else
	int code = ir_info.power_key;
#endif

	dprintk(DEBUG_SUSPEND, "enter laterresume: sunxi_ir_rx_resume. \n");

	arisc_query_wakeup_source(&ir_event);
	dprintk(DEBUG_SUSPEND, "%s: event 0x%x\n", __func__, ir_event);
	if (CPUS_WAKEUP_IR&ir_event) {
		input_report_key(ir_dev, code, 1);
		input_sync(ir_dev);
		msleep(1);
		input_report_key(ir_dev, code, 0);
		input_sync(ir_dev);
	}

	ir_code = 0;
	timer_used = 0;
	ir_reset_rawbuffer();
	clk_prepare_enable(ir_clk);
	ir_reg_cfg();
	enable_irq(IR_IRQNO);

	return ;
}
#else
#ifdef CONFIG_PM
static int sunxi_ir_suspend(struct device *dev)
{

	dprintk(DEBUG_SUSPEND, "enter: sunxi_ir_rx_suspend. \n");

	disable_irq_nosync(IR_IRQNO);

	if(NULL == ir_clk || IS_ERR(ir_clk)) {
		pr_err("ir_clk handle is invalid, just return!\n");
		return -1;
	} else {
		clk_disable_unprepare(ir_clk);
	}
	return 0;
}

/* ���»��� */
static int sunxi_ir_resume(struct device *dev)
{
	unsigned int ir_event = 0;
#ifdef CONFIG_SUNXI_ANYIR_SUPPORT
	int code = sunxi_multi_ir_remap_code((cpus_wakeup.powerkey<<16)|cpus_wakeup.address);
#else
	int code = ir_info.power_key;
#endif

	dprintk(DEBUG_SUSPEND, "enter: sunxi_ir_rx_resume. \n");

	arisc_query_wakeup_source(&ir_event);
	dprintk(DEBUG_SUSPEND, "%s: event 0x%x\n", __func__, ir_event);
	if (CPUS_WAKEUP_IR&ir_event) {
		input_report_key(ir_dev, code, 1);
		input_sync(ir_dev);
		msleep(1);
		input_report_key(ir_dev, code, 0);
		input_sync(ir_dev);
	}

	ir_code = 0;
	timer_used = 0;
	ir_reset_rawbuffer();
	clk_prepare_enable(ir_clk);
	ir_reg_cfg();
	enable_irq(IR_IRQNO);

	return 0;
}
#endif
#endif

#ifdef CONFIG_SUNXI_ANYIR_SUPPORT

static int sunxi_multi_ir_major = -1;
static struct class *sunxi_multi_ir_class = NULL;
static struct device *sunxi_multi_ir_device = NULL;

static int sunxi_multi_ir_open(struct inode *inode, struct file *file)
{
	file->private_data = &multi_ir_info;
	return 0;
}

static int sunxi_multi_ir_release(struct inode *inode, struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static int sunxi_multi_ir_add_mapping(struct multi_ir_info_t *info, struct mapping_table_t *table)
{
	int i;
	struct mapping_table_t *p = NULL;

	/* lock and modify the mapping_table */
	mutex_lock(&info->table_mutex);

	for (i=0; i<info->mapping_cnt; i++) {
		if (info->mapping_table[i]->identity == table->identity) {
			p = info->mapping_table[i];
			break;
		}
	}

	if (p) {  											/* if existed, overwrite the old config */
		vfree(p);
		info->mapping_table[i] = table;
	} else {											/* else, add a new mapping */
		if (info->mapping_cnt==MAX_MAPPING_TABLE) {
			info->mapping_cnt--;
			vfree(info->mapping_table[info->mapping_cnt]);
		}
		info->mapping_table[info->mapping_cnt] = table;
		info->mapping_cnt++;
	}
	mutex_unlock(&info->table_mutex);

	pr_info("multi-ir: add mapping table, identity 0x%04x, powerkey 0x%02x\n", table->identity, table->powerkey);
	return 0;
}

static int sunxi_multi_ir_remove_all_mapping(struct multi_ir_info_t *info)
{
	int i;

	mutex_lock(&info->table_mutex);
	for (i=0; i<info->mapping_cnt; i++) {
		if (info->mapping_table[i]!=NULL) {
			vfree(info->mapping_table[i]);
			info->mapping_table[i] = NULL;
		}
	}
	info->mapping_cnt = 0;
	mutex_unlock(&info->table_mutex);

	return 0;
}

static int sunxi_multi_ir_remap_code(unsigned int rawcode)
{
	static struct mapping_table_t **prv_mapping = NULL;

	int identity = (rawcode & 0xffff);
	int code = (rawcode >> 16) & 0xff;
	int i, remap_code = KEY_RESERVED;

	if (!multi_ir_info.mapping_cnt)	/* if no mapping, return the raw code default */
		return code;

	if ( prv_mapping!=NULL && *prv_mapping!=NULL && (*prv_mapping)->identity == identity) {
		remap_code = (*prv_mapping)->value[code];
	} else {
		for (i=0; i<multi_ir_info.mapping_cnt; i++) {
			if (multi_ir_info.mapping_table[i]->identity == identity) {
				prv_mapping = &(multi_ir_info.mapping_table[i]);
				remap_code = (*prv_mapping)->value[code];

				/* when switch ir, do cpus wakup config */
				cpus_wakeup.address = identity;
				cpus_wakeup.powerkey = (*prv_mapping)->powerkey;
				schedule_work(&cpus_wakeup.work);
				break;
			}
		}
	}

	return remap_code;
}

static long sunxi_multi_ir_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	struct multi_ir_info_t *info = (struct multi_ir_info_t *)file->private_data;
	void __user *p = (void __user *)arg;
	int tmp;
	struct mapping_table_t *table;

	switch (cmd) {
		case MULTI_IR_IOC_REQ_MAP:
			tmp = MAX_MAPPING_TABLE;
			ret = copy_to_user(p, &tmp, sizeof(tmp));
			break;
		case MULTI_IR_IOC_SET_MAP:
			table = vmalloc(sizeof(*table));
			ret = copy_from_user(table, p, sizeof(*table));
			sunxi_multi_ir_add_mapping(info, table);
			break;
		default:
			break;
	}

	return ret;
}

void cpus_wakeup_config_handler(struct work_struct *work)
{
	struct cpus_wakeup_config_t *config = container_of(work, struct cpus_wakeup_config_t, work);
	pr_info("%s: address: 0x%04x, powerkey: 0x%02x\n", __func__, config->address, config->powerkey);

	arisc_config_ir_paras(config->powerkey, config->address);

	pr_info("%s: ok\n", __func__);
}

static const struct file_operations sunxi_multi_ir_fops = {
    .owner = THIS_MODULE,
	.open = sunxi_multi_ir_open,
	.unlocked_ioctl = sunxi_multi_ir_ioctl,
	.release = sunxi_multi_ir_release,
};

int sunxi_multi_ir_device_register(void)
{
	sunxi_multi_ir_major = register_chrdev(0, MULTI_IR_DEV_NAME, &sunxi_multi_ir_fops);
	if (sunxi_multi_ir_major<0) {
		pr_err("Failed to register multi ir device '%s'\n", MULTI_IR_DEV_NAME);
		return -1;
	}

	sunxi_multi_ir_class = class_create(THIS_MODULE, MULTI_IR_DEV_NAME);
	sunxi_multi_ir_device = device_create(sunxi_multi_ir_class, NULL,
								MKDEV(sunxi_multi_ir_major, 0), NULL, MULTI_IR_DEV_NAME);

	multi_ir_info.mapping_cnt = 0;

	INIT_WORK(&cpus_wakeup.work, cpus_wakeup_config_handler);
	mutex_init(&multi_ir_info.table_mutex);

	return 0;
}

void sunxi_multi_ir_device_unregister(void)
{
	sunxi_multi_ir_remove_all_mapping(&multi_ir_info);
	unregister_chrdev(sunxi_multi_ir_major, MULTI_IR_DEV_NAME);
	if (sunxi_multi_ir_class) {
		if (sunxi_multi_ir_device) device_destroy(sunxi_multi_ir_class, MKDEV(sunxi_multi_ir_major, 0));
		class_destroy(sunxi_multi_ir_class);
	}
}
#endif


static int __init ir_rx_init(void)
{
	int i,ret;

	if (ir_sys_cfg()!=0) {
		pr_err("ir_rx_init: sys_config exception, exit\n");
		return -EBUSY;
	}

	/* register input device for ir */
	ir_dev = input_allocate_device();
	if (!ir_dev) {
		pr_err("ir_dev: not enough memory for input device\n");
		return -ENOMEM;
	}

	ir_dev->name = "sunxi-ir";
	ir_dev->phys = "RemoteIR/input1";
	ir_dev->id.bustype = BUS_HOST;
	ir_dev->id.vendor = 0x0001;
	ir_dev->id.product = 0x0001;
	ir_dev->id.version = 0x0100;

    #ifdef REPORT_REPEAT_KEY_VALUE
	ir_dev->evbit[0] = BIT_MASK(EV_KEY)|BIT_MASK(EV_REP) ;
    #else
	ir_dev->evbit[0] = BIT_MASK(EV_KEY);
    #endif
	for (i = 0; i < 256; i++)
		set_bit(ir_keycodes[i], ir_dev->keybit);

	ret = input_register_device(ir_dev);
	if (ret) {
		input_free_device(ir_dev);
		pr_err("ir_rx_init: register input device exception, exit\n");
		return -EBUSY;
	}

	/* modify repeat delay */
	ir_dev->rep[REP_DELAY]  = SUNXI_REP_DELAY;
	ir_dev->rep[REP_PERIOD] = SUNXI_REP_PERIOD;

	/* pin and register config */
	ir_dev->dev.init_name = &ir_dev_name[0];
	ir_info.dev = &(ir_dev->dev);
	ret = input_init_platform_resource(&(ir_info.input_type));
	if (0 != ret) {
		pr_err("%s: config ir rx pin err.\n", __func__);
		goto err_pinctrl_get;
	}
	ir_setup();

	s_timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
	if (!s_timer) {
		ret =  - ENOMEM;
		pr_err(" IR FAIL TO  Request Timer\n");
		goto err_alloc_timer;
	}
	init_timer(s_timer);
	s_timer->function = &ir_timer_handle;

	if (request_irq(IR_IRQNO, ir_irq_service, 0, "RemoteIR_RX", ir_dev)) {
		ret = -EBUSY;
		goto err_request_irq;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	dprintk(DEBUG_INIT, "register_early_suspend\n");
	ir_data = kzalloc(sizeof(*ir_data), GFP_KERNEL);
	if (ir_data == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	ir_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ir_data->early_suspend.suspend = sunxi_ir_early_suspend;
	ir_data->early_suspend.resume	= sunxi_ir_late_resume;
	register_early_suspend(&ir_data->early_suspend);
#else
#ifdef CONFIG_PM
	ir_data = kzalloc(sizeof(*ir_data), GFP_KERNEL);
	if (ir_data == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	ir_data->ir_pm_domain.ops.suspend = sunxi_ir_suspend;
	ir_data->ir_pm_domain.ops.resume = sunxi_ir_resume;
	ir_dev->dev.pm_domain = &(ir_data->ir_pm_domain);
#endif
#endif

#ifdef CONFIG_SUNXI_ANYIR_SUPPORT
	sunxi_multi_ir_device_register();
#endif


	dprintk(DEBUG_INIT, "ir_rx_init end\n");
	return 0;

#if defined(CONFIG_HAS_EARLYSUSPEND) || defined(CONFIG_PM)
err_alloc_data_failed:
#endif
	free_irq(IR_IRQNO, ir_dev);
err_request_irq:
	kfree(s_timer);
err_alloc_timer:
	ir_setup_destroy();
	input_free_platform_resource(&(ir_info.input_type));
err_pinctrl_get:
	input_unregister_device(ir_dev);
	return ret;
}

static void __exit ir_rx_exit(void)
{

#ifdef CONFIG_SUNXI_ANYIR_SUPPORT
	sunxi_multi_ir_device_unregister();
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ir_data->early_suspend);
#endif

	free_irq(IR_IRQNO, ir_dev);
	kfree(s_timer);
	ir_setup_destroy();
	input_free_platform_resource(&(ir_info.input_type));
	input_unregister_device(ir_dev);
}

module_init(ir_rx_init);
module_exit(ir_rx_exit);
module_param_named(debug_mask, debug_mask, int, 0644);
MODULE_DESCRIPTION("Remote IR driver");
MODULE_AUTHOR("DanielWang");
MODULE_LICENSE("GPL");

