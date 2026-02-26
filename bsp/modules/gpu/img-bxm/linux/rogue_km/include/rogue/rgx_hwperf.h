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

#if !defined(__KERNEL__)
/* User-mode and Firmware definitions only */

#if defined(RGX_BVNC_CORE_KM_HEADER) && defined(RGX_BNC_CONFIG_KM_HEADER)

/* HWPerf interface assumption checks */
static_assert(RGX_FEATURE_NUM_CLUSTERS <= 16U, "Cluster count too large for HWPerf protocol definition");

/*! The number of indirectly addressable TPU_MSC blocks in the GPU */
# define RGX_HWPERF_PHANTOM_INDIRECT_BY_DUST MAX(((IMG_UINT32)RGX_FEATURE_NUM_CLUSTERS >> 1), 1U)

/*! The number of indirectly addressable USC blocks in the GPU */
# define RGX_HWPERF_PHANTOM_INDIRECT_BY_CLUSTER (RGX_FEATURE_NUM_CLUSTERS)

# if defined(RGX_FEATURE_S7_TOP_INFRASTRUCTURE)

 /*! Defines the number of performance counter blocks that are directly
  * addressable in the RGX register map for S. */
#  define RGX_HWPERF_MAX_DIRECT_ADDR_BLKS      1 /* JONES */
#  define RGX_HWPERF_INDIRECT_BY_PHANTOM       (RGX_NUM_PHANTOMS)
#  define RGX_HWPERF_PHANTOM_NONDUST_BLKS      1 /* BLACKPEARL */
#  define RGX_HWPERF_PHANTOM_DUST_BLKS         2 /* TPU, TEXAS */
#  define RGX_HWPERF_PHANTOM_DUST_CLUSTER_BLKS 2 /* USC, PBE */

# elif defined(RGX_FEATURE_XT_TOP_INFRASTRUCTURE)

  /*! Defines the number of performance counter blocks that are directly
   * addressable in the RGX register map. */
#   define RGX_HWPERF_MAX_DIRECT_ADDR_BLKS    2 /* TORNADO, TA */

#  define RGX_HWPERF_INDIRECT_BY_PHANTOM       (RGX_NUM_PHANTOMS)
#  define RGX_HWPERF_PHANTOM_NONDUST_BLKS      2 /* RASTER, TEXAS */
#  define RGX_HWPERF_PHANTOM_DUST_BLKS         1 /* TPU */
#  define RGX_HWPERF_PHANTOM_DUST_CLUSTER_BLKS 1 /* USC */

# else /* !defined(RGX_FEATURE_S7_TOP_INFRASTRUCTURE) && !defined(RGX_FEATURE_XT_TOP_INFRASTRUCTURE) i.e. S6 */

 /*! Defines the number of performance counter blocks that are
  * addressable in the RGX register map for Series 6. */
#  define RGX_HWPERF_MAX_DIRECT_ADDR_BLKS      3 /* TA, RASTER, HUB */
#  define RGX_HWPERF_INDIRECT_BY_PHANTOM       0 /* PHANTOM is not there in Rogue1. Just using it to keep naming same as later series (RogueXT n Rogue XT+) */
#  define RGX_HWPERF_PHANTOM_NONDUST_BLKS      0
#  define RGX_HWPERF_PHANTOM_DUST_BLKS         1 /* TPU */
#  define RGX_HWPERF_PHANTOM_DUST_CLUSTER_BLKS 1 /* USC */

# endif

/*! The number of performance counters in each layout block defined for UM/FW code */
#if defined(RGX_FEATURE_CLUSTER_GROUPING)
  #define RGX_HWPERF_CNTRS_IN_BLK 6
 #else
  #define RGX_HWPERF_CNTRS_IN_BLK 4
#endif

#endif /* #if defined(RGX_BVNC_CORE_KM_HEADER) && defined(RGX_BNC_CONFIG_KM_HEADER) */
#else /* defined(__KERNEL__) */
/* Kernel/server definitions - not used, hence invalid definitions */

# define RGX_HWPERF_NUM_BLOCK_UNITS_RUNTIME_CALC 0xFF

