#include "core/audio.h"
#include "core/fc.h"
#include "core/main_controller.h"
#include "core/packets.h"
#include "core/hdmi_reg.h"
#include "core/hdmi_core.h"
#include "util/util.h"
#include "bsp/access.h"
#include "vo_sys.h"

#define ENABLE_HLOCK_MASK        0x8
#define SELECT_BURST_TYPE_MASK   0x6
#define BURST_EN_MASK            0x1
#define FIFO_THRESHOLD_MASK      0xff
#define INITIAL_ADDR_MASK        0xff
#define FINAL_ADDR_MASK          0xff
#define AHB_DMA_START_MASK       0x1
#define MAS_AUDIO_SAMPLE_MASK    0x1
#define HBR_AUDIO_SAMPLE_MASK    0x10
#define AUTO_START_AND_EN_MASK   0x3
#define MULTI_STREAM_MASK        0x1
#define INSERT_PCUV_MASK         0x40

typedef struct audio_n_computation {
	u32 pixel_clock;
	u32 n;
} audio_n_computation_t;

typedef union iec {
	u32 frequency;
	u8 sample_size;
} iec_t;

typedef struct iec_sampling_freq {
	iec_t iec;
	u8 value;
} iec_params_t;

typedef struct channel_count {
	unsigned char channel_allocation;
	unsigned char channel_count;
} channel_count_t;


iec_params_t iec_original_sampling_freq_values[] = {
		{{.frequency = 44100}, 0xF},
		{{.frequency = 88200}, 0x7},
		{{.frequency = 22050}, 0xB},
		{{.frequency = 176400}, 0x3},
		{{.frequency = 48000}, 0xD},
		{{.frequency = 96000}, 0x5},
		{{.frequency = 24000}, 0x9},
		{{.frequency = 192000}, 0x1},
		{{.frequency =  8000}, 0x6},
		{{.frequency = 11025}, 0xA},
		{{.frequency = 12000}, 0x2},
		{{.frequency = 32000}, 0xC},
		{{.frequency = 16000}, 0x8},
		{{.frequency = 0}, 0x0}
};

iec_params_t iec_sampling_freq_values[] = {
		{{.frequency = 22050}, 0x4},
		{{.frequency = 44100}, 0x0},
		{{.frequency = 88200}, 0x8},
		{{.frequency = 17640}, 0xC},
		{{.frequency = 24000}, 0x6},
		{{.frequency = 48000}, 0x2},
		{{.frequency = 96000}, 0xA},
		{{.frequency = 192000}, 0xE},
		{{.frequency = 32000}, 0x3},
		{{.frequency = 768000}, 0x9},
		{{.frequency = 0}, 0x0}
};

iec_params_t iec_word_length[] = {
		{{.sample_size = 16}, 0x2},
		{{.sample_size = 17}, 0xC},
		{{.sample_size = 18}, 0x4},
		{{.sample_size = 19}, 0x8},
		{{.sample_size = 20}, 0x3},
		{{.sample_size = 21}, 0xD},
		{{.sample_size = 22}, 0x5},
		{{.sample_size = 23}, 0x9},
		{{.sample_size = 24}, 0xB},
		{{.sample_size = 0},  0x0}
};

static channel_count_t channel_cnt[] = {
		{0x00, 1},
		{0x01, 2},
		{0x02, 2},
		{0x04, 2},
		{0x03, 3},
		{0x05, 3},
		{0x06, 3},
		{0x08, 3},
		{0x14, 3},
		{0x07, 4},
		{0x09, 4},
		{0x0A, 4},
		{0x0C, 4},
		{0x15, 4},
		{0x16, 4},
		{0x18, 4},
		{0x0B, 5},
		{0x0D, 5},
		{0x0E, 5},
		{0x10, 5},
		{0x17, 5},
		{0x19, 5},
		{0x1A, 5},
		{0x1C, 5},
		{0x20, 5},
		{0x22, 5},
		{0x24, 5},
		{0x26, 5},
		{0x0F, 6},
		{0x11, 6},
		{0x12, 6},
		{0x1B, 6},
		{0x1D, 6},
		{0x1E, 6},
		{0x21, 6},
		{0x23, 6},
		{0x25, 6},
		{0x27, 6},
		{0x28, 6},
		{0x2A, 6},
		{0x2C, 6},
		{0x2E, 6},
		{0x30, 6},
		{0x13, 7},
		{0x1F, 7},
		{0x29, 7},
		{0x2B, 7},
		{0x2D, 7},
		{0x2F, 7},
		{0x31, 7},
		{0, 0},
};

