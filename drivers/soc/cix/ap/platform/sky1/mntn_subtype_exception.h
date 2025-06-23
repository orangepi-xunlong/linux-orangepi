#ifndef __MNTN_SUBTYPE_EXCEPTION_H__
#define __MNTN_SUBTYPE_EXCEPTION_H__

//This area store the exception info from SE and PM
#define RDR_SE_EXCEPTION_AREA     (0x83DA3E00)
#define RDR_PM_EXCEPTION_AREA     (0x83DBBE00)
#define RDR_SE_EXCEPTION_AREA_LEN (0x200)
#define RDR_PM_EXCEPTION_AREA_LEN (0x200)

enum appanic_subtype
{
	HI_APPANIC_RESERVED = 0x0,
	HI_APPANIC_BC_PANIC = 0x1,
	HI_APPANIC_L3CACHE_ECC1 = 0x2,
	HI_APPANIC_SOFTLOCKUP = 0x3,
	HI_APPANIC_OHARDLOCKUP = 0x4,
	HI_APPANIC_HARDLOCKUP = 0x5,
	HI_APPANIC_L3CACHE_ECC2 = 0x6,
	HI_APPANIC_Storage = 0x7,
	HI_APPANIC_ISP = 0x9,
	HI_APPANIC_IVP = 0xa,
	HI_APPANIC_GPU = 0xc,
	HI_APPANIC_MODEM = 0xd,
	HI_APPANIC_CPU0_L1_CE = 0x10,
	HI_APPANIC_CPU1_L1_CE = 0x11,
	HI_APPANIC_CPU2_L1_CE = 0x12,
	HI_APPANIC_CPU3_L1_CE = 0x13,
	HI_APPANIC_CPU4_L1_CE = 0x14,
	HI_APPANIC_CPU5_L1_CE = 0x15,
	HI_APPANIC_CPU6_L1_CE = 0x16,
	HI_APPANIC_CPU7_L1_CE = 0x17,
	HI_APPANIC_CPU0_L2_CE = 0x18,
	HI_APPANIC_CPU1_L2_CE = 0x19,
	HI_APPANIC_CPU2_L2_CE = 0x1a,
	HI_APPANIC_CPU3_L2_CE = 0x1b,
	HI_APPANIC_CPU4_L2_CE = 0x1c,
	HI_APPANIC_CPU5_L2_CE = 0x1d,
	HI_APPANIC_CPU6_L2_CE = 0x1e,
	HI_APPANIC_CPU7_L2_CE = 0x1f,
	HI_APPANIC_CPU0_L1_UE = 0x20,
	HI_APPANIC_CPU1_L1_UE = 0x21,
	HI_APPANIC_CPU2_L1_UE = 0x22,
	HI_APPANIC_CPU3_L1_UE = 0x23,
	HI_APPANIC_CPU4_L1_UE = 0x24,
	HI_APPANIC_CPU5_L1_UE = 0x25,
	HI_APPANIC_CPU6_L1_UE = 0x26,
	HI_APPANIC_CPU7_L1_UE = 0x27,
	HI_APPANIC_CPU0_L2_UE = 0x28,
	HI_APPANIC_CPU1_L2_UE = 0x29,
	HI_APPANIC_CPU2_L2_UE = 0x2a,
	HI_APPANIC_CPU3_L2_UE = 0x2b,
	HI_APPANIC_CPU4_L2_UE = 0x2c,
	HI_APPANIC_CPU5_L2_UE = 0x2d,
	HI_APPANIC_CPU6_L2_UE = 0x2e,
	HI_APPANIC_CPU7_L2_UE = 0x2f,
	HI_APPANIC_L3_CE = 0x30,
	HI_APPANIC_L3_UE = 0x31,
	HI_APPANIC_LB = 0x32,
	HI_APPANIC_PLL_UNLOCK = 0x33,
	HI_APPANIC_CSI = 0x34,
	HI_APPANIC_CSIDMA = 0x35,
};

