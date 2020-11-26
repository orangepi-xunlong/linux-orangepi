#ifndef SUNXI_KEYBOARD_H
#define SUNXI_KEYBOARD_H

#define INPUT_DEV_NAME          ("sunxi-keyboard")

#define KEY_MAX_CNT             (13)

#define LRADC_CTRL              (0x00)
#define LRADC_INTC              (0x04)
#define LRADC_INT_STA           (0x08)
#define LRADC_DATA0             (0x0c)
#define LRADC_DATA1             (0x10)

#define FIRST_CONCERT_DLY       (0<<24)
#define CHAN                    (0x0)
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

#define ONE_CHANNEL
#define MODE_0V2

#ifdef MODE_0V2
/* standard of key maping
 * 0.2V mode
 */
#define REPORT_KEY_LOW_LIMIT_COUNT  (1)

#endif

struct sunxi_adc_disc {
	u32 measure;
	u32 resol;
};

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