# define RGX_HWPERF_PHANTOM_INDIRECT_BY_DUST    RGX_HWPERF_NUM_BLOCK_UNITS_RUNTIME_CALC
# define RGX_HWPERF_PHANTOM_INDIRECT_BY_CLUSTER RGX_HWPERF_NUM_BLOCK_UNITS_RUNTIME_CALC

# define RGX_HWPERF_MAX_DIRECT_ADDR_BLKS      RGX_HWPERF_NUM_BLOCK_UNITS_RUNTIME_CALC
# define RGX_HWPERF_INDIRECT_BY_PHANTOM       RGX_HWPERF_NUM_BLOCK_UNITS_RUNTIME_CALC
# define RGX_HWPERF_PHANTOM_NONDUST_BLKS      RGX_HWPERF_NUM_BLOCK_UNITS_RUNTIME_CALC
# define RGX_HWPERF_PHANTOM_DUST_BLKS         RGX_HWPERF_NUM_BLOCK_UNITS_RUNTIME_CALC
# define RGX_HWPERF_PHANTOM_DUST_CLUSTER_BLKS RGX_HWPERF_NUM_BLOCK_UNITS_RUNTIME_CALC

#endif

/*! The number of custom non-mux counter blocks supported */
#define RGX_HWPERF_MAX_CUSTOM_BLKS 5U

/*! The number of counters supported in each non-mux counter block */
#define RGX_HWPERF_MAX_CUSTOM_CNTRS 8U


/******************************************************************************
 * Data Stream Common Types
 *****************************************************************************/

/*! All the Data Masters HWPerf is aware of. When a new DM is added to this
 * list, it should be appended at the end to maintain backward compatibility
 * of HWPerf data.
 */
#define	RGX_HWPERF_DM_GP                  0x00000000U
#define	RGX_HWPERF_DM_2D                  0x00000001U
#define	RGX_HWPERF_DM_TA                  0x00000002U
#define	RGX_HWPERF_DM_3D                  0x00000003U
#define	RGX_HWPERF_DM_CDM                 0x00000004U
#define	RGX_HWPERF_DM_RTU                 0x00000005U
#define	RGX_HWPERF_DM_SHG                 0x00000006U
#define	RGX_HWPERF_DM_TDM                 0x00000007U

#define	RGX_HWPERF_DM_LAST                0x00000008U

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

/*! Directly addressable counter blocks */
#if defined(DOXYGEN)
/*! _RGX_HWPERF_CNTBLK_ID */
#endif
#define	RGX_CNTBLK_ID_TA			 0x0000U
#define	RGX_CNTBLK_ID_RASTER		 0x0001U /*!< Non-cluster grouping cores */
#define	RGX_CNTBLK_ID_HUB			 0x0002U /*!< Non-cluster grouping cores */
#define	RGX_CNTBLK_ID_TORNADO		 0x0003U /*!< XT cores */
#define	RGX_CNTBLK_ID_JONES			 0x0004U /*!< S7 cores */
#define	RGX_CNTBLK_ID_DIRECT_LAST	 0x0005U /*!< Indirect blocks start from here */
#define	RGX_CNTBLK_ID_BF_DEPRECATED	 0x0005U /*!< Doppler unit (DEPRECATED) */
#define	RGX_CNTBLK_ID_BT_DEPRECATED	 0x0006U /*!< Doppler unit (DEPRECATED) */
#define	RGX_CNTBLK_ID_RT_DEPRECATED	 0x0007U /*!< Doppler unit (DEPRECATED) */
#define	RGX_CNTBLK_ID_SH_DEPRECATED	 0x0008U /*!< Ray tracing unit (DEPRECATED) */


/*! Indirectly addressable counter blocks. DA blocks indicate counter blocks
 *  where the counter registers are directly accessible
 */
