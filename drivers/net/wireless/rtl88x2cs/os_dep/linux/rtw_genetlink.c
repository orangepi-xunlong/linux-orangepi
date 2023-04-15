/******************************************************************************
 *
 * Copyright(c) 2007 - 2019 Realtek Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 *****************************************************************************/

#include <net/genetlink.h>
#include <linux/kernel.h>
#include <drv_types.h>


#if defined(CONFIG_ALIBABA_ZEROCONFIG)

#define GENL_CUSTOM_FAMILY_NAME	"WIFI_NL_CUSTOM"
#define MAX_CUSTOM_PKT_LENGTH	2048
#ifndef CONFIG_APPEND_VENDOR_IE_ENABLE
#define WLAN_MAX_VENDOR_IE_LEN 255
#endif
enum {
	__GENL_CUSTOM_ATTR_INVALID,
	GENL_CUSTOM_ATTR_MSG,	/* message */
	__GENL_CUSTOM_ATTR_MAX,
};
#define GENL_CUSTOM_ATTR_MAX       (__GENL_CUSTOM_ATTR_MAX - 1)

enum {
	__GENLL_CUSTOM_COMMAND_INVALID,
	GENL_CUSTOM_COMMAND_BIND,	/* bind */
	GENL_CUSTOM_COMMAND_SEND,	/* user -> kernel */
	GENL_CUSTOM_COMMAND_RECV,	/* kernel -> user */
	__GENL_CUSTOM_COMMAND_MAX,
};
#define GENL_CUSTOM_COMMAND_MAX    (__GENL_CUSTOM_COMMAND_MAX - 1)


static int rtw_genl_bind(struct sk_buff *skb, struct genl_info *info);
static int rtw_genl_recv(struct sk_buff *skb, struct genl_info *info);
#define GENLMSG_UNICAST_RETRY_LIMIT 5
static int rtw_genl_send_retry_cnt = 0;

static struct genl_family rtw_genl_family = {
	.id = GENL_ID_GENERATE,
	.hdrsize = 0,
	.name = GENL_CUSTOM_FAMILY_NAME,
	.version = 1,
	.maxattr = GENL_CUSTOM_ATTR_MAX,
};

static struct nla_policy rtw_genl_policy[GENL_CUSTOM_ATTR_MAX + 1] = {
	[GENL_CUSTOM_ATTR_MSG] = {.type = NLA_NUL_STRING},
};

static struct genl_ops rtw_genl_ops[] = {
	{
		.cmd = GENL_CUSTOM_COMMAND_BIND,
		.flags = 0,
		.policy = rtw_genl_policy,
		.doit = rtw_genl_bind,
		.dumpit = NULL,
	},
	{
		.cmd = GENL_CUSTOM_COMMAND_SEND,
		.flags = 0,
		.policy = rtw_genl_policy,
		.doit = rtw_genl_recv,
		.dumpit = NULL,
	},
};


static _adapter *padapter_for_genl = NULL;

void rtw_genl_init(_adapter *padapter)
{
	if (genl_register_family_with_ops(&rtw_genl_family, rtw_genl_ops) != 0){
		RTW_INFO("%s(): GE_NELINK family registration fail\n", __func__);
	} else {
		padapter_for_genl = padapter;
		padapter_for_genl->genl_bind_pid = -1;
		_rtw_memset(padapter_for_genl->target_macaddr, 0, ETH_ALEN);
	}
}

void rtw_genl_deinit(void)
{
	genl_unregister_family(&rtw_genl_family);
	if(padapter_for_genl != NULL) {
		padapter_for_genl->genl_bind_pid = -1;
		_rtw_memset(padapter_for_genl->target_macaddr, 0, ETH_ALEN);
		padapter_for_genl = NULL;
	}
}

