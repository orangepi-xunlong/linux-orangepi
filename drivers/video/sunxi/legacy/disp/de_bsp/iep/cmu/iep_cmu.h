//------------------------------------------------------------------------------------------------------------
// 
// iep_cmu.h
// 
// WDK
// 
// 2012-05-21 13:55:40
// 
//------------------------------------------------------------------------------------------------------------
#ifndef __REG33__CMU__H__
#define __REG33__CMU__H__

#include "../iep.h"

#ifdef __cplusplus
extern "C" {
#endif

//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

//
// Register Offset
//
#define IMGEHC_CMU_CTL_REG_OFF            		0x000
#define IMGEHC_CMU_STS_REG_OFF            		0x004
#define IMGEHC_CMU_REGBUFFCTL_REG_OFF     		0x008
#define IMGEHC_CMU_INSIZE_REG_OFF         		0x010
#define IMGEHC_CMU_OWP_REG0_OFF           		0x018
#define IMGEHC_CMU_OWP_REG1_OFF           		0x01c
#define IMGEHC_CMU_HRANGE_REG0_OFF        		0x040+0*0x4
#define IMGEHC_CMU_HRANGE_REG1_OFF        		0x040+1*0x4
#define IMGEHC_CMU_HRANGE_REG2_OFF        		0x040+2*0x4
#define IMGEHC_CMU_HRANGE_REG3_OFF        		0x040+3*0x4
#define IMGEHC_CMU_HRANGE_REG4_OFF        		0x040+4*0x4
#define IMGEHC_CMU_HRANGE_REG5_OFF        		0x040+5*0x4
#define IMGEHC_CMU_HRANGE_REG6_OFF        		0x040+6*0x4
#define IMGEHC_CMU_VRANGE_HLGAIN_REG0_OFF 		0x070+0*0x4
#define IMGEHC_CMU_VRANGE_HLGAIN_REG1_OFF 		0x070+1*0x4
#define IMGEHC_CMU_VRANGE_HLGAIN_REG2_OFF 		0x070+2*0x4
#define IMGEHC_CMU_VRANGE_HLGAIN_REG3_OFF 		0x070+3*0x4
#define IMGEHC_CMU_VRANGE_HLGAIN_REG4_OFF 		0x070+4*0x4
#define IMGEHC_CMU_VRANGE_HLGAIN_REG5_OFF 		0x070+5*0x4
#define IMGEHC_CMU_VRANGE_HLGAIN_REG6_OFF 		0x070+6*0x4
#define IMGEHC_CMU_HGGAIN_REG_OFF         		0x09c
#define IMGEHC_CMU_SRANGE_REG0_OFF        		0x0a0+0*0x4
#define IMGEHC_CMU_SRANGE_REG1_OFF        		0x0a0+1*0x4
#define IMGEHC_CMU_SRANGE_REG2_OFF        		0x0a0+2*0x4
#define IMGEHC_CMU_SRANGE_REG3_OFF        		0x0a0+3*0x4
#define IMGEHC_CMU_SRANGE_REG4_OFF        		0x0a0+4*0x4
#define IMGEHC_CMU_SRANGE_REG5_OFF        		0x0a0+5*0x4
#define IMGEHC_CMU_SRANGE_REG6_OFF        		0x0a0+6*0x4
#define IMGEHC_CMU_LOCAL_SVGAIN_REG0_OFF  		0x0d0+0*0x4
#define IMGEHC_CMU_LOCAL_SVGAIN_REG1_OFF  		0x0d0+1*0x4
#define IMGEHC_CMU_LOCAL_SVGAIN_REG2_OFF  		0x0d0+2*0x4
#define IMGEHC_CMU_LOCAL_SVGAIN_REG3_OFF  		0x0d0+3*0x4
#define IMGEHC_CMU_LOCAL_SVGAIN_REG4_OFF  		0x0d0+4*0x4
#define IMGEHC_CMU_LOCAL_SVGAIN_REG5_OFF  		0x0d0+5*0x4
#define IMGEHC_CMU_LOCAL_SVGAIN_REG6_OFF  		0x0d0+6*0x4
#define IMGEHC_CMU_GLOBAL_SVGAIN_REG_OFF  		0x0fc

//
// Register Address
//
#define IMGEHC_CMU_CTL_REG_ADDR           		( CMU_VBASE + IMGEHC_CMU_CTL_REG_OFF            )		// CMU control register
#define IMGEHC_CMU_STS_REG_ADDR           		( CMU_VBASE + IMGEHC_CMU_STS_REG_OFF            )		// CMU status register
#define IMGEHC_CMU_REGBUFFCTL_REG_ADDR    		( CMU_VBASE + IMGEHC_CMU_REGBUFFCTL_REG_OFF     )		// CMU register buffer control register
#define IMGEHC_CMU_INSIZE_REG_ADDR        		( CMU_VBASE + IMGEHC_CMU_INSIZE_REG_OFF         )		// CMU input size setting register
#define IMGEHC_CMU_OWP_REG0_ADDR          		( CMU_VBASE + IMGEHC_CMU_OWP_REG0_OFF           )		// CMU output window position register0
#define IMGEHC_CMU_OWP_REG1_ADDR          		( CMU_VBASE + IMGEHC_CMU_OWP_REG1_OFF           )		// CMU output window position register1
#define IMGEHC_CMU_HRANGE_REG0_ADDR       		( CMU_VBASE + IMGEHC_CMU_HRANGE_REG0_OFF        )		// CMU hue range register
#define IMGEHC_CMU_HRANGE_REG1_ADDR       		( CMU_VBASE + IMGEHC_CMU_HRANGE_REG1_OFF        )		// CMU hue range register
#define IMGEHC_CMU_HRANGE_REG2_ADDR       		( CMU_VBASE + IMGEHC_CMU_HRANGE_REG2_OFF        )		// CMU hue range register
#define IMGEHC_CMU_HRANGE_REG3_ADDR       		( CMU_VBASE + IMGEHC_CMU_HRANGE_REG3_OFF        )		// CMU hue range register
#define IMGEHC_CMU_HRANGE_REG4_ADDR       		( CMU_VBASE + IMGEHC_CMU_HRANGE_REG4_OFF        )		// CMU hue range register
#define IMGEHC_CMU_HRANGE_REG5_ADDR       		( CMU_VBASE + IMGEHC_CMU_HRANGE_REG5_OFF        )		// CMU hue range register
#define IMGEHC_CMU_HRANGE_REG6_ADDR       		( CMU_VBASE + IMGEHC_CMU_HRANGE_REG6_OFF        )		// CMU hue range register
#define IMGEHC_CMU_VRANGE_HLGAIN_REG0_ADDR		( CMU_VBASE + IMGEHC_CMU_VRANGE_HLGAIN_REG0_OFF )		// CMU brightness range and hue local gain control register
#define IMGEHC_CMU_VRANGE_HLGAIN_REG1_ADDR		( CMU_VBASE + IMGEHC_CMU_VRANGE_HLGAIN_REG1_OFF )		// CMU brightness range and hue local gain control register
#define IMGEHC_CMU_VRANGE_HLGAIN_REG2_ADDR		( CMU_VBASE + IMGEHC_CMU_VRANGE_HLGAIN_REG2_OFF )		// CMU brightness range and hue local gain control register
#define IMGEHC_CMU_VRANGE_HLGAIN_REG3_ADDR		( CMU_VBASE + IMGEHC_CMU_VRANGE_HLGAIN_REG3_OFF )		// CMU brightness range and hue local gain control register
#define IMGEHC_CMU_VRANGE_HLGAIN_REG4_ADDR		( CMU_VBASE + IMGEHC_CMU_VRANGE_HLGAIN_REG4_OFF )		// CMU brightness range and hue local gain control register
#define IMGEHC_CMU_VRANGE_HLGAIN_REG5_ADDR		( CMU_VBASE + IMGEHC_CMU_VRANGE_HLGAIN_REG5_OFF )		// CMU brightness range and hue local gain control register
#define IMGEHC_CMU_VRANGE_HLGAIN_REG6_ADDR		( CMU_VBASE + IMGEHC_CMU_VRANGE_HLGAIN_REG6_OFF )		// CMU brightness range and hue local gain control register
#define IMGEHC_CMU_HGGAIN_REG_ADDR        		( CMU_VBASE + IMGEHC_CMU_HGGAIN_REG_OFF         )		// CMU hue global gain register
#define IMGEHC_CMU_SRANGE_REG0_ADDR       		( CMU_VBASE + IMGEHC_CMU_SRANGE_REG0_OFF        )		// CMU saturation range register
#define IMGEHC_CMU_SRANGE_REG1_ADDR       		( CMU_VBASE + IMGEHC_CMU_SRANGE_REG1_OFF        )		// CMU saturation range register
#define IMGEHC_CMU_SRANGE_REG2_ADDR       		( CMU_VBASE + IMGEHC_CMU_SRANGE_REG2_OFF        )		// CMU saturation range register
#define IMGEHC_CMU_SRANGE_REG3_ADDR       		( CMU_VBASE + IMGEHC_CMU_SRANGE_REG3_OFF        )		// CMU saturation range register
#define IMGEHC_CMU_SRANGE_REG4_ADDR       		( CMU_VBASE + IMGEHC_CMU_SRANGE_REG4_OFF        )		// CMU saturation range register
#define IMGEHC_CMU_SRANGE_REG5_ADDR       		( CMU_VBASE + IMGEHC_CMU_SRANGE_REG5_OFF        )		// CMU saturation range register
#define IMGEHC_CMU_SRANGE_REG6_ADDR       		( CMU_VBASE + IMGEHC_CMU_SRANGE_REG6_OFF        )		// CMU saturation range register
#define IMGEHC_CMU_LOCAL_SVGAIN_REG0_ADDR 		( CMU_VBASE + IMGEHC_CMU_LOCAL_SVGAIN_REG0_OFF  )		// CMU saturation and brightness local gain register
#define IMGEHC_CMU_LOCAL_SVGAIN_REG1_ADDR 		( CMU_VBASE + IMGEHC_CMU_LOCAL_SVGAIN_REG1_OFF  )		// CMU saturation and brightness local gain register
#define IMGEHC_CMU_LOCAL_SVGAIN_REG2_ADDR 		( CMU_VBASE + IMGEHC_CMU_LOCAL_SVGAIN_REG2_OFF  )		// CMU saturation and brightness local gain register
#define IMGEHC_CMU_LOCAL_SVGAIN_REG3_ADDR 		( CMU_VBASE + IMGEHC_CMU_LOCAL_SVGAIN_REG3_OFF  )		// CMU saturation and brightness local gain register
#define IMGEHC_CMU_LOCAL_SVGAIN_REG4_ADDR 		( CMU_VBASE + IMGEHC_CMU_LOCAL_SVGAIN_REG4_OFF  )		// CMU saturation and brightness local gain register
#define IMGEHC_CMU_LOCAL_SVGAIN_REG5_ADDR 		( CMU_VBASE + IMGEHC_CMU_LOCAL_SVGAIN_REG5_OFF  )		// CMU saturation and brightness local gain register
#define IMGEHC_CMU_LOCAL_SVGAIN_REG6_ADDR 		( CMU_VBASE + IMGEHC_CMU_LOCAL_SVGAIN_REG6_OFF  )		// CMU saturation and brightness local gain register
#define IMGEHC_CMU_GLOBAL_SVGAIN_REG_ADDR 		( CMU_VBASE + IMGEHC_CMU_GLOBAL_SVGAIN_REG_OFF  )		// CMU saturation and brightness global gain register

// 
// Detail information of registers
//

typedef union
{
	u32 dwval;
	struct
	{
		u32 cmu_en          :  1 ;    // Default: 0; 
		u32 res0            :  1 ;    // Default: ; 
		u32 owinen          :  1 ;    // Default: 0; 
		u32 res1            :  5 ;    // Default: ; 
		u32 res2            :  1 ;    // Default: ; 
		u32 hwerrirq_en     :  1 ;    // Default: 0; 
		u32 res3            :  6 ;    // Default: ; 
		u32 order           :  2 ;    // Default: 0; 
		u32 res4            : 14 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_CTL_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 res0            :  8 ;    // Default: ; 
		u32 hwerrirq_flag   :  1 ;    // Default: 0; 
		u32 res1            : 23 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_STS_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 cmu_regloadctl  :  1 ;    // Default: 0; 
		u32 cmu_regautoload :  1 ;    // Default: 0; 
		u32 res0            : 30 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_REGBUFFCTL_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 cmu_inwidth     : 13 ;    // Default: 0; 
		u32 res0            :  3 ;    // Default: ; 
		u32 cmu_inheight    : 13 ;    // Default: 0; 
		u32 res1            :  3 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_INSIZE_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 cmu_win_left    : 13 ;    // Default: 0; 
		u32 res0            :  3 ;    // Default: ; 
		u32 cmu_win_top     : 13 ;    // Default: 0; 
		u32 res1            :  3 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_OWP_REG0_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 cmu_win_right   : 13 ;    // Default: 0; 
		u32 res0            :  3 ;    // Default: ; 
		u32 cmu_win_bot     : 13 ;    // Default: 0; 
		u32 res1            :  3 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_OWP_REG1_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hmin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 hmax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_HRANGE_REG0_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hmin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 hmax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_HRANGE_REG1_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hmin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 hmax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_HRANGE_REG2_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hmin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 hmax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_HRANGE_REG3_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hmin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 hmax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_HRANGE_REG4_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hmin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 hmax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_HRANGE_REG5_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hmin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 hmax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_HRANGE_REG6_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hgain_l         : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 vmin            :  8 ;    // Default: 0; 
		u32 vmax            :  8 ;    // Default: 0; 
	} bits;
} IMGEHC_CMU_VRANGE_HLGAIN_REG0_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hgain_l         : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 vmin            :  8 ;    // Default: 0; 
		u32 vmax            :  8 ;    // Default: 0; 
	} bits;
} IMGEHC_CMU_VRANGE_HLGAIN_REG1_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hgain_l         : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 vmin            :  8 ;    // Default: 0; 
		u32 vmax            :  8 ;    // Default: 0; 
	} bits;
} IMGEHC_CMU_VRANGE_HLGAIN_REG2_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hgain_l         : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 vmin            :  8 ;    // Default: 0; 
		u32 vmax            :  8 ;    // Default: 0; 
	} bits;
} IMGEHC_CMU_VRANGE_HLGAIN_REG3_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hgain_l         : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 vmin            :  8 ;    // Default: 0; 
		u32 vmax            :  8 ;    // Default: 0; 
	} bits;
} IMGEHC_CMU_VRANGE_HLGAIN_REG4_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hgain_l         : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 vmin            :  8 ;    // Default: 0; 
		u32 vmax            :  8 ;    // Default: 0; 
	} bits;
} IMGEHC_CMU_VRANGE_HLGAIN_REG5_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hgain_l         : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 vmin            :  8 ;    // Default: 0; 
		u32 vmax            :  8 ;    // Default: 0; 
	} bits;
} IMGEHC_CMU_VRANGE_HLGAIN_REG6_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 hgain_g         : 12 ;    // Default: 0; 
		u32 res0            : 20 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_HGGAIN_REG_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 smin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 smax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_SRANGE_REG0_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 smin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 smax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_SRANGE_REG1_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 smin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 smax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_SRANGE_REG2_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 smin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 smax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_SRANGE_REG3_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 smin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 smax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_SRANGE_REG4_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 smin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 smax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_SRANGE_REG5_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 smin            : 12 ;    // Default: 0; 
		u32 res0            :  4 ;    // Default: ; 
		u32 smax            : 12 ;    // Default: 0; 
		u32 res1            :  4 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_SRANGE_REG6_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 vgain_l         :  9 ;    // Default: 0; 
		u32 res0            :  7 ;    // Default: ; 
		u32 sgain_l         : 13 ;    // Default: 0; 
		u32 res1            :  3 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_LOCAL_SVGAIN_REG0_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 vgain_l         :  9 ;    // Default: 0; 
		u32 res0            :  7 ;    // Default: ; 
		u32 sgain_l         : 13 ;    // Default: 0; 
		u32 res1            :  3 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_LOCAL_SVGAIN_REG1_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 vgain_l         :  9 ;    // Default: 0; 
		u32 res0            :  7 ;    // Default: ; 
		u32 sgain_l         : 13 ;    // Default: 0; 
		u32 res1            :  3 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_LOCAL_SVGAIN_REG2_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 vgain_l         :  9 ;    // Default: 0; 
		u32 res0            :  7 ;    // Default: ; 
		u32 sgain_l         : 13 ;    // Default: 0; 
		u32 res1            :  3 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_LOCAL_SVGAIN_REG3_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 vgain_l         :  9 ;    // Default: 0; 
		u32 res0            :  7 ;    // Default: ; 
		u32 sgain_l         : 13 ;    // Default: 0; 
		u32 res1            :  3 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_LOCAL_SVGAIN_REG4_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 vgain_l         :  9 ;    // Default: 0; 
		u32 res0            :  7 ;    // Default: ; 
		u32 sgain_l         : 13 ;    // Default: 0; 
		u32 res1            :  3 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_LOCAL_SVGAIN_REG5_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 vgain_l         :  9 ;    // Default: 0; 
		u32 res0            :  7 ;    // Default: ; 
		u32 sgain_l         : 13 ;    // Default: 0; 
		u32 res1            :  3 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_LOCAL_SVGAIN_REG6_t;

