/*
 * vin_ispstat.h
 *
 */

#ifndef VIN_ISP_STAT_H
#define VIN_ISP_STAT_H

#include <linux/types.h>
#include <media/v4l2-event.h>
#include <media/v4l2-device.h>

#include "../vin-isp/sunxi_isp.h"

#define STAT_MAX_BUFS		5
#define STAT_NEVENTS		8

#define STAT_BUF_DONE		0	/* Buffer is ready */
#define STAT_NO_BUF		1	/* An error has occurred */

struct isp_stat;

struct ispstat_buffer {
	void *virt_addr;
	void *dma_addr;
	void *phy_addr;
	struct timespec ts;
	u32 buf_size;
	u32 frame_number;
	u16 config_counter;
	u8 empty;
};

struct ispstat_ops {
	/*
	 * Validate new params configuration.
	 * new_conf->buf_size value must be changed to the exact buffer size
	 * necessary for the new configuration if it's smaller.
	 */
	int (*validate_params)(struct isp_stat *stat, void *new_conf);

	/*
	 * Save new params configuration.
	 * stat->priv->buf_size value must be set to the exact buffer size for
	 * the new configuration.
	 * stat->update is set to 1 if new configuration is different than
	 * current one.
	 */
	void (*set_params)(struct isp_stat *stat, void *new_conf);

	/* Apply stored configuration. */
	void (*setup_regs)(struct isp_stat *stat, void *priv);

	/* Enable/Disable module. */
	void (*enable)(struct isp_stat *stat, int enable);

	/* Verify is module is busy. */
	int (*busy)(struct isp_stat *stat);

	/* Used for specific operations during generic buf process task. */
	int (*buf_process)(struct isp_stat *stat);
};

enum ispstat_state_t {
	ISPSTAT_DISABLED = 0,
	ISPSTAT_DISABLING,
	ISPSTAT_ENABLED,
	ISPSTAT_ENABLING,
	ISPSTAT_SUSPENDED,
};

struct isp_stat {
	struct v4l2_subdev sd;
	struct media_pad pad;	/* sink pad */

	/* Control */
	unsigned configured:1;
	unsigned update:1;
	unsigned buf_processing:1;
	unsigned ovl_recover:1;
	u8 inc_config;
	atomic_t buf_err;
	enum ispstat_state_t state;	/* enabling/disabling state */
	struct isp_dev *isp;
	void *priv;		/* pointer to priv config struct */
	void *recover_priv;	/* pointer to recover priv configuration */
	struct mutex ioctl_lock; /* serialize private ioctl */

	const struct ispstat_ops *ops;

	/* Buffer */
	u16 config_counter;
	u32 frame_number;
	u32 buf_size;
	u32 buf_alloc_size;
	unsigned long event_type;
	struct vin_mm ion_man[STAT_MAX_BUFS]; /* for ion alloc/free manage */
	struct ispstat_buffer *buf;
	struct ispstat_buffer *active_buf;
	struct ispstat_buffer *locked_buf;
};

struct ispstat_generic_config {
	u32 buf_size;
	u16 config_counter;
};
/*
struct vin_isp_stat_data {
	struct timeval ts;
	void __user *buf;
	__u32 buf_size;
	__u16 frame_number;
	__u16 cur_frame;
	__u16 config_counter;
};
*/
int vin_isp_stat_config(struct isp_stat *stat, void *new_conf);
int vin_isp_stat_request_statistics(struct isp_stat *stat,
				     struct vin_isp_stat_data *data);
int vin_isp_stat_init(struct isp_stat *stat, const char *name,
		       const struct v4l2_subdev_ops *sd_ops);
void vin_isp_stat_cleanup(struct isp_stat *stat);
int vin_isp_stat_subscribe_event(struct v4l2_subdev *subdev,
				  struct v4l2_fh *fh,
				  struct v4l2_event_subscription *sub);
int vin_isp_stat_unsubscribe_event(struct v4l2_subdev *subdev,
				    struct v4l2_fh *fh,
				    struct v4l2_event_subscription *sub);
int vin_isp_stat_s_stream(struct v4l2_subdev *subdev, int enable);

int vin_isp_stat_pcr_busy(struct isp_stat *stat);
int vin_isp_stat_busy(struct isp_stat *stat);
void vin_isp_stat_suspend(struct isp_stat *stat);
void vin_isp_stat_resume(struct isp_stat *stat);
int vin_isp_stat_enable(struct isp_stat *stat, u8 enable);
void vin_isp_stat_overflow(struct isp_stat *stat);
void vin_isp_stat_isr(struct isp_stat *stat);
void vin_isp_stat_isr_frame_sync(struct isp_stat *stat);

#endif /* VIN_ISP_STAT_H */
