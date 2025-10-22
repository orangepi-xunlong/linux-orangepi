/*
 * Broadcom Dongle Host Driver (DHD), Channel State information Module
 *
 * Copyright (C) 2024 Synaptics Incorporated. All rights reserved.
 *
 * This software is licensed to you under the terms of the
 * GNU General Public License version 2 (the "GPL") with Broadcom special exception.
 *
 * INFORMATION CONTAINED IN THIS DOCUMENT IS PROVIDED "AS-IS," AND SYNAPTICS
 * EXPRESSLY DISCLAIMS ALL EXPRESS AND IMPLIED WARRANTIES, INCLUDING ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
 * AND ANY WARRANTIES OF NON-INFRINGEMENT OF ANY INTELLECTUAL PROPERTY RIGHTS.
 * IN NO EVENT SHALL SYNAPTICS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, PUNITIVE, OR CONSEQUENTIAL DAMAGES ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OF THE INFORMATION CONTAINED IN THIS DOCUMENT, HOWEVER CAUSED
 * AND BASED ON ANY THEORY OF LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, AND EVEN IF SYNAPTICS WAS ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE. IF A TRIBUNAL OF COMPETENT JURISDICTION
 * DOES NOT PERMIT THE DISCLAIMER OF DIRECT DAMAGES OR ANY OTHER DAMAGES,
 * SYNAPTICS' TOTAL CUMULATIVE LIABILITY TO ANY PARTY SHALL NOT
 * EXCEED ONE HUNDRED U.S. DOLLARS
 *
 * Copyright (C) 2024, Broadcom.
 *
 *      Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2 (the "GPL"),
 * available at http://www.broadcom.com/licenses/GPLv2.php, with the
 * following added to such license:
 *
 *      As a special exception, the copyright holders of this software give you
 * permission to link this software with independent modules, and to copy and
 * distribute the resulting executable under terms of your choice, provided that
 * you also meet, for each linked independent module, the terms and conditions of
 * the license of that module.  An independent module is a module which is not
 * derived from this software.  The special exception does not apply to any
 * modifications of the software.
 *
 *
 * <<Broadcom-WL-IPTag/Open:>>
 *
 * $Id$
 */

#include <linux/list.h>
#include <linux/sort.h>
#include <linux/inetdevice.h> /* for '__in_dev_get_rcu' */

#include "osl.h"
#include "bcmutils.h"
#include "bcmendian.h"
#include "linuxver.h"
#include "dngl_stats.h"
#include "wlioctl.h"
#include "bcmevent.h"
#include "dhd.h"
#include "dhd_dbg.h"
#include "dhd_csi.h"
#include "dhd_linux.h"
#include "bcmudp.h"

/* For kernel < 3.12 */
#ifndef list_last_entry
#define list_last_entry(ptr, type, member) list_entry((ptr)->prev, type, member)
#endif /* list_last_entry */

#ifdef CSI_SUPPORT

#define SYNA_ALIGN_BYTES      (sizeof(uint32))
#define SYNA_ALIGN_PADDING    (SYNA_ALIGN_BYTES -1LL)
#define SYNA_ALIGN_MASK       (~SYNA_ALIGN_PADDING)
#define SYNA_ALIGN(value) \
(\
	(SYNA_ALIGN_PADDING + ((long long int)(value))) \
	& SYNA_ALIGN_MASK \
)

#define SYNA_SOCKET_PORT_DEFAULT           (9999)

static const uint8  _gpMAC_zero[6] = {0, 0, 0, 0, 0, 0};
static const uint8  gpDHD_csi_network_template[14 + 20 + 8] = {
	/* ethernet header(14) */
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x08, 0x00,
	/* ip header(20) */
	0x45, 0x00,
	0x00, 0x00, /* len */
	0x00, 0x00, /* id */
	0x00, 0x00, /* flags */
	0x40, /* ttl */
	0x11, /* protocol */
	0x00, 0x00, /* checksum */
	8, 8, 8, 8, /* src ip */
	127, 0, 0, 1, /* dst ip */
	/* udp header(8) */
	0x00, 0x00, /* src port */
	0x00, 0x00, /* dst port */
	0x00, 0x00, /* length */
	0x00, 0x00, /* checksum */
	/* real syna csi header start from here */
};

struct syna_csi_rx_mode_config {
	uint32  mode;

	uint8   data_version;
	uint8   data_header_version_output;
	uint16  port;
	uint32  ip;
};

static const struct {
	uint         error_code;
	const char  *error_string;
} CONST_CSI_ERROR_STRING[] = {
	{SYNA_CSI_FLAG_ERROR_NONE, "NONE"},
	{SYNA_CSI_FLAG_ERROR_FAILURE, "FAILURE"},
	{SYNA_CSI_FLAG_ERROR_CHECKSUM, "WRONG_CHECKSUM"},
	{SYNA_CSI_FLAG_ERROR_GENERIC, "ERROR_GENERIC"},
	{SYNA_CSI_FLAG_ERROR_OTHER, "ERROR_OTHER"},

	{SYNA_CSI_FLAG_ERROR_WRONG_CHANNEL, "WRONG_CHANNEL"},
	{SYNA_CSI_FLAG_ERROR_WRONG_MODE, "WRONG_MODE"},

	{SYNA_CSI_FLAG_ERROR_TX_FAIL, "TX_FAIL"},
	{SYNA_CSI_FLAG_ERROR_TX_NO_ACK, "TX_NO_ACK"},

	{SYNA_CSI_FLAG_ERROR_DATA_FEEDBACK_MISS, "NO_DATA_FEEDBACK"},
	{SYNA_CSI_FLAG_ERROR_TX_FEEDBACK_MISS, "NO_TX_FEEDBACK"},
	{SYNA_CSI_FLAG_ERROR_POWERSAVING, "ERROR_POWERSAVING"},

	{SYNA_CSI_FLAG_ERROR_SUPPRESS, "SUPPRESSED"},
	{SYNA_CSI_FLAG_ERROR_SUPP_PMQ, "SUPPRESSED_PMQ"},
	{SYNA_CSI_FLAG_ERROR_SUPP_FLUSH, "SUPPRESSED_FLUSH"},
	{SYNA_CSI_FLAG_ERROR_SUPP_FRAG, "SUPPRESSED_FRAG"},
	{SYNA_CSI_FLAG_ERROR_SUPP_TBTT, "SUPPRESSED_TBTT"},
	{SYNA_CSI_FLAG_ERROR_SUPP_BADCH, "SUPPRESSED_BADCH"},
	{SYNA_CSI_FLAG_ERROR_SUPP_EXPTIME, "SUPPRESSED_EXPTIME"},
	{SYNA_CSI_FLAG_ERROR_SUPP_UF, "SUPPRESSED_UF"},
	{SYNA_CSI_FLAG_ERROR_SUPP_ABS, "SUPPRESSED_ABSENT"},
	{SYNA_CSI_FLAG_ERROR_SUPP_PPS, "SUPPRESSED_PPS"},
	{SYNA_CSI_FLAG_ERROR_SUPP_NOKEY, "SUPPRESSED_NOKEY"},
	{SYNA_CSI_FLAG_ERROR_SUPP_DMA_XFER, "SUPPRESSED_DMA"},
	{SYNA_CSI_FLAG_ERROR_SUPP_TWT, "SUPPRESSED_TWT"},

	{SYNA_CSI_FLAG_ERROR_LAST, ""}
};
#define CONST_CSI_ERROR_STRING_SIZE    ARRAYSIZE(CONST_CSI_ERROR_STRING)

static const char * dhd_csi_get_error_string(uint  error_code)
{
	const char  *error_string = NULL;
	int i;

	i = CONST_CSI_ERROR_STRING_SIZE - 1;
	error_string = CONST_CSI_ERROR_STRING[i].error_string;
	for (i = 0; i < (CONST_CSI_ERROR_STRING_SIZE - 1); i++) {
		if (CONST_CSI_ERROR_STRING[i].error_code == error_code) {
			error_string = CONST_CSI_ERROR_STRING[i].error_string;
			break;
		}
	}

	return error_string;
}