#define	RGX_CNTBLK_ID_TPU_MCU0		 0x0010U /*!< Addressable by Dust */
#define RGX_CNTBLK_ID_TPU_MCU0_DA	 0x8010U
#define	RGX_CNTBLK_ID_TPU_MCU1		 0x0011U
#define RGX_CNTBLK_ID_TPU_MCU1_DA	 0x8011U
#define	RGX_CNTBLK_ID_TPU_MCU2		 0x0012U
#define RGX_CNTBLK_ID_TPU_MCU2_DA	 0x8012U
#define	RGX_CNTBLK_ID_TPU_MCU3		 0x0013U
#define RGX_CNTBLK_ID_TPU_MCU3_DA	 0x8013U
#define	RGX_CNTBLK_ID_TPU_MCU4		 0x0014U
#define RGX_CNTBLK_ID_TPU_MCU4_DA	 0x8014U
#define	RGX_CNTBLK_ID_TPU_MCU5		 0x0015U
#define RGX_CNTBLK_ID_TPU_MCU5_DA	 0x8015U
#define	RGX_CNTBLK_ID_TPU_MCU6		 0x0016U
#define RGX_CNTBLK_ID_TPU_MCU6_DA	 0x8016U
#define	RGX_CNTBLK_ID_TPU_MCU7		 0x0017U
#define RGX_CNTBLK_ID_TPU_MCU7_DA	 0x8017U
#define	RGX_CNTBLK_ID_TPU_MCU_ALL	 0x4010U
#define	RGX_CNTBLK_ID_TPU_MCU_ALL_DA	 0xC010U

#define	RGX_CNTBLK_ID_USC0			 0x0020U /*!< Addressable by Cluster */
#define RGX_CNTBLK_ID_USC0_DA		 0x8020U
#define	RGX_CNTBLK_ID_USC1			 0x0021U
#define RGX_CNTBLK_ID_USC1_DA		 0x8021U
#define	RGX_CNTBLK_ID_USC2			 0x0022U
#define RGX_CNTBLK_ID_USC2_DA		 0x8022U
#define	RGX_CNTBLK_ID_USC3			 0x0023U
#define RGX_CNTBLK_ID_USC3_DA		 0x8023U
#define	RGX_CNTBLK_ID_USC4			 0x0024U
#define RGX_CNTBLK_ID_USC4_DA		 0x8024U
#define	RGX_CNTBLK_ID_USC5			 0x0025U
#define RGX_CNTBLK_ID_USC5_DA		 0x8025U
#define	RGX_CNTBLK_ID_USC6			 0x0026U
#define RGX_CNTBLK_ID_USC6_DA		 0x8026U
#define	RGX_CNTBLK_ID_USC7			 0x0027U
#define RGX_CNTBLK_ID_USC7_DA		 0x8027U
#define	RGX_CNTBLK_ID_USC8			 0x0028U
#define RGX_CNTBLK_ID_USC8_DA		 0x8028U
#define	RGX_CNTBLK_ID_USC9			 0x0029U
#define RGX_CNTBLK_ID_USC9_DA		 0x8029U
#define	RGX_CNTBLK_ID_USC10			 0x002AU
#define RGX_CNTBLK_ID_USC10_DA		 0x802AU
#define	RGX_CNTBLK_ID_USC11			 0x002BU
#define RGX_CNTBLK_ID_USC11_DA		 0x802BU
#define	RGX_CNTBLK_ID_USC12			 0x002CU
#define RGX_CNTBLK_ID_USC12_DA		 0x802CU
#define	RGX_CNTBLK_ID_USC13			 0x002DU
#define RGX_CNTBLK_ID_USC13_DA		 0x802DU
#define	RGX_CNTBLK_ID_USC14			 0x002EU
#define RGX_CNTBLK_ID_USC14_DA		 0x802EU
#define	RGX_CNTBLK_ID_USC15			 0x002FU
#define RGX_CNTBLK_ID_USC15_DA		 0x802FU
#define	RGX_CNTBLK_ID_USC_ALL		 0x4020U
#define	RGX_CNTBLK_ID_USC_ALL_DA	 0xC020U

#define	RGX_CNTBLK_ID_TEXAS0		 0x0030U /*!< Addressable by Phantom in XT, Dust in S7 */
#define	RGX_CNTBLK_ID_TEXAS1		 0x0031U
#define	RGX_CNTBLK_ID_TEXAS2		 0x0032U
#define	RGX_CNTBLK_ID_TEXAS3		 0x0033U
#define	RGX_CNTBLK_ID_TEXAS4		 0x0034U
#define	RGX_CNTBLK_ID_TEXAS5		 0x0035U
#define	RGX_CNTBLK_ID_TEXAS6		 0x0036U
#define	RGX_CNTBLK_ID_TEXAS7		 0x0037U
#define	RGX_CNTBLK_ID_TEXAS_ALL		 0x4030U

