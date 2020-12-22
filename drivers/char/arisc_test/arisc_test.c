 /*
 * drivers/char/arisc_test/arisc_test.c
 * (C) Copyright 2010-2015
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * sunny <sunny@allwinnertech.com>
 *
 * sun6i arisc test driver
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 */

#include "arisc_test.h"
#include <asm/div64.h>
#include <linux/math64.h>

static unsigned int test_dvfs     = 0;
static unsigned int test_axp      = 0;
static unsigned int test_audio    = 0;
static unsigned int test_loopback = 0;
static unsigned int test_rsb      = 0;
static unsigned int test_p2wi     = 0;
static unsigned int test_chipid   = 0;
static unsigned int devaddr       = RSB_RTSADDR_AXP809;
static spinlock_t arisc_spinlock;
static struct arisc_audio_period play_period[4];
static struct arisc_audio_period capture_period[4];
static struct arisc_audio_period play_runtime_period;
static struct arisc_audio_period capture_runtime_period;

module_param(test_dvfs, uint, 0644);
module_param(test_axp, uint, 0644);
module_param(test_audio, uint, 0644);
module_param(test_loopback, uint, 0644);
module_param(test_rsb, uint, 0644);
module_param(test_p2wi, uint, 0644);
module_param(test_chipid, uint, 0644);
module_param(devaddr, uint, 0644);

MODULE_PARM_DESC(test_dvfs, "Enable testing dvfs function");
MODULE_PARM_DESC(test_axp, "Enable testing axp function");
MODULE_PARM_DESC(test_audio, "Enable testing audio function");
MODULE_PARM_DESC(test_loopback, "Enable testing loopback function");
MODULE_PARM_DESC(test_rsb, "Enable testing rsb function");
MODULE_PARM_DESC(test_p2wi, "Enable testing p2wi function");
MODULE_PARM_DESC(test_chipid, "Enable testing get chip id function");
MODULE_PARM_DESC(devaddr, "rsb device address");

static u64 __arisc_counter_read(void)
{
	volatile u32 low;
	volatile u32 high;
	u64    	     counter;

	spin_lock(&arisc_spinlock);
	//latch 64bit counter and wait ready for read
	low = readl((void __iomem*)CNT64_CTRL_REG);
	low |= (1<<1);
	writel(low, (void __iomem*)CNT64_CTRL_REG);
	while(readl((void __iomem*)CNT64_CTRL_REG) & (1<<1));

	low  = readl((void __iomem*)CNT64_LOW_REG);
	high  = readl((void __iomem*)CNT64_HIGH_REG);
	counter = high;
	counter = (counter << 32) + low;

	do_div(counter, 24);
	spin_unlock(&arisc_spinlock);

	return counter;
}

static int __arisc_dvfs_cb(void *arg)
{
	printk("dvfs fail once\n");
	return 0;
}

static void __arisc_dvfs_test(void)
{
	u32 i;
	u32 ret = 0;
	u64 total_time;
	u64 begin_time;
	u32 times;
	u32 freq_table[] = {
		120000,
		240000,
		360000,
		480000,
		600000,
		720000,
		840000,
		960000,
		840000,
		720000,
		600000,
		480000,
		360000,
		240000,
		120000,
	};

	printk("dvfs test pll1.\n");
	total_time = 0;
	times = sizeof(freq_table) / sizeof(unsigned int);
	for (i = 0; i < times; i++) {
		printk("dvfs request freq: %d\n", freq_table[i]);
		begin_time = __arisc_counter_read();
		ret |= arisc_dvfs_set_cpufreq(freq_table[i], ARISC_DVFS_PLL1, ARISC_DVFS_SYN, NULL, __arisc_dvfs_cb);
		total_time += (__arisc_counter_read() - begin_time);
	}

	if (ret == 0) {
    	/* dump time information */
    	printk("dvfs times %u: total time: %lluus, once: %lluus\n", times, total_time, 	div_u64(total_time, times));

    	/* test succeeded */
    	printk("dvfs test succeeded\n");
	} else {
	    /* test failed */
    	printk("dvfs test failed\n");
	}

#if defined CONFIG_ARCH_SUN9IW1P1
	printk("dvfs test pll2.\n");
	total_time = 0;
	times = sizeof(freq_table) / sizeof(unsigned int);
	for (i = 0; i < times; i++) {
		printk("dvfs request freq: %d\n", freq_table[i]);
		begin_time = __arisc_counter_read();
		ret |= arisc_dvfs_set_cpufreq(freq_table[i], ARISC_DVFS_PLL2, ARISC_DVFS_SYN, NULL, __arisc_dvfs_cb);
		total_time += (__arisc_counter_read() - begin_time);
	}

	if (ret == 0) {
		/* dump time information */
		printk("dvfs times %u: total time: %lluus, once: %lluus\n", times, total_time,	div_u64(total_time, times));

		/* test succeeded */
		printk("dvfs test succeeded\n");
	} else {
		/* test failed */
		printk("dvfs test failed\n");
	}
#endif
}

