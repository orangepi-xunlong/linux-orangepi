#include "pbu.h"
#include "main_helper.h"
#include "venc_help.h"

extern int swap_endian(unsigned long core_idx, unsigned char *data, int len, int endian);

Uint32 venc_help_sei_encode(CodStd format, Uint8 *pSrc, Uint32 srcLen, Uint8 *pBuffer,
         Uint32 bufferSize)
{
    spp_enc_context spp;
    Uint32 code;

    Uint32 put_bit_byte_size;
    Uint32 i;

    const Uint32 nuh_layer_id = 0;
    const Uint32 nuh_temporal_id_plus1 = 1;

    spp = spp_enc_init(pBuffer, bufferSize, 1);

    // put start code
    spp_enc_put_nal_byte(spp, 1, 4);

    // put nal header
    if (format == STD_AVC) {
        code = AVC_NUT_SEI;
        spp_enc_put_nal_byte(spp, code, 1);
    } else if (format == STD_HEVC) {
        code = ((SNT_PREFIX_SEI & 0x3f) << 9) |
               ((nuh_layer_id & 0x3f) << 3) |
               ((nuh_temporal_id_plus1 & 0x3) << 0);
        spp_enc_put_nal_byte(spp, code, 2);
    }

    // put payload type
    spp_enc_put_nal_byte(spp, 0x05, 1);

    // put payload size
    for (i = 0; i < (srcLen + 16) / 0xff; i++)
        spp_enc_put_nal_byte(spp, 0xff, 1);
    spp_enc_put_nal_byte(spp, (srcLen + 16) % 0xff, 1);

    // put uuid(16 byte nouse)
    for (i = 0; i < 8; i++)
        spp_enc_put_nal_byte(spp, 0x03, 2);

    //put userdata
    for (i = 0; i < srcLen; i++)
        spp_enc_put_bits(spp, pSrc[i], 8);

    spp_enc_put_byte_align(spp, 1);
    spp_enc_flush(spp);

    put_bit_byte_size = spp_enc_get_nal_cnt(spp);

    spp_enc_deinit(spp);
    return put_bit_byte_size;
}

int get_stop_one_bit_idx(Uint8 byte)
{
    int i;

    for (i = 0; i < 7; i++) {
        if ((byte >> i) & 0x1)
            break;
    }

    return i;
}

static void encode_vui_aspect_ratio_info(spp_enc_context spp,
                     VuiAspectRatio *pari)
{
    spp_enc_put_bits(spp, pari->aspect_ratio_info_present_flag ? 1 : 0, 1);
    if (pari->aspect_ratio_info_present_flag) {
        spp_enc_put_bits(spp, pari->aspect_ratio_idc, 8);
        if (pari->aspect_ratio_idc == EXTENDED_SAR) {
            spp_enc_put_bits(spp, pari->sar_width, 16);
            spp_enc_put_bits(spp, pari->sar_height, 16);
        }
    }
}

static void encode_vui_overscan_info(spp_enc_context spp, VuiAspectRatio *pari)
{
    spp_enc_put_bits(spp, pari->overscan_info_present_flag ? 1 : 0, 1);
    if (pari->overscan_info_present_flag) {
        spp_enc_put_bits(spp, pari->overscan_appropriate_flag ? 1 : 0,
                 1);
    }
}

static void encode_vui_videosignal_type(spp_enc_context spp,
                     VuiVideoSignalType *pvst)
{
    spp_enc_put_bits(spp, pvst->video_signal_type_present_flag ? 1 : 0, 1);
    if (pvst->video_signal_type_present_flag) {
        spp_enc_put_bits(spp, pvst->video_format, 3);
        spp_enc_put_bits(spp, pvst->video_full_range_flag, 1);
        spp_enc_put_bits(
            spp, pvst->colour_description_present_flag ? 1 : 0, 1);
        if (pvst->colour_description_present_flag) {
            spp_enc_put_bits(spp, pvst->colour_primaries, 8);
            spp_enc_put_bits(spp, pvst->transfer_characteristics,
                     8);
            spp_enc_put_bits(spp, pvst->matrix_coefficients, 8);
        }
    }
}

static void encode_vui_h264_timinginfo(spp_enc_context spp,
                    VuiH264TimingInfo *pti)
{
    spp_enc_put_bits(spp, pti->timing_info_present_flag ? 1 : 0, 1);
    if (pti->timing_info_present_flag) {
        spp_enc_put_bits(spp, pti->num_units_in_tick, 32);
        spp_enc_put_bits(spp, pti->time_scale, 32);
        spp_enc_put_bits(spp, pti->fixed_frame_rate_flag, 1);
    }
}