enum apwdt_subtype
{
	HI_APWDT_HW = 0x0,
	HI_APWDT_LPM3 = 0x1,
	HI_APWDT_BL31 = 0x2,
	HI_APWDT_BL31LPM3 = 0x3,
	HI_APWDT_AP = 0x4,
	HI_APWDT_BL31AP = 0x6,
	HI_APWDT_APBL31LPM3 = 0x7,
};

enum npu_subtype
{
	AICORE_EXCP = 0x0,
	AICORE_TIMEOUT,
	TS_RUNNING_EXCP,
	TS_RUNNING_TIMEOUT,
	TS_INIT_EXCP,
	AICPU_INIT_EXCP,
	AICPU_HEARTBEAT_EXCP,
	POWERUP_FAIL,
	POWERDOWN_FAIL,
	NPU_NOC_ERR,
	SMMU_EXCP,
	HWTS_EXCP,
};

typedef enum {
	RDR_REG_BACKUP_IDEX_0 = 0,
	RDR_REG_BACKUP_IDEX_1,
	RDR_REG_BACKUP_IDEX_2,
	RDR_REG_BACKUP_IDEX_3,
	RDR_REG_BACKUP_IDEX_MAX
} RDR_REG_BACKUP_IDEX;

typedef enum {
	CATEGORY_START = 0x0,
	NORMALBOOT,
	PANIC,
	HWWATCHDOG,
	LPM3EXCEPTION,
	BOOTLOADER_CRASH,
	TRUSTZONE_REBOOTSYS,
	MODEM_REBOOTSYS,
	BOOTFAIL,
	HARDWARE_FAULT,
	MODEMCRASH,
	HIFICRASH,
	AUDIO_CODEC_CRASH,
	SENSORHUBCRASH,
	ISPCRASH,
	CSICRASH,
	CSIDMACRASH,
	IVPCRASH,
	TRUSTZONECRASH,
	HISEECRASH,
	UNKNOWNS,
	PRESS10S,
	PRESS6S,
	NPUEXCEPTION,
	CONNEXCEPTION,
	FDULCRASH,
	DSSCRASH,
	SUBTYPE = 0xff,
}CATEGORY_SOURCE;

struct exp_subtype {
	unsigned int exception;
	unsigned char category_name[24];
	unsigned char subtype_name[24];
	unsigned int subtype_num;
};

typedef struct exc_special_backup_data {
	unsigned char reset_reason[RDR_REG_BACKUP_IDEX_MAX];
	unsigned int slice;
	unsigned int rtc;
	unsigned int REG_Reg13;
	unsigned int REG_LR1;
	unsigned int REG_PC;
	unsigned int REG_XPSR;
	unsigned int NVIC_CFSR;
	unsigned int NVIC_HFSR;
	unsigned int NVIC_BFAR;
	unsigned char exc_trace;
	unsigned char ddr_exc;
	unsigned short irq_id;
	unsigned int task_id;
} EXC_SPECIAL_BACKUP_DATA_STRU;

typedef struct rdr_reg_backup_head {
	unsigned int idex;
	unsigned int reason[RDR_REG_BACKUP_IDEX_MAX - 1];
} RDR_REG_BACKUP_HEAD_STRU;

typedef struct rdr_reg_backup_data {
	unsigned int Reg0;
	unsigned int Reg1;
	unsigned int Reg2;
	unsigned int Reg3;
	unsigned int Reg4;
	unsigned int Reg5;
	unsigned int Reg6;
	unsigned int Reg7;
	unsigned int Reg8;
	unsigned int Reg9;
	unsigned int Reg10;
	unsigned int Reg11;
	unsigned int Reg12;
	unsigned int Reg13;
	unsigned int MSP;
	unsigned int PSP;
	unsigned int LR0_CONTROL;
	unsigned int LR1;
	unsigned int PC;
	unsigned int XPSR;
	unsigned int PRIMASK;
	unsigned int BASEPRI;
	unsigned int FAULTMASK;
	unsigned int CONTROL;
} RDR_REG_BACKUP_DATA_STRU;
#endif