audio_n_computation_t n_values_32[] = {
	{0, 4096},
	{25175, 4576},
	{25200, 4096},
	{27000, 4096},
	{54000, 4096},
	{74250, 4096},
	{148500, 4096},
	{297000, 3072},
	{594000, 3072},
	{0, 0}
};

audio_n_computation_t n_values_44p1[] = {
	{0, 6272},
	{25175, 7007},
	{25200, 6272},
	{27000, 6272},
	{54000, 6272},
	{74250, 6272},
	{148500, 6272},
	{297000, 4704},
	{594000, 9408},
	{0, 0}
};

audio_n_computation_t n_values_48[] = {
	{0, 6144},
	{25175, 6864},
	{25200, 6144},
	{27000, 6144},
	{54000, 6144},
	{74250, 6144},
	{148500, 6144},
	{297000, 5120},
	{594000, 6144},
	{0, 0}
};


/**********************************************
 * Internal functions
 */
void _audio_clock_n(hdmi_tx_dev_t *dev, u32 value)
{
	/* 19-bit width */
	dev_write_mask(AUD_N3, AUD_N3_AUDN_MASK, (u8)(value >> 16));
	dev_write(AUD_N2, (u8)(value >> 8));
	dev_write(AUD_N1, (u8)(value >> 0));


	/* no shift */
	dev_write_mask(AUD_CTS3, AUD_CTS3_N_SHIFT_MASK, 0);
}

void _audio_clock_cts(hdmi_tx_dev_t *dev, u32 value)
{
	if(value > 0){
		/* 19-bit width */
		dev_write_mask(AUD_CTS3, AUD_CTS3_AUDCTS_MASK, (u8)(value >> 16));
		dev_write_mask(AUD_CTS3, AUD_CTS3_CTS_MANUAL_MASK, 1);
		dev_write(AUD_CTS2, (u8)(value >> 8));
		dev_write(AUD_CTS1, (u8)(value >> 0));
	}
	else{
		/* Set to automatic generation of CTS values */
		dev_write_mask(AUD_CTS3, AUD_CTS3_CTS_MANUAL_MASK, 0);
	}
}

void _audio_clock_atomic(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(AUD_N3, AUD_N3_NCTS_ATOMIC_WRITE_MASK, value);
}

void _audio_clock_f(hdmi_tx_dev_t *dev, u8 value)
{
	dev_write_mask(AUD_INPUTCLKFS, AUD_INPUTCLKFS_IFSFACTOR_MASK, value);
}

void audio_ahbdma(hdmi_tx_dev_t *dev, audio_params_t * audio)
{
	int dma_channel = 0;
	u32 high_bit;

	pr_debug("start_addr:%llx, stop_addr:%llx\n", audio->start_addr, audio->stop_addr);
	dma_channel = audio_channel_count(dev, audio) + 1;

	dev_write_mask(AHB_DMA_STOP, 0x1, 0x1);
	high_bit = audio->start_addr >> 32;
	extern_axi_to_36bit(high_bit);

	dev_write(IH_AHBDMAAUD_STAT0, 0x0);
	dev_write_mask(AHB_DMA_CONF0, ENABLE_HLOCK_MASK, 0x1);      //enable_hlock
	dev_write_mask(AHB_DMA_CONF0, INSERT_PCUV_MASK, 0x1);       //insert PCUV
	dev_write_mask(AHB_DMA_CONF0, SELECT_BURST_TYPE_MASK, 0x0); //Select the desired burst type
	dev_write_mask(AHB_DMA_CONF0, BURST_EN_MASK, 0x1);          // select burst_en  burst mode
	dev_write(AHB_DMA_CONF1, 0xff >> (8 - dma_channel));        // enable dma channels
	dev_write_mask(AHB_DMA_THRSLD, FIFO_THRESHOLD_MASK, 0x80);  //Select the FIFO threshold
	dev_write(AHB_DMA_STRADDR_SET0,  audio->start_addr);
	dev_write(AHB_DMA_STRADDR_SET0 + 4, audio->start_addr >> 8);
	dev_write(AHB_DMA_STRADDR_SET0 + 8, audio->start_addr >> 16);
	dev_write(AHB_DMA_STRADDR_SET0 + 12, audio->start_addr >> 24);
	dev_write(AHB_DMA_STPADDR_SET0, audio->stop_addr);          //final_addr[31:0] bit fields
	dev_write(AHB_DMA_STPADDR_SET0 + 4, audio->stop_addr >> 8);
	dev_write(AHB_DMA_STPADDR_SET0 + 8, audio->stop_addr >> 16);
	dev_write(AHB_DMA_STPADDR_SET0 + 12, audio->stop_addr >> 24);

#if 0
	dev_write(AHB_DMA_CONF2, 0x3);  // loop config
#endif

	dev_write_mask(AHB_DMA_START, AHB_DMA_START_MASK, 0x1);     //Order the AHB DMA to start reading

}
/**********************************************
 * External functions
 */
