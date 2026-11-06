#ifndef __CDMA_H__
#define __CDMA_H__

enum cdma_datd_type {
	CDMA_DATA_TYPE_8BIT = 0,
	CDMA_DATA_TYPE_16BIT,
};

struct cdma_1d_param {
	u64 src_addr;
	u64 dst_addr;
	u32 len;
	enum cdma_datd_type data_type;
};

struct cdma_2d_param {
	u64 src_addr;
	u64 dst_addr;
	u16 width;
	u16 height;
	u16 src_stride;
	u16 dst_stride;
	enum cdma_datd_type data_type;
	bool fixed_enable;
	u16 fixed_value;
};

void cdma_set_base_addr(void *cdma_base);
void cdma_irq_handler(void);

int cdma_copy1d(struct cdma_1d_param *param);
int cdma_copy2d(struct cdma_2d_param *param);

#endif