typedef union
{
	u32 dwval;
	struct
	{
		u32 vgain_g         :  9 ;    // Default: 0; 
		u32 res0            :  7 ;    // Default: ; 
		u32 sgain_g         : 13 ;    // Default: 0; 
		u32 res1            :  3 ;    // Default: ; 
	} bits;
} IMGEHC_CMU_GLOBAL_SVGAIN_REG_t;

extern __u32 cmu_reg_base[2];
#define CMU_GET_REG_BASE(sel)	(cmu_reg_base[sel])

#define get_bvalue(n)	(*((volatile __u8 *)(n)))			/* byte input */
#define put_bvalue(n,c)	(*((volatile __u8 *)(n)) = (c))		/* byte output */
#define get_hvalue(n)	(*((volatile __u16 *)(n)))			/* half word input */
#define put_hvalue(n,c)	(*((volatile __u16 *)(n)) = (c))	/* half word output */
#define get_wvalue(n)	(*((volatile __u32 *)(n)))			/* word input */
#define put_wvalue(n,c)	(*((volatile __u32 *)(n)) = (c))	/* word output */

#define CMU_RUINT8(sel,offset)				get_bvalue(CMU_GET_REG_BASE(sel)+(offset))
#define CMU_WUINT8(sel,offset,value)		put_bvalue(CMU_GET_REG_BASE(sel)+(offset),value)  
#define CMU_RUINT16(sel,offset)				get_hvalue(CMU_GET_REG_BASE(sel)+(offset))
#define CMU_WUINT16(sel,offset,value)		put_hvalue(CMU_GET_REG_BASE(sel)+(offset),value)
#define CMU_RUINT32(sel,offset)				get_wvalue(CMU_GET_REG_BASE(sel)+(offset))
#define CMU_WUINT32(sel,offset,value)		put_wvalue(CMU_GET_REG_BASE(sel)+(offset),value)

__u32 IEP_CMU_Set_Reg_Base(__u32 sel, __u32 address);
__u32 IEP_CMU_Reg_Init(__u32 sel);
__s32 IEP_CMU_Enable(__u32 sel, __u32 enable);
__s32 IEP_CMU_Disable(__u32 sel);
__s32 IEP_CMU_Set_Data_Flow(__u8 sel, __u32 data_flow);
__s32 IEP_CMU_Set_Window(__u8 sel, __disp_rect_t *window);
__s32 IEP_CMU_Set_Imgsize(__u8 sel, __u32 width, __u32 height);
__s32 IEP_CMU_Set_Par(__u8 sel, __u32 hue, __u32 saturaion, __u32 brightness, __u32 mode);
__s32 IEP_CMU_Operation_In_Vblanking(__u32 sel);
__s32 iep_cmu_early_suspend(__u32 sel);
__s32 iep_cmu_suspend(__u32 sel);
__s32 iep_cmu_resume(__u32 sel);
__s32 iep_cmu_late_resume(__u32 sel);


//------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------

#ifdef __cplusplus
}
#endif

//------------------------------------------------------------------------------------------------------------
// 
//------------------------------------------------------------------------------------------------------------

#endif // __REG33__CMU__H__
