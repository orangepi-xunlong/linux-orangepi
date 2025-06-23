// SPDX-License-Identifier: Apache-2.0 OR GPL-2.0
/*
 * Copyright (c) 2022 Arm Limited. All rights reserved.
 * Copyright 2024 Cix Technology Group Co., Ltd.
 */

#ifndef __DMA350_CH_DRV_H
#define __DMA350_CH_DRV_H

#include "dma350_regdef.h"
#include <linux/io.h>
#include <linux/compiler_types.h>
#ifdef __cplusplus
extern "C" {
#endif

/* DMA350_CH register mask definitions */

#define DMA350_CH_TMPLTCFG_SRCTMPLTSIZE_MAX 0x1FUL
#define DMA350_CH_TMPLTCFG_DESTMPLTSIZE_MAX 0x1FUL

#define DMA350_CH_CTRL_RESET_VALUE        0x00200200
#define DMA350_CH_INTREN_RESET_VALUE      0x00000000
#define DMA350_CH_LINKADDR_RESET_VALUE    0x00000000
#define DMA350_CH_DESTRANSCFG_RESET_VALUE 0x000F0400
#define DMA350_CH_SRCTRANSCFG_RESET_VALUE 0x000F0400
#define DMA350_CH_AUTOCFG_RESET_VALUE     0x00000000

#define DMA350_CMDLINK_CLEAR_SET     (0x1UL)
#define DMA350_CMDLINK_INTREN_SET       (0x1UL << 2)
#define DMA350_CMDLINK_CTRL_SET         (0x1UL << 3)
#define DMA350_CMDLINK_SRC_ADDR_SET     (0x1UL << 4)
#define DMA350_CMDLINK_SRC_ADDRHI_SET   (0x1UL << 5)
#define DMA350_CMDLINK_DES_ADDR_SET     (0x1UL << 6)
#define DMA350_CMDLINK_DES_ADDRHI_SET   (0x1UL << 7)
#define DMA350_CMDLINK_XSIZE_SET        (0x1UL << 8)
#define DMA350_CMDLINK_XSIZEHI_SET      (0x1UL << 9)
#define DMA350_CMDLINK_SRCTRANSCFG_SET  (0x1UL << 10)
#define DMA350_CMDLINK_DESTRANSCFG_SET  (0x1UL << 11)
#define DMA350_CMDLINK_XADDRINC_SET     (0x1UL << 12)
#define DMA350_CMDLINK_YADDRSTRIDE_SET  (0x1UL << 13)
#define DMA350_CMDLINK_FILLVAL_SET      (0x1UL << 14)
#define DMA350_CMDLINK_YSIZE_SET        (0x1UL << 15)
#define DMA350_CMDLINK_TMPLTCFG_SET     (0x1UL << 16)
#define DMA350_CMDLINK_SRCTMPLT_SET     (0x1UL << 17)
#define DMA350_CMDLINK_DESTMPLT_SET     (0x1UL << 18)
#define DMA350_CMDLINK_SRCTRIGINCFG_SET (0x1UL << 19)
#define DMA350_CMDLINK_DESTRIGINCFG_SET (0x1UL << 20)
#define DMA350_CMDLINK_TRIGOUTCFG_SET   (0x1UL << 21)
#define DMA350_CMDLINK_GPOEN0_SET       (0x1UL << 22)
#define DMA350_CMDLINK_GPOVAL0_SET      (0x1UL << 24)
#define DMA350_CMDLINK_STREAMINTCFG_SET (0x1UL << 26)
#define DMA350_CMDLINK_LINKATTR_SET     (0x1UL << 28)
#define DMA350_CMDLINK_AUTOCFG_SET      (0x1UL << 29)
#define DMA350_CMDLINK_LINKADDR_SET     (0x1UL << 30)
#define DMA350_CMDLINK_LINKADDRHI_SET   (0x1UL << 31)

#define ARM_DMA350_BUSWIDTHS \
	(BIT(DMA_SLAVE_BUSWIDTH_UNDEFINED) | \
	BIT(DMA_SLAVE_BUSWIDTH_1_BYTE) | \
	BIT(DMA_SLAVE_BUSWIDTH_2_BYTES) | \
	BIT(DMA_SLAVE_BUSWIDTH_4_BYTES) | \
	BIT(DMA_SLAVE_BUSWIDTH_8_BYTES))

/* Update a addrister (addr) at position (POS) with value (VAL).
 * Affected bits are defined by a mask (MSK) */

#define SET_FIELD(addr, VAL, POS, MSK)                                          \
    do {                                                                       \
       writel((readl(addr) & ~MSK) | (((uint32_t)(VAL) << POS) & MSK),addr);                 \
    } while (0)

/* ARM DMA350 DMA Channel status bits */
enum dma350_ch_stat_t {
    DMA350_CH_STAT_DONE = DMA350_CH_STATUS_STAT_DONE,
    DMA350_CH_STAT_ERR = DMA350_CH_STATUS_STAT_ERR,
    DMA350_CH_STAT_DISABLED = DMA350_CH_STATUS_STAT_DISABLED,
    DMA350_CH_STAT_STOPPED = DMA350_CH_STATUS_STAT_STOPPED,
    DMA350_CH_STAT_SRCTRIGINWAIT = DMA350_CH_STATUS_STAT_SRCTRIGINWAIT,
    DMA350_CH_STAT_DESTRIGINWAIT = DMA350_CH_STATUS_STAT_DESTRIGINWAIT,
    DMA350_CH_STAT_TRIGOUTACKWAIT = DMA350_CH_STATUS_STAT_TRIGOUTACKWAIT,
    DMA350_CH_STAT_ALL =
        DMA350_CH_STATUS_STAT_DONE | DMA350_CH_STATUS_STAT_ERR |
        DMA350_CH_STATUS_STAT_DISABLED | DMA350_CH_STATUS_STAT_STOPPED |
        DMA350_CH_STATUS_STAT_SRCTRIGINWAIT | DMA350_CH_STATUS_STAT_DESTRIGINWAIT |
        DMA350_CH_STATUS_STAT_TRIGOUTACKWAIT
};

/* ARM DMA350 DMA Channel interrupt bits */
enum dma350_ch_intr_t {
    DMA350_CH_INTREN_DONE = DMA350_CH_INTREN_INTREN_DONE,
    DMA350_CH_INTREN_ERR = DMA350_CH_INTREN_INTREN_ERR,
    DMA350_CH_INTREN_DISABLED = DMA350_CH_INTREN_INTREN_DISABLED,
    DMA350_CH_INTREN_STOPPED = DMA350_CH_INTREN_INTREN_STOPPED,
    DMA350_CH_INTREN_SRCTRIGINWAIT = DMA350_CH_INTREN_INTREN_SRCTRIGINWAIT,
    DMA350_CH_INTREN_DESTRIGINWAIT = DMA350_CH_INTREN_INTREN_DESTRIGINWAIT,
    DMA350_CH_INTREN_TRIGOUTACKWAIT = DMA350_CH_INTREN_INTREN_TRIGOUTACKWAIT,
    DMA350_CH_INTREN_ALL =
        DMA350_CH_INTREN_INTREN_DONE | DMA350_CH_INTREN_INTREN_ERR |
        DMA350_CH_INTREN_INTREN_DISABLED | DMA350_CH_INTREN_INTREN_STOPPED |
        DMA350_CH_INTREN_INTREN_SRCTRIGINWAIT |
        DMA350_CH_INTREN_INTREN_DESTRIGINWAIT | DMA350_CH_INTREN_INTREN_TRIGOUTACKWAIT
};


/* ARM DMA350 DMA Channel XTYPE */
enum dma350_ch_xtype_t {
    DMA350_CH_XTYPE_DISABLE = 0,
    DMA350_CH_XTYPE_CONTINUE = DMA350_CH_CTRL_XTYPE_0,
    DMA350_CH_XTYPE_WRAP = DMA350_CH_CTRL_XTYPE_1,
    DMA350_CH_XTYPE_FILL = DMA350_CH_CTRL_XTYPE_1 | DMA350_CH_CTRL_XTYPE_0
};

/* ARM DMA350 DMA Channel Done type */
enum dma350_ch_donetype_t {
    DMA350_CH_DONETYPE_NONE = 0,
    DMA350_CH_DONETYPE_END_OF_CMD = DMA350_CH_CTRL_DONETYPE_0,
    DMA350_CH_DONETYPE_END_OF_AUTORESTART =
        DMA350_CH_CTRL_DONETYPE_1 | DMA350_CH_CTRL_DONETYPE_0
};

/* ARM DMA350 DMA Channel Source Trigger Input Type */
enum dma350_ch_srctrigintype_t {
    DMA350_CH_SRCTRIGINTYPE_SOFTWARE_ONLY = 0,
    DMA350_CH_SRCTRIGINTYPE_HW = DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_1,
    DMA350_CH_SRCTRIGINTYPE_INTERNAL = DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_1 |
                                       DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_0
};

/* ARM DMA350 DMA Channel Source Trigger Input Mode */
enum dma350_ch_srctriginmode_t {
    DMA350_CH_SRCTRIGINMODE_CMD = 0,
    DMA350_CH_SRCTRIGINMODE_DMA_FLOW_CTRL = DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_1,
    DMA350_CH_SRCTRIGINMODE_PERIPH_FLOW_CTRL =
        DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_1 |
        DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_0
};

/* ARM DMA350 DMA Channel Destination Trigger Input Type */
enum dma350_ch_destrigintype_t {
    DMA350_CH_DESTRIGINTYPE_SOFTWARE_ONLY = 0,
    DMA350_CH_DESTRIGINTYPE_HW = DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_1,
    DMA350_CH_DESTRIGINTYPE_INTERNAL = DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_1 |
                                       DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_0
};

/* ARM DMA350 DMA Channel Destination Trigger Input Mode */
enum dma350_ch_destriginmode_t {
    DMA350_CH_DESTRIGINMODE_CMD = 0,
    DMA350_CH_DESTRIGINMODE_DMA_FLOW_CTRL = DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_1,
    DMA350_CH_DESTRIGINMODE_PERIPH_FLOW_CTRL =
        DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_1 |
        DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_0
};


/* ARM DMA350 DMA Command link addrister structure */
/* Note: Field order must match the order of the bits in the header */
struct dma350_cmdlink_addr_t {
    /* Note: addrCLEAR (Bit 0) has no associated field and Bit 1 is reserved */
    uint32_t intren;       /* Bit 2  */
    uint32_t ctrl;         /* Bit 3  */
    uint32_t srcaddr;      /* Bit 4  */
    uint32_t srcaddrhi;    /* Bit 5  */
    uint32_t desaddr;      /* Bit 6  */
    uint32_t desaddrhi;    /* Bit 7  */
    uint32_t xsize;        /* Bit 8  */
    uint32_t xsizehi;      /* Bit 9  */
    uint32_t srctranscfg;  /* Bit 10 */
    uint32_t destranscfg;  /* Bit 11 */
    uint32_t xaddrinc;     /* Bit 12 */
    uint32_t yaddrstride;  /* Bit 13 */
    uint32_t fillval;      /* Bit 14 */
    uint32_t ysize;        /* Bit 15 */
    uint32_t tmpltcfg;     /* Bit 16 */
    uint32_t srctmplt;     /* Bit 17 */
    uint32_t destmplt;     /* Bit 18 */
    uint32_t srctrigincfg; /* Bit 19 */
    uint32_t destrigincfg; /* Bit 20 */
    uint32_t trigoutcfg;   /* Bit 21 */
    uint32_t gpoen0;       /* Bit 22 */
    uint32_t reserved0;    /* Bit 23 */
    uint32_t gpoval0;      /* Bit 24 */
    uint32_t reserved1;    /* Bit 25 */
    uint32_t streamintcfg; /* Bit 26 */
    uint32_t reserved2;    /* Bit 27 */
    uint32_t linkattr;     /* Bit 28 */
    uint32_t autocfg;      /* Bit 29 */
    uint32_t linkaddr;     /* Bit 30 */
    uint32_t linkaddrhi;   /* Bit 31 */
};

/* ARM DMA350 DMA Command link generator config structure */
struct dma350_cmdlink_gencfg_t {
    uint32_t header;
    struct dma350_cmdlink_addr_t cfg;
};


/**
 * \brief Sets source address[31:0] of channel
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] src_addr    source address, where to copy from
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_set_src(void __iomem * addr, uint32_t src_addr);

/**
 * \brief Sets destination address[31:0] of channel
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] des_addr    destination address, where to copy to
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_set_des(void __iomem * addr, uint32_t des_addr);

/**
 * \brief Sets Channel Priority
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] chprio        Number of priority
 *
 * \return void
 */
static inline
void dma350_ch_set_chprio(void __iomem * addr, uint8_t chprio);


/**
 * \brief Sets number of copies in the x dimension
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] src_xsize   number of source elements in the x dimension
 * \param[in] des_xsize   number of destination elements in the x dimension
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_set_xsize32(void __iomem * addr, uint32_t src_xsize,
                           uint32_t des_xsize);

/**
 * \brief Sets size of each transfer
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] transize    size of each transfer \ref dma350_ch_transize_t
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_set_transize(void __iomem * addr,uint32_t transize);

/**
 * \brief Sets type of operation in the x dimension
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] xtype       type of operation in the x dimension
 *                        \ref dma350_ch_xtype_t
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_set_xtype(void __iomem * addr,
                         enum dma350_ch_xtype_t xtype);



/**
 * \brief Configures when STAT_DONE flag is asserted for this command
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] donetype    donetype of command \ref dma350_ch_donetype_t
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_set_donetype(void __iomem * addr,
                            enum dma350_ch_donetype_t donetype);

/**
 * \brief Enables Source Trigger Input use for this command
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_enable_srctrigin(void __iomem * addr);

/**
 * \brief Disables Source Trigger Input use for this command
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_disable_srctrigin(void __iomem * addr);

/**
 * \brief Enables Destination Trigger Input use for this command
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_enable_destrigin(void __iomem * addr);

/**
 * \brief Disables Destination Trigger Input use for this command
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_disable_destrigin(void __iomem * addr);


/**
 * \brief Sets Source Transfer Memory Attribute field[3:0]
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] memattrlo     Attribute field (4 bits)
 *
 * \return void
 */
static inline
void dma350_ch_set_srcmemattrlo(void __iomem * addr, uint8_t memattrlo);

/**
 * \brief Sets Source Transfer Memory Attribute field[7:4]
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] memattrhi     Attribute field (4 bits)
 *
 * \return void
 */
static inline
void dma350_ch_set_srcmemattrhi(void __iomem * addr, uint8_t memattrhi);

/**
 * \brief Sets Destination Transfer Memory Attribute field[3:0]
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] memattrlo     Attribute field (4 bits)
 *
 * \return void
 */
static inline
void dma350_ch_set_desmemattrlo(void __iomem * addr, uint8_t memattrlo);

/**
 * \brief Sets Destination Transfer Memory Attribute field[7:4]
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] memattrhi     Attribute field (4 bits)
 *
 * \return void
 */
static inline
void dma350_ch_set_desmemattrhi(void __iomem * addr, uint8_t memattrhi);

static inline
void dma350_ch_set_srcmaxburstlen(void __iomem * addr, uint8_t length);

/**
 * \brief Sets Destination Max Burst Lenght
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] length        Value of max burst length
 *
 * \return void
 */
static inline
void dma350_ch_set_desmaxburstlen(void __iomem * addr, uint8_t length);

/**
 * \brief Sets source and destination address increment after each transfer
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] src_xaddr_inc  increment of source address
 * \param[in] des_xaddr_inc  increment of destination address
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_set_xaddr_inc(void __iomem * addr, int32_t src_xaddr_inc,
                             int32_t des_xaddr_inc);


/**
 * \brief Commands a channel of DMA350 DMA.
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] cmd         DMA350 DMA channel command \ref dma350_ch_cmd_t
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_cmd(void __iomem * addr, uint32_t cmd);

/**
 * \brief Enables Interrupt for DMA350 DMA channel
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] intr        Interrupt(s) to enable \ref dma350_ch_intr_t
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_enable_intr(void __iomem * addr,
                           enum dma350_ch_intr_t intr);

/**
 * \brief Disables Interrupt for DMA350 DMA channel
 *
 * \param[in] dev         DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] intr        Interrupt(s) to disable \ref dma350_ch_intr_t
 *
 * \return void
 *
 * \note This function doesn't check if dev is NULL or if it has been init.
 */
static inline
void dma350_ch_disable_intr(void __iomem * addr,
                            enum dma350_ch_intr_t intr);
/**
 * \brief Sets Source Trigger Input Select
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] srctriginsel  Input Select value
 *
 * \return void
 */
static inline
void dma350_ch_set_srctriginsel(void __iomem * addr,
                                uint8_t srctriginsel);

/**
 * \brief Sets Source Trigger Input Type
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] type          Input type \ref dma350_ch_srctrigintype_t
 *
 * \return void
 */
static inline
void dma350_ch_set_srctrigintype(void __iomem * addr,
                                 enum dma350_ch_srctrigintype_t type);

/**
 * \brief Sets Source Trigger Input Mode
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] mode          Mode \ref dma350_ch_srctriginmode_t
 *
 * \return void
 */
static inline
void dma350_ch_set_srctriginmode(void __iomem * addr,
                                 enum dma350_ch_srctriginmode_t mode);

/**
 * \brief Sets Source Trigger Input Default Transfer Size
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] blksize       Size value
 *
 * \return void
 */
static inline
void dma350_ch_set_srctriginblksize(void __iomem * addr,
                                    uint8_t blksize);

/**
 * \brief Sets Destination Trigger Input Select
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] destriginsel  Input Select value
 *
 * \return void
 */
static inline
void dma350_ch_set_destriginsel(void __iomem * addr,
                                uint8_t destriginsel);

/**
 * \brief Sets Destination Trigger Input Type
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] type          Input type \ref dma350_ch_destrigintype_t
 *
 * \return void
 */
static inline
void dma350_ch_set_destrigintype(void __iomem * addr,
                                 enum dma350_ch_destrigintype_t type);

/**
 * \brief Sets Destination Trigger Input Mode
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] mode          Mode \ref dma350_ch_destriginmode_t
 *
 * \return void
 */
static inline
void dma350_ch_set_destriginmode(void __iomem * addr,
                                 enum dma350_ch_destriginmode_t mode);

/**
 * \brief Sets Destination Trigger Input Default Transfer Size
 *
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] blksize       Size value
 *
 * \return void
 */
static inline
void dma350_ch_set_destriginblksize(void __iomem * addr,
                                    uint8_t blksize);


/**
 * \brief Sets Link Address Read Transfer Memory Attribute[3:0] field
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] memattrlo     Attribute field
 *
 * \return void
 */
static inline
void dma350_ch_set_linkmemattrlo(void __iomem * addr,
                                 uint8_t memattrlo);

/**
 * \brief Sets Link Address Read Transfer Memory Attribute[7:4] field
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] memattrlo     Attribute field
 *
 * \return void
 */
static inline
void dma350_ch_set_linkmemattrhi(void __iomem * addr,
                                 uint8_t memattrhi);

/**
 * \brief Sets Link Address Transfer Shareability Attribute
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] shareattr     Attribute field
 *
 * \return void
 */
static inline
void dma350_ch_set_linkshareattr(void __iomem * addr,
                                 uint8_t shareattr);

/**
 * \brief Enables Link Address
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 *
 * \return void
 */
static inline
void dma350_ch_enable_linkaddr(void __iomem * addr);

/**
 * \brief Disables Link Address
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 *
 * \return void
 */
static inline
void dma350_ch_disable_linkaddr(void __iomem * addr);

/**
 * \brief Sets Link Address Pointer [31:2]
 *
 * \param[in] dev           DMA350 channel device struct \ref dma350_ch_dev_t
 * \param[in] linkaddr      Memory location of the destination
 *
 * \return void
 */
static inline
void dma350_ch_set_linkaddr32(void __iomem * addr, uint32_t linkaddr);


/**
 * \brief Sets addrCLEAR header bit in the command structure which clears all
 *        previous settings from the channel addristers
 *
 * \param[in] cmldink_cfg   Command structure for DMA350 DMA command linking
 *                          feature \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_regclear(struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Enables Interrupt for DMA350 DMA channel in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] intr        Interrupt(s) to enable \ref dma350_ch_intr_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_enable_intr(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                enum dma350_ch_intr_t intr);

/**
 * \brief Disables Interrupt for DMA350 DMA channel in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] intr        Interrupt(s) to disable \ref dma350_ch_intr_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_disable_intr(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                 enum dma350_ch_intr_t intr);

/**
 * \brief Sets Transfer Enitity Size in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] transize      size in bytes \ref dma350_ch_transize_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_transize(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                 uint32_t transize);

/**
 * \brief Sets Channel Priority in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] chprio        Number of priority
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_chprio(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                               uint8_t chprio);

/**
 * \brief Sets operation type for X direction in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] xtype         type \ref dma350_ch_xtype_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_xtype(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                              enum dma350_ch_xtype_t xtype);

/**
 * \brief Sets when the STAT_DONE status flag is asserted during the command
 *        operation in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] donetype      Done type selection \ref dma350_ch_donetype_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_donetype(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                 enum dma350_ch_donetype_t donetype);

/**
 * \brief Enables the automatic pause request for the current DMA operation
 *        if the STAT_DONE flag is asserted in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_enable_donepause(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Disables the automatic pause request for the current DMA operation
 *        if the STAT_DONE flag is asserted in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_disable_donepause(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Enables Source Trigger Input use in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_enable_srctrigin(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Disables Source Trigger Input use in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_disable_srctrigin(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Enables Destination Trigger Input use in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_enable_destrigin(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Disables Destination Trigger Input use in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_disable_destrigin(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Sets Source Address[31:0] in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] src_addr      Memory location of the source
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_srcaddr32(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                  uint32_t src_addr);

/**
 * \brief Sets Destination Address[31:0] in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] des_addr      Memory location of the destination
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_desaddr32(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                  uint32_t des_addr);

/**
 * \brief Sets the number of data in the command structure units
 *        copied during the DMA command up to 16 bits in the X dimension
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] src_xsize     Source number of transfers
 * \param[in] des_xsize     Destination number of transfers
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_xsize16(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                uint16_t src_xsize, uint16_t des_xsize);

/**
 * \brief Sets the number of data in the command structure units
 *        copied during the DMA command up to 32 bits in the X dimension
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] src_xsize     Source number of transfers
 * \param[in] des_xsize     Destination number of transfers
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_xsize32(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                uint32_t src_xsize, uint32_t des_xsize);

/**
 * \brief Sets Source Transfer Memory Attribute field[3:0]
 *        in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] memattrlo     Attribute field
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_srcmemattrlo(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t memattrlo);

/**
 * \brief Sets Source Transfer Memory Attribute field[7:4]
 *        in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] memattrhi     Attribute field
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_srcmemattrhi(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t memattrhi);

/**
 * \brief Sets Source Transfer Shareability Attribute in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] shareattr     Attribute field
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_srcshareattr(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t shareattr);

/**
 * \brief Sets Destination Transfer Memory Attribute field[3:0] in the command
 *        structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] memattrlo     Attribute field
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_desmemattrlo(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t memattrlo);

/**
 * \brief Sets Destination Transfer Memory Attribute field[7:4] in the command
 *        structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] memattrhi     Attribute field
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_desmemattrhi(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t memattrhi);

/**
 * \brief Sets Destination Transfer Shareability Attribute in the command
 *        structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] shareattr     Attribute field
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_desshareattr(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t shareattr);

/**
 * \brief Sets Source Transfer Attribute to secure in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_src_trans_secure(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Sets Source Transfer Attribute to non secure in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_src_trans_nonsecure(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Sets Destination Transfer Attribute to secure in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_des_trans_secure(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Sets Destination Transfer Attribute to non secure
 *        in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_des_trans_nonsecure(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Sets Source Transfer Privilege to privileged
 *        in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_src_trans_privileged(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Sets Source Transfer Privilege to unprivileged
 *        in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_src_trans_unprivileged(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Sets Destination Transfer Privilege to privileged
 *        in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_des_trans_privileged(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Sets Destination Transfer Privilege to unprivileged
 *        in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_des_trans_unprivileged(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Sets Source Max Burst Lenght in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] length        Value of max burst length
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_srcmaxburstlen(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t length);

/**
 * \brief Sets Destination Max Burst Lenght in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] length        Value of max burst length
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_desmaxburstlen(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t length);

/**
 * \brief Sets the increment value in the command structure that is used to
 *        update the source and the destination addresses after each
 *        transfered data unit
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] src_xaddrinc  Source X dimension address increment value
 * \param[in] des_xaddrinc  Destination X dimension address increment value
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_xaddrinc(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                 uint16_t src_xaddrinc, uint16_t des_xaddrinc);

/**
 * \brief Sets Source Trigger Input Select in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] srctriginsel  Input Select value
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_srctriginsel(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t srctriginsel);

/**
 * \brief Sets Source Trigger Input Type in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] type          Input type \ref dma350_ch_srctrigintype_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_srctrigintype(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
    enum dma350_ch_srctrigintype_t type);

/**
 * \brief Sets Source Trigger Input Mode in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] mode          Mode \ref dma350_ch_srctriginmode_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_srctriginmode(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
    enum dma350_ch_srctriginmode_t mode);

/**
 * \brief Sets Source Trigger Input Default Transfer Size
 *        in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] blksize       Size value
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_srctriginblksize(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t blksize);

/**
 * \brief Sets Destination Trigger Input Select in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] destriginsel  Input Select value
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_destriginsel(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t destriginsel);

/**
 * \brief Sets Destination Trigger Input Type in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] type          Input type \ref dma350_ch_destrigintype_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_destrigintype(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
    enum dma350_ch_destrigintype_t type);

/**
 * \brief Sets Destination Trigger Input Mode in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] mode          Mode \ref dma350_ch_destriginmode_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_destriginmode(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
    enum dma350_ch_destriginmode_t mode);

/**
 * \brief Sets Destination Trigger Input Default Transfer Size
 *        in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] blksize       Size value
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_destriginblksize(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t blksize);

/**
 * \brief Sets Link Address Read Transfer Memory Attribute[3:0] field
 *        in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] memattrlo     Attribute field
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_linkmemattrlo(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t memattrlo);

/**
 * \brief Sets Link Address Read Transfer Memory Attribute[7:4] field
 *        in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] memattrlo     Attribute field
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_linkmemattrhi(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t memattrhi);

/**
 * \brief Enables Infinite Automatic Command Restart in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_enable_cmdrestartinfen(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Disables Infinite Automatic Command Restart in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_disable_cmdrestartinfen(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Enables Link Address in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_enable_linkaddr(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Disables Link Address in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static inline
void dma350_cmdlink_disable_linkaddr(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

/**
 * \brief Sets Link Address Pointer [31:2] in the command structure
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 * \param[in] linkaddr      Memory location of the destination
 *
 * \return void
 */
static inline
void dma350_cmdlink_set_linkaddr32(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                   uint32_t linkaddr);
/**
 * \brief Initialize command structure for dma command linking feature
 *
 * \param[in] cmldink_cfg   DMA350 command structure for command linking feature
 *                          \ref dma350_cmdlink_gencfg_t
 *
 * \return void
 */
static void dma350_cmdlink_init(struct dma350_cmdlink_gencfg_t *cmdlink_cfg);

static inline
void dma350_ch_set_src(void __iomem * addr, uint32_t src_addr)
{
	writel(src_addr,addr);
}

static inline
void dma350_ch_set_des(void __iomem * addr, uint32_t des_addr)
{
	writel(des_addr,addr);
}

static inline
void dma350_ch_set_xsize32(void __iomem * addr, uint32_t src_xsize,
                           uint32_t des_xsize)
{   
       writel(((des_xsize & 0x0000FFFFUL) << 16U) | (src_xsize & 0x0000FFFFUL),addr); 
       writel((des_xsize & 0xFFFF0000UL) | ((src_xsize & 0xFFFF0000UL) >> 16U),addr + 0x4);
}


static inline
void dma350_ch_set_transize(void __iomem * addr,uint32_t transize)
{		
	    SET_FIELD(addr, transize, DMA350_CH_CTRL_TRANSIZE_Pos,
              DMA350_CH_CTRL_TRANSIZE_Msk);	
}

static inline
void dma350_ch_set_chprio(void __iomem * addr, uint8_t chprio)
{
    SET_FIELD(addr, chprio, DMA350_CH_CTRL_CHPRIO_Pos,
              DMA350_CH_CTRL_CHPRIO_Msk);
}

static inline
void dma350_ch_set_xtype(void __iomem * addr,
                         enum dma350_ch_xtype_t xtype)
{
	writel((readl(addr) & ~DMA350_CH_CTRL_XTYPE_Msk) | ((uint32_t)(xtype)),addr);
}


static inline
void dma350_ch_set_donetype(void __iomem * addr,
                            enum dma350_ch_donetype_t donetype)
{		
	SET_FIELD(addr, donetype, DMA350_CH_CTRL_DONETYPE_Pos,
	  DMA350_CH_CTRL_DONETYPE_Msk);		
}


static inline
void dma350_ch_enable_srctrigin(void __iomem * addr)
{   
     writel(readl(addr) | DMA350_CH_CTRL_USESRCTRIGIN_Msk,addr);
}

static inline
void dma350_ch_disable_srctrigin(void __iomem * addr)
{
	 writel(readl(addr) & (~DMA350_CH_CTRL_USESRCTRIGIN_Msk),addr);		
}

static inline
void dma350_ch_enable_destrigin(void __iomem * addr)
{		
	writel(readl(addr) | DMA350_CH_CTRL_USEDESTRIGIN_Msk,addr);
}

static inline
void dma350_ch_disable_destrigin(void __iomem * addr)
{		
	 writel(readl(addr) & (~DMA350_CH_CTRL_USEDESTRIGIN_Msk),addr);			
}


static inline
void dma350_ch_set_srcmemattrlo(void __iomem * addr, uint8_t memattrlo)
{
    SET_FIELD(addr, memattrlo,
              DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Pos,
              DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Msk);
}

static inline
void dma350_ch_set_srcmemattrhi(void __iomem * addr, uint8_t memattrhi)
{
    SET_FIELD(addr, memattrhi,
              DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Pos,
              DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Msk);
}

static inline
void dma350_ch_src_memattr(void __iomem * addr, u32 memattrhi,u32 memattrlo)
{	

	    SET_FIELD(addr, memattrhi,
              DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Pos,
              DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Msk);
		SET_FIELD(addr, memattrlo,
              DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Pos,
              DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Msk);	 	
}

static inline
void dma350_ch_set_desmemattrlo(void __iomem * addr, uint8_t memattrlo)
{
    SET_FIELD(addr, memattrlo,
              DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Pos,
              DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Msk);
}

static inline
void dma350_ch_set_desmemattrhi(void __iomem * addr, uint8_t memattrhi)
{
    SET_FIELD(addr, memattrhi,
              DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Pos,
              DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Msk);
}

static inline
void dma350_ch_des_memattr(void __iomem * addr,u32 memattrhi,u32 memattrlo)
{	
    SET_FIELD(addr, memattrhi,
              DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Pos,
              DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Msk);
			  
	SET_FIELD(addr, memattrlo,
              DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Pos,
              DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Msk);			  
}


static inline
void dma350_ch_set_srcmaxburstlen(void __iomem * addr, uint8_t length)
{
    SET_FIELD(addr, length,
              DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Pos,
              DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Msk);
}

static inline
void dma350_ch_set_desmaxburstlen(void __iomem * addr, uint8_t length)
{
    SET_FIELD(addr, length,
              DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Pos,
              DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Msk);			  
}


static inline
void dma350_ch_set_xaddr_inc(void __iomem * addr, int32_t src_xaddr_inc,
                             int32_t des_xaddr_inc)
{
    writel(((des_xaddr_inc & 0x0000FFFFUL) << DMA350_CH_XADDRINC_DESXADDRINC_Pos) |
        (src_xaddr_inc & 0x0000FFFFUL),addr);
}


static inline
void dma350_ch_set_srctriginsel(void __iomem * addr,
                                uint8_t srctriginsel)
{
    SET_FIELD(addr, srctriginsel,
              DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Pos,
              DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Msk);
}

static inline
void dma350_ch_set_srctrigintype(void __iomem * addr,
                                 enum dma350_ch_srctrigintype_t type)
{	
	writel((readl(addr) & ~DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_Msk) | ((uint32_t)(type)),addr);
			  
}

static inline
void dma350_ch_set_srctriginmode(void __iomem * addr,
                                 enum dma350_ch_srctriginmode_t mode)
{
	writel((readl(addr) & ~DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_Msk) | ((uint32_t)(mode)),addr);		
}

static inline
void dma350_ch_set_srctriginblksize(void __iomem * addr,
                                    uint8_t blksize)
{
    SET_FIELD(addr, blksize,
              DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Pos,
              DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Msk);			  
		  
			  
}

static inline
void dma350_ch_set_destriginsel(void __iomem * addr,
                                uint8_t destriginsel)
{
    SET_FIELD(addr, destriginsel,
              DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Pos,
              DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Msk);
}

static inline
void dma350_ch_set_destrigintype(void __iomem * addr,
                                 enum dma350_ch_destrigintype_t type)
{
	writel((readl(addr) & ~DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_Msk) | ((uint32_t)(type)),addr);		  
}

static inline
void dma350_ch_set_destriginmode(void __iomem * addr,
                                 enum dma350_ch_destriginmode_t mode)
{	
	writel((readl(addr) & ~DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_Msk) | ((uint32_t)(mode)),addr);		  
	  
}

static inline
void dma350_ch_set_destriginblksize(void __iomem * addr,
                                    uint8_t blksize)
{
    SET_FIELD(addr, blksize,
              DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Pos,
              DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Msk);			  
}

static inline
void dma350_ch_set_linkmemattrlo(void __iomem * addr, uint8_t memattrlo)
{
    SET_FIELD(addr, memattrlo,
              DMA350_CH_LINKATTR_LINKMEMATTRLO_Pos,
              DMA350_CH_LINKATTR_LINKMEMATTRLO_Msk);
}

static inline
void dma350_ch_set_linkmemattrhi(void __iomem * addr, uint8_t memattrhi)
{
    SET_FIELD(addr, memattrhi,
              DMA350_CH_LINKATTR_LINKMEMATTRHI_Pos,
              DMA350_CH_LINKATTR_LINKMEMATTRHI_Msk);
}

static inline
void dma350_ch_set_linkshareattr(void __iomem * addr, uint8_t shareattr)
{
    SET_FIELD(addr, shareattr,
              DMA350_CH_LINKATTR_LINKSHAREATTR_Pos,
              DMA350_CH_LINKATTR_LINKSHAREATTR_Msk);
}

static inline
void dma350_ch_enable_linkaddr(void __iomem * addr)
{
	writel(readl(addr) | DMA350_CH_LINKADDR_LINKADDREN_Msk,addr);
}

static inline
void dma350_ch_set_linkaddr32(void __iomem * addr, uint32_t linkaddr)
{
	writel(linkaddr, addr);
}

static inline
void dma350_ch_disable_linkaddr(void __iomem * addr)
{
	writel(0,addr);
}

static inline
void dma350_cmdlink_set_regclear(struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CLEAR_SET;
}

static inline
void dma350_cmdlink_enable_intr(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                enum dma350_ch_intr_t intr)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_INTREN_SET;
    cmdlink_cfg->cfg.intren |= intr;
}

