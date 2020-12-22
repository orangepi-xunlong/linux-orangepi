#include "pm_i.h"

//for io-measure time
#define PORT_E_CONFIG (0xf1c20890)
#define PORT_E_DATA (0xf1c208a0)
#define PORT_CONFIG PORT_E_CONFIG
#define PORT_DATA PORT_E_DATA

volatile int  print_flag = 0;

void busy_waiting(void)
{
#if 1
	volatile __u32 loop_flag = 1;
	while(1 == loop_flag);
	
#endif
	return;
}

/*
 * notice: when resume, boot0 need to clear the flag, 
 * in case the data in dram be destoryed result in the system is re-resume in cycle.
*/
void save_mem_flag(void)
{
#if 0
	__u32 saved_flag = *(volatile __u32 *)(PERMANENT_REG);
	saved_flag &= 0xfffffffe;
	saved_flag |= 0x00000001;
	*(volatile __u32 *)(PERMANENT_REG) = saved_flag;
#endif
	return;
}

/*
 * before enter suspend, need to clear mem flag, 
 * in case the flag is failed to clear by resume code 
 * 
*/
void clear_mem_flag(void)
{
#if 0
	__u32 saved_flag = *(volatile __u32 *)(PERMANENT_REG);
	saved_flag &= 0xfffffffe;
	*(volatile __u32 *)(PERMANENT_REG) = saved_flag;
#endif
	return ;
}

#if defined(CONFIG_ARCH_SUN8IW6P1) || defined(CONFIG_ARCH_SUN9IW1P1)
static volatile __r_prcm_pio_pad_hold *status_reg;
static __r_prcm_pio_pad_hold status_reg_tmp;
static volatile __r_prcm_pio_pad_hold *status_reg_pa; 
static __r_prcm_pio_pad_hold status_reg_pa_tmp; 
void mem_status_init(void)
{
	status_reg	= (volatile __r_prcm_pio_pad_hold *)(STANDBY_STATUS_REG);
	status_reg_pa	= (volatile __r_prcm_pio_pad_hold *)(STANDBY_STATUS_REG_PA);

	//init spinlock for sync
	hwspinlock_init(1);
}

void mem_status_init_nommu(void)
{
	status_reg	= (volatile __r_prcm_pio_pad_hold *)(STANDBY_STATUS_REG);
	status_reg_pa	= (volatile __r_prcm_pio_pad_hold *)(STANDBY_STATUS_REG_PA);

	//init spinlock for sync
	hwspinlock_init(0);
}

void mem_status_clear(void)
{
	int i = 1;

	status_reg_tmp.dwval = (*status_reg).dwval;
	if (!hwspin_lock_timeout(MEM_RTC_REG_HWSPINLOCK, 20000)) {
		while(i < STANDBY_STATUS_REG_NUM){
			status_reg_tmp.bits.reg_sel = i;
			status_reg_tmp.bits.data_wr = 0;
			(*status_reg).dwval = status_reg_tmp.dwval;
			status_reg_tmp.bits.wr_pulse = 0;
			(*status_reg).dwval = status_reg_tmp.dwval;
			status_reg_tmp.bits.wr_pulse = 1;
			(*status_reg).dwval = status_reg_tmp.dwval;
			status_reg_tmp.bits.wr_pulse = 0;
			(*status_reg).dwval = status_reg_tmp.dwval;
			i++;
	    	}
		hwspin_unlock(MEM_RTC_REG_HWSPINLOCK);
	}
}

void mem_status_exit(void)
{
	return ;
}

void save_mem_status(volatile __u32 val)
{
	if (!hwspin_lock_timeout(MEM_RTC_REG_HWSPINLOCK, 20000)) {
		status_reg_tmp.bits.reg_sel = 1;
		status_reg_tmp.bits.data_wr = val;
		(*status_reg).dwval = status_reg_tmp.dwval;
		status_reg_tmp.bits.wr_pulse = 0;
		(*status_reg).dwval = status_reg_tmp.dwval;
		status_reg_tmp.bits.wr_pulse = 1;
		(*status_reg).dwval = status_reg_tmp.dwval;
		status_reg_tmp.bits.wr_pulse = 0;
		(*status_reg).dwval = status_reg_tmp.dwval;
		hwspin_unlock(MEM_RTC_REG_HWSPINLOCK);
	}

	asm volatile ("dsb");
	asm volatile ("isb");
	return;
}