static int rtw_genl_bind(struct sk_buff *skb, struct genl_info *info)
{
	struct nlattr *na;
	char* pData = NULL;
	int DataLen = 0;
	struct mlme_priv *pmlmepriv = &(padapter_for_genl->mlmepriv);

	RTW_INFO("%s : net_device name : %s\n",__func__, padapter_for_genl->pnetdev->name);


	if (info == NULL) {
		RTW_ERR("%s : genl_info is NULL\n", __func__);
		return -1;
	}

	na = info->attrs[GENL_CUSTOM_ATTR_MSG];
	if (na) {
		pData = (char*) nla_data(na);
		DataLen = nla_len(na);
		#if defined(CONFIG_ALIBABA_ZEROCONFIG_DBG)
		RTW_INFO("%s nla_len(na) : %d\n", __func__, DataLen);
		RTW_INFO_DUMP(NULL, pData, DataLen);
		#endif
	}

	if (strcmp(pData, "BIND") == 0) {
		padapter_for_genl->genl_bind_pid = info->snd_portid;
		#if defined(CONFIG_ALIBABA_ZEROCONFIG_DBG)
		RTW_INFO("BIND\n");
		RTW_INFO("%s : pid  = %d\n", __func__, info->snd_portid);
		RTW_INFO("%s : padapter_for_genl->genl_bind_pid  = %d\n", __func__, padapter_for_genl->genl_bind_pid);
		#endif

#if defined(CONFIG_POWER_SAVING)
		RTW_INFO("%s : disable ips & lps\n", __func__);
		rtw_pm_set_lps(padapter_for_genl, PS_MODE_ACTIVE);
		rtw_pm_set_ips(padapter_for_genl, IPS_NONE);
#endif
	} else if (strcmp(pData, "UNBIND") == 0) {
		#if defined(CONFIG_ALIBABA_ZEROCONFIG_DBG)
		RTW_INFO("UNBIND\n");
		RTW_INFO("%s : pid  = %d\n", __func__, info->snd_portid);
		RTW_INFO("%s : padapter_for_genl->genl_bind_pid  = %d\n", __func__, padapter_for_genl->genl_bind_pid);
		#endif
		padapter_for_genl->genl_bind_pid = -1;
		_rtw_memset(padapter_for_genl->target_macaddr, 0, ETH_ALEN);
		
#if defined(CONFIG_POWER_SAVING)
		RTW_INFO("%s() enable ips & lps\n", __func__);
		rtw_pm_set_lps(padapter_for_genl, PS_MODE_MAX);
		rtw_pm_set_ips(padapter_for_genl, IPS_NORMAL);
#endif
	} else {
		RTW_INFO("%s(): Unknown cmd %s\n", __func__, pData);
	}

	return 0;
}

static int rtw_genl_send_probersp(char* buf, int buf_len){

	struct mlme_priv *pmlmepriv = &(padapter_for_genl->mlmepriv);
	char da[6];
	char* pVendorIE = NULL;
	int VendorIELength = 0;

	if(buf == NULL || buf_len <= 0){
		RTW_ERR("%s buf is NULL or buf_len <= 0\n", __func__);
		return -1;
	}

	_rtw_memcpy(da, (buf + 4), MACADDRLEN);
	pVendorIE = rtw_get_ie((buf + 36), _VENDOR_SPECIFIC_IE_, &VendorIELength, buf_len - 36);
	
	// VendorIELength must include element id and length field
	VendorIELength += 2;
	if(VendorIELength > WLAN_MAX_VENDOR_IE_LEN) {
		RTW_ERR("VendorIELength exceeding 255\n");
		RTW_INFO("%s dump vendor ie\n", __func__);
		RTW_INFO_DUMP(NULL, pVendorIE, VendorIELength);
		return -1;
	}
	#if defined(CONFIG_ALIBABA_ZEROCONFIG_DBG)
	RTW_INFO("%s da:"MAC_FMT"\n", __func__, MAC_ARG(da));
	RTW_INFO("%s vendor ie length : %d \n", __func__, VendorIELength);
	RTW_INFO("%s dump vendor ie\n", __func__);
	RTW_INFO_DUMP(NULL, pVendorIE, VendorIELength);
	#endif

	//issue_probersp(padapter_for_genl, da, 0);

	issue_probersp_zeroconf(padapter_for_genl, buf, buf_len);

	return 0;
}
static int rtw_genl_recv(struct sk_buff *skb, struct genl_info *info)
{
	struct nlattr *na;
	char* pData = NULL;
	int DataLen = 0;

	if (info == NULL) {
		RTW_ERR("%s : genl_info is NULL\n", __func__);
		return -1;
	}
	RTW_INFO("%s : recv from process %d\n", __func__, info->snd_portid);
	na = info->attrs[GENL_CUSTOM_ATTR_MSG];

	if (na) {
		pData = (char*) nla_data(na);
		DataLen = nla_len(na);
		#if defined(CONFIG_ALIBABA_ZEROCONFIG_DBG)
		RTW_INFO("%s nla_len(na) : %d\n", __func__, DataLen);
		RTW_INFO_DUMP(NULL, pData, DataLen);
		#endif
	}

	if(*pData == 0x50) {
		RTW_INFO("%s : probe rsp\n", __func__);
		rtw_genl_send_probersp(pData, DataLen);
	} else if(*pData == 0x40) {
		RTW_ERR("%s : probe req\n", __func__);
	} else {
		RTW_ERR("%s : Unexpected pkt\n", __func__);
		RTW_INFO_DUMP(NULL, pData, DataLen);
	}

	return 0;
}

