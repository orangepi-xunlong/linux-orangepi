#include "de_feat.h"

static const struct de_feat *de_cur_features;

static const int sun50iw2_de_num_chns[] = {
	/* DISP0 */
	4,
	/* DISP1 */
	2,
};

static const int sun50iw2_de_num_vi_chns[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	1,
};

static const int sun50iw2_de_num_layers[] = {
	/* DISP0 CH0 */
	4,
	/* DISP0 CH1 */
	4,
	/* DISP0 CH2 */
	4,
	/* DISP0 CH3 */
	4,
	/* DISP1 CH0 */
	4,
	/* DISP1 CH1 */
	4,
};

static const int sun50iw2_de_is_support_vep[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun50iw2_de_is_support_smbl[] = {
	/* CH0 */
	0,
	/* CH1 */
	0,
};

static const int sun50iw2_de_supported_output_types[] = {
	/* DISP0 */
#if defined(TCON1_DRIVE_PANEL)
	DE_OUTPUT_TYPE_LCD,
#else
	DE_OUTPUT_TYPE_HDMI,
#endif
	/* DISP1 */
	DE_OUTPUT_TYPE_TV,
};

static const int sun50iw2_de_is_support_wb[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	0,
};

static const int sun50iw2_de_is_support_scale[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	1,
	/* DISP0 CH2 */
	1,
	/* DISP0 CH3 */
	1,
	/* DISP1 CH0 */
	1,
	/* DISP1 CH1 */
	1,
};

static const int sun50iw2_de_scale_line_buffer[] = {
	/* DISP0 */
	4096,
	/* DISP1 */
	2048,
};

static const struct de_feat sun50iw2_de_features = {
	.num_screens = DE_NUM,
	.num_devices = DEVICE_NUM,
	.num_chns = sun50iw2_de_num_chns,
	.num_vi_chns = sun50iw2_de_num_vi_chns,
	.num_layers = sun50iw2_de_num_layers,
	.is_support_vep = sun50iw2_de_is_support_vep,
	.is_support_smbl = sun50iw2_de_is_support_smbl,
	.is_support_wb = sun50iw2_de_is_support_wb,
	.supported_output_types = sun50iw2_de_supported_output_types,
	.is_support_scale = sun50iw2_de_is_support_scale,
	.scale_line_buffer = sun50iw2_de_scale_line_buffer,
};

static const int sun50iw1_de_num_chns[] = {
	/* DISP0 */
	4,
	/* DISP1 */
	2,
};

static const int sun50iw1_de_num_vi_chns[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	1,
};

static const int sun50iw1_de_num_layers[] = {
	/* DISP0 CH0 */
	4,
	/* DISP0 CH1 */
	4,
	/* DISP0 CH2 */
	4,
	/* DISP0 CH3 */
	4,
	/* DISP1 CH0 */
	4,
	/* DISP1 CH1 */
	4,
};

static const int sun50iw1_de_is_support_vep[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun50iw1_de_is_support_smbl[] = {
	/* CH0 */
	1,
	/* CH1 */
	0,
};

static const int sun50iw1_de_supported_output_types[] = {
	/* DISP0 */
	DE_OUTPUT_TYPE_LCD,
	/* DISP1 */
	DE_OUTPUT_TYPE_HDMI,
};

static const int sun50iw1_de_is_support_wb[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	0,
};

static const int sun50iw1_de_is_support_scale[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	1,
	/* DISP0 CH2 */
	1,
	/* DISP0 CH3 */
	1,
	/* DISP1 CH0 */
	1,
	/* DISP1 CH1 */
	1,
};

static const int sun50iw1_de_scale_line_buffer[] = {
	/* DISP0 */
	4096,
	/* DISP1 */
	2048,
};

static const struct de_feat sun50iw1_de_features = {
	.num_screens = DE_NUM,
	.num_devices = DEVICE_NUM,
	.num_chns = sun50iw1_de_num_chns,
	.num_vi_chns = sun50iw1_de_num_vi_chns,
	.num_layers = sun50iw1_de_num_layers,
	.is_support_vep = sun50iw1_de_is_support_vep,
	.is_support_smbl = sun50iw1_de_is_support_smbl,
	.is_support_wb = sun50iw1_de_is_support_wb,
	.supported_output_types = sun50iw1_de_supported_output_types,
	.is_support_scale = sun50iw1_de_is_support_scale,
	.scale_line_buffer = sun50iw1_de_scale_line_buffer,
};

static const int sun8iw11_de_num_chns[] = {
	/* DISP0 */
	4,
	/* DISP1 */
	2,
};

static const int sun8iw11_de_num_vi_chns[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	1,
};

static const int sun8iw11_de_num_layers[] = {
	/* DISP0 CH0 */
	4,
	/* DISP0 CH1 */
	4,
	/* DISP0 CH2 */
	4,
	/* DISP0 CH3 */
	4,
	/* DISP1 CH0 */
	4,
	/* DISP1 CH1 */
	4,
};

static const int sun8iw11_de_is_support_vep[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int sun8iw11_de_is_support_smbl[] = {
	/* CH0 */
	1,
	/* CH1 */
	0,
};

static const int sun8iw11_de_supported_output_types[] = {
	/* tcon0 */
	DE_OUTPUT_TYPE_LCD,
	/* tcon0 */
	DE_OUTPUT_TYPE_LCD,
	/* tcon2 */
	DE_OUTPUT_TYPE_TV | DE_OUTPUT_TYPE_HDMI | DE_OUTPUT_TYPE_VGA,
	/* tcon3 */
	DE_OUTPUT_TYPE_TV | DE_OUTPUT_TYPE_HDMI | DE_OUTPUT_TYPE_VGA,
};

static const int sun8iw11_de_is_support_wb[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	0,
};

static const int sun8iw11_de_is_support_scale[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	1,
	/* DISP0 CH2 */
	1,
	/* DISP0 CH3 */
	1,
	/* DISP1 CH0 */
	1,
	/* DISP1 CH1 */
	1,
};

static const int sun8iw11_de_scale_line_buffer[] = {
	/* DISP0 */
	2048,
	/* DISP1 */
	2048,
};

static const struct de_feat sun8iw11_de_features = {
	.num_screens = DE_NUM,
	.num_devices = DEVICE_NUM,
	.num_chns = sun8iw11_de_num_chns,
	.num_vi_chns = sun8iw11_de_num_vi_chns,
	.num_layers = sun8iw11_de_num_layers,
	.is_support_vep = sun8iw11_de_is_support_vep,
	.is_support_smbl = sun8iw11_de_is_support_smbl,
	.is_support_wb = sun8iw11_de_is_support_wb,
	.supported_output_types = sun8iw11_de_supported_output_types,
	.is_support_scale = sun8iw11_de_is_support_scale,
	.scale_line_buffer = sun8iw11_de_scale_line_buffer,
};

static const int default_de_num_chns[] = {
	/* DISP0 */
	4,
	/* DISP1 */
	2,
};

static const int default_de_num_vi_chns[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	1,
};

static const int default_de_num_layers[] = {
	/* DISP0 CH0 */
	4,
	/* DISP0 CH1 */
	4,
	/* DISP0 CH2 */
	4,
	/* DISP0 CH3 */
	4,
	/* DISP1 CH0 */
	4,
	/* DISP1 CH1 */
	4,
};

static const int default_de_is_support_vep[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	0,
	/* DISP0 CH2 */
	0,
	/* DISP0 CH3 */
	0,
	/* DISP1 CH0 */
	0,
	/* DISP1 CH1 */
	0,
};

static const int default_de_is_support_smbl[] = {
	/* CH0 */
	1,
	/* CH1 */
	0,
};

static const int default_de_supported_output_types[] = {
	/* DISP0 */
	DE_OUTPUT_TYPE_LCD,
	/* DISP1 */
	DE_OUTPUT_TYPE_HDMI,
};

static const int default_de_is_support_wb[] = {
	/* DISP0 */
	1,
	/* DISP1 */
	0,
};

static const int default_de_is_support_scale[] = {
	/* DISP0 CH0 */
	1,
	/* DISP0 CH1 */
	1,
	/* DISP0 CH2 */
	1,
	/* DISP0 CH3 */
	1,
	/* DISP1 CH0 */
	1,
	/* DISP1 CH1 */
	1,
};

static const int default_de_scale_line_buffer[] = {
	/* DISP0 */
	4096,
	/* DISP1 */
	2048,
};

static const struct de_feat default_de_features = {
	.num_screens = DE_NUM,
	.num_devices = DEVICE_NUM,
	.num_chns = default_de_num_chns,
	.num_vi_chns = default_de_num_vi_chns,
	.num_layers = default_de_num_layers,
	.is_support_vep = default_de_is_support_vep,
	.is_support_smbl = default_de_is_support_smbl,
	.is_support_wb = default_de_is_support_wb,
	.supported_output_types = default_de_supported_output_types,
	.is_support_scale = default_de_is_support_scale,
	.scale_line_buffer = default_de_scale_line_buffer,
};

int de_feat_get_num_screens(void)
{
	return de_cur_features->num_screens;
}

int de_feat_get_num_devices(void)
{
	return de_cur_features->num_devices;
}

int de_feat_get_num_chns(unsigned int disp)
{
	return de_cur_features->num_chns[disp];
}

int de_feat_get_num_vi_chns(unsigned int disp)
{
	return de_cur_features->num_vi_chns[disp];
}

int de_feat_get_num_ui_chns(unsigned int disp)
{
	return de_cur_features->num_chns[disp] -
	    de_cur_features->num_vi_chns[disp];
}

int de_feat_get_num_layers(unsigned int disp)
{
	unsigned int i, index = 0, num_channels = 0;
	int num_layers = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);

	num_channels = de_feat_get_num_chns(disp);
	for (i = 0; i < num_channels; i++, index++)
		num_layers += de_cur_features->num_layers[index];

	return num_layers;
}

int de_feat_get_num_layers_by_chn(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features->num_layers[index];
}

int de_feat_is_support_vep(unsigned int disp)
{
	unsigned int i, index = 0, num_channels = 0;
	int is_support_vep = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);

	num_channels = de_feat_get_num_chns(disp);
	for (i = 0; i < num_channels; i++, index++)
		is_support_vep += de_cur_features->is_support_vep[index];

	return is_support_vep;
}

int de_feat_is_support_vep_by_chn(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features->is_support_vep[index];
}

int de_feat_is_support_smbl(unsigned int disp)
{
	return de_cur_features->is_support_smbl[disp];
}

int de_feat_is_supported_output_types(unsigned int disp,
				      unsigned int output_type)
{
	return de_cur_features->supported_output_types[disp] & output_type;
}

int de_feat_is_support_wb(unsigned int disp)
{
	return de_cur_features->is_support_wb[disp];
}

int de_feat_is_support_scale(unsigned int disp)
{
	unsigned int i, index = 0, num_channels = 0;
	int is_support_scale = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);

	num_channels = de_feat_get_num_chns(disp);
	for (i = 0; i < num_channels; i++, index++)
		is_support_scale += de_cur_features->is_support_scale[index];

	return is_support_scale;
}

int de_feat_is_support_scale_by_chn(unsigned int disp, unsigned int chn)
{
	unsigned int i, index = 0;

	if (disp >= de_feat_get_num_screens())
		return 0;
	if (chn >= de_feat_get_num_chns(disp))
		return 0;

	for (i = 0; i < disp; i++)
		index += de_feat_get_num_chns(i);
	index += chn;

	return de_cur_features->is_support_scale[index];
}

int de_feat_get_scale_linebuf(unsigned int disp)
{
		return de_cur_features->scale_line_buffer[disp];
}

int de_feat_init(void)
{
#if defined(CONFIG_ARCH_SUN50IW2)
	de_cur_features = &sun50iw2_de_features;
#elif defined(CONFIG_ARCH_SUN50IW1)
	de_cur_features = &sun50iw1_de_features;
#elif defined(CONFIG_ARCH_SUN8IW11)
	de_cur_features = &sun8iw11_de_features;
#else
	de_cur_features = &default_de_features;
#endif
	return 0;
}