static void encode_vui_h265_timinginfo(spp_enc_context spp,
                    VuiH265TimingInfo *pti)
{
    spp_enc_put_bits(spp, pti->timing_info_present_flag ? 1 : 0, 1);
    if (pti->timing_info_present_flag) {
        spp_enc_put_bits(spp, pti->num_units_in_tick, 32);
        spp_enc_put_bits(spp, pti->time_scale, 32);
        spp_enc_put_bits(spp, 0,
                 1); // vui_poc_proportional_to_timing_flag = 0;
        spp_enc_put_bits(spp, 0,
                 1); // vui_hrd_parameters_present_flag = 0;
    }
}

BOOL venc_help_h264_sps_add_vui(H264Vui *pVui, void **ppBuffer, Int32 *pBufferSize, Int32 *pBufferBitSize)
{
    int dst_size = 64;
    unsigned char *dst_buf = NULL;
    int vui_byte_size = 0, vui_bit_size = 0;
    spp_enc_context spp;

    if (!pVui->aspect_ratio_info.aspect_ratio_info_present_flag &&
        !pVui->aspect_ratio_info.overscan_info_present_flag &&
        !pVui->video_signal_type.video_signal_type_present_flag &&
        !pVui->timing_info.timing_info_present_flag) {
        // none of the supported flags are present. do nothing.
        return TRUE;
    }

    dst_buf = osal_kmalloc(dst_size);
    osal_memset(dst_buf, 0, dst_size);
    spp = spp_enc_init(dst_buf, dst_size, 1);

    // vui_parameters_present_flag = 1
    // spp_enc_put_bits(spp, 1, 1);

    encode_vui_aspect_ratio_info(spp, &pVui->aspect_ratio_info);
    encode_vui_overscan_info(spp, &pVui->aspect_ratio_info);
    encode_vui_videosignal_type(spp, &pVui->video_signal_type);

    // chroma_loc_info_present_flag = 0
    spp_enc_put_bits(spp, 0, 1);

    encode_vui_h264_timinginfo(spp, &pVui->timing_info);

    // nal_hrd_parameters_present_flag = 0
    // vcl_hrd_parameters_present_flag = 0
    // pic_struct_present_flag = 0
    // bitstream_restriction_flag = 0
    spp_enc_put_bits(spp, 0, 4);

    // spp_enc_put_byte_align(spp, 1);
    spp_enc_flush(spp);

    // update vui rbsp length
    vui_byte_size = spp_enc_get_nal_cnt(spp);
    vui_bit_size = spp_enc_get_rbsp_bit(spp);
    spp_enc_deinit(spp);
    // swap_endian(0, dst_buf, vui_byte_size, VDI_128BIT_BIG_ENDIAN);

    *ppBuffer = dst_buf;
    *pBufferSize = vui_byte_size;
    *pBufferBitSize = vui_bit_size;
    return TRUE;
}