static int __arisc_audio_play_perdone_cb(void *arg)
{
	printk("audio play perdone\n");

	play_runtime_period = *(play_runtime_period.next);
	arisc_add_period(AUDIO_PLAY, play_runtime_period.period_addr);

	return 0;
}

static int __arisc_audio_capture_perdone_cb(void *arg)
{
	printk("audio capture perdone\n");

	capture_runtime_period = *(capture_runtime_period.next);
	arisc_add_period(AUDIO_CAPTURE, capture_runtime_period.period_addr);

	return 0;
}

static void __arisc_audio_test(void)
{
    u32 ret = 0;
    u32 i;
    arisc_audio_mem_t audio_mem[2];
    arisc_audio_tdm_t audio_tdm[2];
    u32 play_per_num, capture_per_num;

    printk("test audio interfaces begin...\n");

    /* register perdone callback functions */
    ret = arisc_audio_cb_register(AUDIO_PLAY, __arisc_audio_play_perdone_cb, NULL);
    if (ret != 0) {
        printk("register audio play perdone cb failed...\n");
    } else {
        printk("register audio play perdone cb success...\n");
    }

    ret = arisc_audio_cb_register(AUDIO_CAPTURE, __arisc_audio_capture_perdone_cb, NULL);
    if (ret != 0) {
        printk("register audio capture perdone cb failed...\n");
    } else {
        printk("register audio capture perdone cb success...\n");
    }

    /* configurate audio sram buffer and period parameters */
    audio_mem[AUDIO_PLAY].mode           = AUDIO_PLAY;
    audio_mem[AUDIO_PLAY].sram_base_addr = AUDIO_SRAM_BASE_PALY;
    audio_mem[AUDIO_PLAY].buffer_size    = AUDIO_SRAM_BUF_SIZE_32K;
    audio_mem[AUDIO_PLAY].period_size    = AUDIO_SRAM_PER_SIZE_08K;
    play_per_num = audio_mem[AUDIO_PLAY].buffer_size/audio_mem[AUDIO_PLAY].period_size;
    ret = arisc_buffer_period_paras(audio_mem[AUDIO_PLAY]);
    if (ret != 0) {
        printk("configurate audio play mem paras failed...\n");
    } else {
        printk("configurate audio play mem paras success...\n");
    }

    audio_mem[AUDIO_CAPTURE].mode           = AUDIO_CAPTURE;
    audio_mem[AUDIO_CAPTURE].sram_base_addr = AUDIO_SRAM_BASE_CAPTURE;
    audio_mem[AUDIO_CAPTURE].buffer_size    = AUDIO_SRAM_BUF_SIZE_32K;
    audio_mem[AUDIO_CAPTURE].period_size    = AUDIO_SRAM_PER_SIZE_08K;
    capture_per_num = audio_mem[AUDIO_CAPTURE].buffer_size/audio_mem[AUDIO_CAPTURE].period_size;
    ret = arisc_buffer_period_paras(audio_mem[AUDIO_CAPTURE]);
    if (ret != 0) {
        printk("configurate audio capture mem paras failed...\n");
    } else {
        printk("configurate audio capture mem paras success...\n");
    }

     /* init play_period */
    for (i = 0; i < play_per_num - 1; i++)
    {
        play_period[i].period_addr = audio_mem[AUDIO_PLAY].sram_base_addr +  \
                                            (audio_mem[AUDIO_PLAY].period_size * i);
        play_period[i].next = &play_period[i + 1];
    }
    play_period[play_per_num - 1].period_addr = audio_mem[AUDIO_PLAY].sram_base_addr + \
                                    (audio_mem[AUDIO_PLAY].period_size * (play_per_num - 1));
    play_period[play_per_num - 1].next = &play_period[0];

    /* init capture_period */
    for (i = 0; i < play_per_num - 1; i++)
    {
        capture_period[i].period_addr = audio_mem[AUDIO_CAPTURE].sram_base_addr + \
                                            (audio_mem[AUDIO_CAPTURE].period_size * i);
        capture_period[i].next = &capture_period[i + 1];
    }
    capture_period[capture_per_num - 1].period_addr = audio_mem[AUDIO_CAPTURE].sram_base_addr + \
                                    (audio_mem[AUDIO_CAPTURE].period_size * (capture_per_num - 1));
    capture_period[capture_per_num - 1].next = &capture_period[0];

    memset((void __force *)IO_ADDRESS(AUDIO_SRAM_BASE_PALY), 'a', AUDIO_SRAM_BUF_SIZE_32K);
    memset((void __force *)IO_ADDRESS(AUDIO_SRAM_BASE_CAPTURE), 'b', AUDIO_SRAM_BUF_SIZE_32K);

    /* configurate tdm parameters include samplerate and channel number */
    audio_tdm[AUDIO_PLAY].mode       = AUDIO_PLAY;
    audio_tdm[AUDIO_PLAY].samplerate = 48000;
    audio_tdm[AUDIO_PLAY].channum    = 2;
    ret = arisc_tdm_paras(audio_tdm[AUDIO_PLAY]);
    if (ret != 0) {
        printk("configurate audio play tdm paras failed...\n");
    } else {
        printk("configurate audio play tdm paras success...\n");
    }

    audio_tdm[AUDIO_CAPTURE].mode       = AUDIO_CAPTURE;
    audio_tdm[AUDIO_CAPTURE].samplerate = 48000;
    audio_tdm[AUDIO_CAPTURE].channum    = 2;
    ret = arisc_tdm_paras(audio_tdm[AUDIO_CAPTURE]);
    if (ret != 0) {
        printk("configurate audio capture tdm paras failed...\n");
    } else {
        printk("configurate audio capture tdm paras success...\n");
    }

    /* add period before start audio */
    for (i = 0; i < play_per_num; i++)
    {
        arisc_add_period(AUDIO_PLAY, play_period[i].period_addr);

    }

    for (i = 0; i < capture_per_num; i++)
    {
        arisc_add_period(AUDIO_CAPTURE, capture_period[i].period_addr);
    }

    /* init runtime_period */
    play_runtime_period = play_period[play_per_num - 1];
    capture_runtime_period = capture_period[capture_per_num - 1];

    /* start play */
    ret = arisc_audio_start(AUDIO_PLAY);
    if (ret != 0) {
        printk("start play failed...\n");
    } else {
        printk("start play success...\n");
    }

    /* start capture */
    ret = arisc_audio_start(AUDIO_CAPTURE);
    if (ret != 0) {
        printk("start capture failed...\n");
    } else {
        printk("start capture success...\n");
    }

    msleep(10000);

    /* stop capture */
    ret = arisc_audio_stop(AUDIO_CAPTURE);
    if (ret != 0) {
        printk("stop capture failed...\n");
    } else {
        printk("stop capture success...\n");
    }

    /* stop play */
    ret = arisc_audio_stop(AUDIO_PLAY);
    if (ret != 0) {
        printk("stop play failed...\n");
    } else {
        printk("stop play success...\n");
    }

	ret = memcmp((const void __force *)IO_ADDRESS(AUDIO_SRAM_BASE_PALY), \
			(const void __force *)IO_ADDRESS(AUDIO_SRAM_BASE_CAPTURE), AUDIO_SRAM_BUF_SIZE_32K);

    if (ret)
    {
        printk("audio test fail\n");
    }
    else
    {
        printk("audio test success\n");
    }

    printk("test audio interfaces end...\n");
}