/* Different FW using different CSI data formats, and here
 * the  DHD driver will directly convert input DATA to the
 * common header format(latest full set version CSI header
 * format which contains much more items), and then it will
 * be converted to corresponding version CSI header which
 * applicaiton required.
 *
 *        FW                   DHD-Driver                 Application
 * dhd_cfr_header_rev_0 \
 * dhd_cfr_header_rev_1  \                              / syna_csi_header_v1
 * dhd_cfr_header_rev_2   --> syna_csi_common_header -->  syna_csi_header_v2
 * dhd_cfr_header_rev_3  /                              \      ...
 *     ...              /
 */

static struct csi_cfr_node *dhd_csi_allocate_pkt(dhd_pub_t *dhdp,
                                                 uint       pkt_len)
{
	int                   ret = BCME_OK;
	struct csi_cfr_node  *ptr = NULL;
	uint8                *p = NULL;
	uint                  total_size = 0;

	if (SYNA_CSI_DATA_MODE_UDP_SOCKET == dhdp->csi_data_send_manner) {
		void * skb = NULL;
		total_size =   CONST_SYNA_CSI_CFR_NODE_LEN
		             + sizeof(syna_csi_common_header)
		             + pkt_len;
		skb = PKTGET(dhdp->osh, total_size, TRUE);
		if (!skb) {
			DHD_ERROR(("%s-%d: *Error, malloc %d for cfr dump list error\n",
			           __func__, __LINE__, total_size));
			ret = BCME_NOMEM;
			goto done;
		}
		p = PKTDATA(dhdp->osh, skb);
		ptr = (struct csi_cfr_node *)p;
		ptr->pNode = skb;
		ptr->send_manner = dhdp->csi_data_send_manner;
		ptr->total_size = total_size;
		p += sizeof(struct csi_cfr_node);
		ptr->pHeader_csi = (syna_csi_common_header *)p;
	} else {
		total_size =   sizeof(struct csi_cfr_node)
		             + sizeof(syna_csi_common_header)
		             + pkt_len;
		p = MALLOCZ(dhdp->osh, total_size);
		if (!p) {
			DHD_ERROR(("%s-%d: *Error, malloc %d for cfr dump list error\n",
			           __func__, __LINE__, total_size));
			ret = BCME_NOMEM;
			goto done;
		}
		ptr = (struct csi_cfr_node *)p;
		ptr->pNode = p;
		ptr->send_manner = dhdp->csi_data_send_manner;
		ptr->total_size = total_size;
		p += sizeof(struct csi_cfr_node);
		ptr->pHeader_csi = (syna_csi_common_header *)p;
	}

done:
	if (0 > ret) {
		ptr = NULL;
	}

	return ptr;
}

static int dhd_csi_free_pkt(dhd_pub_t *dhdp,
                            void      *p)
{
	struct csi_cfr_node  *ptr = p;
	int                   ret = BCME_OK;

	if (SYNA_CSI_DATA_MODE_UDP_SOCKET == ptr->send_manner) {
		PKTFREE(dhdp->osh, ptr->pNode, FALSE);
	} else {
		MFREE(dhdp->osh, ptr->pNode, ptr->total_size);
	}

	return ret;
}

static int dhd_csi_data_output_common_to_v1(struct csi_cfr_node *ptr)
{
	syna_csi_common_header        *pCommon = ptr->pHeader_csi;
	struct syna_csi_header_v1      v1, *pV1 = &(v1);
	int                            len_common_header = 0;
	int                            len_v1_header = 0;
	int                            delta = 0;
	int                            ret = BCME_OK;

	len_common_header = sizeof(syna_csi_common_header);
	len_v1_header     = sizeof(struct syna_csi_header_v1);

	/* conversion from pCommon to pV1 */
	memset(pV1, 0, len_v1_header);

	pV1->magic_flag = pCommon->magic_flag;

	pV1->version = pCommon->version;
	pV1->format_type = pCommon->format_type;
	pV1->fc = pCommon->frame_control;
	pV1->flags = pCommon->flag_error;
	if (SYNA_CSI_FLAG_FEATURE_FFT_NEGATIVE_FIRST & pCommon->flag_feature) {
		pV1->flags |= SYNA_CSI_LEGACY_FLAG_FFT_NEGATIVE_FIRST;
	}

	memcpy(pV1->client_ea, pCommon->peer_mac,  ETHER_ADDR_LEN);
	memcpy(pV1->bsscfg_ea, pCommon->local_mac, ETHER_ADDR_LEN);
	pV1->band = pCommon->band;
	pV1->bandwidth = pCommon->bandwidth;
	pV1->channel = pCommon->channel;

	pV1->report_tsf = pCommon->report_tsf;

	pV1->num_txstream = pCommon->qty_txstream;
	pV1->num_rxchain = pCommon->qty_rxchain;
	pV1->num_subcarrier = pCommon->qty_subcarrier;

	pV1->rssi = pCommon->rssi;
	pV1->noise = pCommon->noise;
	pV1->global_id = pCommon->global_id;

	memcpy(pV1->rssi_ant, pCommon->rssi_ant, sizeof(pV1->rssi_ant));

	pV1->data_length = pCommon->data_length;
	pV1->data_checksum = pCommon->data_checksum;

	/* length of 'pV1' is less than length of 'pCommon' */
	delta  = len_common_header - len_v1_header;

	ptr->pHeader_csi = (syna_csi_common_header *)(delta + ((unsigned char *)ptr->pHeader_csi));
	memcpy(ptr->pHeader_csi, pV1, sizeof(struct syna_csi_header_v1));
	ptr->convert_pull = delta;
	ptr->length_data_total = pV1->data_length + sizeof(struct syna_csi_header_v1);

	return ret;
}

static int dhd_csi_data_output_common_to_v2(struct csi_cfr_node *ptr)
{
	syna_csi_common_header      *pCommon = ptr->pHeader_csi;
	struct syna_csi_header_v2    v2, *pV2 = &v2;
	int                          delta = 0;
	int                          ret = BCME_OK;

	pCommon = ptr->pHeader_csi;

	/* nothing to do when the 'syna_csi_common_header'
	 * is mapping to the 'syna_csi_header_v2'
	 */
	UNUSED_PARAMETER(pCommon);
	UNUSED_PARAMETER(pV2);
	UNUSED_PARAMETER(delta);
	return ret;
}

int dhd_csi_data_output_convert(dhd_pub_t *dhdp, struct csi_cfr_node *ptr)
{
	int    ret = BCME_OK;

	DHD_INFO(("%s: will convert common header version to version %d\n",
	          __func__, dhdp->csi_header_output_version));

	if (CONST_SYNA_CSI_COMMON_HEADER_VERSION
	    != dhdp->csi_header_output_version) {
		switch (dhdp->csi_header_output_version) {
			case CONST_SYNA_CSI_HEADER_VERSION_V1:
				ret = dhd_csi_data_output_common_to_v1(ptr);
				break;
			case CONST_SYNA_CSI_HEADER_VERSION_V2:
				ret = dhd_csi_data_output_common_to_v2(ptr);
				break;
			default:
				DHD_ERROR(("%s: ***Error, unknow error data"
				           " header output version=%d\n",
				           __func__,
				           dhdp->csi_header_output_version));
				ret = BCME_UNSUPPORTED;
				break;
		}
	}

	return ret;
}

