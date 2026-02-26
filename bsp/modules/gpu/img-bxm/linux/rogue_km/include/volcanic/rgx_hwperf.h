/*************************************************************************/ /*!
@File
@Title          RGX HWPerf and Debug Types and Defines Header
@Copyright      Copyright (c) Imagination Technologies Ltd. All Rights Reserved
@Description    Common data types definitions for hardware performance API
@License        Dual MIT/GPLv2

The contents of this file are subject to the MIT license as set out below.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Alternatively, the contents of this file may be used under the terms of
the GNU General Public License Version 2 ("GPL") in which case the provisions
of GPL are applicable instead of those above.

If you wish to allow use of your version of this file only under the terms of
GPL, and not to allow others to use your version of this file under the terms
of the MIT license, indicate your decision by deleting the provisions above
and replace them with the notice and other provisions required by GPL as set
out in the file called "GPL-COPYING" included in this distribution. If you do
not delete the provisions above, a recipient may use your version of this file
under the terms of either the MIT license or GPL.

This License is also included in this distribution in the file called
"MIT-COPYING".

EXCEPT AS OTHERWISE STATED IN A NEGOTIATED AGREEMENT: (A) THE SOFTWARE IS
PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT; AND (B) IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/ /**************************************************************************/
#ifndef RGX_HWPERF_H_
#define RGX_HWPERF_H_