static struct work_struct axp_work;

#if defined CONFIG_ARCH_SUN8IW1P1
static void __axp_irq_work(struct work_struct *work)
{
	u32 i;
	u8 addr[5];
	u8 data[5];
	struct arisc_p2wi_block_cfg cfg;

	printk("axp irq work running...\n");

	cfg.len  = 5;
    cfg.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	cfg.addr = addr;
	cfg.data = data;

	/* read out irq status and dump irq status
	 * only AW1655 has interrupt function
	 * AW1657 only has power output function
	 */
	printk("read axp irq status\n");
	for (i = 0; i < 5; i++) {
		addr[i] = 0x48 + i;
	}

	arisc_p2wi_read_block_data(&cfg);
	printk("axp irq status information:\n");
	for (i = 0; i < 5; i++) {
		printk("addr:0x%x, data:0x%x\n", addr[i], data[i]);
	}

	printk("clear axp status\n");
	arisc_p2wi_write_block_data(&cfg);

	printk("re-enable axp irq of arisc\n");
	arisc_enable_nmi_irq();

	printk("axp irq handle end\n");
}
#elif (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW6P1)|| (defined CONFIG_ARCH_SUN9IW1P1)
static void __axp_irq_work(struct work_struct *work)
{
	u32 i;
	u8 regaddr[5];
	u32 data[5];
	struct arisc_rsb_block_cfg cfg;

	printk("axp irq work running...\n");

	cfg.len  = 5;
	cfg.datatype = RSB_DATA_TYPE_BYTE;
	cfg.msgattr = ARISC_MESSAGE_ATTR_SOFTSYN;
	cfg.devaddr = devaddr;
	cfg.regaddr = regaddr;
	cfg.data = data;

	/* read out irq status and dump irq status
	 * only AW1655 has interrupt function
	 * AW1657 only has power output function
	 */
	printk("read axp irq status\n");
	for (i = 0; i < 5; i++) {
		regaddr[i] = 0x48 + i;
	}

	arisc_rsb_read_block_data(&cfg);
	printk("axp irq status information:\n");
	for (i = 0; i < 5; i++) {
		printk("devaddr:0x%x, regaddr:0x%x, data:0x%x\n", devaddr, regaddr[i], data[i]);
	}

	printk("clear axp status\n");
	arisc_rsb_read_block_data(&cfg);

	printk("re-enable axp irq of arisc\n");
	arisc_enable_nmi_irq();

	printk("axp irq handle end\n");
}
#endif