static inline
void dma350_cmdlink_disable_intr(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                 enum dma350_ch_intr_t intr)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_INTREN_SET;
    cmdlink_cfg->cfg.intren &= (~intr);
}

static inline
void dma350_cmdlink_set_transize(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                 uint32_t transize)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl =
        (cmdlink_cfg->cfg.ctrl & (~DMA350_CH_CTRL_TRANSIZE_Msk)) |
        (transize & DMA350_CH_CTRL_TRANSIZE_Msk);
}

static inline
void dma350_cmdlink_set_chprio(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                               uint8_t chprio)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl =
        (cmdlink_cfg->cfg.ctrl & (~DMA350_CH_CTRL_CHPRIO_Msk)) |
        (((chprio & 0x000000FFUL) << DMA350_CH_CTRL_CHPRIO_Pos) &
         DMA350_CH_CTRL_CHPRIO_Msk);
}

static inline
void dma350_cmdlink_set_xtype(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                              enum dma350_ch_xtype_t xtype)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl = (cmdlink_cfg->cfg.ctrl & (~DMA350_CH_CTRL_XTYPE_Msk)) |
                            (xtype & DMA350_CH_CTRL_XTYPE_Msk);
}

static inline
void dma350_cmdlink_set_donetype(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                 enum dma350_ch_donetype_t donetype)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl =
        (cmdlink_cfg->cfg.ctrl & (~DMA350_CH_CTRL_DONETYPE_Msk)) |
        (donetype & DMA350_CH_CTRL_DONETYPE_Msk);
}