#if defined(__cplusplus)
extern "C" {
#endif

/* These structures are used on both GPU and CPU and must be a size that is a
 * multiple of 64 bits, 8 bytes to allow the FW to write 8 byte quantities at
 * 8 byte aligned addresses. RGX_FW_STRUCT_*_ASSERT() is used to check this.
 */

/******************************************************************************
 * Includes and Defines
 *****************************************************************************/

#include "img_types.h"
#include "img_defs.h"

#include "rgx_common.h"
#include "rgx_hwperf_common.h"

#if defined(RGX_BVNC_CORE_KM_HEADER) && defined(RGX_BNC_CONFIG_KM_HEADER)
/* HWPerf interface assumption checks */
static_assert(RGX_FEATURE_NUM_CLUSTERS <= 16U,
			  "Cluster count too large for HWPerf protocol definition");
#endif

/*! Perf counter control words */
#define RGX_HWPERF_CTRL_NOP						(0)			/*!< only update HW counters */
#define RGX_HWPERF_CTRL_STATE_UPDATE_EN			(1U << 31)	/*!< persistent state update; see other flags below */
#define RGX_HWPERF_CTRL_GEOM_FULLRANGE			(1U)		/*!< selectable geom and 3D counters are full range */
#define RGX_HWPERF_CTRL_COMP_FULLRANGE			(2U)		/*!< selectable compute counters are full range */
#define RGX_HWPERF_CTRL_TDM_FULLRANGE			(4U)		/*!< selectable TDM counters are full range */


/******************************************************************************
 * Data Stream Common Types
 *****************************************************************************/

/*! All the Data Masters HWPerf is aware of. When a new DM is added to this
 * list, it should be appended at the end to maintain backward compatibility
 * of HWPerf data.
 */
#define RGX_HWPERF_DM_GP                  0x00000000U
#define	RGX_HWPERF_DM_TDM                 0x00000001U
#define	RGX_HWPERF_DM_GEOM                0x00000002U
#define	RGX_HWPERF_DM_3D                  0x00000003U
#define	RGX_HWPERF_DM_CDM                 0x00000004U
#define	RGX_HWPERF_DM_RTU                 0x00000005U

#define	RGX_HWPERF_DM_LAST                0x00000006U

#define	RGX_HWPERF_DM_INVALID             0x1FFFFFFFU

/******************************************************************************
 * API Types
 *****************************************************************************/

/*! Counter block IDs for all the hardware blocks with counters.
 * Directly addressable blocks must have a value between 0..15 [0..0xF].
 * Indirect groups have following encoding:
 * First hex digit (LSB) represents a unit number within the group
 * and the second hex digit represents the group number.
 * Group 0 is the direct group, all others are indirect groups.
 */
typedef IMG_UINT32 RGX_HWPERF_CNTBLK_ID;

/*! Directly addressable non bank-switched counter blocks */
#define RGX_CNTBLK_ID_JONES          0x0000U
#define RGX_CNTBLK_ID_SLC            0x0001U	/*!< SLC-specific counter control */
#define RGX_CNTBLK_ID_FBCDC          0x0002U
#define RGX_CNTBLK_ID_FW_CUSTOM      0x0003U	/*!< Custom FW provided counters */

/*! Directly addressable SLC counter blocks - presence depends on GPU. */
#define RGX_CNTBLK_ID_SLCBANK0       0x0004U	/*!< SLCBANK0 counter control */
#define RGX_CNTBLK_ID_SLCBANK1       0x0005U	/*!< SLCBANK1 counter control */
#define RGX_CNTBLK_ID_SLCBANK2       0x0006U	/*!< SLCBANK2 counter control */
#define RGX_CNTBLK_ID_SLCBANK3       0x0007U	/*!< SLCBANK3 counter control */
#define RGX_CNTBLK_ID_SLCBANK_ALL    0x4004U	/*!< SLC ALL block ID */

#define RGX_CNTBLK_ID_DIRECT_LAST    0x0008U	/*!< Indirect blocks start from here */

/*! Indirectly addressable counter blocks */
#define RGX_CNTBLK_ID_ISP0           0x0010U	/*!< ISP 1..N ISP */
#define RGX_CNTBLK_ID_ISP1           0x0011U
#define RGX_CNTBLK_ID_ISP2           0x0012U
#define RGX_CNTBLK_ID_ISP3           0x0013U
#define RGX_CNTBLK_ID_ISP4           0x0014U
#define RGX_CNTBLK_ID_ISP5           0x0015U
#define RGX_CNTBLK_ID_ISP6           0x0016U
#define RGX_CNTBLK_ID_ISP7           0x0017U
#define RGX_CNTBLK_ID_ISP8           0x0018U
#define RGX_CNTBLK_ID_ISP9           0x0019U
#define RGX_CNTBLK_ID_ISP_ALL        0x4010U

#define RGX_CNTBLK_ID_MERCER0        0x0020U	/*!< MERCER 1..N MERCER */
#define RGX_CNTBLK_ID_MERCER1        0x0021U
#define RGX_CNTBLK_ID_MERCER2        0x0022U
#define RGX_CNTBLK_ID_MERCER3        0x0023U
#define RGX_CNTBLK_ID_MERCER4        0x0024U
#define RGX_CNTBLK_ID_MERCER5        0x0025U
#define RGX_CNTBLK_ID_MERCER6        0x0026U
#define RGX_CNTBLK_ID_MERCER7        0x0027U
#define RGX_CNTBLK_ID_MERCER8        0x0028U
#define RGX_CNTBLK_ID_MERCER9        0x0029U
#define RGX_CNTBLK_ID_MERCER_ALL     0x4020U

#define RGX_CNTBLK_ID_PBE0           0x0030U	/*!< PBE 1..N PBE_PER_SPU x N SPU */
#define RGX_CNTBLK_ID_PBE1           0x0031U
#define RGX_CNTBLK_ID_PBE2           0x0032U
#define RGX_CNTBLK_ID_PBE3           0x0033U
#define RGX_CNTBLK_ID_PBE4           0x0034U
#define RGX_CNTBLK_ID_PBE5           0x0035U
#define RGX_CNTBLK_ID_PBE6           0x0036U
#define RGX_CNTBLK_ID_PBE7           0x0037U
#define RGX_CNTBLK_ID_PBE_ALL        0x4030U

#define RGX_CNTBLK_ID_PBE_SHARED0    0x0040U	/*!< PBE_SHARED 1..N SPU */
#define RGX_CNTBLK_ID_PBE_SHARED1    0x0041U
#define RGX_CNTBLK_ID_PBE_SHARED2    0x0042U
#define RGX_CNTBLK_ID_PBE_SHARED3    0x0043U
#define RGX_CNTBLK_ID_PBE_SHARED_ALL 0x4040U

#define RGX_CNTBLK_ID_USC0           0x0050U	/*!< USC 1..N USC */
#define RGX_CNTBLK_ID_USC1           0x0051U
#define RGX_CNTBLK_ID_USC2           0x0052U
#define RGX_CNTBLK_ID_USC3           0x0053U
#define RGX_CNTBLK_ID_USC4           0x0054U
#define RGX_CNTBLK_ID_USC5           0x0055U
#define RGX_CNTBLK_ID_USC6           0x0056U
#define RGX_CNTBLK_ID_USC7           0x0057U
#define RGX_CNTBLK_ID_USC8           0x0058U
#define RGX_CNTBLK_ID_USC9           0x0059U
#define RGX_CNTBLK_ID_USC_ALL        0x4050U

#define RGX_CNTBLK_ID_TPU0           0x0060U	/*!< TPU 1..N TPU */
#define RGX_CNTBLK_ID_TPU1           0x0061U
#define RGX_CNTBLK_ID_TPU2           0x0062U
#define RGX_CNTBLK_ID_TPU3           0x0063U
#define RGX_CNTBLK_ID_TPU4           0x0064U
#define RGX_CNTBLK_ID_TPU5           0x0065U
#define RGX_CNTBLK_ID_TPU6           0x0066U
#define RGX_CNTBLK_ID_TPU7           0x0067U
#define RGX_CNTBLK_ID_TPU8           0x0068U
#define RGX_CNTBLK_ID_TPU9           0x0069U
#define RGX_CNTBLK_ID_TPU_ALL        0x4060U

#define RGX_CNTBLK_ID_SWIFT0         0x0070U	/*!< SWIFT 1..N SWIFT */
#define RGX_CNTBLK_ID_SWIFT1         0x0071U
#define RGX_CNTBLK_ID_SWIFT2         0x0072U
#define RGX_CNTBLK_ID_SWIFT3         0x0073U
#define RGX_CNTBLK_ID_SWIFT4         0x0074U
#define RGX_CNTBLK_ID_SWIFT5         0x0075U
#define RGX_CNTBLK_ID_SWIFT6         0x0076U
#define RGX_CNTBLK_ID_SWIFT7         0x0077U
#define RGX_CNTBLK_ID_SWIFT8         0x0078U
#define RGX_CNTBLK_ID_SWIFT9         0x0079U
#define RGX_CNTBLK_ID_SWIFT_ALL      0x4070U

#define RGX_CNTBLK_ID_TEXAS0         0x0080U	/*!< TEXAS 1..N TEXAS */
#define RGX_CNTBLK_ID_TEXAS1         0x0081U
#define RGX_CNTBLK_ID_TEXAS2         0x0082U
#define RGX_CNTBLK_ID_TEXAS3         0x0083U
#define RGX_CNTBLK_ID_TEXAS_ALL      0x4080U

#define RGX_CNTBLK_ID_RAC0           0x0090U	/*!< RAC 1..N RAC */
#define RGX_CNTBLK_ID_RAC1           0x0091U
#define RGX_CNTBLK_ID_RAC2           0x0092U
#define RGX_CNTBLK_ID_RAC3           0x0093U
#define RGX_CNTBLK_ID_RAC_ALL        0x4090U

#define RGX_CNTBLK_ID_LAST           0x0094U	/*!< End of RAC block */

/*! Masks for the counter block ID*/
#define RGX_CNTBLK_ID_UNIT_MASK      (0x000FU)	/*!< Unit within group */
#define RGX_CNTBLK_ID_GROUP_MASK     (0x00F0U)	/*!< Group value */
#define RGX_CNTBLK_ID_GROUP_SHIFT    (4U)
#define RGX_CNTBLK_ID_MC_GPU_MASK    (0x0F00U)	/*!< GPU ID for MC use */
#define RGX_CNTBLK_ID_MC_GPU_SHIFT   (8U)
#define RGX_CNTBLK_ID_UNIT_ALL_MASK  (0x4000U)	/*!< Program all units within a group */

static_assert(
	((RGX_CNTBLK_ID_DIRECT_LAST + ((RGX_CNTBLK_ID_LAST & RGX_CNTBLK_ID_GROUP_MASK) >> RGX_CNTBLK_ID_GROUP_SHIFT)) <= RGX_HWPERF_MAX_BVNC_BLOCK_LEN),
	"RGX_HWPERF_MAX_BVNC_BLOCK_LEN insufficient");

#define RGX_HWPERF_EVENT_MASK_VALUE(e)      (IMG_UINT64_C(1) << (e))

/* When adding new counters here, make sure changes are made to rgxfw_hwperf_fwblk_valid() as well */
#define RGX_CUSTOM_FW_CNTRS	\
                X(TA_LOCAL_FL_SIZE,    0x0, RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_TAKICK) |         \
                                            RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_PMOOM_TAPAUSE) |  \
                                            RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_PMOOM_TARESUME) | \
                                            RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_TAFINISHED))      \
                                                                                                        \
                X(TA_GLOBAL_FL_SIZE,   0x1, RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_TAKICK) |         \
                                            RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_PMOOM_TAPAUSE) |  \
                                            RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_PMOOM_TARESUME) | \
                                            RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_TAFINISHED))      \
                                                                                                        \
                X(3D_LOCAL_FL_SIZE,    0x2, RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_3DKICK) |         \
                                            RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_3DFINISHED))      \
                                                                                                        \
                X(3D_GLOBAL_FL_SIZE,   0x3, RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_3DKICK) |         \
                                            RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_3DFINISHED))      \
                                                                                                        \
                X(ISP_TILES_IN_FLIGHT, 0x4, RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_3DKICK) |         \
                                            RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_3DSPMKICK))