static int __arisc_axp_cb(void *arg)
{
	printk("axp irq coming...\n");

	(void)schedule_work(&axp_work);

	printk("axp irq handle end\n");
	return 0;
}

static void __arisc_loopback_test(void)
{
	u32 i;
	u32 ret = 0;
	u32 times;
	u64 total_time;
	u64 begin_time;

	total_time = 0;
	times = 100;
	for (i = 0; i < times; i++) {
		begin_time = __arisc_counter_read();
		ret |= arisc_message_loopback();
		total_time += (__arisc_counter_read() - begin_time);
	}

	if (ret == 0) {
		/* dump time information */
    	printk("loopback times %u: total time: %lluus, once: %lluus\n", times, total_time, div_u64(total_time, times));

    	/* test succeeded */
    	printk("loopback test succeeded\n");
	} else {
	    /* test failed */
    	printk("loopback test failed\n");
	}

}

static void __arisc_axp_test(void)
{
    printk("test axp write call-back begin...\n");
	/* test axp interrupt call-back */
	printk("test axp call-back begin...\n");
	INIT_WORK(&axp_work, __axp_irq_work);
	if(arisc_nmi_cb_register(NMI_INT_TYPE_PMU, __arisc_axp_cb, NULL)) {
		printk("test axp reg cb failed\n");
	}
	printk("test axp call-back succeeded\n");
}

static void __arisc_get_chipid_test(void)
{
	u32 i, j;
	u32 ret = 0;
	u32 times;
	u64 total_time;
	u64 begin_time;
	u8 chip_id[16];

	memset(chip_id, 0, 16);

	total_time = 0;
	times = 10;
	for (i = 0; i < times; i++) {
		begin_time = __arisc_counter_read();
		ret |= arisc_axp_get_chip_id(chip_id);
		total_time += (__arisc_counter_read() - begin_time);

		for (j = 0; j < 16; j++) {
			printk("chip_id[%d] = 0x%x\n", j, chip_id[j]);
		}
	}

	if (ret == 0) {
		/* dump time information */
    	printk("get chip id times %u: total time: %lluus, once: %lluus\n", times, total_time, div_u64(total_time, times));

    	/* test succeeded */
    	printk("get chip id test succeeded\n");
	} else {
	    /* test failed */
    	printk("get chip id test failed\n");
	}

}