static inline
void dma350_cmdlink_enable_donepause(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl |= DMA350_CH_CTRL_DONEPAUSEEN_Msk;
}

static inline
void dma350_cmdlink_disable_donepause(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl &= (~DMA350_CH_CTRL_DONEPAUSEEN_Msk);
}

static inline
void dma350_cmdlink_enable_srctrigin(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl |= DMA350_CH_CTRL_USESRCTRIGIN_Msk;
}

static inline
void dma350_cmdlink_disable_srctrigin(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl &= (~DMA350_CH_CTRL_USESRCTRIGIN_Msk);
}

static inline
void dma350_cmdlink_enable_destrigin(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl |= DMA350_CH_CTRL_USEDESTRIGIN_Msk;
}

static inline
void dma350_cmdlink_disable_destrigin(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl &= (~DMA350_CH_CTRL_USEDESTRIGIN_Msk);
}

static inline
void dma350_cmdlink_enable_trigout(struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl |= DMA350_CH_CTRL_USETRIGOUT_Msk;
}

static inline
void dma350_cmdlink_disable_trigout(struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl &= (~DMA350_CH_CTRL_USETRIGOUT_Msk);
}

static inline
void dma350_cmdlink_enable_gpo(struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl |= DMA350_CH_CTRL_USEGPO_Msk;
}