int audio_Initialize(hdmi_tx_dev_t *dev)
{
	// Mask all interrupts
	return audio_mute(dev,  1);
}

int audio_configure(hdmi_tx_dev_t *dev, audio_params_t * audio)
{
	u32 n = 0;
	u32 cts = 0;

	if((dev == NULL) || (audio == NULL)){
		pr_err("Improper function arguments");
		return HDMI_ERR_AUDIO_ARGS_INVALID;
	}

	if(dev->snps_hdmi_ctrl.audio_on == 0){
		pr_debug("Audio is not enabled");
		mc_audio_sampler_clock_enable(dev, 1);
		return 0;
	}

	audio->minterface_type = DMA;
	audio->mcoding_type = PCM;

	pr_debug("Audio frequency = %xkHz", audio->msampling_frequency);
	pr_debug("Audio sample size = %d", audio->msample_size);
	pr_debug("Audio FS factor = %d\n", audio->mclock_fsfactor);

	// Set PCUV info from external source
	audio->mgpa_insert_pucv = 1;

	audio_mute(dev, 1);

	// Configure Frame Composer audio parameters
	fc_audio_config(dev, audio);

	n = hdmi_compute_n(audio->msampling_frequency, dev->snps_hdmi_ctrl.pixel_clock);
	dev->snps_hdmi_ctrl.n = n;

	mc_audio_sampler_clock_enable(dev, 0);
	cts = hdmi_compute_cts(n, dev->snps_hdmi_ctrl.pixel_clock, audio->msampling_frequency);
	dev->snps_hdmi_ctrl.cts = cts;

	_audio_clock_atomic(dev, 1);
	_audio_clock_cts(dev, cts);
	_audio_clock_n(dev, n);

	audio_mute(dev, 0);

	// Configure audio info frame packets
	fc_audio_info_config(dev, audio);

	audio_ahbdma(dev, audio);

	return 0;
}

int audio_mute(hdmi_tx_dev_t *dev, u8 state)
{
	/* audio mute priority: AVMUTE, sample flat, validity */
	/* AVMUTE also mutes video */
	// TODO: Check the audio mute process
	if(state){
		fc_audio_mute(dev);
	}
	else{
		fc_audio_unmute(dev);
	}

	dev->snps_hdmi_ctrl.audio_mute = state ? TRUE : FALSE;

	return TRUE;
}

u32 hdmi_compute_n(u32 freq, u32 pixel_clk)
{
	u32 n = (128 * freq) / 1000;
	u32 mult = 1;

	while (freq > 48000) {
		mult *= 2;
		freq /= 2;
	}

	switch (freq) {
	case 32000:
		if (pixel_clk == 25175)
			n = 4576;
		else if (pixel_clk == 27027)
			n = 4096;
		else if (pixel_clk == 74176 || pixel_clk == 148352)
			n = 11648;
		else
			n = 4096;
		n *= mult;
		break;

	case 44100:
		if (pixel_clk == 25175)
			n = 7007;
		else if (pixel_clk == 74176)
			n = 17836;
		else if (pixel_clk == 148352)
			n = 8918;
		else
			n = 6272;
		n *= mult;
		break;

	case 48000:
		if (pixel_clk == 25175)
			n = 6864;
		else if (pixel_clk == 27027)
			n = 6144;
		else if (pixel_clk == 74176)
			n = 11648;
		else if (pixel_clk == 148352)
			n = 5824;
		else
			n = 6144;
		n *= mult;
		break;

	default:
		break;
	}

	return n;
}


