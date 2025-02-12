// SPDX-License-Identifier: GPL-2.0
/*
 * x1vi.c - x1isp vi platform device driver
 *
 * Copyright(C) 2023 KY Micro Limited
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/of_device.h>
#include <linux/slab.h>
#include "x1vi.h"
#include "fe_isp.h"
#include "../ky_videobuf2.h"
#include "../vdev.h"
#include "../subdev.h"
#include "../vsensor.h"
#define CAM_MODULE_TAG CAM_MDL_VI
#include <cam_dbg.h>

static DEFINE_MUTEX(g_init_mlock);
static struct isp_firm *g_isp_firm = NULL;
static struct platform_device *g_pdev = NULL;
static struct v4l2_device *g_v4l2_dev = NULL;

static int x1vi_create_entities(struct platform_device *pdev, struct x1vi_platform_data *drvdata)
{
	int i = 0;
	char name[KY_VI_ENTITY_NAME_LEN];
	struct device *dev = &pdev->dev;
	unsigned int min_buffers_needed = 0;

	//2 SENSORs
	for (i = 0; i < SENSOR_NUM; i++) {
		drvdata->entities[MIPI][SENSOR][i] = (struct media_entity*)spm_sensor_create(SD_GRP_ID(MIPI, SENSOR, i), dev);
		if (!drvdata->entities[MIPI][SENSOR][i]) {
			cam_err("%s create sensor%d failed.", __func__, i);
			goto SENSORs_FAIL;
		}
	}
	//2 AINs
	for (i = 0; i < AIN_NUM; i++) {
		snprintf(name, 32, "ain%d", i);
		drvdata->entities[DMA][AIN][i] =
		    (struct media_entity *)spm_vdev_create_vnode(name, KY_VNODE_DIR_IN, i, g_v4l2_dev, dev, 0);
		if (!drvdata->entities[DMA][AIN][i]) {
			cam_err("%s create ain%d failed.", __func__, i);
			goto AINs_FAIL;
		}
	}
	//14 AOUTs
	for (i = 0; i < AOUT_NUM; i++) {
		snprintf(name, 32, "aout%d", i);
		if (i == 12 || i == 13) {	//raw dump
			min_buffers_needed = 0;
		} else {
			min_buffers_needed = 0;
		}
		drvdata->entities[DMA][AOUT][i] =
		    (struct media_entity *)spm_vdev_create_vnode(name, KY_VNODE_DIR_OUT, i, g_v4l2_dev, dev,
								 min_buffers_needed);
		if (!drvdata->entities[DMA][AOUT][i]) {
			cam_err("%s create aout%d failed.", __func__, i);
			goto AOUTs_FAIL;
		}
	}
	//2 RAWDUMPs
	for (i = 0; i < RAWDUMP_NUM; i++) {
		drvdata->entities[FE_ISP][RAWDUMP][i] =
		    (struct media_entity *)fe_rawdump_create(SD_GRP_ID(FE_ISP, RAWDUMP, i), drvdata->isp_ctx);
		if (!drvdata->entities[FE_ISP][RAWDUMP][i]) {
			cam_err("%s create rawdump%d failed.", __func__, i);
			goto RAWDUMPs_FAIL;
		}
	}
	//OFFLINE_CHANNELs
	for (i = 0; i < OFFLINE_CH_NUM; i++) {
		drvdata->entities[FE_ISP][OFFLINE_CHANNEL][i] =
		    (struct media_entity *)fe_offline_channel_create(SD_GRP_ID(FE_ISP, OFFLINE_CHANNEL, i), drvdata->isp_ctx);
		if (!drvdata->entities[FE_ISP][OFFLINE_CHANNEL][i]) {
			cam_err("%s create offline_channel%d failed", __func__, i);
			goto OFFLINE_CHANNELs_FAIL;
		}
	}
	//FORMATTERs
	for (i = 0; i < FORMATTER_NUM; i++) {
		drvdata->entities[FE_ISP][FORMATTER][i] =
		    (struct media_entity *)fe_formatter_create(SD_GRP_ID(FE_ISP, FORMATTER, i), drvdata->isp_ctx);
		if (!drvdata->entities[FE_ISP][FORMATTER][i]) {
			cam_err("%s create formatter%d failed", __func__, i);
			goto FORMATTERs_FAIL;
		}
	}
	//DWT0
	for (i = 1; i <= DWT_LAYER_NUM; i++) {
		drvdata->entities[FE_ISP][DWT0][i] =
		    (struct media_entity *)fe_dwt_create(SD_GRP_ID(FE_ISP, DWT0, i),
							 drvdata->isp_ctx);
		if (!drvdata->entities[FE_ISP][DWT0][i]) {
			cam_err("%s create dwt0_layer%d failed", __func__, i);
			goto DWT0_FAIL;
		}
	}
	//DWT1
	for (i = 1; i <= DWT_LAYER_NUM; i++) {
		drvdata->entities[FE_ISP][DWT1][i] =
		    (struct media_entity *)fe_dwt_create(SD_GRP_ID(FE_ISP, DWT1, i), drvdata->isp_ctx);
		if (!drvdata->entities[FE_ISP][DWT1][i]) {
			cam_err("%s create dwt1_layer%d failed", __func__, i);
			goto DWT1_FAIL;
		}
	}
	//PIPEs
	for (i = 0; i < PIPE_NUM; i++) {
		drvdata->entities[FE_ISP][PIPE][i] =
		    (struct media_entity *)fe_pipe_create(SD_GRP_ID(FE_ISP, PIPE, i), drvdata->isp_ctx);
		if (!drvdata->entities[FE_ISP][PIPE][i]) {
			cam_err("%s create pipe%d failed", __func__, i);
			goto PIPEs_FAIL;
		}
	}
	//HDR_COMBINE
	drvdata->entities[FE_ISP][HDR_COMBINE][0] =
	    (struct media_entity *)fe_hdr_combine_create(SD_GRP_ID(FE_ISP, HDR_COMBINE, 0), drvdata->isp_ctx);
	if (!drvdata->entities[FE_ISP][HDR_COMBINE][0]) {
		cam_err("%s create hdr_combine failed", __func__);
		goto HDR_COMBINE_FAIL;
	}
	//3 CSI_MAINs
	for (i = 0; i < CSI_NUM; i++) {
		drvdata->entities[MIPI][CSI_MAIN][i] =
		    (struct media_entity *)csi_create(SD_GRP_ID(MIPI, CSI_MAIN, i), drvdata->isp_ctx);
		if (!drvdata->entities[MIPI][CSI_MAIN][i]) {
			cam_err("%s create csi%d_main failed", __func__, i);
			goto CSI_MAIN_FAIL;
		}
	}
	//3 CSI_VCDTs
	for (i = 0; i < CSI_NUM; i++) {
		drvdata->entities[MIPI][CSI_VCDT][i] =
		    (struct media_entity *)csi_create(SD_GRP_ID(MIPI, CSI_VCDT, i), drvdata->isp_ctx);
		if (!drvdata->entities[MIPI][CSI_VCDT][i]) {
			cam_err("%s create csi%d_main failed", __func__, i);
			goto CSI_VCDT_FAIL;
		}
	}
	return 0;
CSI_VCDT_FAIL:
	while (i > 0) {
		spm_camera_block_put(drvdata->entities[MIPI][CSI_VCDT][--i]);
		drvdata->entities[MIPI][CSI_VCDT][i] = NULL;
	}
	i = CSI_NUM;
CSI_MAIN_FAIL:
	while (i > 0) {
		spm_camera_block_put(drvdata->entities[MIPI][CSI_MAIN][--i]);
		drvdata->entities[MIPI][CSI_MAIN][i] = NULL;
	}
	spm_camera_block_put(drvdata->entities[FE_ISP][HDR_COMBINE][0]);
	drvdata->entities[FE_ISP][HDR_COMBINE][0] = NULL;
HDR_COMBINE_FAIL:
	i = PIPE_NUM;
PIPEs_FAIL:
	while (i > 0) {
		spm_camera_block_put(drvdata->entities[FE_ISP][PIPE][--i]);
		drvdata->entities[FE_ISP][PIPE][i] = NULL;
	}
	i = DWT_LAYER_NUM + 1;
DWT1_FAIL:
	while (i > 1) {
		spm_camera_block_put(drvdata->entities[FE_ISP][DWT1][--i]);
		drvdata->entities[FE_ISP][DWT1][i] = NULL;
	}
	i = DWT_LAYER_NUM + 1;
DWT0_FAIL:
	while (i > 1) {
		spm_camera_block_put(drvdata->entities[FE_ISP][DWT0][--i]);
		drvdata->entities[FE_ISP][DWT0][i] = NULL;
	}
	i = FORMATTER_NUM;
FORMATTERs_FAIL:
	while (i > 0) {
		spm_camera_block_put(drvdata->entities[FE_ISP][FORMATTER][--i]);
		drvdata->entities[FE_ISP][FORMATTER][i] = NULL;
	}
	i = OFFLINE_CH_NUM;
OFFLINE_CHANNELs_FAIL:
	while (i > 0) {
		spm_camera_block_put(drvdata->entities[FE_ISP][OFFLINE_CHANNEL][--i]);
		drvdata->entities[FE_ISP][OFFLINE_CHANNEL][i] = NULL;
	}
	i = RAWDUMP_NUM;
RAWDUMPs_FAIL:
	while (i > 0) {
		spm_camera_block_put(drvdata->entities[FE_ISP][RAWDUMP][--i]);
		drvdata->entities[FE_ISP][RAWDUMP][i] = NULL;
	}
	i = AOUT_NUM;
AOUTs_FAIL:
	while (i > 0) {
		spm_camera_block_put(drvdata->entities[DMA][AOUT][--i]);
		drvdata->entities[DMA][AOUT][i] = NULL;
	}
	i = AIN_NUM;
AINs_FAIL:
	while (i > 0) {
		spm_camera_block_put(drvdata->entities[DMA][AIN][--i]);
		drvdata->entities[DMA][AIN][i] = NULL;
	}
	i = SENSOR_NUM;
SENSORs_FAIL:
	while (i > 0) {
		spm_camera_block_put(drvdata->entities[MIPI][SENSOR][--i]);
		drvdata->entities[MIPI][SENSOR][i] = NULL;
	}
	return -1;
}

static void x1vi_release_entities(struct x1vi_platform_data *drvdata)
{
	int i = 0, j = 0, k = 0;

	for (i = 0; i < GRP_MAX; i++) {
		for (j = 0; j < SUB_MAX; j++) {
			for (k = 0; k < ID_MAX; k++) {
				if (drvdata->entities[i][j][k]) {
					spm_camera_block_put(drvdata->entities[i][j][k]);
					drvdata->entities[i][j][k] = NULL;
				}
			}
		}
	}
}

#define ENTITY(a, b, c)		(drvdata->entities[(a)][(b)][(c)])
#define CREATE_LINK(source, source_pad, sink, sink_pad)		do { \
	ret = KY_MEDIA_CREATE_LINK((source), (source_pad), (sink), (sink_pad)); \
	if (ret) { \
		cam_err("create link " #source "-" #source_pad " <=> " #sink "-" #sink_pad " failed."); \
		goto LINKS_FAIL; \
	} \
} while (0)

static int x1vi_create_entity_links(struct x1vi_platform_data *drvdata)
{
	int ret = 0, i = 0, j = 0, k = 0;

	//SENSOR => CSI_MAIN and CSI_VCDT links
	CREATE_LINK(ENTITY(MIPI, SENSOR, 0), SENSOR_PAD_CSI_MAIN,
		    ENTITY(MIPI, CSI_MAIN, 0), CSI_PAD_IN);
	CREATE_LINK(ENTITY(MIPI, SENSOR, 0), SENSOR_PAD_CSI_VCDT,
		    ENTITY(MIPI, CSI_VCDT, 0), CSI_PAD_IN);
	CREATE_LINK(ENTITY(MIPI, SENSOR, 1), SENSOR_PAD_CSI_MAIN,
		    ENTITY(MIPI, CSI_MAIN, 1), CSI_PAD_IN);
	CREATE_LINK(ENTITY(MIPI, SENSOR, 1), SENSOR_PAD_CSI_VCDT,
		    ENTITY(MIPI, CSI_VCDT, 1), CSI_PAD_IN);
	CREATE_LINK(ENTITY(MIPI, SENSOR, 2), SENSOR_PAD_CSI_MAIN,
		    ENTITY(MIPI, CSI_MAIN, 2), CSI_PAD_IN);
	CREATE_LINK(ENTITY(MIPI, SENSOR, 2), SENSOR_PAD_CSI_VCDT,
		    ENTITY(MIPI, CSI_VCDT, 2), CSI_PAD_IN);

	//CSI_MAIN => RAWDUMP links
	CREATE_LINK(ENTITY(MIPI, CSI_MAIN, 0), CSI_PAD_RAWDUMP0,
		    ENTITY(FE_ISP, RAWDUMP, 0), PAD_IN);
	CREATE_LINK(ENTITY(MIPI, CSI_MAIN, 0), CSI_PAD_RAWDUMP1,
		    ENTITY(FE_ISP, RAWDUMP, 1), PAD_IN);
	CREATE_LINK(ENTITY(MIPI, CSI_MAIN, 1), CSI_PAD_RAWDUMP0,
		    ENTITY(FE_ISP, RAWDUMP, 0), PAD_IN);
	CREATE_LINK(ENTITY(MIPI, CSI_MAIN, 1), CSI_PAD_RAWDUMP1,
		    ENTITY(FE_ISP, RAWDUMP, 1), PAD_IN);
	CREATE_LINK(ENTITY(MIPI, CSI_MAIN, 2), CSI_PAD_RAWDUMP0,
		    ENTITY(FE_ISP, RAWDUMP, 0), PAD_IN);
	CREATE_LINK(ENTITY(MIPI, CSI_MAIN, 2), CSI_PAD_RAWDUMP1,
		    ENTITY(FE_ISP, RAWDUMP, 1), PAD_IN);

	//CSI_MAIN => PIPE links
	CREATE_LINK(ENTITY(MIPI, CSI_MAIN, 0), CSI_PAD_PIPE0,
		    ENTITY(FE_ISP, PIPE, 0), PIPE_PAD_IN);
	CREATE_LINK(ENTITY(MIPI, CSI_MAIN, 0), CSI_PAD_PIPE1,
		    ENTITY(FE_ISP, PIPE, 1), PIPE_PAD_IN);
	CREATE_LINK(ENTITY(MIPI, CSI_MAIN, 1), CSI_PAD_PIPE0,
		    ENTITY(FE_ISP, PIPE, 0), PIPE_PAD_IN);
	CREATE_LINK(ENTITY(MIPI, CSI_MAIN, 1), CSI_PAD_PIPE1,
		    ENTITY(FE_ISP, PIPE, 1), PIPE_PAD_IN);
	CREATE_LINK(ENTITY(MIPI, CSI_MAIN, 2), CSI_PAD_PIPE0,
		    ENTITY(FE_ISP, PIPE, 0), PIPE_PAD_IN);
	CREATE_LINK(ENTITY(MIPI, CSI_MAIN, 2), CSI_PAD_PIPE1,
		    ENTITY(FE_ISP, PIPE, 1), PIPE_PAD_IN);

	//PIPE => FORMATTER links
	CREATE_LINK(ENTITY(FE_ISP, PIPE, 0), PIPE_PAD_F0OUT,
		    ENTITY(FE_ISP, FORMATTER, 0), FMT_PAD_IN);
	CREATE_LINK(ENTITY(FE_ISP, PIPE, 1), PIPE_PAD_F1OUT,
		    ENTITY(FE_ISP, FORMATTER, 1), FMT_PAD_IN);
/*
	//PIPE0 => RAWDUMP0 links
	CREATE_LINK(ENTITY(FE_ISP, PIPE, 0), PIPE_PAD_RAWDUMP0OUT,
				ENTITY(FE_ISP, RAWDUMP, 0), PAD_IN);
	//PIPE => HDR_COMBINE links
	CREATE_LINK(ENTITY(FE_ISP, PIPE, 0), PIPE_PAD_HDROUT,
				ENTITY(FE_ISP, HDR_COMBINE, 0), HDR_PAD_P0IN);
	CREATE_LINK(ENTITY(FE_ISP, PIPE, 1), PIPE_PAD_HDROUT,
				ENTITY(FE_ISP, HDR_COMBINE, 0), HDR_PAD_P1IN);
	//HDR_COMBINE => FORMATTER links
	for (i = 0; i < FORMATTER_NUM; i++) {
		CREATE_LINK(ENTITY(FE_ISP, HDR_COMBINE, 0), HDR_PAD_F0OUT + i,
					ENTITY(FE_ISP, FORMATTER, i), FMT_PAD_IN);
	}
*/
	//RAWDUMP => AOUT links
	for (i = 0; i < RAWDUMP_NUM; i++) {
		for (j = 0; j < AOUT_NUM; j++) {
			CREATE_LINK(ENTITY(FE_ISP, RAWDUMP, i), PAD_OUT,
				    ENTITY(DMA, AOUT, j), VNODE_PAD_IN);
		}
	}
	//FORMATTER => AOUT links
	for (i = 0; i < FORMATTER_NUM; i++) {
		for (j = 0; j < AOUT_NUM; j++) {
			CREATE_LINK(ENTITY(FE_ISP, FORMATTER, i), FMT_PAD_AOUT,
				    ENTITY(DMA, AOUT, j), VNODE_PAD_IN);
		}
	}
	//FORMATTER => DWT links
	for (i = 0; i < FORMATTER_NUM; i++) {
		for (j = DWT0; j <= DWT1; j++) {
			for (k = FMT_PAD_D1OUT; k <= FMT_PAD_D4OUT; k++) {
				CREATE_LINK(ENTITY(FE_ISP, FORMATTER, i), k,
					    ENTITY(FE_ISP, j, 1 + k - FMT_PAD_D1OUT),
					    PAD_IN);
			}
		}
	}
	//DWT => AOUT links
	for (i = DWT0; i <= DWT1; i++) {
		for (j = 1; j <= DWT_LAYER_NUM; j++) {
			for (k = 0; k < AOUT_NUM; k++) {
				CREATE_LINK(ENTITY(FE_ISP, i, j), PAD_OUT,
					    ENTITY(DMA, AOUT, k), VNODE_PAD_IN);
			}
		}
	}
	//AIN => OFFLINE_CHANNEL links
	CREATE_LINK(ENTITY(DMA, AIN, 0), VNODE_PAD_OUT,
		    ENTITY(FE_ISP, OFFLINE_CHANNEL, 0), OFFLINE_CH_PAD_IN);
	CREATE_LINK(ENTITY(DMA, AIN, 1), VNODE_PAD_OUT,
		    ENTITY(FE_ISP, OFFLINE_CHANNEL, 1), OFFLINE_CH_PAD_IN);
	//OFFLINE_CHANNEL => PIPE links
	CREATE_LINK(ENTITY(FE_ISP, OFFLINE_CHANNEL, 0), OFFLINE_CH_PAD_P0OUT,
		    ENTITY(FE_ISP, PIPE, 0), PIPE_PAD_IN);
	CREATE_LINK(ENTITY(FE_ISP, OFFLINE_CHANNEL, 0), OFFLINE_CH_PAD_P1OUT,
		    ENTITY(FE_ISP, PIPE, 1), PIPE_PAD_IN);
	CREATE_LINK(ENTITY(FE_ISP, OFFLINE_CHANNEL, 1), OFFLINE_CH_PAD_P0OUT,
		    ENTITY(FE_ISP, PIPE, 0), PIPE_PAD_IN);
	CREATE_LINK(ENTITY(FE_ISP, OFFLINE_CHANNEL, 1), OFFLINE_CH_PAD_P1OUT,
		    ENTITY(FE_ISP, PIPE, 1), PIPE_PAD_IN);

	return 0;