static inline
void dma350_cmdlink_disable_gpo(struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl &= (~DMA350_CH_CTRL_USEGPO_Msk);
}

static inline
void dma350_cmdlink_enable_stream(struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl |= DMA350_CH_CTRL_USESTREAM_Msk;
}

static inline
void dma350_cmdlink_disable_stream(struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_CTRL_SET;
    cmdlink_cfg->cfg.ctrl &= (~DMA350_CH_CTRL_USESTREAM_Msk);
}

static inline
void dma350_cmdlink_set_srcaddr32(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                  uint32_t src_addr)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_SRC_ADDR_SET;
    cmdlink_cfg->header &= (~DMA350_CMDLINK_SRC_ADDRHI_SET);
    cmdlink_cfg->cfg.srcaddr = src_addr;
}

static inline
void dma350_cmdlink_set_desaddr32(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                  uint32_t des_addr)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_DES_ADDR_SET;
    cmdlink_cfg->header &= (~DMA350_CMDLINK_DES_ADDRHI_SET);
    cmdlink_cfg->cfg.desaddr = des_addr;
}

static inline
void dma350_cmdlink_set_xsize16(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                uint16_t src_xsize, uint16_t des_xsize)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_XSIZE_SET;
    cmdlink_cfg->header &= (~DMA350_CMDLINK_XSIZEHI_SET);
    cmdlink_cfg->cfg.xsize = (des_xsize & 0x0000FFFFUL)
                             << DMA350_CH_XSIZE_DESXSIZE_Pos;
    cmdlink_cfg->cfg.xsize |= (src_xsize & 0x0000FFFFUL)
                              << DMA350_CH_XSIZE_SRCXSIZE_Pos;
}