/*! Counter IDs for the firmware held statistics */
typedef enum
{
#define X(ctr, id, allow_mask)	RGX_CUSTOM_FW_CNTR_##ctr = id,
	RGX_CUSTOM_FW_CNTRS
#undef X

	/* always the last entry in the list */
	RGX_CUSTOM_FW_CNTR_LAST
} RGX_HWPERF_CUSTOM_FW_CNTR_ID;

/*! Identifier for each counter in a performance counting module */
typedef IMG_UINT32 RGX_HWPERF_CNTBLK_COUNTER_ID;

/* sets all the bits from bit _b1 to _b2, in a IMG_UINT64 type */
#define MASK_RANGE_IMPL(b1, b2)	((IMG_UINT64)((IMG_UINT64_C(1) << ((IMG_UINT32)(b2)-(IMG_UINT32)(b1) + 1U)) - 1U) << (IMG_UINT32)b1)
#define MASK_RANGE(R)			MASK_RANGE_IMPL(R##_FIRST_TYPE, R##_LAST_TYPE)
#define RGX_HWPERF_HOST_EVENT_MASK_VALUE(e) (IMG_UINT32_C(1) << (e))

/*! Mask macros for use with RGXCtrlHWPerf() API.
 */
#define RGX_HWPERF_EVENT_MASK_NONE          (IMG_UINT64_C(0x0000000000000000))
#define RGX_HWPERF_EVENT_MASK_DEFAULT       RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_FWACT) | \
                                            RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_PWR_CHG) | \
                                            RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_CLKS_CHG)