LINKS_FAIL:
	for (i = 0; i < GRP_MAX; i++) {
		for (j = 0; j < SUB_MAX; j++) {
			for (k = 0; k < ID_MAX; k++) {
				if (drvdata->entities[i][j][k]) {
#ifdef MODULE
					__spm_media_entity_remove_links(drvdata->entities[i][j][k]);
#else
					__media_entity_remove_links(drvdata->entities[i][j][k]);
#endif
				}
			}
		}
	}
	return ret;
}

struct platform_device *x1vi_get_platform_device(void)
{
	return g_pdev;
}

extern struct spm_camera_vi_ops vi_ops;

int x1vi_register_isp_firmware(struct isp_firm *isp_firm)
{
	struct x1vi_platform_data *drvdata = NULL;

	mutex_lock(&g_init_mlock);
	if (!g_pdev) {
		cam_err("%s g_pdev was null", __func__);
		mutex_unlock(&g_init_mlock);
		return -1;
	}
	if (!g_isp_firm) {
		g_isp_firm = devm_kzalloc(&g_pdev->dev, sizeof(*g_isp_firm), GFP_KERNEL);
		if (!g_isp_firm) {
			cam_err("%s no enough mem", __func__);
			mutex_unlock(&g_init_mlock);
			return -ENOMEM;
		}
	}
	drvdata = platform_get_drvdata(g_pdev);
	if (isp_firm) {
		*g_isp_firm = *isp_firm;
		drvdata->isp_firm = g_isp_firm;
		isp_firm->vi_ops = &vi_ops;
	} else {
		drvdata->isp_firm = NULL;
	}
	mutex_unlock(&g_init_mlock);

	return 0;
}