static inline
void dma350_cmdlink_set_xsize32(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                uint32_t src_xsize, uint32_t des_xsize)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_XSIZE_SET;
    cmdlink_cfg->header |= DMA350_CMDLINK_XSIZEHI_SET;
    cmdlink_cfg->cfg.xsize = (des_xsize & 0x0000FFFFUL)
                             << DMA350_CH_XSIZE_DESXSIZE_Pos;
    cmdlink_cfg->cfg.xsize |= (src_xsize & 0x0000FFFFUL)
                              << DMA350_CH_XSIZE_SRCXSIZE_Pos;
    cmdlink_cfg->cfg.xsizehi = (des_xsize & 0xFFFF0000UL);
    cmdlink_cfg->cfg.xsizehi |= (src_xsize & 0xFFFF0000UL) >> 16;
}

static inline
void dma350_cmdlink_set_srcmemattrlo(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t memattrlo)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_SRCTRANSCFG_SET;
    cmdlink_cfg->cfg.srctranscfg =
        (cmdlink_cfg->cfg.srctranscfg &
         (~DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Msk)) |
        (((memattrlo & 0x000000FFUL) << DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Pos) &
         DMA350_CH_SRCTRANSCFG_SRCMEMATTRLO_Msk);
}

static inline
void dma350_cmdlink_set_srcmemattrhi(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t memattrhi)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_SRCTRANSCFG_SET;
    cmdlink_cfg->cfg.srctranscfg =
        (cmdlink_cfg->cfg.srctranscfg &
         (~DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Msk)) |
        (((memattrhi & 0x000000FFUL) << DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Pos) &
         DMA350_CH_SRCTRANSCFG_SRCMEMATTRHI_Msk);

}