BOOL venc_help_h265_sps_add_vui(H265Vui *pVui, void **ppBuffer, Int32 *pBufferSize, Int32 *pBufferBitSize)
{

    int dst_size = 64;
    unsigned char *dst_buf = NULL;
    int vui_byte_size = 0, vui_bit_size = 0;
    spp_enc_context spp;

    if (!pVui->aspect_ratio_info.aspect_ratio_info_present_flag &&
        !pVui->aspect_ratio_info.overscan_info_present_flag &&
        !pVui->video_signal_type.video_signal_type_present_flag &&
        !pVui->timing_info.timing_info_present_flag) {
        // none of the supported flags are present. do nothing.
        return TRUE;
    }

    dst_buf = osal_kmalloc(dst_size);
    osal_memset(dst_buf, 0, dst_size);
    spp = spp_enc_init(dst_buf, dst_size, 1);

    // vui_parameters_present_flag = 1
    // spp_enc_put_bits(spp, 1, 1);

    encode_vui_aspect_ratio_info(spp, &pVui->aspect_ratio_info);
    encode_vui_overscan_info(spp, &pVui->aspect_ratio_info);
    encode_vui_videosignal_type(spp, &pVui->video_signal_type);

    // chroma_loc_info_present_flag = 0
    // neutral_chroma_indication_flag = 0
    // field_seq_flag = 0
    // frame_field_info_present_flag = 0
    // default_display_window_flag = 0
    spp_enc_put_bits(spp, 0, 5);

    encode_vui_h265_timinginfo(spp, &pVui->timing_info);

    // bitstream_restriction_flag = 0
    spp_enc_put_bits(spp, 0, 1);

    // spp_enc_put_byte_align(spp, 1);
    spp_enc_flush(spp);

    // update vui rbsp length
    vui_byte_size = spp_enc_get_nal_cnt(spp);
    vui_bit_size = spp_enc_get_rbsp_bit(spp);
    spp_enc_deinit(spp);

    *ppBuffer = dst_buf;
    *pBufferSize = vui_byte_size;
    *pBufferBitSize = vui_bit_size;

    return TRUE;
}
void venc_help_gen_qpmap_from_roiregion(RoiParam *roiParam, Uint32 picWidth, Uint32 picHeight, int initQp, Uint8 *roiCtuMap, CodStd bitstreamFormat) {

    Int32 i, blk_addr;
    Uint32 mapWidth, mapHeight = 0;
    Uint32 roi_map_size;
    int blkSize, operand;
    VpuRect *region = NULL;
    VpuRect *rect;
    Uint32 x, y;
    VpuRect *roi;
    region = (VpuRect *)osal_kmalloc(sizeof(VpuRect)*MAX_ROI_NUMBER);

    if (bitstreamFormat == STD_AVC) {
        operand = 4;
        mapWidth = VPU_ALIGN16(picWidth) >> operand;
        mapHeight = VPU_ALIGN16(picHeight) >> operand;
    }else if (bitstreamFormat == STD_HEVC) {
        operand = 5;
        mapWidth = VPU_ALIGN64(picWidth) >> operand;
        mapHeight = VPU_ALIGN64(picHeight) >> operand;
    }

    blkSize = 1 << operand;
    roi_map_size      = mapWidth * mapHeight;

    for(i = 0; i < MAX_ROI_NUMBER; i++) {
        if (roiParam[i].roi_enable_flag == FALSE)
            continue;
        rect = region + i;
        rect->left = CLIP3( roiParam[i].roi_rect_x >> operand, 0, picWidth);
        rect->top = CLIP3( roiParam[i].roi_rect_y >> operand, 0, picHeight);

        rect->right = CLIP3( rect->left + ((roiParam[i].roi_rect_width + blkSize -1) >> operand) - 1, 0, picWidth);
        rect->bottom = CLIP3( rect->top + ((roiParam[i].roi_rect_height + blkSize -1) >> operand) - 1, 0, picHeight);
    }

    for (blk_addr=0; blk_addr<(Int32)roi_map_size; blk_addr++)
        *(roiCtuMap + blk_addr) = initQp;

    for (i = 0; i < MAX_ROI_NUMBER; i++)
    {
        if (roiParam[i].roi_enable_flag == FALSE)
            continue;
        roi = region + i;

        for (y = roi->top; y <= roi->bottom; y++)
        {
            for (x = roi->left; x <= roi->right; x++)
            {
                if (roiParam[i].is_absolute_qp == TRUE) {
                    roiCtuMap[y*mapWidth + x] = roiParam[i].roi_qp;
                } else {
                    roiCtuMap[y*mapWidth + x] = (roiParam[i].roi_qp + initQp);
                }

            }
        }
    }

    if (region) {
        osal_kfree(region);
        region = NULL;
    }
}