static int dhd_csi_data_queue_notify(dhd_pub_t *dhdp, const char * func, int line)
{
	struct csi_cfr_node *ptr = NULL, *next = NULL;
	int                  ret = BCME_OK;

	mutex_lock(&dhdp->csi_lock);

	if (list_empty(&dhdp->csi_list)) {
		ret = BCME_ERROR;
		goto done;
	}

	list_for_each_entry_safe(ptr, next, &dhdp->csi_list, list) {
		struct ether_header *pEthernet = NULL;
		struct ipv4_hdr     *pIP = NULL;
		struct bcmudp_hdr   *pUDP = NULL;
		int                  len = 0;

		if ((!ptr->length_data_remain)
		    || (SYNA_CSI_DATA_MODE_UDP_SOCKET != ptr->send_manner)) {
			continue;
		}

		/* format of skb:
		 *   alignment_padding(6)
		 *   header_ethernet(14)
		 *   header_ip(20)
		 *   header_udp(8)
		 *     csi_header(80)
		 *     csi_data(N)
		 */

		/* cut from queue */
		list_del(&ptr->list);
		dhdp->csi_count--;

		/* get pointer to fill */
		len =   CONST_SYNA_CSI_CFR_NODE_LEN + ptr->convert_pull
		      - CONST_SYNA_SOCKET_HEADER_BYTES;
		PKTPULL(dhdp->osh, ptr->pNode, len);
		pEthernet = (struct ether_header *)(PKTDATA(dhdp->osh, ptr->pNode));
		pIP = (struct ipv4_hdr *)(ETHER_HDR_LEN + (uint8 *)(pEthernet));
		pUDP = (struct bcmudp_hdr *)(IPV4_HLEN_MIN + (uint8 *)(pIP));

		memcpy(pEthernet, gpDHD_csi_network_template, sizeof(gpDHD_csi_network_template));
		/*** UDP */
		len = ptr->length_data_total;
		len += UDP_HDR_LEN;
		/* early change since psudo header counting use this */
		if (dhdp->csi_notify_ip) {
			memcpy(pIP->dst_ip, &(dhdp->csi_notify_ip), IPV4_ADDR_LEN);
		}
		pUDP->src_port = hton16(SYNA_SOCKET_PORT_DEFAULT);
		/* memcpy(pIP->src_ip, &(dhdp->csi_local_ip), IPV4_ADDR_LEN); */
		pUDP->dst_port = hton16(dhdp->csi_notify_port);
		pUDP->len = hton16(len);
		pUDP->chksum = 0x00;
		/* psudo header includes:
		 *    source_ip(4) + destination_ip(4)
		 *  + 0x0011(protocol 2) + UDP_length(2)
		 */
		pUDP->chksum = ip_cksum(hton16(0x0011L + len), 0 - 8 + ((uint8 *)pUDP), len + 8);
		/* IP */
		len += IPV4_HLEN_MIN;
		pIP->tot_len = hton16(len);
		pIP->id = hton16(ptr->pHeader_csi->global_id);
		pIP->hdr_chksum = 0x00;
		pIP->hdr_chksum = ip_cksum(0x0, (uint8 *)pIP, IPV4_HLEN_MIN);
		memcpy(pEthernet->ether_dhost, ptr->pHeader_csi->local_mac, 6);
		memcpy(pEthernet->ether_shost, ptr->pHeader_csi->peer_mac, 6);

		dhd_rx_frame(dhdp, ptr->ifidx, ptr->pNode, 1, 0x0);

		ret += 1;
	}

done:
	mutex_unlock(&dhdp->csi_lock);

	return ret;
}

int dhd_csi_data_queue_polling(dhd_pub_t *dhdp, char *buf, uint count)
{
	struct csi_cfr_node *ptr = NULL, *next = NULL;
	int   ret = BCME_OK;
	char *pbuf = buf;
	int   num = 0;
	int   copy_len = 0;
	int   left_data = 0;
	int   total_copy_len = 0;

	NULL_CHECK(dhdp, "dhdp is NULL", ret);

	mutex_lock(&dhdp->csi_lock);
	if (!list_empty(&dhdp->csi_list)) {
		list_for_each_entry_safe(ptr, next, &dhdp->csi_list, list) {
			/* do not touch 'ptr->pHeader_csi' since it's
			 * format maybe already changed before send up
			 */
			if (ptr->length_data_remain) {
				DHD_ERROR(("%s-%d: *Warning, data not ready "
				           "for %02X:%02X:%02X:%02X:%02X:%02X,"
				           " global_id=%d, length_data_total=%d, "
				           "length_data_remain=%d\n",
				           __func__, __LINE__,
				           ptr->peer_mac[0], ptr->peer_mac[1],
				           ptr->peer_mac[2], ptr->peer_mac[3],
				           ptr->peer_mac[4], ptr->peer_mac[5],
				           ptr->global_id,
				           ptr->length_data_total,
				           ptr->length_data_remain));
				continue;
			}

			left_data =   ptr->length_data_total
			            - ptr->length_data_copied;
			copy_len = count;
			if (0 > left_data) {
				DHD_ERROR(("%s-%d: *Error, invalid case, "
				           "total_len=%d, "
				           "remain_len=%d, copied_len=%d\n",
				           __func__, __LINE__,
				           ptr->length_data_total,
				           ptr->length_data_remain,
				           ptr->length_data_copied));
				copy_len = 0;
			} else if (copy_len > left_data) {
				copy_len = left_data;
			}

			DHD_TRACE(("%s-%d: Packet[%d]: "
			           "left_room=%d, copy_len=%d, "
			           "pNode_data_len=%d, pNode_reamin_len=%d, "
			           "pNode_copied_len=%d, csi_count=%d\n",
			           __func__, __LINE__,
			           num, count, copy_len,
			           ptr->length_data_total,
			           ptr->length_data_remain,
			           ptr->length_data_copied,
			           dhdp->csi_count));

			if (0 < copy_len) {
				memcpy(pbuf,
				       ptr->length_data_copied + (uint8 *)(ptr->pHeader_csi),
				       copy_len);
				ptr->length_data_copied += copy_len;
				count -= copy_len;
				pbuf  += copy_len;
				num++;
			}

			if (ptr->length_data_copied >= ptr->length_data_total) {
				list_del(&ptr->list);
				dhd_csi_free_pkt(dhdp, ptr);
				dhdp->csi_count--;
			}

			if (0 >= copy_len) {
				break;
			}
		}
	}
	mutex_unlock(&dhdp->csi_lock);
	total_copy_len = pbuf - buf;
	DHD_TRACE(("%s-%d: dump %d record %d bytes\n",
	           __func__, __LINE__, num, total_copy_len));

	return total_copy_len;
}

static int dhd_csi_data_append(dhd_pub_t *dhdp, struct csi_cfr_node *ptr,
                               uint32 data_length, uint32 remain_length,
                               const uint8 *pData)
{
	syna_csi_common_header  *pEntry = NULL;
	int                      error_status = 0;
	int                      ret = BCME_OK;
	int                      append_len = 0;

	if ((!pData) || (!ptr)) {
		DHD_ERROR(("%s-%d: *Error, invalid parameter, "
		           "pData=0x%px, ptr=0x%px, pNode=0x%px\n",
		           __func__, __LINE__,
		           pData, ptr, ptr?(ptr->pNode):(NULL)));
		ret = BCME_BADARG;
		goto done;
	} else {
		pEntry = ptr->pHeader_csi;
		error_status = pEntry->flag_error;
	}
	/* find the append length */
	if (remain_length) {
		append_len =   data_length
		             - ptr->length_data_received
		             - remain_length;
	} else if (ptr->length_data_remain) {
		append_len = ptr->length_data_remain;
	} else if (SYNA_CSI_CFR_NODE_FREE_LEN(ptr) >= data_length) {
		append_len = data_length;
	}

	DHD_TRACE(("%s-%d: data_length=%d, remain=%d, append=%d    "
	           "pNode: pEntry=0x%px, global_id=%d, flags=0x%x, "
	           "data_len=%d, remain_len=%d, copied_len=%d\n",
	           __func__, __LINE__,
	           data_length, remain_length, append_len,
	           pEntry, pEntry->global_id, pEntry->flag_feature,
	           pEntry->data_length,
	           ptr->length_data_remain, ptr->length_data_copied));

	/* copy and update */
	if (append_len) {
		memcpy((ptr->length_data_received + ((uint8 *)pEntry->data)),
		       pData, append_len);
		ptr->length_data_received += append_len;

		if ((!error_status)
		    && (!remain_length)
		    && (pEntry->data_checksum)) {
			uint16  data_checksum = 0;
			uint    i;
			pData = (uint8 *)pEntry->data;
			for (i = 0; i < data_length; i++) {
				data_checksum += pData[i];
			}
			if (data_checksum != pEntry->data_checksum) {
				DHD_ERROR(("%s-%d: *Error, global_id=%d, "
				           "error=0x%X, checksum=0x%X "
				           "mismatch to FW=0x%X, "
				           "data_length=%d, remain_length=%d, "
				           "pData=0x%px\n",
				           __func__, __LINE__,
				           pEntry->global_id, error_status,
				           data_checksum, pEntry->data_checksum,
				           data_length, remain_length, pData));
				pEntry->flag_error = SYNA_CSI_LEGACY_FLAG_ERROR_CHECKSUM;
				prhex("DATA", pData, 16);
			}
		}
	} else if (!error_status) {
		/* some BC/MC manner may not have CSI DATA feedback */
		if (pEntry->qty_subcarrier) {
			DHD_ERROR(("%s-%d: *Error, no data append, "
			           "total_size=%d, free_len=%d, "
			           "in_data_length=%d, in_remain_length=%d, "
			           "    Node: global_id=%d, flags=0x%X, "
			           "received_length=%d, remain_length=%d\n",
			           __func__, __LINE__,
			           ptr->total_size,
			           (int)SYNA_CSI_CFR_NODE_FREE_LEN(ptr),
			           data_length, remain_length,
			           pEntry->global_id, pEntry->flag_error,
			           ptr->length_data_received, ptr->length_data_remain));
		}
	} else {
		const char *  error_string = NULL;
		error_string = dhd_csi_get_error_string(pEntry->flag_error);
		UNUSED_PARAMETER(error_string);
		/* FW reported errors can be DHD_ERROR() for output if needs */
		DHD_INFO(("%s-%d: *Warning, error report, "
		           "size=%d, free=%d, "
		           "in_data_len=%d, in_remain_len=%d, "
		           " Node: global_id=%d, flags=0x%X %s, "
		           "received_length=%d, remain_len=%d\n",
		           __func__, __LINE__,
		           ptr->total_size,
		           (int)SYNA_CSI_CFR_NODE_FREE_LEN(ptr),
		           data_length, remain_length,
		           pEntry->global_id,
		           pEntry->flag_error, error_string,
		           ptr->length_data_received, ptr->length_data_remain));
	}

	/* make this as final step to avoid packet early retrieved */
	ptr->length_data_remain = remain_length;

	/* mapping to expected header when receiving done */
	if (!remain_length) {
		ptr->length_data_copied = 0;
		ptr->global_id = pEntry->global_id;
		memcpy(ptr->peer_mac, pEntry->peer_mac, ETHER_ADDR_LEN);
		dhd_csi_data_output_convert(dhdp, ptr);
	}

	ret = remain_length;
done:
	return ret;
}