static inline
void dma350_cmdlink_set_srcshareattr(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t shareattr)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_SRCTRANSCFG_SET;
    cmdlink_cfg->cfg.srctranscfg =
        (cmdlink_cfg->cfg.srctranscfg &
         (~DMA350_CH_SRCTRANSCFG_SRCSHAREATTR_Msk)) |
        (((shareattr & 0x000000FFUL) << DMA350_CH_SRCTRANSCFG_SRCSHAREATTR_Pos) &
         DMA350_CH_SRCTRANSCFG_SRCSHAREATTR_Msk);
}

static inline
void dma350_cmdlink_set_desmemattrlo(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t memattrlo)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_DESTRANSCFG_SET;
    cmdlink_cfg->cfg.destranscfg =
        (cmdlink_cfg->cfg.destranscfg &
         (~DMA350_CH_DESTRANSCFG_DESSHAREATTR_Msk)) |
        (((memattrlo & 0x000000FFUL) << DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Pos) &
         DMA350_CH_DESTRANSCFG_DESMEMATTRLO_Msk);
}

static inline
void dma350_cmdlink_set_desmemattrhi(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t memattrhi)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_DESTRANSCFG_SET;
    cmdlink_cfg->cfg.destranscfg =
        (cmdlink_cfg->cfg.destranscfg &
         (~DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Msk)) |
        (((memattrhi & 0x000000FFUL) << DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Pos) &
         DMA350_CH_DESTRANSCFG_DESMEMATTRHI_Msk);
}

