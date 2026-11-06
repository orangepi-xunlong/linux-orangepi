
#include "vpuapi.h"
#include "header_struct.h"
#include "h265_interface.h"
#undef CLIP3
#define CLIP3(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

typedef struct UserDataList_struct {
    struct list_head list;
    Uint8 *userDataBuf;
    Uint32 userDataLen;
} UserDataList;

Uint32 venc_help_sei_encode(CodStd format, Uint8 *pSrc, Uint32 srcLen, Uint8 *pBuffer,
         Uint32 bufferSize);
BOOL venc_help_h264_sps_add_vui(H264Vui *pVui, void **ppBuffer, Int32 *pBufferSize, Int32 *pBufferBitSize);
BOOL venc_help_h265_sps_add_vui(H265Vui *pVui, void **ppBuffer, Int32 *pBufferSize, Int32 *pBufferBitSize);
void venc_help_gen_qpmap_from_roiregion(RoiParam *roiParam, Uint32 picWidth, Uint32 picHeight, int initQp, Uint8 *roiCtuMap, CodStd bitstreamFormat);
int venc_help_set_mapdata(int core_idx, RoiParam *roi_rect, int roi_base_qp, int picWidth, int picHeight, WaveCustomMapOpt *customMapOpt,
                                                CodStd  bitstreamFormat, PhysicalAddress *addr);