static int dhd_csi_data_input_revision0(dhd_pub_t *dhdp, const wl_event_msg_t *event,
	struct dhd_cfr_header_rev_0 *pCFR, const uint8 *pData)
{
	struct csi_cfr_node     *ptr = NULL;
	syna_csi_common_header  *pEntry = NULL;
	int                      ret = BCME_ERROR;
	uint32                   remain_length = 0;
	uint32                   data_length = 0;
	uint64                   report_tsf = 0;
	uint32                   chanspec = 0;

	report_tsf = OSL_SYSUPTIME_US();

	ptr = dhd_csi_allocate_pkt(dhdp, pCFR->cfr_dump_length);
	if (!ptr) {
		DHD_ERROR(("%s-%d: *Error, malloc cfr dump list error\n",
		           __func__, __LINE__));
		return BCME_NOMEM;
	}
	ptr->ifidx = event->ifidx;
	pEntry = ptr->pHeader_csi;

	DHD_TRACE(("%s-%d: ptr=0x%p, ptr=0x%px, pEntry=0x%px, "
	           "delta=%d, header_size=%d\n",
	           __func__, __LINE__, ptr, ptr, pEntry,
	           (int)((uintptr)pEntry - (uintptr)ptr),
	           (int)CONST_SYNA_CSI_CFR_NODE_LEN));

	/* process one by one for alignment consideration */
	memset(pEntry, 0, sizeof(syna_csi_common_header));

	pEntry->magic_flag = CONST_SYNA_CSI_MAGIC_FLAG;
	pEntry->version = CONST_SYNA_CSI_COMMON_HEADER_VERSION;
	pEntry->format_type = SYNA_CSI_FORMAT_Q8;
	pEntry->frame_control = 0;
	pEntry->global_id = dhdp->packet_global_id_last++;
	pEntry->header_length = sizeof(syna_csi_common_header);
	memcpy(pEntry->peer_mac, pCFR->peer_macaddr, ETHER_ADDR_LEN);
	if (pCFR->status) {
		pEntry->flag_error = SYNA_CSI_LEGACY_FLAG_ERROR_GENERIC;
	}
	if (!memcmp(_gpMAC_zero, pEntry->local_mac, ETHER_ADDR_LEN)) {
		dhd_if_t  *ifp = dhd_get_ifp(dhdp, event->ifidx);
		if (!ifp) {
			DHD_ERROR(("%s-%d: *Error, get ifp error\n",
			           __func__, __LINE__));
		} else {
			memcpy(pEntry->local_mac, ifp->mac_addr, ETHER_ADDR_LEN);
		}
	}

	pEntry->qty_txstream = pCFR->sts;
	pEntry->qty_rxchain = pCFR->num_rx;
	pEntry->qty_subcarrier = ltoh16_ua(&(pCFR->num_carrier));
	if (!pEntry->report_tsf) {
		pEntry->report_tsf = report_tsf;
	}

	pEntry->band = SYNA_CSI_BAND_UNKNOWN;
	pEntry->bandwidth = SYNA_CSI_BW_UNKNOWN;
	pEntry->channel = 0;
	if ((ret = dhd_iovar(dhdp, event->ifidx, "chanspec",
	                     NULL, 0, (char*)&chanspec,
	                     sizeof(chanspec), FALSE) == BCME_OK)) {
		switch (CHSPEC_BAND(chanspec)) {
		case WL_CHANSPEC_BAND_2G:
			pEntry->band = SYNA_CSI_BAND_2G;
			break;
		case WL_CHANSPEC_BAND_5G:
			pEntry->band = SYNA_CSI_BAND_5G;
			break;
		case WL_CHANSPEC_BAND_6G:
			pEntry->band = SYNA_CSI_BAND_6G;
			break;
		default:
			pEntry->band = SYNA_CSI_BAND_UNKNOWN;
			break;
		}

		switch (CHSPEC_BW(chanspec)) {
		case WL_CHANSPEC_BW_20:
			pEntry->bandwidth = SYNA_CSI_BW_20MHz;
			break;
		case WL_CHANSPEC_BW_40:
			pEntry->bandwidth = SYNA_CSI_BW_40MHz;
			break;
		case WL_CHANSPEC_BW_80:
			pEntry->bandwidth = SYNA_CSI_BW_80MHz;
			break;
		case WL_CHANSPEC_BW_160:
			pEntry->bandwidth = SYNA_CSI_BW_160MHz;
			break;
		case WL_CHANSPEC_BW_320:
			pEntry->bandwidth = SYNA_CSI_BW_320MHz;
			break;
		default:
			pEntry->bandwidth = SYNA_CSI_BAND_UNKNOWN;
			break;
		}

		pEntry->channel = CHSPEC_CHANNEL(chanspec);
	}
	pEntry->rssi = pCFR->rssi;

	pEntry->data_length = pCFR->cfr_dump_length;
	pEntry->data_checksum = 0;

	DHD_TRACE(("%s-%d: Entry data_size=%d, remain_size=%d, @%llu\n",
	           __func__, __LINE__,
	           ltoh32_ua(&(pCFR->cfr_dump_length)),
	           ltoh32_ua(&(pCFR->remain_length)),
	           ltoh64_ua(&(pEntry->report_tsf))));

	data_length = ltoh32_ua(&(pCFR->cfr_dump_length));
	remain_length = ltoh32_ua(&(pCFR->remain_length));

	ptr->length_data_total = data_length + sizeof(syna_csi_common_header);
	ptr->length_data_received = 0;
	ptr->length_data_remain = 0;
	ptr->length_data_copied = 0;

	ret = dhd_csi_data_append(dhdp, ptr,
	                          data_length, remain_length, pData);
	if (0 <= ret) {
		INIT_LIST_HEAD(&(ptr->list));
		mutex_lock(&dhdp->csi_lock);
		list_add_tail(&(ptr->list), &dhdp->csi_list);
		dhdp->csi_count++;
		mutex_unlock(&dhdp->csi_lock);
	} else {
		dhd_csi_free_pkt(dhdp, ptr);
	}

	return ret;
}

static int dhd_csi_data_input_revision1(dhd_pub_t *dhdp, const wl_event_msg_t *event,
	struct dhd_cfr_header_rev_1 *pCFR, const uint8 *pData)
{
	struct csi_cfr_node     *ptr = NULL;
	syna_csi_common_header  *pEntry = NULL;
	int                      ret = BCME_ERROR;
	uint32                   remain_length = 0;
	uint32                   data_length = 0;
	uint64                   report_tsf = 0;
	uint32                   chanspec = 0;

