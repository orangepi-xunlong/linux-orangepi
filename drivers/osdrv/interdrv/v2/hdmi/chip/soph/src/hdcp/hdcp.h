#ifndef _HDCP_H_
#define _HDCP_H_

#include "core/hdmitx_dev.h"
#include "util/util.h"
#include "core/video.h"

#define BSTATUS_HDMI_MODE_MASK              0x1000
#define BSTATUS_MAX_CASCADE_EXCEEDED_MASK   0x0800
#define BSTATUS_DEPTH_MASK                  0x0700
#define BSTATUS_MAX_DEVS_EXCEEDED_MASK      0x0080
#define BSTATUS_DEVICE_COUNT_MASK           0x007F

#define KSV_MSK     0x7F
#define VRL_LENGTH  0x05
#define VRL_HEADER  5
#define VRL_NUMBER  3
#define HEADER      10
#define SHAMAX      20
#define DSAMAX      20

typedef enum {
	HDCP_IDLE = 0,
	HDCP_KSV_LIST_READY,
	HDCP_ERR_KSV_LIST_NOT_VALID,
	HDCP_KSV_LIST_ERR_DEPTH_EXCEEDED,
	HDCP_KSV_LIST_ERR_MEM_ACCESS,
	HDCP_ENGAGED,
	HDCP_FAILED
} hdcp_status_t;

typedef struct {
	u8 mlength[8];
	u8 mblock[64];
	int mindex;
	int mcomputed;
	int mcorrupted;
	unsigned mdigest[5];
} sha_t;

/**
 * @param dev Device structure
 * @param data_enable_polarity
 * @return TRUE if successful
 */
int hdcp_initialize(hdmi_tx_dev_t *dev);

/**
 * HDCP configuration - HDMI initialization Step G (Controller User Guide)
 * @param dev Device structure
 * @param params HDCP parameters
 * @param mode HDMI or DVI
 * @param hsPol HSYNC polarity
 * @param vsPol VSYNC polarity
 * @return TRUE if successful
 */
int hdcp_configure(hdmi_tx_dev_t *dev, hdcp_params_t * hdcp, video_params_t *video);

/**
 * The method handles DONE and ERROR events.
 * A DONE event will trigger the retrieving the read byte, and sending a request to read the following byte. The EDID is read until the block is done and then the reading moves to the next block.
 *  When the block is successfully read, it is sent to be parsed.
 * @param dev Device structure
 * @param hpd on or off
 * @param state of the HDCP engine interrupts
 * @param param to be returned to application:
 * 		no of KSVs in KSV LIST if KSV_LIST_EVENT
 * 		1 (engaged) 0 (fail) if HDCP_EGNAGED_EVENT
 * @return the state of which the event was handled (FALSE for fail)
 */
/* @param ksvHandler Handler to call when KSV list is ready*/
u8 hdcp_event_handler(hdmi_tx_dev_t *dev, int *param);

/**
 * Enable/disable HDCP 1.4
 * @param dev Device structure
 * @param enable
 */
void hdcp_rxdetect(hdmi_tx_dev_t *dev, u8 enable);

/**
 * Enter or exit AV mute mode
 * @param dev Device structure
 * @param enable the HDCP AV mute
 */
void hdcp_av_mute(hdmi_tx_dev_t *dev, int enable);

/**
 * Bypass data encryption stage
 * @param dev Device structure
 * @param bypass the HDCP AV mute
 */
//void hdcp_BypassEncryption(hdmi_tx_dev_t *dev, int bypass);

void hdcp_sw_reset(hdmi_tx_dev_t *dev);

void hdcp_key_write(hdmi_tx_dev_t *dev);

/**
 *@param dev Device structure
 * @param disable the HDCP encrption
 */
void hdcp_disable_encryption(hdmi_tx_dev_t *dev, int disable);

/**
 * @param baseAddr base address of HDCP module registers
 * @return HDCP interrupts state
 */
u8 hdcp_interrupt_status(hdmi_tx_dev_t *dev);

/**
 * Clear HDCP interrupts
 * @param dev Device structure
 * @param value mask of interrupts to clear
 * @return TRUE if successful
 */
int hdcp_interrupt_clear(hdmi_tx_dev_t *dev, u8 value);

void sha_reset(hdmi_tx_dev_t *dev, sha_t * sha);

int sha_result(hdmi_tx_dev_t *dev, sha_t * sha);

void sha_input(hdmi_tx_dev_t *dev, sha_t * sha, const u8 * data, size_t size);

void sha_process_block(hdmi_tx_dev_t *dev, sha_t * sha);

void sha_pad_message(hdmi_tx_dev_t *dev, sha_t * sha);

int hdcp_verify_dsa(hdmi_tx_dev_t *dev, const u8 * m, size_t n, const u8 * r, const u8 * s);

int hdcp_array_add(hdmi_tx_dev_t *dev, u8 * r, const u8 * a, const u8 * b, size_t n);

int hdcp_array_cmp(hdmi_tx_dev_t *dev, const u8 * a, const u8 * b, size_t n);

void hdcp_array_cpy(hdmi_tx_dev_t *dev, u8 * dst, const u8 * src, size_t n);

int hdcp_array_div(hdmi_tx_dev_t *dev, u8 * r, const u8 * dd, const u8 * d, size_t n);

int hdcp_array_mac(hdmi_tx_dev_t *dev, u8 * r, const u8 * mm, const u8 m, size_t n);

int hdcp_array_mul(hdmi_tx_dev_t *dev, u8 * r, const u8 * mm, const u8 * m, size_t n);

void hdcp_array_set(hdmi_tx_dev_t *dev, u8 * dst, const u8 src, size_t n);

int hdcp_array_usb(hdmi_tx_dev_t *dev, u8 * r, const u8 * a, const u8 * b, size_t n);

void hdcp_array_swp(hdmi_tx_dev_t *dev, u8 * r, size_t n);

int hdcp_array_tst(hdmi_tx_dev_t *dev, const u8 * a, const u8 b, size_t n);

int hdcp_compute_exp(hdmi_tx_dev_t *dev, u8 * c, const u8 * m, const u8 * e, const u8 * p, size_t n, size_t ne);

int hdcp_compute_inv(hdmi_tx_dev_t *dev, u8 * out, const u8 * z, const u8 * a, size_t n);

int hdcp_compute_mod(hdmi_tx_dev_t *dev, u8 * dst, const u8 * src, const u8 * p, size_t n);

int hdcp_compute_mul(hdmi_tx_dev_t *dev, u8 * p, const u8 * a, const u8 * b, const u8 * m, size_t n);

int hdcp_verify_ksv(hdmi_tx_dev_t *dev, const u8 * data, size_t size);

int hdcp_verify_srm(hdmi_tx_dev_t *dev, const u8 * data, size_t size);

void hdcp_params_reset(hdmi_tx_dev_t *dev, hdcp_params_t * params);
#endif	/* _HDCP_H_ */