#define	RGX_CNTBLK_ID_RASTER0		 0x0040U /*!< Addressable by Phantom, XT only */
#define	RGX_CNTBLK_ID_RASTER1		 0x0041U
#define	RGX_CNTBLK_ID_RASTER2		 0x0042U
#define	RGX_CNTBLK_ID_RASTER3		 0x0043U
#define	RGX_CNTBLK_ID_RASTER_ALL	 0x4040U

#define	RGX_CNTBLK_ID_BLACKPEARL0	 0x0050U /*!< Addressable by Phantom, S7, only */
#define	RGX_CNTBLK_ID_BLACKPEARL1	 0x0051U
#define	RGX_CNTBLK_ID_BLACKPEARL2	 0x0052U
#define	RGX_CNTBLK_ID_BLACKPEARL3	 0x0053U
#define	RGX_CNTBLK_ID_BLACKPEARL_ALL 0x4050U

#define	RGX_CNTBLK_ID_PBE0			 0x0060U /*!< Addressable by Cluster in S7 and PBE2_IN_XE */
#define	RGX_CNTBLK_ID_PBE1			 0x0061U
#define	RGX_CNTBLK_ID_PBE2			 0x0062U
#define	RGX_CNTBLK_ID_PBE3			 0x0063U
#define	RGX_CNTBLK_ID_PBE4			 0x0064U
#define	RGX_CNTBLK_ID_PBE5			 0x0065U
#define	RGX_CNTBLK_ID_PBE6			 0x0066U
#define	RGX_CNTBLK_ID_PBE7			 0x0067U
#define	RGX_CNTBLK_ID_PBE8			 0x0068U
#define	RGX_CNTBLK_ID_PBE9			 0x0069U
#define	RGX_CNTBLK_ID_PBE10			 0x006AU
#define	RGX_CNTBLK_ID_PBE11			 0x006BU
#define	RGX_CNTBLK_ID_PBE12			 0x006CU
#define	RGX_CNTBLK_ID_PBE13			 0x006DU
#define	RGX_CNTBLK_ID_PBE14			 0x006EU
#define	RGX_CNTBLK_ID_PBE15			 0x006FU
#define	RGX_CNTBLK_ID_PBE_ALL		 0x4060U

#define	RGX_CNTBLK_ID_LAST			 0x0070U /*!< End of PBE block */

#define	RGX_CNTBLK_ID_BX_TU0_DEPRECATED		 0x0070U /*!< Doppler unit, DEPRECATED */
#define	RGX_CNTBLK_ID_BX_TU1_DEPRECATED		 0x0071U
#define	RGX_CNTBLK_ID_BX_TU2_DEPRECATED		 0x0072U
#define	RGX_CNTBLK_ID_BX_TU3_DEPRECATED		 0x0073U
#define	RGX_CNTBLK_ID_BX_TU_ALL_DEPRECATED	 0x4070U

#define	RGX_CNTBLK_ID_CUSTOM0		 0x70F0U
#define	RGX_CNTBLK_ID_CUSTOM1		 0x70F1U
#define	RGX_CNTBLK_ID_CUSTOM2		 0x70F2U
#define	RGX_CNTBLK_ID_CUSTOM3		 0x70F3U
#define	RGX_CNTBLK_ID_CUSTOM4_FW	 0x70F4U /*!< Custom block used for getting statistics held in the FW */
#define	RGX_CNTBLK_ID_CUSTOM_MASK	 0x70FFU


/* Masks for the counter block ID*/
#define	RGX_CNTBLK_ID_UNIT_MASK      (0x000FU)
#define	RGX_CNTBLK_ID_GROUP_MASK     (0x00F0U)
#define	RGX_CNTBLK_ID_GROUP_SHIFT    (4U)
#define	RGX_CNTBLK_ID_MC_GPU_MASK    (0x0F00U)
#define	RGX_CNTBLK_ID_MC_GPU_SHIFT   (8U)
#define	RGX_CNTBLK_ID_UNIT_ALL_MASK  (0x4000U)
#define	RGX_CNTBLK_ID_DA_MASK        (0x8000U) /*!< Block with directly accessible counter registers */