	report_tsf = OSL_SYSUPTIME_US();

	ptr = dhd_csi_allocate_pkt(dhdp, pCFR->cfr_dump_length);
	if (!ptr) {
		DHD_ERROR(("%s-%d: *Error, malloc cfr dump list error\n",
		           __func__, __LINE__));
		return BCME_NOMEM;
	}
	ptr->ifidx = event->ifidx;
	pEntry = ptr->pHeader_csi;

	DHD_TRACE(("%s-%d: ptr=0x%p, ptr=0x%px, pEntry=0x%px, "
	           "delta=%d, header_size=%d\n",
	           __func__, __LINE__, ptr, ptr, pEntry,
	           (int)((uintptr)pEntry - (uintptr)ptr),
	           (int)CONST_SYNA_CSI_CFR_NODE_LEN));

	/* process one by one for alignment consideration */
	memset(pEntry, 0, sizeof(syna_csi_common_header));

	pEntry->magic_flag = CONST_SYNA_CSI_MAGIC_FLAG;
	pEntry->version = CONST_SYNA_CSI_COMMON_HEADER_VERSION;
	pEntry->format_type = SYNA_CSI_FORMAT_Q8;
	pEntry->frame_control = pCFR->fc;
	pEntry->global_id = dhdp->packet_global_id_last++;
	pEntry->header_length = sizeof(syna_csi_common_header);
	memcpy(pEntry->peer_mac, pCFR->peer_macaddr, ETHER_ADDR_LEN);
	if (pCFR->status) {
		pEntry->flag_error = SYNA_CSI_LEGACY_FLAG_ERROR_GENERIC;
	}
	if (!memcmp(_gpMAC_zero, pEntry->local_mac, ETHER_ADDR_LEN)) {
		dhd_if_t  *ifp = dhd_get_ifp(dhdp, event->ifidx);
		if (!ifp) {
			DHD_ERROR(("%s-%d: *Error, get ifp error\n",
			           __func__, __LINE__));
		} else {
			memcpy(pEntry->local_mac, ifp->mac_addr, ETHER_ADDR_LEN);
		}
	}

	pEntry->qty_txstream = pCFR->sts;
	pEntry->qty_rxchain = pCFR->num_rx;
	pEntry->qty_subcarrier = ltoh16_ua(&(pCFR->num_carrier));
	pEntry->report_tsf = ltoh64_ua(&(pCFR->cfr_timestamp));
	if (!pEntry->report_tsf) {
		pEntry->report_tsf = report_tsf;
	}

	pEntry->band = SYNA_CSI_BAND_UNKNOWN;
	pEntry->bandwidth = SYNA_CSI_BW_UNKNOWN;
	pEntry->channel = 0;
	if ((ret = dhd_iovar(dhdp, event->ifidx, "chanspec",
	                     NULL, 0, (char*)&chanspec,
	                     sizeof(chanspec), FALSE) == BCME_OK)) {
		switch (CHSPEC_BAND(chanspec)) {
		case WL_CHANSPEC_BAND_2G:
			pEntry->band = SYNA_CSI_BAND_2G;
			break;
		case WL_CHANSPEC_BAND_5G:
			pEntry->band = SYNA_CSI_BAND_5G;
			break;
		case WL_CHANSPEC_BAND_6G:
			pEntry->band = SYNA_CSI_BAND_6G;
			break;
		default:
			pEntry->band = SYNA_CSI_BAND_UNKNOWN;
			break;
		}

		switch (CHSPEC_BW(chanspec)) {
		case WL_CHANSPEC_BW_20:
			pEntry->bandwidth = SYNA_CSI_BW_20MHz;
			break;
		case WL_CHANSPEC_BW_40:
			pEntry->bandwidth = SYNA_CSI_BW_40MHz;
			break;
		case WL_CHANSPEC_BW_80:
			pEntry->bandwidth = SYNA_CSI_BW_80MHz;
			break;
		case WL_CHANSPEC_BW_160:
			pEntry->bandwidth = SYNA_CSI_BW_160MHz;
			break;
		case WL_CHANSPEC_BW_320:
			pEntry->bandwidth = SYNA_CSI_BW_320MHz;
			break;
		default:
			pEntry->bandwidth = SYNA_CSI_BAND_UNKNOWN;
			break;
		}

		pEntry->channel = CHSPEC_CHANNEL(chanspec);
	}
	pEntry->rssi = pCFR->rssi;

	pEntry->data_length = pCFR->cfr_dump_length;
	pEntry->data_checksum = 0;

	DHD_TRACE(("%s-%d: Entry data_size=%d, remain_size=%d, @%llu\n",
	           __func__, __LINE__,
	           ltoh32_ua(&(pCFR->cfr_dump_length)),
	           ltoh32_ua(&(pCFR->remain_length)),
	           ltoh64_ua(&(pEntry->report_tsf))));

	data_length = ltoh32_ua(&(pCFR->cfr_dump_length));
	remain_length = ltoh32_ua(&(pCFR->remain_length));

	ptr->length_data_total = data_length + sizeof(syna_csi_common_header);
	ptr->length_data_received = 0;
	ptr->length_data_remain = 0;
	ptr->length_data_copied = 0;

	ret = dhd_csi_data_append(dhdp, ptr,
	                          data_length, remain_length, pData);
	if (0 <= ret) {
		INIT_LIST_HEAD(&(ptr->list));
		mutex_lock(&dhdp->csi_lock);
		list_add_tail(&(ptr->list), &dhdp->csi_list);
		dhdp->csi_count++;
		mutex_unlock(&dhdp->csi_lock);
	} else {
		dhd_csi_free_pkt(dhdp, ptr);
	}

	return ret;
}

static int dhd_csi_data_input_revision2(dhd_pub_t *dhdp, const wl_event_msg_t *event,
	struct dhd_cfr_header_rev_2 *pCFR, const uint8 *pData)
{
	struct csi_cfr_node     *ptr = NULL;
	syna_csi_common_header  *pEntry = NULL;
	int                      ret = BCME_ERROR;
	uint32                   remain_length = 0;
	uint32                   data_length = 0;
	uint64                   report_tsf = 0;

	report_tsf = OSL_SYSUPTIME_US();

	if (SYNA_ALIGN_PADDING & (uintptr)pCFR) {
		DHD_ERROR(("%s-%d: *Warning, pCFR=0x%px is not aligned!\n",
		           __func__, __LINE__, pCFR));
	}

	ptr = dhd_csi_allocate_pkt(dhdp, pCFR->data_length);
	if (!ptr) {
		DHD_ERROR(("%s-%d: *Error, malloc cfr dump list error\n",
		           __func__, __LINE__));
		return BCME_NOMEM;
	}
	ptr->ifidx = event->ifidx;
	pEntry = ptr->pHeader_csi;

	DHD_TRACE(("%s-%d: ptr=0x%p, ptr=0x%px, pEntry=0x%px, "
	           "delta=%d, header_size=%d\n",
	           __func__, __LINE__, ptr, ptr, pEntry,
	           (int)((uintptr)pEntry - (uintptr)ptr),
	           (int)CONST_SYNA_CSI_CFR_NODE_LEN));

	/* process one by one for alignment consideration */
	memset(pEntry, 0, sizeof(syna_csi_common_header));

	pEntry->magic_flag = CONST_SYNA_CSI_MAGIC_FLAG;
	pEntry->version = CONST_SYNA_CSI_COMMON_HEADER_VERSION;
	pEntry->format_type = pCFR->format_type;
	pEntry->frame_control = pCFR->fc;
	if (!pEntry->format_type) {
		pEntry->format_type = SYNA_CSI_FORMAT_Q8;
	}

	pEntry->global_id = pCFR->global_id;
	pEntry->header_length = sizeof(syna_csi_common_header);
	pEntry->trigger_mode = 0;
	pEntry->dot11_mode = SYNA_CSI_DOT11_UNKNOW;
	if (-1 != dhdp->packet_global_id_last) {
		uint32  delta = 0;

		delta = ((pCFR->global_id + 0x10000UL) -
			dhdp->packet_global_id_last) & 0xFFFFUL;
		if (!delta) {
			DHD_ERROR(("%s-%d: ***Warning, duplicate packet,"
			           " id=%d\n",
			           __func__, __LINE__,
			           dhdp->packet_global_id_last));
			dhdp->packet_qty_duplicate++;
		} else if (1 < delta) {
			DHD_ERROR(("%s-%d: ***Warning, packet missing, "
			           "last_id=%d, now_id=%d, delta=%d\n",
			           __func__, __LINE__,
			           dhdp->packet_global_id_last,
			           pCFR->global_id, delta));
			dhdp->packet_qty_missing += delta - 1;
		}
	}
	dhdp->packet_global_id_last = pCFR->global_id;