int rtw_genl_send(char* buf, int buf_len)
{
	struct sk_buff *skb = NULL;
	char* msg_head = NULL;
	int ret = -1;


	if (padapter_for_genl->genl_bind_pid == -1) {
		RTW_ERR("%s : There is no binded process\n", __func__);
		return -1;
	}
	RTW_INFO("%s : send to process %d\n", __func__, padapter_for_genl->genl_bind_pid);
	if(buf == NULL || buf_len <= 0) {
		RTW_ERR("%s buf is NULL or buf_len : %d\n", __func__, buf_len);
		return -1;
	}
	#if defined(CONFIG_ALIBABA_ZEROCONFIG_DBG)
	RTW_INFO("%s dump buf\n", __func__);
	RTW_INFO_DUMP(NULL, buf, buf_len);
	#endif

	skb = genlmsg_new(MAX_CUSTOM_PKT_LENGTH, GFP_KERNEL);

	if (skb) {
		msg_head = genlmsg_put(skb, 0, 0, &rtw_genl_family, 0, GENL_CUSTOM_COMMAND_RECV);
		if (msg_head == NULL) {
			nlmsg_free(skb);
			RTW_ERR("%s(): genlmsg_put fail\n", __func__);
			return -1;
		}

		ret = nla_put(skb, GENL_CUSTOM_ATTR_MSG, buf_len, buf);
		if (ret != 0) {
			nlmsg_free(skb);
			RTW_INFO("%s(): nla_put fail : %d\n", __func__, ret);
			return ret;
		}

		genlmsg_end(skb, msg_head);

		/* sending message */
		ret = genlmsg_unicast(&init_net, skb, padapter_for_genl->genl_bind_pid);
		if (ret != 0) {
			RTW_ERR("%s(): genlmsg_unicast fail : %d\n", __func__, ret);
			rtw_genl_send_retry_cnt++;
			if(rtw_genl_send_retry_cnt >= GENLMSG_UNICAST_RETRY_LIMIT){
				RTW_ERR("%s(): Exceeding retry cnt %d, process might be killed\n", __func__, rtw_genl_send_retry_cnt);
				RTW_ERR("%s(): Unbind pid : %d\n", __func__, padapter_for_genl->genl_bind_pid);
				padapter_for_genl->genl_bind_pid = -1;
			_rtw_memset(padapter_for_genl->target_macaddr, 0, ETH_ALEN);
		
#if defined(CONFIG_POWER_SAVING)
			RTW_INFO("%s() enable ips & lps\n", __func__);
			rtw_pm_set_lps(padapter_for_genl, PS_MODE_MAX);
			rtw_pm_set_ips(padapter_for_genl, IPS_NORMAL);
#endif				
			}
			return ret;
		}
	} else {
		RTW_ERR("%s(): genlmsg_new fail\n", __func__);
		return -1;
	}
	rtw_genl_send_retry_cnt = 0;
	return 0;	

}
#endif /* CONFIG_ALIBABA_ZEROCONFIG */