static inline
void dma350_cmdlink_set_desshareattr(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t shareattr)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_DESTRANSCFG_SET;
    cmdlink_cfg->cfg.destranscfg =
        (cmdlink_cfg->cfg.destranscfg &
         (~DMA350_CH_DESTRANSCFG_DESSHAREATTR_Msk)) |
        (((shareattr & 0x000000FFUL) << DMA350_CH_DESTRANSCFG_DESSHAREATTR_Pos) &
         DMA350_CH_DESTRANSCFG_DESSHAREATTR_Msk);
}

static inline
void dma350_cmdlink_set_src_trans_secure(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_SRCTRANSCFG_SET;
    cmdlink_cfg->cfg.srctranscfg &= (~DMA350_CH_SRCTRANSCFG_SRCNONSECATTR_Msk);
}

static inline
void dma350_cmdlink_set_src_trans_nonsecure(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->cfg.srctranscfg |= DMA350_CH_SRCTRANSCFG_SRCNONSECATTR_Msk;
    if (cmdlink_cfg->cfg.srctranscfg == DMA350_CH_SRCTRANSCFG_RESET_VALUE) {
        cmdlink_cfg->header &= (~DMA350_CMDLINK_SRCTRANSCFG_SET);
    }
}

static inline
void dma350_cmdlink_set_des_trans_secure(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_DESTRANSCFG_SET;
    cmdlink_cfg->cfg.destranscfg &= (~DMA350_CH_DESTRANSCFG_DESNONSECATTR_Msk);
}

static inline
void dma350_cmdlink_set_des_trans_nonsecure(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->cfg.destranscfg |= DMA350_CH_DESTRANSCFG_DESNONSECATTR_Msk;
    if (cmdlink_cfg->cfg.destranscfg == DMA350_CH_DESTRANSCFG_RESET_VALUE) {
        cmdlink_cfg->header &= (~DMA350_CMDLINK_DESTRANSCFG_SET);
    }
}

static inline
void dma350_cmdlink_set_src_trans_privileged(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_SRCTRANSCFG_SET;
    cmdlink_cfg->cfg.srctranscfg |= DMA350_CH_SRCTRANSCFG_SRCPRIVATTR_Msk;
}

static inline
void dma350_cmdlink_set_src_trans_unprivileged(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->cfg.srctranscfg &= (~DMA350_CH_SRCTRANSCFG_SRCPRIVATTR_Msk);
    if (cmdlink_cfg->cfg.srctranscfg == DMA350_CH_SRCTRANSCFG_RESET_VALUE) {
        cmdlink_cfg->header &= (~DMA350_CMDLINK_SRCTRANSCFG_SET);
    }
}

static inline
void dma350_cmdlink_set_des_trans_privileged(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_DESTRANSCFG_SET;
    cmdlink_cfg->cfg.destranscfg |= DMA350_CH_DESTRANSCFG_DESPRIVATTR_Msk;
}

static inline
void dma350_cmdlink_set_des_trans_unprivileged(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->cfg.destranscfg &= (~DMA350_CH_DESTRANSCFG_DESPRIVATTR_Msk);
    if (cmdlink_cfg->cfg.destranscfg == DMA350_CH_DESTRANSCFG_RESET_VALUE) {
        cmdlink_cfg->header &= (~DMA350_CMDLINK_DESTRANSCFG_SET);
    }
}

static inline
void dma350_cmdlink_set_srcmaxburstlen(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t length)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_SRCTRANSCFG_SET;
    cmdlink_cfg->cfg.srctranscfg =
        (cmdlink_cfg->cfg.srctranscfg &
         (~DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Msk)) |
        (((length & 0x000000FFUL) << DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Pos) &
         DMA350_CH_SRCTRANSCFG_SRCMAXBURSTLEN_Msk);
}

static inline
void dma350_cmdlink_set_desmaxburstlen(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t length)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_DESTRANSCFG_SET;
    cmdlink_cfg->cfg.destranscfg =
        (cmdlink_cfg->cfg.destranscfg &
         (~DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Msk)) |
        (((length & 0x000000FFUL) << DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Pos) &
         DMA350_CH_DESTRANSCFG_DESMAXBURSTLEN_Msk);
}

static inline
void dma350_cmdlink_set_xaddrinc(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                 uint16_t src_xaddrinc, uint16_t des_xaddrinc)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_XADDRINC_SET;
    cmdlink_cfg->cfg.xaddrinc = (des_xaddrinc & 0x0000FFFFUL)
                                << DMA350_CH_XADDRINC_DESXADDRINC_Pos;
    cmdlink_cfg->cfg.xaddrinc |= (src_xaddrinc & 0x0000FFFFUL)
                                 << DMA350_CH_XADDRINC_SRCXADDRINC_Pos;
}


static inline
void dma350_cmdlink_set_srctriginsel(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t srctriginsel)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_SRCTRIGINCFG_SET;
    cmdlink_cfg->cfg.srctrigincfg = (cmdlink_cfg->cfg.srctrigincfg &
                                     (~DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Msk)) |
                                    (((srctriginsel & 0x000000FFUL)
                                      << DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Pos) &
                                     DMA350_CH_SRCTRIGINCFG_SRCTRIGINSEL_Msk);
}

