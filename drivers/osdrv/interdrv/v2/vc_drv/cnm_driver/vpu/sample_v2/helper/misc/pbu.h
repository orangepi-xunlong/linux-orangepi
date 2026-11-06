//-----------------------------------------------------------------------------
// COPYRIGHT (C) 2020   CHIPS&MEDIA INC. ALL RIGHTS RESERVED
//
// This file is distributed under BSD 3 clause and LGPL2.1 (dual license)
// SPDX License Identifier: BSD-3-Clause
// SPDX License Identifier: LGPL-2.1-only
//
// The entire notice above must be reproduced on all authorized copies.
//
// Description  :
//-----------------------------------------------------------------------------

#ifndef __PBU_H__
#define __PBU_H__

#include <vputypes.h>

typedef void* spp_enc_context;

#ifdef __cplusplus
extern "C" {
#endif

    spp_enc_context spp_enc_init(Uint8 *buffer, int buffer_size, int enableEPB);
    void spp_enc_deinit(spp_enc_context context);
    void spp_enc_init_rbsp(spp_enc_context context);

    void spp_enc_put_nal_byte(spp_enc_context context, Uint32 var, int n);
    void spp_enc_put_bits(spp_enc_context context, Uint32 var, int n);

    void spp_enc_flush(spp_enc_context context);

    void spp_enc_put_ue(spp_enc_context context, Uint32 var);
    Uint32 spp_enc_get_ue_bit_size(Uint32 var);
    void spp_enc_put_se(spp_enc_context context, Int32 var);
    void spp_enc_put_byte_align(spp_enc_context context, int has_stop_bit);

    Uint32 spp_enc_get_wbuf_remain(spp_enc_context context);
    Uint32 spp_enc_get_rbsp_bit(spp_enc_context context);
    Uint32 spp_enc_get_nal_cnt(spp_enc_context context);
    Uint8* spp_enc_get_wr_ptr(spp_enc_context context);
    Uint8* spp_enc_get_wr_ptr_only(spp_enc_context context);

    Uint32 spp_enc_get_est_nal_cnt(spp_enc_context context);

#ifdef __cplusplus
}
#endif

#endif