#if defined CONFIG_ARCH_SUN8IW1P1
static void __arisc_p2wi_test(void)
{
	struct arisc_p2wi_block_cfg cfg;
	unsigned char regaddr_table[P2WI_TRANS_BYTE_MAX];
	unsigned char data_table[P2WI_TRANS_BYTE_MAX];
	unsigned int  len;
	int           ret;
	int           i;
	u64 total_time;
	u64 begin_time;

	/* test write regs */
	printk("test p2wi write regs begin...\n");

	len = P2WI_TRANS_BYTE_MAX;
	cfg.msgattr  = ARISC_MESSAGE_ATTR_SOFTSYN;
	cfg.addr  = regaddr_table;
	cfg.data     = data_table;

	for (i = 0; i < P2WI_TRANS_BYTE_MAX; i++) {
		regaddr_table[i] = 0x4 + i;
		data_table[i] = i;
	}
	for (len = 1; len <= P2WI_TRANS_BYTE_MAX; len++) {
		printk("p2wi write regs data:\n");
		for (i = 0; i < len; i++) {
			printk("regaddr:%x, data:%x\n", (unsigned int)regaddr_table[i],
									(unsigned int)data_table[i]);
		}
		cfg.len = len;

		begin_time = __arisc_counter_read();
		ret = arisc_p2wi_write_block_data(&cfg);
		total_time = (__arisc_counter_read() - begin_time);
		if (ret) {
			printk("test p2wi write failed, len = %d, ret = %d\n", len, ret);
		}
		printk("p2wi write regs data [len = %d] succeeded\n", len);
		printk("p2wi write regs time: %lluus\n", total_time);
	}
	printk("test p2wi write regs succeeded\n");

	/* test read regs */
	printk("test p2wi read regs begin...\n");
	len = P2WI_TRANS_BYTE_MAX;

	for (len = 1; len <= P2WI_TRANS_BYTE_MAX; len++) {
	    cfg.len = len;
		begin_time = __arisc_counter_read();
		ret = arisc_p2wi_read_block_data(&cfg);
		total_time = (__arisc_counter_read() - begin_time);
		if (ret) {
			printk("test p2wi read failed, len = %d, ret = %d\n", len, ret);
		}
		printk("read p2wi regs data:\n");
		for (i = 0; i < len; i++) {
			printk("devaddr:%x, regaddr:%x, data:%x\n", (unsigned int)devaddr, (unsigned int)regaddr_table[i],
									(unsigned int)data_table[i]);
		}
		printk("read p2wi regs data [len = %d] succeeded\n", len);
		printk("read p2wi regs time: %lluus\n", total_time);
	}
	printk("test p2wi read regs succeeded\n");
	printk("p2wi test succeeded\n");
}
#elif (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW6P1) || (defined CONFIG_ARCH_SUN9IW1P1)
static void __arisc_rsb_test(void)
{
	struct arisc_rsb_block_cfg cfg;
	unsigned char regaddr_table[RSB_TRANS_BYTE_MAX];
	unsigned int data_table[RSB_TRANS_BYTE_MAX];
	unsigned int  len;
	int           ret;
	int           i;
	u64 total_time;
	u64 begin_time;

	/* test write regs */
	printk("test rsb write regs begin...\n");

	len = RSB_TRANS_BYTE_MAX;
	cfg.datatype = RSB_DATA_TYPE_BYTE;
	cfg.devaddr  = devaddr;
	cfg.msgattr  = ARISC_MESSAGE_ATTR_SOFTSYN;
	cfg.regaddr  = regaddr_table;
	cfg.data     = data_table;

	for (i = 0; i < RSB_TRANS_BYTE_MAX; i++) {
		regaddr_table[i] = 0x4 + i;
		data_table[i] = i;
	}
	for (len = 1; len <= RSB_TRANS_BYTE_MAX; len++) {
		printk("rsb write regs data:\n");
		for (i = 0; i < len; i++) {
			printk("devaddr:%x, regaddr:%x, data:%x\n", (unsigned int)devaddr, (unsigned int)regaddr_table[i],
									(unsigned int)data_table[i]);
		}
		cfg.len = len;

		begin_time = __arisc_counter_read();
		ret = arisc_rsb_write_block_data(&cfg);
		total_time = (__arisc_counter_read() - begin_time);
		if (ret) {
			printk("test rsb write failed, len = %d, ret = %d\n", len, ret);
		}
		printk("rsb write regs data [len = %d] succeeded\n", len);
		printk("rsb write regs time: %lluus\n", total_time);
	}
	printk("test rsb write regs succeeded\n");

	/* test read regs */
	printk("test rsb read regs begin...\n");
	len = RSB_TRANS_BYTE_MAX;

	for (len = 1; len <= RSB_TRANS_BYTE_MAX; len++) {
	    cfg.len = len;
		begin_time = __arisc_counter_read();
		ret = arisc_rsb_read_block_data(&cfg);
		total_time = (__arisc_counter_read() - begin_time);
		if (ret) {
			printk("test rsb read failed, len = %d, ret = %d\n", len, ret);
		}
		printk("read rsb regs data:\n");
		for (i = 0; i < len; i++) {
			printk("devaddr:%x, regaddr:%x, data:%x\n", (unsigned int)devaddr, (unsigned int)regaddr_table[i],
									(unsigned int)data_table[i]);
		}
		printk("read rsb regs data [len = %d] succeeded\n", len);
		printk("read rsb regs time: %lluus\n", total_time);
	}
	printk("test rsb read regs succeeded\n");
	printk("rsb test succeeded\n");
}
#endif