static inline
void dma350_cmdlink_set_srctrigintype(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
    enum dma350_ch_srctrigintype_t type)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_SRCTRIGINCFG_SET;
    cmdlink_cfg->cfg.srctrigincfg =
        (cmdlink_cfg->cfg.srctrigincfg &
         (~DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_Msk)) |
        (type & DMA350_CH_SRCTRIGINCFG_SRCTRIGINTYPE_Msk);
}

static inline
void dma350_cmdlink_set_srctriginmode(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
    enum dma350_ch_srctriginmode_t mode)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_SRCTRIGINCFG_SET;
    cmdlink_cfg->cfg.srctrigincfg =
        (cmdlink_cfg->cfg.srctrigincfg &
         (~DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_Msk)) |
        (mode & DMA350_CH_SRCTRIGINCFG_SRCTRIGINMODE_Msk);
}

static inline
void dma350_cmdlink_set_srctriginblksize(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t blksize)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_SRCTRIGINCFG_SET;
    cmdlink_cfg->cfg.srctrigincfg =
        (cmdlink_cfg->cfg.srctrigincfg &
         (~DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Msk)) |
        (((blksize & 0x000000FFUL)
          << DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Pos) &
         DMA350_CH_SRCTRIGINCFG_SRCTRIGINBLKSIZE_Msk);
}

static inline
void dma350_cmdlink_set_destriginsel(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t destriginsel)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_DESTRIGINCFG_SET;
    cmdlink_cfg->cfg.destrigincfg = (cmdlink_cfg->cfg.destrigincfg &
                                     (~DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Msk)) |
                                    (((destriginsel & 0x000000FFUL)
                                      << DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Pos) &
                                     DMA350_CH_DESTRIGINCFG_DESTRIGINSEL_Msk);
}

static inline
void dma350_cmdlink_set_destrigintype(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
    enum dma350_ch_destrigintype_t type)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_DESTRIGINCFG_SET;
    cmdlink_cfg->cfg.destrigincfg =
        (cmdlink_cfg->cfg.destrigincfg &
         (~DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_Msk)) |
        (type & DMA350_CH_DESTRIGINCFG_DESTRIGINTYPE_Msk);
}

static inline
void dma350_cmdlink_set_destriginmode(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
    enum dma350_ch_destriginmode_t mode)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_DESTRIGINCFG_SET;
    cmdlink_cfg->cfg.destrigincfg =
        (cmdlink_cfg->cfg.destrigincfg &
         (~DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_Msk)) |
        (mode & DMA350_CH_DESTRIGINCFG_DESTRIGINMODE_Msk);
}

static inline
void dma350_cmdlink_set_destriginblksize(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t blksize)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_DESTRIGINCFG_SET;
    cmdlink_cfg->cfg.destrigincfg =
        (cmdlink_cfg->cfg.destrigincfg &
         (~DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Msk)) |
        (((blksize & 0x000000FFUL)
          << DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Pos) &
         DMA350_CH_DESTRIGINCFG_DESTRIGINBLKSIZE_Msk);
}


static inline
void dma350_cmdlink_set_linkmemattrlo(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t memattrlo)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_LINKATTR_SET;
    cmdlink_cfg->cfg.linkattr =
        (cmdlink_cfg->cfg.linkattr & (~DMA350_CH_LINKATTR_LINKMEMATTRLO_Msk)) |
        (((memattrlo & 0x000000FFUL) << DMA350_CH_LINKATTR_LINKMEMATTRLO_Pos) &
         DMA350_CH_LINKATTR_LINKMEMATTRLO_Msk);
}

static inline
void dma350_cmdlink_set_linkmemattrhi(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t memattrhi)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_LINKATTR_SET;
    cmdlink_cfg->cfg.linkattr =
        (cmdlink_cfg->cfg.linkattr & (~DMA350_CH_LINKATTR_LINKMEMATTRHI_Msk)) |
        (((memattrhi & 0x000000FFUL) << DMA350_CH_LINKATTR_LINKMEMATTRHI_Pos) &
         DMA350_CH_LINKATTR_LINKMEMATTRHI_Msk);
}

static inline
void dma350_cmdlink_set_linkshareattr(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg, uint8_t shareattr)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_LINKATTR_SET;
    cmdlink_cfg->cfg.linkattr =
        (cmdlink_cfg->cfg.linkattr & (~DMA350_CH_LINKATTR_LINKSHAREATTR_Msk)) |
        (((shareattr & 0x000000FFUL) << DMA350_CH_LINKATTR_LINKSHAREATTR_Pos) &
         DMA350_CH_LINKATTR_LINKSHAREATTR_Msk);
}


static inline
void dma350_cmdlink_enable_cmdrestartinfen(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_AUTOCFG_SET;
    cmdlink_cfg->cfg.autocfg |= DMA350_CH_AUTOCFG_CMDRESTARTINFEN_Msk;
}

static inline
void dma350_cmdlink_disable_cmdrestartinfen(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_AUTOCFG_SET;
    cmdlink_cfg->cfg.autocfg &= (~DMA350_CH_AUTOCFG_CMDRESTARTINFEN_Msk);
}

static inline
void dma350_cmdlink_enable_linkaddr(struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_LINKADDR_SET;
    cmdlink_cfg->cfg.linkaddr |= DMA350_CH_LINKADDR_LINKADDREN_Msk;
}

static inline
void dma350_cmdlink_disable_linkaddr(
    struct dma350_cmdlink_gencfg_t *cmdlink_cfg)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_LINKADDR_SET;
    cmdlink_cfg->cfg.linkaddr &= ~(DMA350_CH_LINKADDR_LINKADDREN_Msk);
}

static inline
void dma350_cmdlink_set_linkaddr32(struct dma350_cmdlink_gencfg_t *cmdlink_cfg,
                                   uint32_t linkaddr)
{
    cmdlink_cfg->header |= DMA350_CMDLINK_LINKADDR_SET;
   cmdlink_cfg->header &= ~(DMA350_CMDLINK_LINKADDRHI_SET);
    cmdlink_cfg->cfg.linkaddr =
        (cmdlink_cfg->cfg.linkaddr & (~DMA350_CH_LINKADDR_LINKADDR_Msk)) |
       (linkaddr & DMA350_CH_LINKADDR_LINKADDR_Msk);

}

static inline
void dma350_ch_cmd(void __iomem * addr,uint32_t cmd)
{
	writel(cmd,addr);
}

static inline
void dma350_ch_enable_intr(void __iomem * addr,uint32_t intr)
{
    writel(readl(addr) | intr,addr);
}

static inline
void dma350_ch_disable_intr(void __iomem * addr,uint32_t intr)
{	
	 writel(readl(addr) & (~intr),addr);
}

#ifdef __cplusplus
}
#endif
#endif /* __DMA350_CH_DRV_H */