int x1vi_register_sensor_ops(struct spm_camera_sensor_ops *sensor_ops)
{
	struct x1vi_platform_data *drvdata = NULL;

	mutex_lock(&g_init_mlock);
	if (!g_pdev) {
		cam_err("%s g_pdev was null", __func__);
		mutex_unlock(&g_init_mlock);
		return -1;
	}
	drvdata = platform_get_drvdata(g_pdev);
	drvdata->sensor_ops = sensor_ops;
	mutex_unlock(&g_init_mlock);

	return 0;
}

int x1vi_power(int on_off)
{
	struct x1vi_platform_data *drvdata = NULL;

	if (!g_pdev)
		return -1;
	drvdata = platform_get_drvdata(g_pdev);
	return fe_isp_s_power(drvdata->isp_ctx, on_off);
}

EXPORT_SYMBOL_GPL(x1vi_power);

static int x1vi_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct x1vi_platform_data *drvdata = NULL;
	int ret = 0;

	cam_dbg("%s enter.", __func__);
	mutex_lock(&g_init_mlock);
	drvdata = devm_kzalloc(dev, sizeof(*drvdata), GFP_KERNEL);
	if (!drvdata) {
		cam_err("%s not enough mem.", __func__);
		mutex_unlock(&g_init_mlock);
		return -ENOMEM;
	}
	g_v4l2_dev = plat_cam_v4l2_device_get();
	if (!g_v4l2_dev) {
		cam_err("%s get v4l2 device failed", __func__);
		mutex_unlock(&g_init_mlock);
		return -ENODEV;
	}
	drvdata->isp_ctx = fe_isp_create_ctx(pdev);
	if (!drvdata->isp_ctx) {
		cam_err("%s fe_isp_create_ctx failed.", __func__);
		mutex_unlock(&g_init_mlock);
		return -1;
	}