/**
 * __arisc_test_thread - dma test main thread
 * @arg:	thread arg, not used
 *
 * Returns 0 if success, the err line number if failed.
 */
static int __arisc_test_thread(void * arg)
{
	printk("arisc test thread...\n");

	if (test_loopback == 1) {
		printk("arisc message loopback test begin....\n");
		__arisc_loopback_test();
		printk("arisc message loopback test end....\n");
	}

	if (test_dvfs == 1) {
		printk("dvfs test begin....\n");
		__arisc_dvfs_test();
		printk("dvfs test end....\n");
	}

	if (test_axp == 1) {
		printk("axp test begin....\n");
		__arisc_axp_test();
		printk("axp test end....\n");
	}

#if defined CONFIG_ARCH_SUN8IW1P1
	if (test_p2wi == 1) {
		printk("axp p2wi begin....\n");
		__arisc_p2wi_test();
		printk("axp p2wi end....\n");
	}
#elif (defined CONFIG_ARCH_SUN8IW3P1) || (defined CONFIG_ARCH_SUN8IW5P1) || (defined CONFIG_ARCH_SUN8IW6P1) || (defined CONFIG_ARCH_SUN9IW1P1)
	if (test_rsb == 1) {
		printk("axp rsb begin....\n");
		__arisc_rsb_test();
		printk("axp rsb end....\n");
	}
#endif

	if (test_chipid) {
		printk("get chip id test begin....\n");
		__arisc_get_chipid_test();
		printk("get chip id test end....\n");
	}

	if (test_audio == 1) {
		printk("audio test begin....\n");
		__arisc_audio_test();
		printk("audio test end....\n");
	}

	return 0;
}

/**
 * sw_arisc_test_init - enter the dma test module
 */
static int __init sw_arisc_test_init(void)
{
	printk(">>>>arisc test driver init enter<<<<<\n");

	spin_lock_init(&(arisc_spinlock));

	/*
	 * create the test thread
	 */
	//kernel_thread(__arisc_test_thread, NULL, CLONE_FS | CLONE_SIGHAND);
	__arisc_test_thread(NULL);
	printk("arisc test driver test finished\n");
	return 0;
}

/**
 * sw_arisc_test_exit - exit the dma test module
 */
static void __exit sw_arisc_test_exit(void)
{
	printk("sw_arisc_test_exit: enter\n");

	if (test_axp == 1) {
		/* cancel axp work */
		printk("cancel axp work\n");
	    cancel_work_sync(&axp_work);
	    printk("unregister arisc axp callback\n");
	    arisc_nmi_cb_unregister(NMI_INT_TYPE_PMU, __arisc_axp_cb);
		printk("axp test eixt finish\n");
	}
}

module_init(sw_arisc_test_init);
module_exit(sw_arisc_test_exit);
MODULE_LICENSE     ("GPL");
MODULE_AUTHOR      ("superm");
MODULE_DESCRIPTION ("sunxi arisc test driver code");
