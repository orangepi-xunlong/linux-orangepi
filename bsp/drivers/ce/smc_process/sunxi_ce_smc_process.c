/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Copyright(c) 2020 - 2023 Allwinner Technology Co.,Ltd. All rights reserved. */
/*
 * Key blob driver based on SUNXI CE hardware.
 *
 * Copyright (C) 2013 Allwinner.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether exprece or implied.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <sunxi-smc.h>
#include <crypto/scatterwalk.h>
#include "../sunxi_ce_cdev.h"
#include "../sunxi_ce_proc.h"

int __attribute__((weak)) smc_tee_secure_key_encrypt(char *out_buf, char *in_buf, int len, enum secure_key_type key)
{
	pr_err("smc_tee_secure_key_encrypt is weak, please enable smc!\n");
	return -1;
}

int __attribute__((weak)) smc_tee_secure_key_decrypt(char *out_buf, char *in_buf, int len, enum secure_key_type key)
{
	pr_err("smc_tee_secure_key_decrypt is weak, please enable smc!\n");
	return -1;
}

static int ce_aes_setkey(struct crypto_skcipher *tfm, const u8 *key,
				unsigned int keylen)
{
	//This algorithm uses HW unique key, therefore ignore current call
	return 0;
}

static int ce_aes_cbc_huk_encrypt(struct skcipher_request *req)
{
	u8 *back_src = NULL, *back_dst = NULL;
	int ret = 0;

	//pr_err("data len is %d \n", req->cryptlen);
	back_src = kzalloc(req->cryptlen, GFP_KERNEL);
	if (!back_src) {
		pr_err("ce_keyblob: %s can't alloc %d byte for back_src\n", __func__, req->cryptlen);
		ret = -ENOMEM;
		goto out;
	}

	/* copy data from scatterlist to memory buffer */
	scatterwalk_map_and_copy(back_src, req->src, 0, req->cryptlen, 0);

	//pr_err("src0 is %d \n", back_src[0]);
	back_dst = kzalloc(req->cryptlen, GFP_KERNEL);
	if (!back_dst) {
		pr_err("ce_keyblob: %s can't alloc %d byte for back_dst\n", __func__, req->cryptlen);
		ret = -ENOMEM;
		goto out;
	}

	smc_tee_secure_key_encrypt(back_dst, back_src, req->cryptlen, CE_KEY_SELECT_HUK);

	/* copy data back to scatterlist */
	scatterwalk_map_and_copy(back_dst, req->dst, 0, req->cryptlen, 1);

out:
	if (back_dst)
		kfree(back_dst);
	if (back_src)
		kfree(back_src);

	return ret;
}

static int ce_aes_cbc_huk_decrypt(struct skcipher_request *req)
{
	u8 *back_src = NULL, *back_dst = NULL;
	int ret = 0;

	//pr_err("data len is %d \n", req->cryptlen);
	back_src = kzalloc(req->cryptlen, GFP_KERNEL);
	if (!back_src) {
		pr_err("ce_keyblob: %s can't alloc %d byte for back_src\n", __func__, req->cryptlen);
		ret = -ENOMEM;
		goto out;
	}

	/* copy data from scatterlist to memory buffer */
	scatterwalk_map_and_copy(back_src, req->src, 0, req->cryptlen, 0);

	back_dst = kzalloc(req->cryptlen, GFP_KERNEL);
	if (!back_dst) {
		pr_err("ce_keyblob: %s can't alloc %d byte for back_dst\n", __func__, req->cryptlen);
		ret = -ENOMEM;
		goto out;
	}

	smc_tee_secure_key_decrypt(back_dst, back_src, req->cryptlen, CE_KEY_SELECT_HUK);

	/* copy data back to scatterlist */
	scatterwalk_map_and_copy(back_dst, req->dst, 0, req->cryptlen, 1);

out:
	if (back_dst)
		kfree(back_dst);
	if (back_src)
		kfree(back_src);

	return ret;
}