#ifndef CONFIG_KY_XILINX_ZYNQMP
	/* enable runtime pm */
	pm_runtime_enable(&pdev->dev);
	device_init_wakeup(&pdev->dev, true);
#endif

	ret = x1vi_create_entities(pdev, drvdata);
	if (ret) {
		cam_err("%s x1vi_create_entities failed ret=%d.", __func__, ret);
		goto entities_fail;
	}

	ret = x1vi_create_entity_links(drvdata);
	if (ret) {
		cam_err("%s x1vi_create_entity_links failed ret=%d.", __func__, ret);
		goto entities_fail;
	}
	drvdata->isp_firm = g_isp_firm;
	drvdata->pdev = pdev;
	platform_set_drvdata(pdev, drvdata);
	g_pdev = pdev;
	cam_dbg("%s leave.", __func__);
	mutex_unlock(&g_init_mlock);
	return 0;
entities_fail:
	mutex_unlock(&g_init_mlock);
	return ret;
}

static int x1vi_remove(struct platform_device *pdev)
{
	struct x1vi_platform_data *drvdata = platform_get_drvdata(pdev);

	cam_dbg("%s enter.", __func__);
	x1vi_release_entities(drvdata);
	fe_isp_release_ctx(drvdata->isp_ctx);
	g_pdev = NULL;
	plat_cam_v4l2_device_put(g_v4l2_dev);
	g_v4l2_dev = NULL;
	/* disable runtime pm */
#ifndef CONFIG_KY_XILINX_ZYNQMP
	device_init_wakeup(&pdev->dev, false);
	pm_runtime_disable(&pdev->dev);
#endif

	cam_dbg("%s leave.", __func__);
	return 0;
}

static const struct of_device_id x1vi_dt_match[] = {
	{
		.compatible = "ky,x1vi",
	},
	{}
};

MODULE_DEVICE_TABLE(of, x1vi_dt_match);

static struct platform_driver x1vi_driver = {
	.probe = x1vi_probe,
	.remove = x1vi_remove,
	.driver = {
		.name = "ky-x1vi",
		.of_match_table = x1vi_dt_match,
	},
};

static int __init spmisp_init(void)
{
	struct platform_device *pdev = NULL;
	int ret = 0;

	ret = platform_driver_register(&x1vi_driver);
	if (ret) {
		cam_err("%s platform_driver_register failed ret=%d.", __func__, ret);
		goto driver_fail;
	}
	return 0;
driver_fail:
	platform_device_unregister(pdev);
	return ret;
}

module_init(spmisp_init);

static void __exit spmisp_exit(void)
{
	platform_driver_unregister(&x1vi_driver);
}

module_exit(spmisp_exit);

MODULE_LICENSE("GPL");
