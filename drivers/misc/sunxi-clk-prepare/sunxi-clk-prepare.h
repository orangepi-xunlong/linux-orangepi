#ifndef _SUNXI_CLK_PERPARE_H
#define _SUNXI_CLK_PERPARE_H

#define CLK_MAX_MODULE_VALUE  10
#define CLK_MAX_ID_VALUE  32

enum CLK_ID_BIT
{
	CLK_USBPHY0_BIT    = (1<<0x0),
	CLK_USBPHY1_BIT    = (1<<0x1),
	CLK_USBOHCI1_BIT   = (1<<0x2),
	CLK_USBOHCI0_BIT   = (1<<0x3),
	CLK_USBEHCI0_BIT   = (1<<0x4),
	CLK_USBEHCI1_BIT   = (1<<0x5),
	CLK_USBEOTG_BIT    = (1<<0x6),
	CLK_HDMI_BIT       = (1<<0x7),
	CLK_ADDDA_COM_BIT  = (1<<0x8),
	CLK_MAX_INDEX      = (1<<0x9),
};

char *id_name[CLK_MAX_ID_VALUE] = {
	"usbphy0",
	"usbphy1",
	"usbohci1",
	"usbohci0",
	"usbehci0",
	"usbehci1",
	"usbotg",
	"hdmi",
	"adda_com",
};

char *dts_module_id[CLK_MAX_ID_VALUE] = {
	"usbc0",
	"usbc1",
	"usbc1",
	"usbc0",
	"usbc0",
	"usbc1",
	"usbc0",
	"hdmi",
	"codec",
};

typedef struct {
	unsigned int mask_bit;
	char id_name[20];
}bitmap_name_mapping_t;

bitmap_name_mapping_t clk_bitmap_name_mapping[CLK_MAX_MODULE_VALUE] = {
	{CLK_USBPHY0_BIT|CLK_USBPHY1_BIT|CLK_USBOHCI1_BIT|CLK_USBOHCI0_BIT|\
		CLK_USBEHCI0_BIT|CLK_USBEHCI1_BIT|CLK_USBEOTG_BIT|CLK_ADDDA_COM_BIT, "hdmi"},
	{0, "NULL"},
	{0, "NULL"},
	{0, "NULL"},
	{0, "NULL"},
	{0, "NULL"},
	{0, "NULL"},
	{0, "NULL"},
	{0, "NULL"},
	{0, "NULL"},
};

enum {
	DEBUG_INIT = 1U << 0,
	DEBUG_DATA = 1U << 1,
	DEBUG_TEST = 1U << 2,
};

#define dprintk(level_mask, fmt, arg...)	if (unlikely(debug_mask & level_mask)) \
	 printk(KERN_DEBUG fmt , ## arg)

#endif