static int sunxi_ce_skcipher_init(struct crypto_skcipher *tfm)
{
	crypto_skcipher_set_reqsize(tfm, sizeof(ss_aes_req_ctx_t));
	SS_DBG("reqsize = %d\n", tfm->reqsize);

	return 0;
}

static void sunxi_ce_skcipher_exit(struct crypto_skcipher *tfm)
{
	SS_ENTER();
}

#define DECLARE_SS_AES_ALG(utype, ltype, lmode, keyname, block_size, iv_size) \
{ \
	.type = ALG_TYPE_CIPHER, \
	.alg.skcipher = { \
		.base.cra_name	      = #lmode"("#ltype")", \
		.base.cra_driver_name = "ce-"#lmode"-"#ltype"-"#keyname, \
		.base.cra_flags	      = CRYPTO_ALG_ASYNC, \
		.base.cra_blocksize   = block_size, \
		.base.cra_alignmask   = 3, \
		.setkey      = ce_aes_setkey, \
		.encrypt     = ce_##ltype##_##lmode##_##keyname##_encrypt, \
		.decrypt     = ce_##ltype##_##lmode##_##keyname##_decrypt, \
		.min_keysize = utype##_MIN_KEY_SIZE, \
		.max_keysize = utype##_MAX_KEY_SIZE, \
		.ivsize	     = iv_size, \
	} \
}

static struct sunxi_crypto_tmp sunxi_ce_algs[] = {
	DECLARE_SS_AES_ALG(AES, aes, cbc, huk, AES_BLOCK_SIZE, AES_MIN_KEY_SIZE),
};

static int sunxi_ce_alg_register(void)
{
	int i;
	int ret = 0;

	for (i = 0; i < ARRAY_SIZE(sunxi_ce_algs); i++) {
		INIT_LIST_HEAD(&sunxi_ce_algs[i].alg.skcipher.base.cra_list);
		SS_DBG("Add %s...\n", sunxi_ce_algs[i].alg.skcipher.base.cra_name);
		sunxi_ce_algs[i].alg.skcipher.base.cra_priority = SS_ALG_PRIORITY;
		sunxi_ce_algs[i].alg.skcipher.base.cra_ctxsize = sizeof(ss_aes_ctx_t);
		sunxi_ce_algs[i].alg.skcipher.base.cra_module = THIS_MODULE;
		sunxi_ce_algs[i].alg.skcipher.exit = sunxi_ce_skcipher_exit;
		sunxi_ce_algs[i].alg.skcipher.init = sunxi_ce_skcipher_init;

		ret = crypto_register_skcipher(&sunxi_ce_algs[i].alg.skcipher);
		if (ret != 0) {
			SS_ERR("crypto_register_alg(%s) failed! return %d\n",
				sunxi_ce_algs[i].alg.skcipher.base.cra_name, ret);
			return ret;
		}
	}

	return 0;
}

static void sunxi_ce_alg_unregister(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(sunxi_ce_algs); i++)
		crypto_unregister_skcipher(&sunxi_ce_algs[i].alg.skcipher);
}

static int sunxi_ce_probe(void)
{
	int ret = 0;

	ret = sunxi_ce_alg_register();
	if (ret != 0) {
		SS_ERR("sunxi_ce_alg_register() failed! return %d\n", ret);
		goto endfunc;
	}

	SS_DBG("SS is inited!\n");
	return 0;

endfunc:
	return ret;
}

static int sunxi_ce_remove(void)
{
	sunxi_ce_alg_unregister();

	return 0;
}

static int __init sunxi_ce_init(void)
{
	int ret = 0;

	ret = sunxi_ce_probe();
	if (ret < 0) {
		SS_ERR("sunxi ce probe failed, return %d\n", ret);
		return ret;
	}

	return ret;
}

static void __exit sunxi_ce_exit(void)
{
	sunxi_ce_remove();
}

module_init(sunxi_ce_init);
module_exit(sunxi_ce_exit);

MODULE_DESCRIPTION("SUNXI CE Use Secure Key Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0.0");
MODULE_AUTHOR("ssd");
