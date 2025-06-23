/* SPDX-License-Identifier: GPL-2.0 */
/*
 * (C) COPYRIGHT 2022-2023 Arm Technology (China) Co., Ltd.
 * ALL RIGHTS RESERVED
 *
 */
#ifndef _LINLONDP_DEV_H_
#define _LINLONDP_DEV_H_

#include <linux/device.h>
#include <linux/clk.h>
#include <linux/reset.h>
#include <linux/acpi.h>
#include "linlondp_pipeline.h"
#include "linlondp_product.h"
#include "linlondp_format_caps.h"
#include <linux/version.h>

#define LINLONDP_EVENT_VSYNC        BIT_ULL(0)
#define LINLONDP_EVENT_FLIP        BIT_ULL(1)
#define LINLONDP_EVENT_URUN        BIT_ULL(2)
#define LINLONDP_EVENT_IBSY        BIT_ULL(3)
#define LINLONDP_EVENT_OVR        BIT_ULL(4)
#define LINLONDP_EVENT_EOW        BIT_ULL(5)
#define LINLONDP_EVENT_MODE        BIT_ULL(6)
#define LINLONDP_EVENT_FULL        BIT_ULL(7)
#define LINLONDP_EVENT_EMPTY        BIT_ULL(8)

#define LINLONDP_ERR_TETO            BIT_ULL(14)
#define LINLONDP_ERR_TEMR            BIT_ULL(15)
#define LINLONDP_ERR_TITR            BIT_ULL(16)
#define LINLONDP_ERR_CPE            BIT_ULL(17)
#define LINLONDP_ERR_CFGE            BIT_ULL(18)
#define LINLONDP_ERR_AXIE            BIT_ULL(19)
#define LINLONDP_ERR_ACE0            BIT_ULL(20)
#define LINLONDP_ERR_ACE1            BIT_ULL(21)
#define LINLONDP_ERR_ACE2            BIT_ULL(22)
#define LINLONDP_ERR_ACE3            BIT_ULL(23)
#define LINLONDP_ERR_DRIFTTO        BIT_ULL(24)
#define LINLONDP_ERR_FRAMETO        BIT_ULL(25)
#define LINLONDP_ERR_CSCE            BIT_ULL(26)
#define LINLONDP_ERR_ZME            BIT_ULL(27)
#define LINLONDP_ERR_MERR            BIT_ULL(28)
#define LINLONDP_ERR_TCF            BIT_ULL(29)
#define LINLONDP_ERR_TTNG            BIT_ULL(30)
#define LINLONDP_ERR_TTF            BIT_ULL(31)

#define CIX_SIP_DP_GOP_CTRL           (0xc200000f)
#define SKY1_SIP_DP_GOP_GET           0x1
#define SKY1_SIP_DP_GOP_SET           0x2
#define DPU_GOP_MASK                  0x00020000
#define DPU_GOP_SHIFT                  17

#define LINLONDP_ERR_EVENTS    \
    (LINLONDP_EVENT_URUN    | LINLONDP_EVENT_IBSY    | LINLONDP_EVENT_OVR |\
    LINLONDP_ERR_TETO        | LINLONDP_ERR_TEMR    | LINLONDP_ERR_TITR |\
    LINLONDP_ERR_CPE        | LINLONDP_ERR_CFGE    | LINLONDP_ERR_AXIE |\
    LINLONDP_ERR_ACE0        | LINLONDP_ERR_ACE1    | LINLONDP_ERR_ACE2 |\
    LINLONDP_ERR_ACE3        | LINLONDP_ERR_DRIFTTO    | LINLONDP_ERR_FRAMETO |\
    LINLONDP_ERR_ZME        | LINLONDP_ERR_MERR    | LINLONDP_ERR_TCF |\
    LINLONDP_ERR_TTNG        | LINLONDP_ERR_TTF)

#define LINLONDP_WARN_EVENTS    \
    (LINLONDP_ERR_CSCE | LINLONDP_EVENT_FULL | LINLONDP_EVENT_EMPTY)

#define LINLONDP_INFO_EVENTS (0 \
                | LINLONDP_EVENT_VSYNC \
                | LINLONDP_EVENT_FLIP \
                | LINLONDP_EVENT_EOW \
                | LINLONDP_EVENT_MODE \
                )

