/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2015-2023, NVIDIA Corporation.  All rights reserved.
 *
 * camera_common.h - utilities for tegra camera driver
 */

#ifndef __camera_common__
#define __camera_common__

#include <linux/clk.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/platform_device.h>
#include <linux/v4l2-mediabus.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <linux/module.h>

#include "camera_version_utils.h"
#include "nvc_focus.h"
#include "sensor_common.h"
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-ctrls.h>
#include "tegracam_core.h"

/*
 * Scaling factor for converting a Q10.22 fixed point value
 * back to its original floating point value
 */
#define FIXED_POINT_SCALING_FACTOR (1ULL << 22)

struct reg_8 {
	u16 addr;
	u8 val;
};

struct reg_16 {
	u16 addr;
	u16 val;
};

struct camera_common_power_rail {
	struct regulator *dvdd;
	struct regulator *avdd;
	struct regulator *iovdd;
	struct regulator *vcmvdd;
	struct clk *mclk;
	unsigned int pwdn_gpio;
	unsigned int reset_gpio;
	unsigned int af_gpio;
	bool state;
};

struct camera_common_regulators {
	const char *avdd;
	const char *dvdd;
	const char *iovdd;
	const char *vcmvdd;
};

struct camera_common_pdata {
	const char *mclk_name; /* NULL for default default_mclk */
	const char *parentclk_name; /* NULL for no parent clock*/
	unsigned int pwdn_gpio;
	unsigned int reset_gpio;
	unsigned int af_gpio;
	bool ext_reg;
	int (*power_on)(struct camera_common_power_rail *pw);
	int (*power_off)(struct camera_common_power_rail *pw);
	struct camera_common_regulators regulators;
	bool use_cam_gpio;
	bool has_eeprom;
	bool v_flip;
	bool h_mirror;
	unsigned int fuse_id_addr;
	unsigned int avdd_latency;
	unsigned int eeprom_id_addr;
};

struct camera_common_eeprom_data {
	struct i2c_client *i2c_client;
	struct i2c_adapter *adap;
	struct i2c_board_info brd;
	struct regmap *regmap;
};

int
regmap_util_write_table_8(struct regmap *regmap,
			  const struct reg_8 table[],
			  const struct reg_8 override_list[],
			  int num_override_regs,
			  u16 wait_ms_addr, u16 end_addr);

int
regmap_util_write_table_16_as_8(struct regmap *regmap,
				const struct reg_16 table[],
				const struct reg_16 override_list[],
				int num_override_regs,
				u16 wait_ms_addr, u16 end_addr);

enum switch_state {
	SWITCH_OFF,
	SWITCH_ON,
};

static const s64 switch_ctrl_qmenu[] = {
	SWITCH_OFF, SWITCH_ON
};

/*
 * The memory buffers allocated from nvrm are aligned to
 * fullfill the hardware requirements:
 * - size in alignment with a multiple of 128K/64K bytes,
 * see CL http://git-master/r/256468 and bug 1321091.
 */
static const s64 size_align_ctrl_qmenu[] = {
	1, (64 * 1024), (128 * 1024),
};

struct camera_common_frmfmt {
	struct v4l2_frmsize_discrete	size;
	const int	*framerates;
	int	num_framerates;
	bool	hdr_en;
	int	mode;
};

struct camera_common_colorfmt {
	unsigned int			code;
	enum v4l2_colorspace		colorspace;
	int				pix_fmt;
	enum v4l2_xfer_func		xfer_func;
	enum v4l2_ycbcr_encoding	ycbcr_enc;
	enum v4l2_quantization		quantization;
};

struct camera_common_framesync {
	u32 inck;		/* kHz */
	u32 xhs;		/* in inck */
	u32 xvs;		/* in xhs */
	u32 fps;		/* frames in 1000 second */
};

struct tegracam_device;
struct camera_common_data;

struct camera_common_sensor_ops {
	u32 numfrmfmts;
	const struct camera_common_frmfmt *frmfmt_table;
	int (*power_on)(struct camera_common_data *s_data);
	int (*power_off)(struct camera_common_data *s_data);
	int (*write_reg)(struct camera_common_data *s_data,
	  u16 addr, u8 val);
	int (*read_reg)(struct camera_common_data *s_data,
	  u16 addr, u8 *val);
	struct camera_common_pdata *(*parse_dt)(struct tegracam_device *tc_dev);
	int (*power_get)(struct tegracam_device *tc_dev);
	int (*power_put)(struct tegracam_device *tc_dev);
	int (*get_framesync)(struct camera_common_data *s_data,
		struct camera_common_framesync *vshs);
	int (*set_mode)(struct tegracam_device *tc_dev);
	//int (*start_streaming)(struct tegracam_device *tc_dev);
	int (*stop_streaming)(struct tegracam_device *tc_dev);
};