#define RGX_HWPERF_EVENT_MASK_ALL           (IMG_UINT64_C(0xFFFFFFFFFFFFFFFF))

/*! HWPerf Firmware event masks
 * @par
 * All FW Start/End/Debug (SED) events. */
#define RGX_HWPERF_EVENT_MASK_FW_SED    (MASK_RANGE(RGX_HWPERF_FW_EVENT_RANGE))

#define RGX_HWPERF_EVENT_MASK_FW_UFO    (RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_UFO))
#define RGX_HWPERF_EVENT_MASK_FW_CSW    (RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_CSW_START) |\
                                          RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_CSW_FINISHED))
/*! All FW events. */
#define RGX_HWPERF_EVENT_MASK_ALL_FW    (RGX_HWPERF_EVENT_MASK_FW_SED |\
                                          RGX_HWPERF_EVENT_MASK_FW_UFO |\
                                          RGX_HWPERF_EVENT_MASK_FW_CSW)

/*! HW Periodic events (1ms interval). */
#define RGX_HWPERF_EVENT_MASK_HW_PERIODIC   (RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_HW_PERIODIC))
/*! All HW Kick/Finish events. */
#define RGX_HWPERF_EVENT_MASK_HW_KICKFINISH ((MASK_RANGE(RGX_HWPERF_HW_EVENT_RANGE0) |\
                                               MASK_RANGE(RGX_HWPERF_HW_EVENT_RANGE1)) &\
                                              ~(RGX_HWPERF_EVENT_MASK_HW_PERIODIC))

#define RGX_HWPERF_EVENT_MASK_ALL_HW        (RGX_HWPERF_EVENT_MASK_HW_KICKFINISH |\
                                              RGX_HWPERF_EVENT_MASK_HW_PERIODIC)

#define RGX_HWPERF_EVENT_MASK_ALL_PWR_EST   (MASK_RANGE(RGX_HWPERF_PWR_EST_RANGE))

#define RGX_HWPERF_EVENT_MASK_ALL_PWR       (RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_CLKS_CHG) |\
                                              RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_GPU_STATE_CHG) |\
                                              RGX_HWPERF_EVENT_MASK_VALUE(RGX_HWPERF_PWR_CHG))

/*! HWPerf Host event masks
 */
#define RGX_HWPERF_EVENT_MASK_HOST_WORK_ENQ  (RGX_HWPERF_HOST_EVENT_MASK_VALUE(RGX_HWPERF_HOST_ENQ))
#define RGX_HWPERF_EVENT_MASK_HOST_ALL_UFO   (RGX_HWPERF_HOST_EVENT_MASK_VALUE(RGX_HWPERF_HOST_UFO))
#define RGX_HWPERF_EVENT_MASK_HOST_ALL_PWR   (RGX_HWPERF_HOST_EVENT_MASK_VALUE(RGX_HWPERF_HOST_CLK_SYNC))


/*! Type used in the RGX API RGXConfigHWPerfCounters() */
typedef struct
{
	/*! Reserved for future use */
	IMG_UINT32 ui32Reserved;

	/*! Counter block ID, see RGX_HWPERF_CNTBLK_ID */
	IMG_UINT16 ui16BlockID;

	/*! Number of configured counters within this block */
	IMG_UINT16 ui16NumCounters;

	/*! Counter register values */
	IMG_UINT16 ui16Counters[RGX_CNTBLK_COUNTERS_MAX];

} UNCACHED_ALIGN RGX_HWPERF_CONFIG_CNTBLK;

RGX_FW_STRUCT_SIZE_ASSERT(RGX_HWPERF_CONFIG_CNTBLK);


#if defined(__cplusplus)
}
#endif

#endif /* RGX_HWPERF_H_ */

/******************************************************************************
 End of file
******************************************************************************/