	pEntry->report_tsf = pCFR->report_tsf;
	pEntry->flag_error = pCFR->flags & SYNA_CSI_LEGACY_FLAG_ERROR_MASK;
	memcpy(pEntry->peer_mac,  pCFR->client_ea, ETHER_ADDR_LEN);
	memcpy(pEntry->local_mac, pCFR->bsscfg_ea, ETHER_ADDR_LEN);

	if ((pCFR->status_compat) && (!pCFR->flags)) {
		pEntry->flag_error = SYNA_CSI_LEGACY_FLAG_ERROR_GENERIC;
		DHD_ERROR(("%s-%d: *Warning, status=0x%X, "
		           " global_id=%d!\n",
		           __func__, __LINE__,
		           pCFR->status_compat,
		           pCFR->global_id));
	}
	if (pCFR->flags & SYNA_CSI_LEGACY_FLAG_FFT_NEGATIVE_FIRST) {
		pEntry->flag_feature = SYNA_CSI_FLAG_FEATURE_FFT_NEGATIVE_FIRST;
	}
	if (!pEntry->report_tsf) {
		pEntry->report_tsf = report_tsf;
	}
	if (!memcmp(_gpMAC_zero, pEntry->local_mac, ETHER_ADDR_LEN)) {
		dhd_if_t  *ifp = dhd_get_ifp(dhdp, event->ifidx);
		if (!ifp) {
			DHD_ERROR(("%s-%d: *Error, get ifp error\n",
			           __func__, __LINE__));
		} else {
			memcpy(pEntry->local_mac, ifp->mac_addr, ETHER_ADDR_LEN);
		}
	}

	pEntry->qty_txstream = pCFR->num_txstream;
	pEntry->qty_rxchain = pCFR->num_rxchain;
	pEntry->qty_subcarrier = pCFR->num_subcarrier;

	pEntry->chipset_flag = 0;
	pEntry->band = pCFR->band;
	pEntry->side_band = 0;
	pEntry->bandwidth = pCFR->bandwidth;
	pEntry->channel = pCFR->channel;
	pEntry->rx_rate = 0;

	pEntry->rx_bandwidth = 0;
	pEntry->rxchain_idx_mask = 0;
	pEntry->rssi = pCFR->rssi;
	pEntry->noise = pCFR->noise;
	memcpy(pEntry->rssi_ant, pCFR->rssi_ant, sizeof(pEntry->rssi_ant));

	pEntry->data_length = pCFR->data_length;
	pEntry->data_checksum = pCFR->data_checksum;

	DHD_TRACE(("%s-%d: Entry data_size=%d, remain_size=%d, @%llu\n",
	           __func__, __LINE__,
	           pCFR->data_length,
	           pCFR->remain_length,
	           pEntry->report_tsf));

	data_length = pCFR->data_length;
	remain_length = pCFR->remain_length;

	ptr->length_data_total = data_length + sizeof(syna_csi_common_header);
	ptr->length_data_received = 0;
	ptr->length_data_remain = 0;
	ptr->length_data_copied = 0;

	ret = dhd_csi_data_append(dhdp, ptr,
	                          data_length, remain_length, pData);
	if (0 <= ret) {
		INIT_LIST_HEAD(&(ptr->list));
		mutex_lock(&dhdp->csi_lock);
		list_add_tail(&(ptr->list), &dhdp->csi_list);
		dhdp->csi_count++;
		mutex_unlock(&dhdp->csi_lock);
	} else {
		dhd_csi_free_pkt(dhdp, ptr);
	}

	return ret;
}

static int dhd_csi_data_input_revision3(dhd_pub_t *dhdp, const wl_event_msg_t *event,
	struct dhd_cfr_header_rev_3 *pCFR, const uint8 *pData)
{
	struct csi_cfr_node     *ptr = NULL;
	syna_csi_common_header  *pEntry = NULL;
	int                      ret = BCME_ERROR;
	uint32                   remain_length = 0;
	uint32                   data_length = 0;
	uint64                   report_tsf = 0;

	report_tsf = OSL_SYSUPTIME_US();

	if (SYNA_ALIGN_PADDING & (uintptr)pCFR) {
		DHD_ERROR(("%s-%d: *Warning, pCFR=0x%px is not aligned!\n",
		           __func__, __LINE__, pCFR));
	}

	ptr = dhd_csi_allocate_pkt(dhdp, pCFR->data_length);
	if (!ptr) {
		DHD_ERROR(("%s-%d: *Error, malloc cfr dump list error\n",
		           __func__, __LINE__));
		return BCME_NOMEM;
	}
	ptr->ifidx = event->ifidx;
	pEntry = ptr->pHeader_csi;

	DHD_TRACE(("%s-%d: ptr=0x%p, ptr=0x%px, pEntry=0x%px, "
	           "delta=%d, header_size=%d\n",
	           __func__, __LINE__, ptr, ptr, pEntry,
	           (int)((uintptr)pEntry - (uintptr)ptr),
	           (int)CONST_SYNA_CSI_CFR_NODE_LEN));
	/* process one by one for alignment consideration */
	memcpy(pEntry, pCFR, sizeof(syna_csi_common_header));

	pEntry->magic_flag = CONST_SYNA_CSI_MAGIC_FLAG;
	pEntry->version = CONST_SYNA_CSI_COMMON_HEADER_VERSION;
	if (!pEntry->format_type) {
		pEntry->format_type = SYNA_CSI_FORMAT_Q8;
	}

	if (-1 != dhdp->packet_global_id_last) {
		uint32  delta = 0;

		delta = ((pCFR->global_id + 0x10000UL)
			- dhdp->packet_global_id_last)
			& 0xFFFFUL;
		if (!delta) {
			DHD_ERROR(("%s-%d: ***Warning, duplicate packet,"
			           " id=%d\n",
			           __func__, __LINE__,
			           dhdp->packet_global_id_last));
			dhdp->packet_qty_duplicate++;
		} else if (1 < delta) {
			DHD_ERROR(("%s-%d: ***Warning, packet missing, "
			           "last_id=%d, now_id=%d, delta=%d\n",
			           __func__, __LINE__,
			           dhdp->packet_global_id_last,
			           pCFR->global_id, delta));
			dhdp->packet_qty_missing += delta - 1;
		}
	}
	dhdp->packet_global_id_last = pCFR->global_id;

	if ((pCFR->status_compat) && (!pCFR->flag_error)) {
		pEntry->flag_error = SYNA_CSI_FLAG_ERROR_GENERIC;
		DHD_ERROR(("%s-%d: *Warning, status=0x%X, "
		           " global_id=%d!\n",
		           __func__, __LINE__,
		           pCFR->status_compat,
		           pCFR->global_id));
	}
	if (!pEntry->report_tsf) {
		pEntry->report_tsf = report_tsf;
	}
	if (!memcmp(_gpMAC_zero, pEntry->local_mac, ETHER_ADDR_LEN)) {
		dhd_if_t  *ifp = dhd_get_ifp(dhdp, event->ifidx);
		if (!ifp) {
			DHD_ERROR(("%s-%d: *Error, get ifp error\n",
			           __func__, __LINE__));
		} else {
			memcpy(pEntry->local_mac, ifp->mac_addr, ETHER_ADDR_LEN);
		}
	}

	DHD_TRACE(("%s-%d: Entry data_size=%d, remain_size=%d, @%llu\n",
	           __func__, __LINE__,
	           pCFR->data_length,
	           pCFR->remain_length,
	           pEntry->report_tsf));

	data_length = pCFR->data_length;
	remain_length = pCFR->remain_length;

	ptr->length_data_total = data_length + sizeof(syna_csi_common_header);
	ptr->length_data_received = 0;
	ptr->length_data_remain = 0;
	ptr->length_data_copied = 0;