__u32 get_mem_status(void)
{
	int val = 0;
	status_reg_tmp.bits.reg_sel = 1;
    
	if (!hwspin_lock_timeout(MEM_RTC_REG_HWSPINLOCK, 20000)) {
		(*status_reg).dwval = status_reg_tmp.dwval;
		//read
		status_reg_tmp.dwval = (*status_reg).dwval;
		hwspin_unlock(MEM_RTC_REG_HWSPINLOCK);
	}
    
	val = status_reg_tmp.bits.data_rd;
	return (val);
}

void show_mem_status(void)
{
	int i = 1;
	int val = 0;
	while(i < STANDBY_STATUS_REG_NUM) {
		status_reg_tmp.bits.reg_sel = i;
	    
	    //write
	    if (!hwspin_lock_timeout(MEM_RTC_REG_HWSPINLOCK, 20000)) {
		    (*status_reg).dwval = status_reg_tmp.dwval;
		    //read
		    status_reg_tmp.dwval = (*status_reg).dwval;
		    hwspin_unlock(MEM_RTC_REG_HWSPINLOCK);
	    }
	   
	    val = status_reg_tmp.bits.data_rd;
	    printk("addr %x, value = %x. \n", \
		    (i), val);
	    i++;
	}
}

void save_mem_status_nommu(volatile __u32 val)
{
	if (!hwspin_lock_timeout_nommu(MEM_RTC_REG_HWSPINLOCK, 20000)) {
		status_reg_pa_tmp.bits.reg_sel = 1;
		status_reg_pa_tmp.bits.data_wr = val;
		(*status_reg_pa).dwval = status_reg_pa_tmp.dwval;
		status_reg_pa_tmp.bits.wr_pulse = 0;
		(*status_reg_pa).dwval = status_reg_pa_tmp.dwval;
		status_reg_pa_tmp.bits.wr_pulse = 1;
		(*status_reg_pa).dwval = status_reg_pa_tmp.dwval;
		status_reg_pa_tmp.bits.wr_pulse = 0;
		(*status_reg_pa).dwval = status_reg_pa_tmp.dwval;
		hwspin_unlock_nommu(MEM_RTC_REG_HWSPINLOCK);
	}

	return;
}

#elif defined(CONFIG_ARCH_SUN8I)
void mem_status_init(void)
{
    return  ;
}

void mem_status_init_nommu(void)
{
    return  ;
}

void mem_status_clear(void)
{
	int i = 0;

	while(i < STANDBY_STATUS_REG_NUM){
		*(volatile int *)(STANDBY_STATUS_REG + i*4) = 0x0;
		i++;
	}
	return	;

}

void mem_status_exit(void)
{
	return ;
}

void save_mem_status(volatile __u32 val)
{
	*(volatile __u32 *)(STANDBY_STATUS_REG  + 0x04) = val;
	asm volatile ("dsb");
	asm volatile ("isb");
	return;
}

__u32 get_mem_status(void)
{
	return *(volatile __u32 *)(STANDBY_STATUS_REG  + 0x04);
}

void show_mem_status(void)
{
	int i = 0;

	while(i < STANDBY_STATUS_REG_NUM){
		printk("addr %x, value = %x. \n", \
			(STANDBY_STATUS_REG + i*4), *(volatile int *)(STANDBY_STATUS_REG + i*4));
		i++;
	}
}

void save_mem_status_nommu(volatile __u32 val)
{
	*(volatile __u32 *)(STANDBY_STATUS_REG_PA  + 0x04) = val;
	return;
}

void save_cpux_mem_status_nommu(volatile __u32 val)
{
	*(volatile __u32 *)(STANDBY_STATUS_REG_PA  + 0x00) = val;
	return;
}

#endif


/*
 * notice: dependant with perf counter to delay.
 */
void io_init(void)
{
	//config port output
	*(volatile unsigned int *)(PORT_CONFIG)  = 0x111111;
	
	return;
}

void io_init_high(void)
{
	__u32 data;
	
	//set port to high
	data = *(volatile unsigned int *)(PORT_DATA);
	data |= 0x3f;
	*(volatile unsigned int *)(PORT_DATA) = data;

	return;
}

void io_init_low(void)
{
	__u32 data;

	data = *(volatile unsigned int *)(PORT_DATA);
	//set port to low
	data &= 0xffffffc0;
	*(volatile unsigned int *)(PORT_DATA) = data;

	return;
}

/*
 * set pa port to high, num range is 0-7;	
 */
void io_high(int num)
{
	__u32 data;
	data = *(volatile unsigned int *)(PORT_DATA);
	//pull low 10ms
	data &= (~(1<<num));
	*(volatile unsigned int *)(PORT_DATA) = data;
	delay_us(10000);
	//pull high
	data |= (1<<num);
	*(volatile unsigned int *)(PORT_DATA) = data;

	return;
}