struct tegracam_sensor_data {
	struct sensor_blob mode_blob;
	struct sensor_blob ctrls_blob;
};

struct tegracam_ctrl_ops {
	u32 numctrls;
	u32 string_ctrl_size[TEGRA_CAM_MAX_STRING_CONTROLS];
	u32 compound_ctrl_size[TEGRA_CAM_MAX_COMPOUND_CONTROLS];
	const u32 *ctrl_cid_list;
	bool is_blob_supported;
	int (*set_gain)(struct tegracam_device *tc_dev, s64 val);
	int (*set_exposure)(struct tegracam_device *tc_dev, s64 val);
	int (*set_exposure_short)(struct tegracam_device *tc_dev, s64 val);
	int (*set_frame_rate)(struct tegracam_device *tc_dev, s64 val);
	int (*set_group_hold)(struct tegracam_device *tc_dev, bool val);
	int (*set_alternating_exposure)(struct tegracam_device *tc_dev,
			struct alternating_exposure_cfg *val);
	int (*fill_string_ctrl)(struct tegracam_device *tc_dev,
				struct v4l2_ctrl *ctrl);
	int (*fill_compound_ctrl)(struct tegracam_device *tc_dev,
				struct v4l2_ctrl *ctrl);
	int (*set_gain_ex)(struct tegracam_device *tc_dev,
			struct sensor_blob *blob, s64 val);
	int (*set_exposure_ex)(struct tegracam_device *tc_dev,
			struct sensor_blob *blob, s64 val);
	int (*set_frame_rate_ex)(struct tegracam_device *tc_dev,
			struct sensor_blob *blob, s64 val);
	int (*set_group_hold_ex)(struct tegracam_device *tc_dev,
			struct sensor_blob *blob, bool val);
};

struct tegracam_ctrl_handler {
	struct v4l2_ctrl_handler	ctrl_handler;
	const struct tegracam_ctrl_ops	*ctrl_ops;
	struct tegracam_device          *tc_dev;
	struct tegracam_sensor_data	sensor_data;

	int				numctrls;
	struct v4l2_ctrl		*ctrls[MAX_CID_CONTROLS];
};

struct camera_common_data {
	struct camera_common_sensor_ops		*ops;
	struct v4l2_ctrl_handler		*ctrl_handler;
	struct device				*dev;
	const struct camera_common_frmfmt	*frmfmt;
	const struct camera_common_colorfmt	*colorfmt;
	struct dentry				*debugdir;
	struct camera_common_power_rail		*power;

	struct v4l2_subdev			subdev;
	struct v4l2_ctrl			**ctrls;
	struct module				*owner;

	struct sensor_properties		sensor_props;
	/* TODO: cleanup neeeded once all the sensors adapt new framework */
	struct tegracam_ctrl_handler		*tegracam_ctrl_hdl;
	struct regmap				*regmap;
	struct camera_common_pdata		*pdata;
	/* TODO: cleanup needed for priv once all the sensors adapt new framework */
	void *priv;
	int numctrls;
	int csi_port;
	int numlanes;
	int mode;
	int mode_prop_idx;
	int numfmts;
	int def_mode, def_width, def_height;
	int def_clk_freq;
	int fmt_width, fmt_height;
	int sensor_mode_id;
	bool use_sensor_mode_id;
	bool override_enable;
	u32 version;
};

struct camera_common_focuser_data;

struct camera_common_focuser_ops {
	int (*power_on)(struct camera_common_focuser_data *s_data);
	int (*power_off)(struct camera_common_focuser_data *s_data);
	int (*load_config)(struct camera_common_focuser_data *s_data);
	int (*ctrls_init)(struct camera_common_focuser_data *s_data);
};

struct camera_common_focuser_data {
	struct camera_common_focuser_ops	*ops;
	struct v4l2_ctrl_handler		*ctrl_handler;
	struct v4l2_subdev			subdev;
	struct v4l2_ctrl			**ctrls;
	struct device				*dev;

	struct nv_focuser_config		config;
	void					*priv;
	int					pwr_dev;
	int					def_position;
};

static inline void msleep_range(unsigned int delay_base)
{
	usleep_range(delay_base * 1000, delay_base * 1000 + 500);
}

static inline struct camera_common_data *to_camera_common_data(
	const struct device *dev)
{
	if (sensor_common_parse_num_modes(dev))
		return container_of(dev_get_drvdata(dev),
		    struct camera_common_data, subdev);
	return NULL;
}

static inline struct camera_common_focuser_data *to_camera_common_focuser_data(
	const struct device *dev)
{
	return container_of(dev_get_drvdata(dev),
			    struct camera_common_focuser_data, subdev);
}