	ret = dhd_csi_data_append(dhdp, ptr,
	                          data_length, remain_length, pData);
	if (0 <= ret) {
		INIT_LIST_HEAD(&(ptr->list));
		mutex_lock(&dhdp->csi_lock);
		list_add_tail(&(ptr->list), &dhdp->csi_list);
		dhdp->csi_count++;
		mutex_unlock(&dhdp->csi_lock);
	} else {
		dhd_csi_free_pkt(dhdp, ptr);
	}

	return ret;
}

static int dhd_csi_event_handler(dhd_pub_t *dhdp, const wl_event_msg_t *event, uint8 *event_data)
{
	union dhd_cfr_header  cfr;
	union dhd_cfr_header  *pCFR = NULL;
	struct csi_cfr_node   *ptr = NULL, *next = NULL, *save_ptr = NULL;
	const uint8  *pMAC = NULL, *pData = NULL;
	uint8  version = 0xff;
	uint32 data_length = 0, remain_length = 0;
	int    ret = BCME_OK;

	NULL_CHECK(dhdp, "dhdp is NULL", ret);

	DHD_TRACE(("%s-%d: Enter\n", __func__, __LINE__));

	if (!event_data) {
		DHD_ERROR(("%s-%d: event_data is NULL\n",
		           __func__, __LINE__));
		ret = BCME_BADARG;
		goto done;
	}

	pCFR = (union dhd_cfr_header *)event_data;
	if (SYNA_ALIGN_PADDING & (uintptr)pCFR) {
		memcpy(&cfr, event_data, sizeof(union dhd_cfr_header));
		pCFR = &cfr;
	}

	if (pCFR->header_rev_0.status) {
		DHD_INFO(("%s-%d: *Warning, status=0x%X "
		          " may indicate error!\n",
		          __func__, __LINE__,
		          pCFR->header_rev_0.status));
		/* TODO: need to abandon such case packet? */
	}

	version = pCFR->header_rev_0.version;
	switch (version) {
	case CONST_CFR_HEADER_REVISION_0:
		pMAC = pCFR->header_rev_0.peer_macaddr;
		remain_length = ltoh32_ua(&(pCFR->header_rev_0.remain_length));
		data_length = ltoh32_ua(&(pCFR->header_rev_0.cfr_dump_length));
		pData = (uint8 *)(sizeof(struct dhd_cfr_header_rev_0) + event_data);
		break;
	case CONST_CFR_HEADER_REVISION_1:
		pMAC = pCFR->header_rev_1.peer_macaddr;
		remain_length = ltoh32_ua(&(pCFR->header_rev_1.remain_length));
		data_length = ltoh32_ua(&(pCFR->header_rev_1.cfr_dump_length));
		pData = (uint8 *)(sizeof(struct dhd_cfr_header_rev_1) + event_data);
		break;
	case CONST_CFR_HEADER_REVISION_2:
		pMAC = pCFR->header_rev_2.client_ea;
		remain_length = pCFR->header_rev_2.remain_length;
		data_length = pCFR->header_rev_2.data_length;
		pData = (uint8 *)(sizeof(struct dhd_cfr_header_rev_2) + event_data);
		break;
	case CONST_CFR_HEADER_REVISION_3:
		pMAC = pCFR->header_rev_3.peer_mac;
		remain_length = pCFR->header_rev_3.remain_length;
		data_length = pCFR->header_rev_3.data_length;
		pData = (uint8 *)(sizeof(struct dhd_cfr_header_rev_3) + event_data);
		break;
	default:
		DHD_ERROR(("%s-%d: CSI version=%d error\n",
		           __func__, __LINE__, version));
		ret = BCME_BADOPTION;
		goto done;
		break;
	}

	if (CONST_CSI_DATA_BYTES_MAX < remain_length) {
		DHD_ERROR(("%s-%d: *Error, drop too big length, "
		           "global_id=%d, version=%d, status=%d, "
		           "data_length=%d(0x%X), remain_length=%d(0x%X) > %d\n",
		           __func__, __LINE__,
		           pCFR->header_rev_2.global_id,
		           version, pCFR->header_rev_2.status_compat,
		           data_length, data_length,
		           remain_length, remain_length,
		           (int)CONST_CSI_DATA_BYTES_MAX));
		prhex("TOO_BIG_LENGTH", (uchar *)pCFR, 128);
		ret = BCME_BADLEN;
		goto done;
	}

	mutex_lock(&dhdp->csi_lock);
	/* check if this addr exist */
	if (!list_empty(&dhdp->csi_list)) {
		list_for_each_entry_safe(ptr, next, &dhdp->csi_list, list) {
			syna_csi_common_header *pEntry = ptr->pHeader_csi;
			/* check if is new */
			if (bcmp(&pEntry->peer_mac, pMAC, ETHER_ADDR_LEN) == 0) {
				if (ptr->length_data_remain > 0) {
					save_ptr = ptr;
					break;
				}
			}
		}
	}
	/* check if need to drop head packet */
	if ((!save_ptr) && (dhdp->csi_count >= CONST_CSI_QUEUE_MAX_SIZE)) {
		DHD_ERROR(("%s-%d: ***CSI data is full, drop pkt \n",
		           __func__, __LINE__));
		ptr = list_last_entry(&dhdp->csi_list, struct csi_cfr_node, list);
		list_del_init(&ptr->list);
		dhd_csi_free_pkt(dhdp, ptr);
		dhdp->csi_count--;
	}
	mutex_unlock(&dhdp->csi_lock);

	/* try to append new node if needs */
	DHD_TRACE(("%s-%d: save_ptr=0x%px, version=%d\n",
	           __func__, __LINE__, save_ptr, version));
	if (!save_ptr) {
		switch (version) {
		case CONST_CFR_HEADER_REVISION_0:
			ret = dhd_csi_data_input_revision0(
				dhdp, event, &(pCFR->header_rev_0), pData);
			break;
		case CONST_CFR_HEADER_REVISION_1:
			ret = dhd_csi_data_input_revision1(
				dhdp, event, &(pCFR->header_rev_1), pData);
			break;
		case CONST_CFR_HEADER_REVISION_2:
			ret = dhd_csi_data_input_revision2(
				dhdp, event, &(pCFR->header_rev_2), pData);
			break;
		case CONST_CFR_HEADER_REVISION_3:
			ret = dhd_csi_data_input_revision3(
				dhdp, event, &(pCFR->header_rev_3), pData);
			break;
		default:
			DHD_ERROR(("%s-%d: CSI version=%d error\n",
			           __func__, __LINE__, version));
			ret = BCME_BADOPTION;
			goto done;
			break;
		}
	} else {
		ret = dhd_csi_data_append(dhdp, save_ptr,
		                          data_length, remain_length, pData);
	}

	if (0 <= ret) {
		ret = dhd_csi_data_queue_notify(dhdp, __func__, __LINE__);
	}

done:
	return ret;
}

int dhd_csi_event_enqueue(dhd_pub_t  *dhdp, int ifidx, void *pktbuf)
{
	if ((!dhdp) || (!dhdp->csi_init)) {
		DHD_ERROR(("%s-%d: *Error, NOT READY\n", __func__, __LINE__));
		return BCME_NOTREADY;
	}

	skb_queue_tail(&dhdp->csi_raw_skb_queue, pktbuf);

	schedule_work(&dhdp->csi_raw_skb_work);

	return BCME_OK;
}

