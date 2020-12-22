进入standby的时候，无任何master访问DRAM时，调用下面函数：
__dram_power_save_process();


唤醒时调用函数
__dram_power_up_process();

注意代码不能运行在DRAM中





给出我的测试函数仅供参考

#if defined DRAMC_STANDBY_TEST

u32 dramc_w_test(unsigned int dram_size,unsigned int test_length)
{
	/* DRAM Simple write_read test
	 * 2 ranks:  write_read rank0 and rank1;
	 * 1rank  : write_read rank0 base and half size;
	 * */
	unsigned int i;
	unsigned int half_size;
	unsigned int val;
	half_size = ((dram_size >> 1)<<20);
	for(i=0;i<test_length;i++)
	{
		mctl_write_w(0x12345678 + i,(DRAM_MEM_BASE_ADDR + i*4));//rank0
		mctl_write_w(0x87654321 + i,(DRAM_MEM_BASE_ADDR + half_size + i*4));//half size (rank1)
	}
}
u32 dramc_r_cmp_test(unsigned int dram_size,unsigned int test_length)
{
	unsigned int i;
	unsigned int half_size;
	unsigned int val;
	half_size = ((dram_size >> 1)<<20);
	for(i=0;i<test_length;i++)
	{
		val = mctl_read_w(DRAM_MEM_BASE_ADDR + half_size + i*4);
		if(val !=(0x87654321 + i))	/* Write last,read first */
		{
			dram_dbg("DRAM simple test FAIL.\n");
			dram_dbg("%x != %x at address %x\n",val,0x87654321 + i,DRAM_MEM_BASE_ADDR + half_size + i*4);
//			pattern_goto(i);
//			while(1);
			return DRAM_RET_FAIL;
		}
		val = mctl_read_w(DRAM_MEM_BASE_ADDR + i*4);
		if(val != (0x12345678+i))
		{
			dram_dbg("DRAM simple test FAIL.\n");
			dram_dbg("%x != %x at address %x\n",val,0x12345678 + i,DRAM_MEM_BASE_ADDR + i*4);
//			while(1);
			return DRAM_RET_FAIL;
//			pattern_goto(i+1);
		}
	}
//	dram_dbg("DRAM simple test OK\n");
	return DRAM_RET_OK;

u32 dramc_standby_test(u32 dram_size)
{
	u32 ret = 0;
	u32 i,test_length = 0x1000;
	u32 test_cycle = 1000000;
	printk("standby test start,test cycle is %d\n",test_cycle);
	dramc_w_test(dram_size,test_length);
	for(i=0;i<test_cycle;i++)
	{
		printk("test loop %d begin,",i);
		__dram_power_save_process();
		printk("standby entry ok,");
		dram_udelay(10000);
		__dram_power_up_process();
		printk("wake up ok,");
		ret = dramc_r_cmp_test(dram_size,test_length);
			if(ret)
			{
				printk("read compare test fail\n");
				return ret;
			}
			else
				printk("read compare test OK\n");
	}
	 if(ret)
		 printk("standby test fail!!\n");
	 else
		printk("standby test ok!!\n");
	 return ret;
}
#endif
