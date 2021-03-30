#ifndef _MCTL_REG_H
#define _MCTL_REG_H

/*=====================================
 *DRAM驱动专用读写函数
 * ====================================
 */
#define mctl_write_w(v, addr)	(*((volatile unsigned long  *)(addr)) = (unsigned long)(v))
#define mctl_read_w(addr)		(*((volatile unsigned long  *)(addr)))
#define __REG(x)                (*(volatile unsigned int   *)(x))

#define DRAM_RET_OK	0
#define DRAM_RET_FAIL	1

#define DRAM_BASE_ADDR	    0x40000000
#define DRAM_TYPE_ADDR      0x01c6280c
/*该地址用于time out功能验证,bit17 disable high rank, bit16 disable high DQ*/
#define DRAM_SDL_ADDR       0x01c62820
/*该地址用于SDL功能验证
0x820[0]   写1触发
0x820[15:8] 编号：
0x00-0x1F ：DQ0 -DQ31  wsdl
0x20-0x3F ：DQ0 -DQ31  rsdl

0x40-0x43 ：DQS0-DQS3  wsdl
0x44-0x47 ：DQS0-DQS3  rsdl
0x48-0x4b ：DQS0#-DQS3#  rsdl
0x4c-0x4f ：DM0-DM3    wsdl
*/

#define MCTL_COM_BASE		0xf1c62000
#define MCTL_CTL_BASE		0xf1c63000

/*=====================================
 *MSI域寄存器
 *0x01c62000
 * ====================================
 */
#define MC_WORK_MODE			(MCTL_COM_BASE + 0x00)
#define MC_R1_WORK_MODE         (MCTL_COM_BASE + 0x04)
#define PSWMCR                  (MCTL_COM_BASE + 0x04)

#define MC_DBGCR			(MCTL_COM_BASE + 0x08)
#define MC_TMR				(MCTL_COM_BASE + 0x0c)
#define MC_MnCR0(x)             (MCTL_COM_BASE + 0x10 + 0x8 * x)
#define MC_MnCR1(x)             (MCTL_COM_BASE + 0x14 + 0x8 * x)

#define MC_BWCR                 (MCTL_COM_BASE + 0x90)
#define MC_MAER                 (MCTL_COM_BASE + 0x94)
#define MC_MPFADR               (MCTL_COM_BASE + 0x98)
#define MC_MCGCR                (MCTL_COM_BASE + 0x9c)

#define MC_CPU_BWCR             (MCTL_COM_BASE + 0xa0)
#define MC_DISP_BWCR            (MCTL_COM_BASE + 0xac)
#define MC_OTHER_BWCR           (MCTL_COM_BASE + 0xb0)
#define MC_TOTAL_BWCR           (MCTL_COM_BASE + 0xb4)
#define MC_CSI_BWCR             (MCTL_COM_BASE + 0xb8)

#define MC_SWONR				(MCTL_COM_BASE + 0xc0)
#define MC_SWOFFR				(MCTL_COM_BASE + 0xc4)
#define MC_CCCR                 (MCTL_COM_BASE + 0xd0)
#define MC_MDFSCR				(MCTL_COM_BASE + 0x100)
#define MC_MDFSMER				(MCTL_COM_BASE + 0x104)
#define MC_MDFSMRMR				(MCTL_COM_BASE + 0x108)
#define MC_MDFSTR				(MCTL_COM_BASE + 0x10c)
#define MC_MDFSCNTR				(MCTL_COM_BASE + 0x110)
#define MC_MDFS_IRQ_STA0		(MCTL_COM_BASE + 0x114)
#define MC_MDFS_IRQ_STA1		(MCTL_COM_BASE + 0x118)
#define MC_MDFS_IRQ_MASK0		(MCTL_COM_BASE + 0x11c)
#define MC_MDFS_IRQ_MASK1		(MCTL_COM_BASE + 0x120)
#define MC_MDFS_BWC_PRD			(MCTL_COM_BASE + 0x124)
#define MC_MDFS_CPU_BWLR		(MCTL_COM_BASE + 0x128)

#define MC_MDFS_GPU_BWLR		(MCTL_COM_BASE + 0x12c)
#define MC_MDFS_ROT_BWLR		(MCTL_COM_BASE + 0x130)