/* pipeline DT ports */
enum {
    LINLONDP_OF_PORT_OUTPUT        = 0,
    LINLONDP_OF_PORT_COPROC        = 1,
};

/* pipeline ACPI ports */
enum {
    LINLONDP_ACPI_PORT_OUTPUT        = 0,
    LINLONDP_ACPI_PORT_COPROC        = 1,
};

struct linlondp_chip_info {
    u32 arch_id;
    u32 core_id;
    u32 core_info;
    u32 bus_width;
};

struct linlondp_dev;

struct linlondp_events {
    u64 global;
    u64 pipes[LINLONDP_MAX_PIPELINES];
};

/**
 * struct linlondp_dev_funcs
 *
 * Supplied by chip level and returned by the chip entry function xxx_identify,
 */
struct linlondp_dev_funcs {
    /**
     * @init_format_table:
     *
     * initialize &linlondp_dev->format_table, this function should be called
     * before the &enum_resource
     */
    void (*init_format_table)(struct linlondp_dev *mdev);
    /**
     * @enum_resources:
     *
     * for CHIP to report or add pipeline and component resources to CORE
     */
    int (*enum_resources)(struct linlondp_dev *mdev);

    /* Optional: Notify HW to do some hw initialization, generally chip
     * supply this function to configure the register to default value
     */
        int (*init_hw)(struct linlondp_dev *mdev);
    /** @cleanup: call to chip to cleanup linlondp_dev->chip data */
    void (*cleanup)(struct linlondp_dev *mdev);
    /** @connect_iommu: Optional, connect to external iommu */
    int (*connect_iommu)(struct linlondp_dev *mdev);
    /** @disconnect_iommu: Optional, disconnect to external iommu */
    int (*disconnect_iommu)(struct linlondp_dev *mdev);
    /**
     * @irq_handler:
     *
     * for CORE to get the HW event from the CHIP when interrupt happened.
     */
    irqreturn_t (*irq_handler)(struct linlondp_dev *mdev,
                   struct linlondp_events *events);
    /** @enable_irq: enable irq */
    int (*enable_irq)(struct linlondp_dev *mdev);
    /** @disable_irq: disable irq */
    int (*disable_irq)(struct linlondp_dev *mdev);
    /** @on_off_vblank: notify HW to on/off vblank */
    void (*on_off_vblank)(struct linlondp_dev *mdev,
                  int master_pipe, bool on);

    /** @dump_register: Optional, dump registers to seq_file */
    void (*dump_register)(struct linlondp_dev *mdev, struct seq_file *seq);
    /**
     * @change_opmode:
     *
     * Notify HW to switch to a new display operation mode.
     */
    int (*change_opmode)(struct linlondp_dev *mdev, int new_mode);
    /** @flush: Notify the HW to flush or kickoff the update */
    void (*flush)(struct linlondp_dev *mdev,
              int master_pipe, u32 active_pipes);

    /** @close_gop: close dpu if gop is enabled */
    void (*close_gop)(struct linlondp_dev *mdev);
};

/*
 * DISPLAY_MODE describes how many display been enabled, and which will be
 * passed to CHIP by &linlondp_dev_funcs->change_opmode(), then CHIP can do the
 * pipeline resources assignment according to this usage hint.
 * -   LINLONDP_MODE_DISP0: Only one display enabled, pipeline-0 work as master.
 * -   LINLONDP_MODE_DISP1: Only one display enabled, pipeline-0 work as master.
 * -   LINLONDP_MODE_DUAL_DISP: Dual display mode, both display has been enabled.
 * And DP supports assign two pipelines to one single display on mode
 * LINLONDP_MODE_DISP0/DISP1
 */
enum {
    LINLONDP_MODE_INACTIVE    = 0,
    LINLONDP_MODE_DISP0    = BIT(0),
    LINLONDP_MODE_DISP1    = BIT(1),
    LINLONDP_MODE_DUAL_DISP    = LINLONDP_MODE_DISP0 | LINLONDP_MODE_DISP1,
};

/**
 * struct linlondp_dev
 *
 * Pipeline and component are used to describe how to handle the pixel data.
 * linlondp_device is for describing the whole view of the device, and the
 * control-abilites of device.
 */
struct linlondp_dev {
    /** @id device id */
    u32 id;

    /** @enabled_by_gop indicates whether device is enabled in gop */
    u32 enabled_by_gop;