u32 hdmi_compute_cts(u32 n, u32 pixel_clk, u32 sample_rate)
{
	u32 ftdms = pixel_clk;
	u32 cts;
	u64 tmp;

	/*
	* Compute the CTS value from the N value.  Note that CTS and N
	* can be up to 20 bits in total, so we need 64-bit math.  Also
	* note that our TDMS clock is not fully accurate; it is
	* accurate to kHz.  This can introduce an unnecessary remainder
	* in the calculation below, so we don't try to warn about that.
	*/
	tmp = (u64)ftdms * n * 1000;
	do_div(tmp, 128 * sample_rate);
	cts = tmp;

	pr_debug("%s: fs=%uHz ftdms=%u.%03uMHz N=%d cts=%d\n",
			__func__, sample_rate,
			ftdms / 1000, (ftdms) % 1000,
			n, cts);

	return cts;
}

void audio_reset(hdmi_tx_dev_t *dev, audio_params_t * params)
{
	params->minterface_type = DMA;
	params->mcoding_type = PCM;
	params->mchannel_allocation = 0;
	params->msample_size = 16;
	params->msampling_frequency = 32000;
	params->mLevel_shift_value = 0;
	params->mdown_mix_inhibit_flag = 0;
	params->miec_copyright = 1;
	params->miec_cgms_a = 3;
	params->miec_pcm_mode = 0;
	params->miec_category_code = 0;
	params->miec_source_number = 1;
	params->miec_clock_accuracy = 0;
	params->mpacket_type = AUDIO_SAMPLE;
	params->mclock_fsfactor = 128;
	params->mdma_beat_increment = DMA_UNSPECIFIED_INCREMENT;
	params->mdma_threshold = 0;
	params->mdma_hlock = 0;
	params->mgpa_insert_pucv = 0;
}

u8 audio_channel_count(hdmi_tx_dev_t *dev, audio_params_t * params)
{
	int i = 0;

	for (i = 0; channel_cnt[i].channel_count != 0; i++){
		if (channel_cnt[i].channel_allocation == params->mchannel_allocation){
			return channel_cnt[i].channel_count;
		}
	}

	return 0;
}

u8 audio_iec_original_sampling_freq(hdmi_tx_dev_t *dev, audio_params_t * params)
{
	int i = 0;

	for(i = 0; iec_original_sampling_freq_values[i].iec.frequency != 0; i++){
		if(is_equal(params->msampling_frequency,
			iec_original_sampling_freq_values[i].iec.frequency)){
			u8 value = iec_original_sampling_freq_values[i].value;
			return value;
		}
	}

	// Not indicated
	return 0x0;
}

u8 audio_iec_sampling_freq(hdmi_tx_dev_t *dev, audio_params_t * params)
{
	int i = 0;

	for (i = 0; iec_sampling_freq_values[i].iec.frequency != 0; i++){
		if (is_equal(params->msampling_frequency,
			iec_sampling_freq_values[i].iec.frequency)){
			u8 value = iec_sampling_freq_values[i].value;
			return value;
		}
	}

	// Not indicated
	return 0x1;
}

u8 audio_iec_word_length(hdmi_tx_dev_t *dev, audio_params_t * params)
{
	int i = 0;

	for (i = 0; iec_word_length[i].iec.sample_size != 0; i++){
		if (params->msample_size == iec_word_length[i].iec.sample_size){
			return iec_word_length[i].value;
		}
	}

	// Not indicated
	return 0x0;
}

u8 audio_is_channel_en(hdmi_tx_dev_t *dev, audio_params_t * params, u8 channel)
{
	switch (channel) {
	case 0:
	case 1:
		return 1;
	case 2:
		return params->mchannel_allocation & BIT(0);
	case 3:
		return (params->mchannel_allocation & BIT(1)) >> 1;
	case 4:
		if (((params->mchannel_allocation > 0x03) &&
			(params->mchannel_allocation < 0x14)) ||
			((params->mchannel_allocation > 0x17) &&
			(params->mchannel_allocation < 0x20)))
			return 1;
		else
			return 0;
	case 5:
		if (((params->mchannel_allocation > 0x07) &&
			(params->mchannel_allocation < 0x14)) ||
			((params->mchannel_allocation > 0x1C) &&
			(params->mchannel_allocation < 0x20)))
			return 1;
		else
			return 0;
	case 6:
		if ((params->mchannel_allocation > 0x0B) && (params->mchannel_allocation < 0x20))
			return 1;
		else
			return 0;
	case 7:
		return (params->mchannel_allocation & BIT(4)) >> 4;
	default:
		return 0;
	}
}
