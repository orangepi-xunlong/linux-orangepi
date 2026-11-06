#ifndef _SOPHGO_CDMA_H_
#define _SOPHGO_CDMA_H_

/*cdma register*/
#define CDMA_CHL_CTRL         0x800
#define CDMA_INT_MASK         0x808
#define CDMA_INT_STAT         0x80C
#define CDMA_CMD_FIFO_STAT    0x810
#define CDMA_CMD_ACCP0        0x878
#define CDMA_CMD_ACCP1        0x87C
#define CDMA_CMD_ACCP2        0x880
#define CDMA_CMD_ACCP3        0x884
#define CDMA_CMD_ACCP4        0x888
#define CDMA_CMD_ACCP5        0x88C
#define CDMA_CMD_ACCP6        0x890
#define CDMA_CMD_ACCP7        0x894
#define CDMA_MAX_PAYLOAD      0x9F8
#define CDMA_INT_RAW          0x998

/* CDMA bit definition */
/* CHL_CTRL REG */
#define CDMA_CHL_CTRL_WrQOS_VALUE (0xF<<24)
#define CDMA_CHL_CTRL_RdQOS_VALUE (0xF<<20)
#define CDMA_CHL_CTRL_RdQOS_VALUE_LOW (0xF<<16)
#define CDMA_CHL_CTRL_RdQOS_VALUE_NORMAL (0xF<<12)
#define CDMA_CHL_CTRL_RdQOS_VALUE_HIGH (0xF<<8)
#define CDMA_CHL_CTRL_RdQOS_MODE (0x1<<7)
#define CDMA_CHL_CTRL_IRQ_EN (0x1<<3)  // interrupt output
#define CDMA_CHL_CTRL_DMA_EN (0x1)

/* INT_MAST REG */
#define CDMA_INT_RF_CMD_EPT_MASK (0x1<<3)
#define CDMA_INT_RF_DES_MASK   (0x1)

/* INT_STATUS REG */
#define CDMA_INT_RF_CMD_EPT (0x1<<3)
#define CDMA_INT_RF_DES   (0x1)

/* CMD_FIFO_STATUS REG */
#define CDMA_CMD_FIFO_STATUS_MASK 0xFF

/* MAX_PAYLOAD REG */
#define CDMA_MAX_READ_PAYLOAD (0x7<<3)
#define CDMA_MAX_WRITE_PAYLOAD 0x7

/* CDMA Descriptors */
#define CDMA_CMD_2D_FORMAT_MASK (0x1<<7)
#define CDMA_CMD_2D_FORMAT_BYTE (0x0<<7)
#define CDMA_CMD_2D_FORMAT_WORD (0x1<<7)

#define CDMA_CMD_2D_TRANS_MODE_MASK (0x3<<5)
#define CDMA_CMD_TRANS_DATA_ONLY (0x0<<5)
#define CDMA_CMD_TRANS_WITH_FLUSH (0x1<<5)

#define CDMA_CMD_MODE_STRIDE (0x1<<4)
#define CDMA_CMD_MODE_NORMAL (0x0<<4)
#define CDMA_CMD_IRQ_EN (0x1<<3)
#define CDMA_CMD_DESC_EOD (0x1<<2)
#define CDMA_CMD_DESC_VALID 0x1


#define CDMA_CMD_NORMAL_CFG (CDMA_CMD_MODE_NORMAL | CDMA_CMD_IRQ_EN | \
         CDMA_CMD_DESC_EOD | CDMA_CMD_DESC_VALID)
#define CDMA_CMD_STRIDE_CFG (CDMA_CMD_MODE_STRIDE | CDMA_CMD_IRQ_EN | \
         CDMA_CMD_DESC_EOD | CDMA_CMD_DESC_VALID)

#define CDMA_ADDR_WORD_ALIGN 0xFFFFFFFE //for 2-bytes align

enum sg_memcpy_dir {
	HOST2CHIP,
	CHIP2HOST,
	CHIP2CHIP
};

struct sg_cdma_arg {
	unsigned long long src;
	unsigned long long dst;
	unsigned long long size;
	enum sg_memcpy_dir dir;
};

void sg_construct_cdma_arg(struct sg_cdma_arg *p_arg, u64 src, u64 dst, u64 size, enum sg_memcpy_dir dir);

#endif