#define MC_MDFS_MER			(MCTL_COM_BASE + 0x134)
#define MC_MDFS_MSR			(MCTL_COM_BASE + 0x138)
/*protect register*/
#define MC_MPRAR				(MCTL_COM_BASE + 0x800)

/*=====================================
 *PHY域寄存器
 *0x01c63000
 * ====================================
 */
#define PIR			(MCTL_CTL_BASE + 0x00000000)
#define PWRCTL			(MCTL_CTL_BASE + 0x00000004)
#define MRCTRL0			(MCTL_CTL_BASE + 0x00000008)
#define CLKEN			    (MCTL_CTL_BASE + 0x0000000c)
#define PGSR0			(MCTL_CTL_BASE + 0x00000010)
#define PGSR1			(MCTL_CTL_BASE + 0x00000014)
#define STATR			(MCTL_CTL_BASE + 0x00000018)
#define LP3MR11             (MCTL_CTL_BASE + 0x0000002c)
#define DRAM_MR0		(MCTL_CTL_BASE + 0x00000030)
#define DRAM_MR1		(MCTL_CTL_BASE + 0x00000034)
#define DRAM_MR2		(MCTL_CTL_BASE + 0x00000038)
#define DRAM_MR3		(MCTL_CTL_BASE + 0x0000003c)
#define PTR0			(MCTL_CTL_BASE + 0x00000044)
#define PTR2			(MCTL_CTL_BASE + 0x0000004c)
#define PTR3			(MCTL_CTL_BASE + 0x00000050)
#define PTR4			(MCTL_CTL_BASE + 0x00000054)
#define DRAMTMG0		(MCTL_CTL_BASE + 0x00000058)
#define DRAMTMG1		(MCTL_CTL_BASE + 0x0000005c)
#define DRAMTMG2		(MCTL_CTL_BASE + 0x00000060)
#define DRAMTMG3		(MCTL_CTL_BASE + 0x00000064)
#define DRAMTMG4		(MCTL_CTL_BASE + 0x00000068)
#define DRAMTMG5		(MCTL_CTL_BASE + 0x0000006c)
#define DRAMTMG6		(MCTL_CTL_BASE + 0x00000070)
#define DRAMTMG7		(MCTL_CTL_BASE + 0x00000074)
#define DRAMTMG8		(MCTL_CTL_BASE + 0x00000078)
#define ODTCFG			(MCTL_CTL_BASE + 0x0000007c)
#define PITMG0			(MCTL_CTL_BASE + 0x00000080)
#define PITMG1			(MCTL_CTL_BASE + 0x00000084)
#define LPTPR			(MCTL_CTL_BASE + 0x00000088)
#define RFSHCTL0		(MCTL_CTL_BASE + 0x0000008c)
#define RFSHTMG			(MCTL_CTL_BASE + 0x00000090)
#define RFSHCTL1		(MCTL_CTL_BASE + 0x00000094)
#define PWRTMG			(MCTL_CTL_BASE + 0x00000098)
#define VTFCR			(MCTL_CTL_BASE + 0x000000b8)
#define DQSGMR				(MCTL_CTL_BASE + 0x000000bc)
#define DTCR			(MCTL_CTL_BASE + 0x000000c0)
#define DTAR0			(MCTL_CTL_BASE + 0x000000c4)
#define PGCR0			(MCTL_CTL_BASE + 0x00000100)
#define PGCR1			(MCTL_CTL_BASE + 0x00000104)
#define PGCR2			(MCTL_CTL_BASE + 0x00000108)
#define PGCR3			(MCTL_CTL_BASE + 0x0000010c)
#define DXCCR			(MCTL_CTL_BASE + 0x0000011c)
#define ODTMAP			(MCTL_CTL_BASE + 0x00000120)
#define ZQCTL0			(MCTL_CTL_BASE + 0x00000124)
#define ZQCTL1			(MCTL_CTL_BASE + 0x00000128)
#define ZQCR			(MCTL_CTL_BASE + 0x00000140)
#define ZQSR			(MCTL_CTL_BASE + 0x00000144)
#define ZQDR0			(MCTL_CTL_BASE + 0x00000148)
#define ZQDR1			(MCTL_CTL_BASE + 0x0000014c)
#define ZQDR2			(MCTL_CTL_BASE + 0x00000150)
#define SCHED			(MCTL_CTL_BASE + 0x000001c0)
#define PERFHPR0		(MCTL_CTL_BASE + 0x000001c4)
#define PERFHPR1		(MCTL_CTL_BASE + 0x000001c8)
#define PERFLPR0		(MCTL_CTL_BASE + 0x000001cc)
#define PERFLPR1		(MCTL_CTL_BASE + 0x000001d0)
#define PERFWR0			(MCTL_CTL_BASE + 0x000001d4)
#define PERFWR1			(MCTL_CTL_BASE + 0x000001d8)
#define ACMDLR			(MCTL_CTL_BASE + 0x00000200)
#define ACLDLR			(MCTL_CTL_BASE + 0x00000204)
#define ACIOCR0             (MCTL_CTL_BASE + 0x00000208)
#define ACIOCR1(x)          (MCTL_CTL_BASE + 0x00000210 + 0x4 * (x))
#define DXnMDLR(x)			(MCTL_CTL_BASE + 0x00000300 + 0x80 * x)
#define DXnLDLR0(x)		    (MCTL_CTL_BASE + 0x00000304 + 0x80 * x)
#define DXnLDLR1(x)		    (MCTL_CTL_BASE + 0x00000308 + 0x80 * x)
#define DXnLDLR2(x)		    (MCTL_CTL_BASE + 0x0000030c + 0x80 * x)
#define DXIOCR              (MCTL_CTL_BASE + 0x00000310)
#define DATX0IOCR(x)        (MCTL_CTL_BASE + 0x00000310 + 0x4 * (x))
#define DATX1IOCR(x)        (MCTL_CTL_BASE + 0x00000390 + 0x4 * (x))
#define DATX2IOCR(x)        (MCTL_CTL_BASE + 0x00000410 + 0x4 * (x))
#define DATX3IOCR(x)        (MCTL_CTL_BASE + 0x00000490 + 0x4 * (x))
#define DXnSDLR6(x)			(MCTL_CTL_BASE + 0x0000033c + 0x80 * x)
#define DXnGTR(x)			(MCTL_CTL_BASE + 0x00000340 + 0x80 * x)
#define DXnGCR0(x)			(MCTL_CTL_BASE + 0x00000344 + 0x80 * x)
#define DXnGSR0(x)			(MCTL_CTL_BASE + 0x00000348 + 0x80 * x)