int camera_common_g_ctrl(struct camera_common_data *s_data,
			 struct v4l2_control *control);

int camera_common_regulator_get(struct device *dev,
		       struct regulator **vreg, const char *vreg_name);
int camera_common_parse_clocks(struct device *dev,
			struct camera_common_pdata *pdata);
int camera_common_parse_ports(struct device *dev,
			      struct camera_common_data *s_data);
int camera_common_mclk_enable(struct camera_common_data *s_data);
void camera_common_mclk_disable(struct camera_common_data *s_data);
int camera_common_parse_general_properties(struct device *dev,
			      struct camera_common_data *s_data);

int camera_common_debugfs_show(struct seq_file *s, void *unused);
ssize_t camera_common_debugfs_write(
	struct file *file,
	char const __user *buf,
	size_t count,
	loff_t *offset);
int camera_common_debugfs_open(struct inode *inode, struct file *file);
void camera_common_remove_debugfs(struct camera_common_data *s_data);
void camera_common_create_debugfs(struct camera_common_data *s_data,
		const char *name);

const struct camera_common_colorfmt *camera_common_find_datafmt(
		unsigned int code);
int camera_common_enum_mbus_code(struct v4l2_subdev *sd,
				struct v4l2_subdev_state *state,
				struct v4l2_subdev_mbus_code_enum *code);
int camera_common_enum_fmt(struct v4l2_subdev *sd, unsigned int index,
			unsigned int *code);
int camera_common_try_fmt(struct v4l2_subdev *sd,
			   struct v4l2_mbus_framefmt *mf);
int camera_common_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf);
int camera_common_g_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *mf);
int camera_common_enum_framesizes(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *state,
		struct v4l2_subdev_frame_size_enum *fse);
int camera_common_enum_frameintervals(struct v4l2_subdev *sd,
		struct v4l2_subdev_state *state,
		struct v4l2_subdev_frame_interval_enum *fie);
int camera_common_set_power(struct camera_common_data *data, int on);
int camera_common_s_power(struct v4l2_subdev *sd, int on);
void camera_common_dpd_disable(struct camera_common_data *s_data);
void camera_common_dpd_enable(struct camera_common_data *s_data);
int camera_common_get_mbus_config(struct v4l2_subdev *sd,
			      unsigned int pad,
			      struct v4l2_mbus_config *cfg);
int camera_common_get_framesync(struct v4l2_subdev *sd,
		struct camera_common_framesync *vshs);

/* Common initialize and cleanup for camera */
int camera_common_initialize(struct camera_common_data *s_data,
		const char *dev_name);
void camera_common_cleanup(struct camera_common_data *s_data);

/* Focuser */
int camera_common_focuser_init(struct camera_common_focuser_data *s_data);
int camera_common_focuser_s_power(struct v4l2_subdev *sd, int on);

const struct camera_common_colorfmt *camera_common_find_pixelfmt(
	unsigned int pix_fmt);

/* common control layer init */
int tegracam_ctrl_synchronize_ctrls(struct tegracam_ctrl_handler *handler);
int tegracam_ctrl_set_overrides(struct tegracam_ctrl_handler *handler);
int tegracam_ctrl_handler_init(struct tegracam_ctrl_handler *handler);
int tegracam_init_ctrl_ranges(struct tegracam_ctrl_handler *handler);
int tegracam_init_ctrl_ranges_by_mode(
		struct tegracam_ctrl_handler *handler,
		u32 modeidx);

/* Regmap / RTCPU I2C driver interface */
struct tegra_i2c_rtcpu_sensor;
struct tegra_i2c_rtcpu_config;

struct camera_common_i2c {
	struct regmap *regmap;
	struct tegra_i2c_rtcpu_sensor *rt_sensor;
};

int camera_common_i2c_init(
	struct camera_common_i2c *sensor,
	struct i2c_client *client,
	struct regmap_config *regmap_config,
	const struct tegra_i2c_rtcpu_config *rtcpu_config);

int camera_common_i2c_aggregate(
	struct camera_common_i2c *sensor,
	bool start);

int camera_common_i2c_set_frame_id(
	struct camera_common_i2c *sensor,
	int frame_id);

int camera_common_i2c_read_reg8(
	struct camera_common_i2c *sensor,
	unsigned int addr,
	u8 *data,
	unsigned int count);

int camera_common_i2c_write_reg8(
	struct camera_common_i2c *sensor,
	unsigned int addr,
	const u8 *data,
	unsigned int count);

int camera_common_i2c_write_table_8(
	struct camera_common_i2c *sensor,
	const struct reg_8 table[],
	const struct reg_8 override_list[],
	int num_override_regs, u16 wait_ms_addr, u16 end_addr);

#endif /* __camera_common__ */