    /** @dev: the base device structure */
    struct device *dev;
    /** @reg_base: the base address of linlondp io space */
    u32 __iomem   *reg_base;
    /** @dma_parms: the dma parameters of linlondp */

    /** @chip: the basic chip information */
    struct linlondp_chip_info chip;
    /** @fmt_tbl: initialized by &linlondp_dev_funcs->init_format_table */
    struct linlondp_format_caps_table fmt_tbl;
    /** @aclk: HW main engine clk */
    struct clk *aclk;
    /** @aclk_freq: HW main engine clk frequency(hz) */
    unsigned long aclk_freq;
    /** @aclk_freq_fixed: HW main engine clk frequency(hz) specified by dts or sysfs */
    unsigned long aclk_freq_fixed;
    /** @smart_aclk_freq: select HW main engine clk frequency from lut */
    bool smart_aclk_freq;
    struct reset_control *ip_rst;
    struct reset_control *rcsu_rst;

    /** @irq: irq number */
    int irq;

    /** @shutdown: if the device has been shutdown */
    bool shutdown;

    /**
     * @side_by_side:
     *
     * on sbs the whole display frame will be split to two halves (1:2),
     * master pipeline handles the left part, slave for the right part
     */
    bool side_by_side;
    /** @side_by_side_master: master pipe id for side by side */
    int side_by_side_master;

    /** @lock: used to protect dpmode */
    struct mutex lock;
    /** @dpmode: current display mode */
    u32 dpmode;

    /** @n_pipelines: the number of pipe in @pipelines */
    int n_pipelines;
    /** @pipelines: the linlondp pipelines */
    struct linlondp_pipeline *pipelines[LINLONDP_MAX_PIPELINES];

    /** @funcs: chip funcs to access to HW */
    const struct linlondp_dev_funcs *funcs;
    /**
     * @chip_data:
     *
     * chip data will be added by &linlondp_dev_funcs.enum_resources() and
     * destroyed by &linlondp_dev_funcs.cleanup()
     */
    void *chip_data;

    /** @iommu: iommu domain */
    struct iommu_domain *iommu;

    /** @debugfs_root: root directory of linlondp debugfs */
    struct dentry *debugfs_root;
    /**
     * @err_verbosity: bitmask for how much extra info to print on error
     *
     * See LINLONDP_DEV_* macros for details. Low byte contains the debug
     * level categories, the high byte contains extra debug options.
     */
    u16 err_verbosity;
    /* Print a single line per error per frame with error events. */
#define LINLONDP_DEV_PRINT_ERR_EVENTS BIT(0)
    /* Print a single line per warning per frame with error events. */
#define LINLONDP_DEV_PRINT_WARN_EVENTS BIT(1)
    /* Print a single line per info event per frame with error events. */
#define LINLONDP_DEV_PRINT_INFO_EVENTS BIT(2)
    /* Dump DRM state on an error or warning event. */
#define LINLONDP_DEV_PRINT_DUMP_STATE_ON_EVENT BIT(8)
    /* Disable rate limiting of event prints (normally one per commit) */
#define LINLONDP_DEV_PRINT_DISABLE_RATELIMIT BIT(12)
};

static inline bool
linlondp_product_match(struct linlondp_dev *mdev, u32 target)
{
    return LINLONDP_CORE_ID_PRODUCT_ID(mdev->chip.core_id) == target;
}

typedef const struct linlondp_dev_funcs *
(*linlondp_identify_func)(u32 __iomem *reg, struct linlondp_chip_info *chip);

const struct linlondp_dev_funcs *
dp_identify(u32 __iomem *reg, struct linlondp_chip_info *chip);

struct linlondp_dev *linlondp_dev_create(struct device *dev);
void linlondp_dev_destroy(struct linlondp_dev *mdev);

struct linlondp_dev *dev_to_mdev(struct device *dev);

void linlondp_print_events(struct linlondp_events *evts, struct drm_device *dev);

int linlondp_dev_resume(struct linlondp_dev *mdev);
int linlondp_dev_suspend(struct linlondp_dev *mdev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 0, 0)
struct fwnode_handle *
fwnode_graph_get_remote_node(const struct fwnode_handle *fwnode, u32 port_id,
			     u32 endpoint_id);
#endif

#endif /*_LINLONDP_DEV_H_*/