#define RGX_CNTBLK_INDIRECT_COUNT(_class, _n) ((IMG_UINT32)(RGX_CNTBLK_ID_ ## _class ## _n) - (IMG_UINT32)(RGX_CNTBLK_ID_ ## _class ## 0) + 1u)

/*! The number of layout blocks defined with configurable multiplexed
 * performance counters, hence excludes custom counter blocks.
 */
#define RGX_HWPERF_MAX_DEFINED_BLKS  (\
	(IMG_UINT32)RGX_CNTBLK_ID_DIRECT_LAST    +\
	RGX_CNTBLK_INDIRECT_COUNT(TPU_MCU,     7)+\
	RGX_CNTBLK_INDIRECT_COUNT(USC,        15)+\
	RGX_CNTBLK_INDIRECT_COUNT(TEXAS,       7)+\
	RGX_CNTBLK_INDIRECT_COUNT(RASTER,      3)+\
	RGX_CNTBLK_INDIRECT_COUNT(BLACKPEARL,  3)+\
	RGX_CNTBLK_INDIRECT_COUNT(PBE,        15) )
#define RGX_HWPERF_MAX_MUX_BLKS      (\
    RGX_HWPERF_MAX_DEFINED_BLKS      )

static_assert(
	((RGX_CNTBLK_ID_DIRECT_LAST + ((RGX_CNTBLK_ID_LAST & RGX_CNTBLK_ID_GROUP_MASK) >> RGX_CNTBLK_ID_GROUP_SHIFT)) <= RGX_HWPERF_MAX_BVNC_BLOCK_LEN),
	"RGX_HWPERF_MAX_BVNC_BLOCK_LEN insufficient");

#define RGX_HWPERF_EVENT_MASK_VALUE(e)      (IMG_UINT64_C(1) << (IMG_UINT32)(e))

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

#define	RGX_CNTBLK_COUNTER0_ID 0U
#define	RGX_CNTBLK_COUNTER1_ID 1U
#define	RGX_CNTBLK_COUNTER2_ID 2U
#define	RGX_CNTBLK_COUNTER3_ID 3U
#define	RGX_CNTBLK_COUNTER4_ID 4U
#define	RGX_CNTBLK_COUNTER5_ID 5U
	/* MAX value used in server handling of counter config arrays */
#define	RGX_CNTBLK_MUX_COUNTERS_MAX 6U


/* sets all the bits from bit _b1 to _b2, in a IMG_UINT64 type */
#define MASK_RANGE_IMPL(b1, b2)	((IMG_UINT64)((IMG_UINT64_C(1) << ((IMG_UINT32)(b2)-(IMG_UINT32)(b1) + 1U)) - 1U) << (IMG_UINT32)(b1))
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


/*! Type used in the RGX API RGXConfigMuxHWPerfCounters() */
typedef struct
{
	/*! Counter block ID, see RGX_HWPERF_CNTBLK_ID */
	IMG_UINT16 ui16BlockID;

	/*! 4 or 6 LSBs used to select counters to configure in this block. */
	IMG_UINT8  ui8CounterSelect;

	/*! 4 or 6 LSBs used as MODE bits for the counters in the group. */
	IMG_UINT8  ui8Mode;

	/*! 5 or 6 LSBs used as the GROUP_SELECT value for the counter. */
	IMG_UINT8  aui8GroupSelect[RGX_CNTBLK_MUX_COUNTERS_MAX];

	/*! 16 LSBs used as the BIT_SELECT value for the counter. */
	IMG_UINT16 aui16BitSelect[RGX_CNTBLK_MUX_COUNTERS_MAX];

	/*! 14 LSBs used as the BATCH_MAX value for the counter. */
	IMG_UINT32 aui32BatchMax[RGX_CNTBLK_MUX_COUNTERS_MAX];

	/*! 14 LSBs used as the BATCH_MIN value for the counter. */
	IMG_UINT32 aui32BatchMin[RGX_CNTBLK_MUX_COUNTERS_MAX];
} UNCACHED_ALIGN RGX_HWPERF_CONFIG_MUX_CNTBLK;

RGX_FW_STRUCT_SIZE_ASSERT(RGX_HWPERF_CONFIG_MUX_CNTBLK);

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