int venc_help_set_mapdata(int core_idx, RoiParam *roi_rect, int roi_base_qp, int picWidth, int picHeight, WaveCustomMapOpt *customMapOpt,
                                                CodStd  bitstreamFormat, PhysicalAddress *addr){
    int MbWidth  =  VPU_ALIGN16(picWidth) >> 4;
    int MbHeight =  VPU_ALIGN16(picHeight) >> 4;
    int ctuMapWidthCnt  = VPU_ALIGN64(picWidth) >> 6;
    int ctuMapHeightCnt = VPU_ALIGN64(picHeight) >> 6;
    int MB_NUM = MbWidth * MbHeight;
    Uint8 *roiMapBuf = NULL;
    AvcEncCustomMap *AVCcustomMapBuf = NULL;
    EncCustomMap *HEVCcustomMapBuf = NULL;
    int h;
    int w;
    int mbAddr;
    int sumQp = 0;
    int ctuPos;
    Uint8* src = NULL;
    int ctu_num;
    int sub_ctu_num;
    int ctuMapStride;
    int subCtuMapStride;
    int bufSize;

    if (VPU_GetProductId(0) == PRODUCT_ID_521 && bitstreamFormat == STD_AVC) {

        roiMapBuf = (Uint8 *)osal_malloc(sizeof(Uint8) * MB_NUM);
        osal_memset(roiMapBuf, 0, MB_NUM);

        AVCcustomMapBuf =  (AvcEncCustomMap*)osal_malloc(sizeof(AvcEncCustomMap) * MB_NUM);
        osal_memset(AVCcustomMapBuf, 0x00, MB_NUM);

        venc_help_gen_qpmap_from_roiregion(roi_rect, picWidth, picHeight, roi_base_qp, roiMapBuf , STD_AVC); //initial qp should be set ？

        for (h = 0; h < MbHeight; h++) {
            for (w = 0; w < MbWidth; w++) {
                mbAddr = w + h*MbWidth;
                AVCcustomMapBuf[mbAddr].field.mb_qp = MAX(MIN(roiMapBuf[mbAddr], 51), 0);
                sumQp += AVCcustomMapBuf[mbAddr].field.mb_qp;
            }
        }

        customMapOpt->roiAvgQp = (sumQp + (MB_NUM >> 1)) / MB_NUM;
        customMapOpt->addrCustomMap = *addr;

        vdi_write_memory(core_idx, customMapOpt->addrCustomMap, (unsigned char*)AVCcustomMapBuf, MB_NUM, VPU_CUSTOM_MAP_ENDIAN);
        if (roiMapBuf) {
            osal_free(roiMapBuf);
            roiMapBuf = NULL;
        }
        if (AVCcustomMapBuf) {
            osal_free(AVCcustomMapBuf);
            AVCcustomMapBuf = NULL;
        }
    } else {  // HEVC roi custom map
        ctu_num = ctuMapWidthCnt * ctuMapHeightCnt;
        sub_ctu_num = 4 * ctu_num;

        roiMapBuf = (Uint8 *)osal_malloc(sizeof(Uint8) * sub_ctu_num);   //MAX_SUB_CTU_NUM
        if (roiMapBuf == NULL) {
                VLOG(ERR, "%s:%d fail to allocate  buffer\n", __FUNCTION__, __LINE__);
                return RETCODE_FAILURE;
        }
        osal_memset(roiMapBuf, 0, sub_ctu_num);

        HEVCcustomMapBuf = (EncCustomMap*)osal_malloc(sizeof(EncCustomMap) * ctu_num);
        if (HEVCcustomMapBuf == NULL) {
                VLOG(ERR, "%s:%d fail to allocate buffer\n", __FUNCTION__, __LINE__);
                return RETCODE_FAILURE;
        }
        osal_memset(HEVCcustomMapBuf, 0x00, ctu_num * 8);

        ctuMapStride = VPU_ALIGN64(picWidth) >> 6;
        subCtuMapStride = VPU_ALIGN64(picWidth) >> 5;
        bufSize = (VPU_ALIGN64(picWidth) >> 5) * (VPU_ALIGN64(picHeight) >> 5);
        venc_help_gen_qpmap_from_roiregion(roi_rect, picWidth, picHeight, roi_base_qp, roiMapBuf, STD_HEVC); //initial qp should be set ？

        for (h = 0; h < ctuMapHeightCnt; h++) {
            src = roiMapBuf + subCtuMapStride * h * 2;
            for (w = 0; w < ctuMapWidthCnt; w++, src += 2) {
                ctuPos = (h * ctuMapStride + w);

                HEVCcustomMapBuf[ctuPos].field.sub_ctu_qp_0 = MAX(MIN(*src, 51), 0);
                HEVCcustomMapBuf[ctuPos].field.sub_ctu_qp_1 = MAX(MIN(*(src + 1), 51), 0);
                HEVCcustomMapBuf[ctuPos].field.sub_ctu_qp_2 = MAX(MIN(*(src + subCtuMapStride), 51), 0);
                HEVCcustomMapBuf[ctuPos].field.sub_ctu_qp_3 = MAX(MIN(*(src + subCtuMapStride + 1), 51), 0);
                sumQp += (HEVCcustomMapBuf[ctuPos].field.sub_ctu_qp_0 + HEVCcustomMapBuf[ctuPos].field.sub_ctu_qp_1 +  \
                        HEVCcustomMapBuf[ctuPos].field.sub_ctu_qp_2 + HEVCcustomMapBuf[ctuPos].field.sub_ctu_qp_3);
            }
        }

        customMapOpt->roiAvgQp = (sumQp + (bufSize >> 1)) / bufSize;
        customMapOpt->addrCustomMap = *addr;

        vdi_write_memory(core_idx, customMapOpt->addrCustomMap, (unsigned char*)HEVCcustomMapBuf, ctu_num * 8, VPU_CUSTOM_MAP_ENDIAN);

        if (roiMapBuf) {
            osal_free(roiMapBuf);
            roiMapBuf = NULL;
        }
        if (HEVCcustomMapBuf) {
            osal_free(HEVCcustomMapBuf);
            roiMapBuf = NULL;
        }
        src = NULL;
    }
    return 0;
}