static void dhd_csi_event_dequeue(struct work_struct *work_data)
{
	dhd_pub_t        *dhdp = NULL;
	uint32            qlen = 0;
	struct sk_buff   *skb = NULL;
	int               len = 0;
	bcm_event_t      *pvt_data = NULL;
	wl_event_msg_t    event;
	uint8            *event_data = NULL;
	int               ret = BCME_OK;

	dhdp = container_of(work_data, dhd_pub_t, csi_raw_skb_work);

	if ((!dhdp) || (!dhdp->csi_init)) {
		DHD_ERROR(("%s-%d: *Error, NOT READY\n", __func__, __LINE__));
		return;
	}

	qlen = skb_queue_len(&dhdp->csi_raw_skb_queue);
	while (qlen--) {
		skb = skb_dequeue(&dhdp->csi_raw_skb_queue);
		if (!skb) {
			DHD_ERROR(("%s: * invalid NULL skb\n", __func__));
			continue;
		}

		/* In dhd_rx_frame, header is stripped using skb_pull
		 * of size ETH_HLEN, so adjust pktlen accordingly
		 */
		len = skb->len + ETH_HLEN;
		if (sizeof(struct dhd_cfr_header_rev_0) > len) {
			DHD_ERROR(("%s: *Warning, skip too short pkt, len=%d\n", __func__, len));
		} else {
			pvt_data = (bcm_event_t *)skb_mac_header(skb);
			/* copy to get aligned 'event', but can't for 'event_data' */
			memcpy(&event, &(pvt_data->event), sizeof(wl_event_msg_t));
			event_data = (uint8 *)(&pvt_data[1]);

			ret = dhd_csi_event_handler(dhdp, &event, event_data);
			if (BCME_OK != ret) {
				DHD_ERROR(("%s: process event fail, ret=%d\n", __func__, ret));
			}
		}

		/* Free up the packet. */
#ifdef DHD_USE_STATIC_CTRLBUF
		PKTFREE_STATIC(dhdp->osh, skb, FALSE);
#else /* DHD_USE_STATIC_CTRLBUF */
		PKTFREE(dhdp->osh, skb, FALSE);
#endif /* DHD_USE_STATIC_CTRLBUF */
	}
}

static void
dhd_csi_clean_list(dhd_pub_t *dhdp)
{
	struct csi_cfr_node *ptr, *next;
	int num = 0;

	if (!dhdp) {
		DHD_ERROR(("%s-%d: *Error, NULL POINTER\n",
		           __func__, __LINE__));
		return;
	}

	mutex_lock(&dhdp->csi_lock);
	if (!list_empty(&dhdp->csi_list)) {
		list_for_each_entry_safe(ptr, next, &dhdp->csi_list, list) {
			list_del(&ptr->list);
			num++;
			dhd_csi_free_pkt(dhdp, ptr);
		}
	}
	dhdp->csi_count = 0;
	mutex_unlock(&dhdp->csi_lock);

	DHD_TRACE(("%s-%d: Clean up %d record\n",
	           __func__, __LINE__, num));
}

int dhd_csi_config(dhd_pub_t *dhdp, char *pBuf, uint length, bool is_set)
{
	struct syna_csi_rx_mode_config  *pRX_config = NULL;
	int                              ret = BCME_OK;

	if ((!dhdp) || (!pBuf) || (sizeof(uint32) > length)) {
		DHD_ERROR(("%s-%d: *Error, Bad argument\n",
		           __func__, __LINE__));
		ret = BCME_BADARG;
		goto done;
	}

	pRX_config = (struct syna_csi_rx_mode_config *)pBuf;

	if (is_set) {
		/* check mode */
		if (SYNA_CSI_DATA_MODE_LAST <= pRX_config->mode) {
			ret = BCME_BADOPTION;
			goto done;
		} else {
			dhdp->csi_data_send_manner = pRX_config->mode;
		}

		/* initialize for new CSI trigger */
		dhd_csi_clean_list(dhdp);
		dhdp->packet_global_id_last = -1;
		dhdp->packet_qty_duplicate = 0;
		dhdp->packet_qty_missing = 0;
		dhdp->csi_notify_ip = 0;
		dhdp->csi_notify_port = 0;

		/* try to get primary interface IP address */
		if (dhdp) {
			struct net_device  *ndev = NULL;
			struct in_device   *in_dev = NULL;

			ndev = dhd_linux_get_primary_netdev(dhdp);

			rcu_read_lock();
			in_dev = __in_dev_get_rcu(ndev);
			if (in_dev) {
				struct in_ifaddr *ifa = NULL;

				for (ifa = in_dev->ifa_list;
				     ifa != NULL;
				     ifa = ifa->ifa_next) {
					if (ifa->ifa_address) {
						dhdp->csi_local_ip = ifa->ifa_address;
						break;
					}
				}
			}
			rcu_read_unlock();
		}

		/* skip if no more parameter */
		if (!pRX_config->data_version) {
			goto done;
		}

		/* check DATA output version */
		if (CONST_SYNA_CSI_HEADER_VERSION_V2 < pRX_config->data_header_version_output) {
			DHD_ERROR(("%s-%d: *Error, invalid data header output version %d\n",
			           __func__, __LINE__,
			           pRX_config->data_header_version_output));
			ret = BCME_BADOPTION;
			goto done;
		} else if (CONST_SYNA_CSI_HEADER_VERSION_V1 >
			pRX_config->data_header_version_output) {
			dhdp->csi_header_output_version = CONST_SYNA_CSI_HEADER_VERSION_V1;
		} else {
			dhdp->csi_header_output_version = pRX_config->data_header_version_output;
		}

		/* check if using socket manner to send up */
		if (SYNA_CSI_DATA_MODE_UDP_SOCKET == pRX_config->mode) {
			if (!dhdp->csi_notify_ip) {
				dhdp->csi_notify_ip = dhdp->csi_local_ip;
			} else {
				dhdp->csi_notify_ip   = pRX_config->ip;
			}
			if (!dhdp->csi_notify_port) {
				dhdp->csi_notify_port = SYNA_SOCKET_PORT_DEFAULT;
			} else {
				dhdp->csi_notify_port = pRX_config->port;
			}
		}
	} else {
		pRX_config->mode = dhdp->csi_data_send_manner;
		pRX_config->data_version = TRUE;
		pRX_config->data_header_version_output = dhdp->csi_header_output_version;
		pRX_config->ip   = dhdp->csi_notify_ip;
		pRX_config->port = dhdp->csi_notify_port;
	}

done:
	return ret;
}

int dhd_csi_version(dhd_pub_t *dhdp, char *pBuf, uint length, bool is_set)
{
	int  ret = BCME_OK;

	if ((!dhdp) || (!pBuf) || (sizeof(uint8) > length)) {
		DHD_ERROR(("%s-%d: *Error, Bad argument\n",
		           __func__, __LINE__));
		ret = BCME_BADARG;
		goto done;
	}

	if (is_set) {
		ret = BCME_BADOPTION;
	} else {
		memcpy(pBuf, &dhdp->csi_version_capability, sizeof(uint8));
	}

done:
	return ret;
}

int dhd_csi_init(dhd_pub_t *dhdp)
{
	int err = BCME_OK;

	NULL_CHECK(dhdp, "dhdp is NULL", err);

	mutex_init(&(dhdp->csi_lock));

	mutex_lock(&(dhdp->csi_lock));

	skb_queue_head_init(&dhdp->csi_raw_skb_queue);
	INIT_WORK(&dhdp->csi_raw_skb_work, dhd_csi_event_dequeue);

	INIT_LIST_HEAD(&dhdp->csi_list);
	dhdp->csi_count = 0;

	dhdp->csi_version_capability = CONST_SYNA_CSI_HEADER_VERSION_V2;
	/* default DATA CSI header output version */
	dhdp->csi_header_output_version = CONST_CSI_HEADER_VERSION_OUTPUT_DEFAULT;
	dhdp->csi_data_send_manner = SYNA_CSI_DATA_MODE_SYSFS;

	dhdp->csi_init = 1;

	mutex_unlock(&dhdp->csi_lock);

	return err;
}

int
dhd_csi_deinit(dhd_pub_t *dhdp)
{
	int err = BCME_OK;

	NULL_CHECK(dhdp, "dhdp is NULL", err);

	dhd_csi_clean_list(dhdp);

	mutex_lock(&dhdp->csi_lock);

	dhdp->csi_init = 0;

	if (0 < skb_queue_len(&dhdp->csi_raw_skb_queue)) {
		struct sk_buff  *skb = NULL;

		while (NULL != (skb = skb_dequeue(&dhdp->csi_raw_skb_queue))) {
#ifdef DHD_USE_STATIC_CTRLBUF
			PKTFREE_STATIC(dhdp->osh, skb, FALSE);
#else /* DHD_USE_STATIC_CTRLBUF */
			PKTFREE(dhdp->osh, skb, FALSE);
#endif /* DHD_USE_STATIC_CTRLBUF */
		}
	}
	if (dhdp->csi_raw_skb_work.func) {
		cancel_work_sync(&dhdp->csi_raw_skb_work);
	}

	mutex_unlock(&dhdp->csi_lock);

	mutex_destroy(&dhdp->csi_lock);

	return err;
}

#endif /* CSI_SUPPORT */
