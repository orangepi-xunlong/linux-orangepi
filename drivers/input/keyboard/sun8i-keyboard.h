#ifndef _SUN8I_KEYBOARD_H
#define _SUN8I_KEYBOARD_H
/* just for test */
#ifdef CONFIG_AW_FPGA_V4_PLATFORM
#define SW_INT_IRQNO_LRADC      (11 + 32)
#else
#ifdef CONFIG_ARCH_SUN8IW6P1
#define SW_INT_IRQNO_LRADC      (74)
#else
#define SW_INT_IRQNO_LRADC      (62)
#endif
#endif

#define INPUT_DEV_NAME          ("sunxi-keyboard")

#define KEY_MAX_CNT             (13)
 
#ifdef CONFIG_ARCH_SUN9IW1P1
#define KEY_BASSADDRESS         (0xf6001800)
#elif defined(CONFIG_ARCH_SUN8IW6P1)
#define KEY_BASSADDRESS         (0xf1f03c00)
#elif defined(CONFIG_ARCH_SUN8IW7P1)
#define KEY_BASSADDRESS         (0xf1c21800)
#else
#define KEY_BASSADDRESS         (0xf1c22800)
#endif
#define LRADC_CTRL              (0x00)
#define LRADC_INTC              (0x04)
#define LRADC_INT_STA           (0x08)
#define LRADC_DATA0             (0x0c)
#define LRADC_DATA1             (0x10)

#define FIRST_CONCERT_DLY       (0<<24)
#define CHAN                    (0x3)
#define ADC_CHAN_SELECT         (CHAN<<22)
#define LRADC_KEY_MODE          (0)
#define KEY_MODE_SELECT         (LRADC_KEY_MODE<<12)
#define LEVELB_VOL              (0<<4)
#define LRADC_HOLD_EN           (1<<6)
#define LRADC_SAMPLE_32HZ       (3<<2)
#define LRADC_SAMPLE_62HZ       (2<<2)
#define LRADC_SAMPLE_125HZ      (1<<2)
#define LRADC_SAMPLE_250HZ      (0<<2)
#define LRADC_EN                (1<<0)

#define LRADC_ADC1_UP_EN        (1<<12)
#define LRADC_ADC1_DOWN_EN      (1<<9)
#define LRADC_ADC1_DATA_EN      (1<<8)

#define LRADC_ADC0_UP_EN        (1<<4)
#define LRADC_ADC0_DOWN_EN      (1<<1)
#define LRADC_ADC0_DATA_EN      (1<<0)

#define LRADC_ADC1_UPPEND       (1<<12)
#define LRADC_ADC1_DOWNPEND     (1<<9)
#define LRADC_ADC1_DATAPEND     (1<<8)


#define LRADC_ADC0_UPPEND       (1<<4)
#define LRADC_ADC0_DOWNPEND     (1<<1)
#define LRADC_ADC0_DATAPEND     (1<<0)

#define EVB
//#define CUSTUM
#define ONE_CHANNEL
#define MODE_0V2
//#define MODE_0V15
//#define TWO_CHANNEL
#ifdef MODE_0V2
/* standard of key maping 
 * 0.2V mode
 */					
#define REPORT_START_NUM            (2)
#define REPORT_KEY_LOW_LIMIT_COUNT  (1)
#define MAX_CYCLE_COUNTER           (100)
//#define REPORT_REPEAT_KEY_BY_INPUT_CORE
//#define REPORT_REPEAT_KEY_FROM_HW
#define INITIAL_VALUE               (0Xff)
#endif

#ifdef CONFIG_ARCH_SUN8IW1P1
static unsigned int sunxi_scankeycodes[KEY_MAX_CNT] = {
	[0 ] = KEY_VOLUMEUP,
	[1 ] = KEY_VOLUMEDOWN,
	[2 ] = KEY_MENU,
	[3 ] = KEY_ENTER,
	[4 ] = KEY_HOME,
	[5 ] = KEY_RESERVED,
	[6 ] = KEY_RESERVED,
	[7 ] = KEY_RESERVED,
	[8 ] = KEY_RESERVED,
	[9 ] = KEY_RESERVED,
	[10] = KEY_RESERVED,
	[11] = KEY_RESERVED,
	[12] = KEY_RESERVED,
};
#elif defined(CONFIG_ARCH_SUN8IW3P1)
static unsigned int sunxi_scankeycodes[KEY_MAX_CNT] = {
	[0 ] = KEY_VOLUMEUP,
	[1 ] = KEY_VOLUMEDOWN,
	[2 ] = KEY_HOME,
	[3 ] = KEY_ENTER,
	[4 ] = KEY_MENU,
	[5 ] = KEY_RESERVED,
	[6 ] = KEY_RESERVED,
	[7 ] = KEY_RESERVED,
	[8 ] = KEY_RESERVED,
	[9 ] = KEY_RESERVED,
	[10] = KEY_RESERVED,
	[11] = KEY_RESERVED,
	[12] = KEY_RESERVED,
};
#elif defined(CONFIG_ARCH_SUN8IW5P1)
static unsigned int sunxi_scankeycodes[KEY_MAX_CNT] = {
	[0 ] = KEY_VOLUMEUP,
	[1 ] = KEY_VOLUMEDOWN,
	[2 ] = KEY_HOME,
	[3 ] = KEY_MENU,
	[4 ] = KEY_ENTER,
	[5 ] = KEY_RESERVED,
	[6 ] = KEY_RESERVED,
	[7 ] = KEY_RESERVED,
	[8 ] = KEY_RESERVED,
	[9 ] = KEY_RESERVED,
	[10] = KEY_RESERVED,
	[11] = KEY_RESERVED,
	[12] = KEY_RESERVED,
};
#else
static unsigned int sunxi_scankeycodes[KEY_MAX_CNT] = {
	[0 ] = KEY_VOLUMEUP,
	[1 ] = KEY_VOLUMEDOWN,
	[2 ] = KEY_MENU,
	[3 ] = KEY_ENTER,
	[4 ] = KEY_HOME,
	[5 ] = KEY_RESERVED,
	[6 ] = KEY_RESERVED,
	[7 ] = KEY_RESERVED,
	[8 ] = KEY_RESERVED,
	[9 ] = KEY_RESERVED,
	[10] = KEY_RESERVED,
	[11] = KEY_RESERVED,
	[12] = KEY_RESERVED,
};
#endif

enum key_mode {
	CONCERT_DLY_SET = (1<<0),
	ADC_CHAN_SET = (1<<1),
	KEY_MODE_SET = (1<<2),
	LRADC_HOLD_SET = (1<<3),
	LEVELB_VOL_SET = (1<<4),
	LRADC_SAMPLE_SET = (1<<5),
	LRADC_EN_SET = (1<<6),
};

enum int_mode {
	ADC0_DOWN_INT_SET = (1<<0),
	ADC0_UP_INT_SET = (1<<1),
	ADC0_DATA_INT_SET = (1<<2),
};
#endif