/*=====================================
 *PROTECT域寄存器
 *0x01c63800
 * ====================================
 */
#define UPD0                (MCTL_CTL_BASE + 0x00000880)
#define UPD1                (MCTL_CTL_BASE + 0x00000884)
#define UPD2                (MCTL_CTL_BASE + 0x00000888)
#define UPD3                (MCTL_CTL_BASE + 0x0000088c)
#define PLPCFG0             (MCTL_CTL_BASE + 0x00000890)
/*=====================================
 *MMCU域寄存器--系统资源
 *0x01c20000
 * ====================================
 */
#define _CCM_BASE				     0xf1c20000
#define _CCM_PLL_DDR0_REG			(_CCM_BASE+0x20)
#define _CCM_PLL_DDR1_REG           (_CCM_BASE+0x4c)
#define _CCM_PLL_DDR_AUX_REG        (_CCM_BASE+0xf0)
#define _CCM_DRAMCLK_CFG_REG		(_CCM_BASE+0xf4)
#define _CCM_PLL_DDR1_CFG_REG		(_CCM_BASE+0xf8)
#define MBUS_RESET_REG              (_CCM_BASE+0xfc)
#define MBUS_CLK_CTL_REG            (_CCM_BASE+0x15c)
#define BUS_CLK_GATE_REG0		    (_CCM_BASE+0x060)

#define PLL_DDR0_PAT_CTL_REG		(_CCM_BASE+0x290)
#define PLL_DDR1_PAT_CTL_REG		(_CCM_BASE+0x2ac)

#define BUS_RST_REG0			    (_CCM_BASE+0x2c0)
#define CCU_PLL_LOCK_CTRL_REG       (_CCM_BASE+0x320)
/*=====================================
 *PAD HOLD寄存器
 * ====================================
 */
#define VDD_SYS_PWROFF_GATING        (0xf1c20400 + 0x1f4)
#endif
