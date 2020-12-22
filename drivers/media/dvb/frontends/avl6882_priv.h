#ifndef AVL6882_PRIV_H
#define AVL6882_PRIV_H

#include "dvb_frontend.h"


#define MAX_CHANNEL_INFO 256

typedef struct s_DVBTx_Channel_TS
{
    // number, example 474*1000 is RF frequency 474MHz.
    int channel_freq_khz;
    // number, example 8000 is 8MHz bandwith channel.
    int channel_bandwith_khz;

    u8 channel_type;
    // 0 - Low priority layer, 1 - High priority layer
    u8 dvbt_hierarchy_layer;
    // data PLP id, 0 to 255; for single PLP DVBT2 channel, this ID is 0; for DVBT channel, this ID isn't used.
    u8 data_plp_id;
    u8 channel_profile;
}s_DVBTx_Channel_TS;



struct avl6882_priv {
	struct i2c_adapter *i2c;
 	struct avl6882_config *config;
	struct dvb_frontend frontend;
	enum fe_delivery_system delivery_system;

	/* DVB-Tx */
	u16 g_nChannel_ts_total;
	s_DVBTx_Channel_TS global_channel_ts_table[MAX_CHANNEL_INFO];
};





typedef  int AVL_int32;     ///< 32 bits signed char data type.
typedef  unsigned char * AVL_puchar; ///< pointer to a 8 bits unsigned char data type.
typedef  unsigned short * AVL_puint16;  ///< pointer to a 16 bits unsigned char data type.
typedef  unsigned int * AVL_puint32; ///< pointer to a 32 bits unsigned char data type.
typedef unsigned char AVL_semaphore;    ///< the semaphore data type.






/* known registers */

/* TS */
#define REG_TS_OUTPUT			(0x130420)
#define TS_OUTPUT_ENABLE		(0)
#define TS_OUTPUT_DISABLE		(0xfff)


/* GPIO */
#define REG_GPIO_BASE			(0x120000)

#define GPIO_AGC_DVBTC			(0x00)	/* agc1_sel*/
#define GPIO_AGC_DVBS			(0x10)	/* agc2_sel*/
#define GPIO_LNB_VOLTAGE		(0x08)	/* pin 38 - lnb_ctrl_1_sel */
#define GPIO_LNB_PWR			(0x0c)	/* pin 37 - lnb_ctrl_0_sel */

#define GPIO_RD_MASK			(0x40)

#define GPIO_0				(0)
#define GPIO_1				(1)
#define GPIO_Z				(2)
#define GPIO_AGC_ON			(6)



//GPIO control
#define lnb_cntrl_1_sel_offset                   0x08 
#define lnb_cntrl_0_sel_offset                   0x0c 

#define lnb_cntrl_1_i_offset                     0x48
#define lnb_cntrl_0_i_offset                     0x4c


typedef enum AVL_GPIOPinNumber
{
    AVL_Pin37 = 0,
    AVL_Pin38 = 1
}AVL_GPIOPinNumber;















#define MAX_II2C_READ_SIZE  64
#define MAX_II2C_WRITE_SIZE 64


//SDK Version
#define AVL_API_VER_MAJOR                   0x02
#define AVL_API_VER_MINOR                   0x03
#define AVL_API_VER_BUILD                   0x02


#define AVL68XX                            0x68624955

#define AVL_FW_CMD_IDLE                          0
#define AVL_FW_CMD_LD_DEFAULT                    1
#define AVL_FW_CMD_ACQUIRE                       2
#define AVL_FW_CMD_HALT                          3 
#define AVL_FW_CMD_DEBUG                         4
#define AVL_FW_CMD_SLEEP                         7
#define AVL_FW_CMD_WAKE                          8
#define AVL_FW_CMD_BLIND_SCAN                    9
#define AVL_FW_CMD_SDRAM_TEST                   16
#define AVL_FW_CMD_INIT_SDRAM                   17
#define AVL_FW_CMD_INIT_ADC                     18
#define AVL_FW_CMD_CHANGE_MODE                  19

#define AVL_FW_CMD_DMA                          21
#define AVL_FW_CMD_CALC_CRC                     22
#define AVL_FW_CMD_PING                         23
#define AVL_FW_CMD_DECOMPRESS                   24



/*
 * Patch file stuff
 */
#define PATCH_VAR_ARRAY_SIZE                32

#define PATCH_CMD_VALIDATE_CRC              0
#define PATCH_CMD_PING                      1 
#define PATCH_CMD_LD_TO_DEVICE              2 
#define PATCH_CMD_DMA                       3 
#define PATCH_CMD_DECOMPRESS                4
#define PATCH_CMD_ASSERT_CPU_RESET          5 
#define PATCH_CMD_RELEASE_CPU_RESET         6
#define PATCH_CMD_LD_TO_DEVICE_IMM          7
#define PATCH_CMD_RD_FROM_DEVICE            8
#define PATCH_CMD_DMA_HW                    9
#define PATCH_CMD_SET_COND_IMM              10
#define PATCH_CMD_EXIT                      11


#define PATCH_CMP_TYPE_ZLIB                 0
#define PATCH_CMP_TYPE_ZLIB_NULL            1
#define PATCH_CMP_TYPE_GLIB                 2
#define PATCH_CMP_TYPE_NONE                 3

//Addr modes 2 bits
#define PATCH_OP_ADDR_MODE_VAR_IDX          0
#define PATCH_OP_ADDR_MODE_IMMIDIATE        1

//Unary operators 6 bits
#define PATCH_OP_UNARY_NOP                  0
#define PATCH_OP_UNARY_LOGICAL_NEGATE       1
#define PATCH_OP_UNARY_BITWISE_NEGATE       2
#define PATCH_OP_UNARY_BITWISE_AND          3
#define PATCH_OP_UNARY_BITWISE_OR           4

//Binary operators 1 Byte
#define PATCH_OP_BINARY_LOAD                0
#define PATCH_OP_BINARY_AND                 1
#define PATCH_OP_BINARY_OR                  2
#define PATCH_OP_BINARY_BITWISE_AND         3
#define PATCH_OP_BINARY_BITWISE_OR          4
#define PATCH_OP_BINARY_EQUALS              5
#define PATCH_OP_BINARY_STORE               6
#define PATCH_OP_BINARY_NOT_EQUALS          7

#define PATCH_COND_EXIT_AFTER_LD            8

#define PATCH_STD_DVBC                      0 
#define PATCH_STD_DVBSx                     1
#define PATCH_STD_DVBTx                     2
#define PATCH_STD_ISDBT                     3



#define tuner_i2c_srst_offset                0x0
#define tuner_i2c_cntrl_offset               0x4
#define tuner_i2c_bit_rpt_clk_div_offset     0x18
#define tuner_i2c_bit_rpt_cntrl_offset       0x1C

#define esm_cntrl_offset                    0x4
#define bit_error_offset                    0x8
#define byte_num_offset                     0xC
#define packet_error_offset                 0x10
#define packet_num_offset                   0x14
#define tick_clear_offset                   0x88
#define tick_type_offset                    0x8C
#define time_tick_low_offset                0x90
#define time_tick_high_offset               0x94
#define byte_tick_low_offset                0x98
#define byte_tick_high_offset               0x9C
#define esm_mode_offset                     0xC0

#define rs_current_active_mode_iaddr_offset 0x24
#define rc_fw_command_saddr_offset          0x00
#define rs_core_ready_word_iaddr_offset     0xa0
#define rc_sdram_test_return_iaddr_offset   0x3C
#define rc_sdram_test_result_iaddr_offset   0x40
#define rs_rf_agc_saddr_offset              0x44

#define rc_fw_command_args_addr_iaddr_offset 0x58

#define rc_ts_cntns_clk_frac_d_iaddr_offset                 0x0000007c
#define rc_ts_cntns_clk_frac_n_iaddr_offset                 0x00000078
#define rc_enable_ts_continuous_caddr_offset                0x0000003a
#define rc_ts_clock_edge_caddr_offset                       0x0000003b
#define rc_ts_serial_caddr_offset                           0x0000003c
#define rc_ts_serial_outpin_caddr_offset                    0x0000003f
#define rc_ts_serial_msb_caddr_offset                       0x0000003e
#define rc_ts_packet_len_caddr_offset                       0x00000039
#define rc_ts_packet_order_caddr_offset                     rc_ts_serial_msb_caddr_offset
#define rc_ts_error_bit_en_caddr_offset                     0x000000000
#define rc_ts_error_polarity_caddr_offset                   0x00000041
#define rc_ts_valid_polarity_caddr_offset                   0x00000040
#define rc_ts_sync_pulse_caddr_offset                       0x00000097
#define ts_clock_phase_caddr_offset                         0x00000096

#define rs_patch_ver_iaddr_offset           0x00000004




#define hw_AVL_rx_rf_aagc_gain              0x160888


//Define ADC channel selection
typedef enum AVL_ADC_Channel
{
    AVL_ADC_CHAN2   =   0,
    AVL_ADC_CHAN1   =   1,
    AVL_ADC_OFF     =   2
}AVL_ADC_Channel;

typedef enum AVL_ADC_Output_format
{
    AVL_2COMP    =   0,
    AVL_OFFBIN   =   1
}AVL_ADC_Output_format;

//Input_select enumeration definitions
typedef enum AVL_DDC_Input
{
    AVL_DIG_IN       =   0,
    AVL_ADC_IN       =   1,
    AVL_VEC_IN       =   2,
    AVL_VEC1x_IN     =   3,
    AVL_DIG1x_IN     =   4
}AVL_DDC_Input;

// Defines BER type
typedef enum AVL_BER_Type
{
    AVL_PRE_VITERBI_BER     =   0,                      // previous viterbi BER will be acquired.
    AVL_POST_VITERBI_BER    =   1,                      // post viterbi BER will be acquired.
    AVL_PRE_LDPC_BER        =   2,                      // previous LDPC BER will be acquired.
    AVL_POST_LDPC_BER       =   3,                      // post LDPC BER will be acquired.
    AVL_FINAL_BER           =   4                       // final BER will be acquired.
}AVL_BER_Type;

// Defines different standards supported by the demod.
typedef enum AVL_DemodMode
{
    AVL_DVBC = 0,
    AVL_DVBSX = 1,
    AVL_DVBTX = 2,
    AVL_ISDBT = 3,
    AVL_DTMB = 4,
    AVL_ISDBS = 5,
    AVL_ABSS = 6,
    AVL_ATSC = 7,
    AVL_DVBC2 = 8
} AVL_DemodMode;

// Defines the channel lock mode.
typedef enum AVL_LockMode
{
    AVL_LOCK_MODE_AUTO      =   0,                      // lock channel automatically.
    AVL_LOCK_MODE_MANUAL    =   1                       // lock channel manually.
}AVL_LockMode;

// Defines channel lock status
typedef enum AVL_LockStatus
{
    AVL_STATUS_UNLOCK   =   0,                          // channel isn't locked
    AVL_STATUS_LOCK     =   1                           // channel is in locked state.
}AVL_LockStatus;

typedef enum AVL_TSMode
{
    AVL_TS_PARALLEL = 0, 
    AVL_TS_SERIAL =   1
}AVL_TSMode; 

typedef enum AVL_TSClockEdge
{
    AVL_MPCM_FALLING   =   0, 
    AVL_MPCM_RISING    =   1  
} AVL_TSClockEdge; 

typedef enum AVL_TSClockMode
{
    AVL_TS_CONTINUOUS_ENABLE = 0,   
    AVL_TS_CONTINUOUS_DISABLE = 1      
} AVL_TSClockMode; 

typedef enum AVL_TSSerialPin
{
    AVL_MPSP_DATA0  =   0, 
    AVL_MPSP_DATA7  =   1  
} AVL_TSSerialPin; 

typedef enum AVL_TSSerialOrder
{
    AVL_MPBO_LSB = 0, 
    AVL_MPBO_MSB = 1 
} AVL_TSSerialOrder; 

typedef enum AVL_TSSerialSyncPulse
{
    AVL_TS_SERIAL_SYNC_8_PULSE    =   0,         
    AVL_TS_SERIAL_SYNC_1_PULSE      =   1        
} AVL_TSSerialSyncPulse; 

typedef enum AVL_TSErrorBit
{
    AVL_TS_ERROR_BIT_DISABLE  =   0,  
    AVL_TS_ERROR_BIT_ENABLE   =   1  
} AVL_TSErrorBit; 

typedef enum AVL_TSErrorPolarity
{
    AVL_MPEP_Normal = 0,  
    AVL_MPEP_Invert = 1  
} AVL_TSErrorPolarity; 

typedef enum AVL_TSValidPolarity
{
    AVL_MPVP_Normal     =   0, 
    AVL_MPVP_Invert     =   1   
} AVL_TSValidPolarity; 

typedef enum AVL_TSPacketLen
{
    AVL_TS_188 = 0,
    AVL_TS_204 = 1
} AVL_TSPacketLen; 

typedef enum AVL_AGCPola
{
    AVL_AGC_NORMAL  =   0,        //  normal AGC polarity. Used for a tuner whose gain increases with increased AGC voltage.
    AVL_AGC_INVERTED=   1         //  inverted AGC polarity. Used for tuner whose gain decreases with increased AGC voltage.
}AVL_AGCPola;

typedef enum AVL_TSParallelOrder
{
    AVL_TS_PARALLEL_ORDER_INVERT =   0,
    AVL_TS_PARALLEL_ORDER_NORMAL =   1
} AVL_TSParallelOrder; 

typedef enum AVL_TSParallelPhase
{
    AVL_TS_PARALLEL_PHASE_0 = 0,
    AVL_TS_PARALLEL_PHASE_1 = 1,
    AVL_TSG_PARALLEL_PHASE_2 = 2,
    AVL_TS_PARALLEL_PHASE_3 = 3
}AVL_TSParallelPhase;

// Stores an unsigned 64-bit integer
typedef struct AVLuint64
{
    u32 uiHighWord;                  // The most significant 32-bits of the unsigned 64-bit integer
    u32 uiLowWord;                   // The least significant 32-bits of the unsigned 64-bit integer
}AVLuint64;


// Defines whether the feeback bit of the LFSR used to generate the BER/PER test pattern is inverted.
typedef enum AVL_LFSR_FbBit
{
    AVL_LFSR_FB_NOT_INVERTED    =   0,          // LFSR feedback bit isn't inverted
    AVL_LFSR_FB_INVERTED        =   1           // LFSR feedback bit is inverted
}AVL_LFSR_FbBit;

// Defines the test pattern being used for BER/PER measurements.
typedef enum AVL_TestPattern
{
    AVL_TEST_LFSR_15    =   0,                  // BER test pattern is LFSR15
    AVL_TEST_LFSR_23    =   1                   // BER test pattern is LFSR23        
}AVL_TestPattern;

// Defines the type of auto error statistics 
typedef enum AVL_AutoErrorStat_Type
{
    AVL_ERROR_STAT_BYTE     =   0,                      // error statistics will be reset according to the number of received bytes.
    AVL_ERROR_STAT_TIME     =   1                       // error statistics will be reset according to time interval.
}AVL_AutoErrorStat_Type;

// Defines Error statistics mode
typedef enum AVL_ErrorStat_Mode
{
    AVL_ERROR_STAT_MANUAL   =   0,
    AVL_ERROR_STAT_AUTO     =   1
}AVL_ErrorStat_Mode;

//Emerald2  PLL
#define hw_E2_AVLEM61_reset_register                       0x00100000
#define hw_E2_AVLEM61_dll_init                             0x00100008
#define hw_E2_AVLEM61_deglitch_mode                        0x00100010
#define hw_E2_AVLEM61_sys_pll_bypass                       0x00100040
#define hw_E2_AVLEM61_sys_pll_enable                       0x00100044
#define hw_E2_AVLEM61_sys_pll_divr                         0x00100048
#define hw_E2_AVLEM61_sys_pll_divf                         0x0010004c
#define hw_E2_AVLEM61_sys_pll_divq                         0x00100050
#define hw_E2_AVLEM61_sys_pll_range                        0x00100054
#define hw_E2_AVLEM61_sys_pll_lock                         0x00100058
#define hw_E2_AVLEM61_mpeg_pll_bypass                      0x0010005c
#define hw_E2_AVLEM61_mpeg_pll_enable                      0x00100060
#define hw_E2_AVLEM61_mpeg_pll_divr                        0x00100064
#define hw_E2_AVLEM61_mpeg_pll_divf                        0x00100068
#define hw_E2_AVLEM61_mpeg_pll_divq                        0x0010006c
#define hw_E2_AVLEM61_mpeg_pll_range                       0x00100070
#define hw_E2_AVLEM61_mpeg_pll_lock                        0x00100074
#define hw_E2_AVLEM61_adc_pll_bypass                       0x00100078 
#define hw_E2_AVLEM61_adc_pll_enable                       0x0010007c
#define hw_E2_AVLEM61_adc_pll_divr                         0x00100080
#define hw_E2_AVLEM61_adc_pll_divf                         0x00100084
#define hw_E2_AVLEM61_adc_pll_divq                         0x00100088
#define hw_E2_AVLEM61_adc_pll_range                        0x0010008c
#define hw_E2_AVLEM61_adc_pll_lock                         0x00100090
#define hw_E2_AVLEM61_mpeg_pll_reset                       0x00100094
#define hw_E2_AVLEM61_adc_pll_reset                        0x00100098
#define hw_E2_AVLEM61_sys_pll_reset                        0x0010009c
#define hw_E2_AVLEM61_sys_pll_enable2                      0x001000b4
#define hw_E2_AVLEM61_sys_pll_enable3                      0x001000b8
#define hw_E2_AVLEM61_sys_pll_divq2                        0x001000bc
#define hw_E2_AVLEM61_sys_pll_divq3                        0x001000c0
#define hw_E2_AVLEM61_mpeg_pll_enable2                     0x001000c4
#define hw_E2_AVLEM61_mpeg_pll_enable3                     0x001000c8
#define hw_E2_AVLEM61_mpeg_pll_divq2                       0x001000cc
#define hw_E2_AVLEM61_mpeg_pll_divq3                       0x001000d0
#define hw_E2_AVLEM61_adc_pll_enable2                      0x001000d4
#define hw_E2_AVLEM61_adc_pll_enable3                      0x001000d8
#define hw_E2_AVLEM61_adc_pll_divq2                        0x001000dc
#define hw_E2_AVLEM61_adc_pll_divq3                        0x001000e0
#define hw_E2_AVLEM61_ddc_clk_sel                          0x001000e4
#define hw_E2_AVLEM61_sdram_clk_sel                        0x001000e8
#define hw_E2_AVLEM61_dll_out_phase                        0x00000100
#define hw_E2_AVLEM61_dll_rd_phase                         0x00000104



















// Defines the DiSEqC status
typedef enum AVL_DiseqcStatus
{
    AVL_DOS_Uninitialized = 0,                  // DiSEqC has not been initialized yet.
    AVL_DOS_Initialized   = 1,                  // DiSEqC has been initialized.
    AVL_DOS_InContinuous  = 2,                  // DiSEqC is in continuous mode.
    AVL_DOS_InTone        = 3,                  // DiSEqC is in tone burst mode.
    AVL_DOS_InModulation  = 4                   // DiSEqC is in modulation mode.
}AVL_DiseqcStatus;

// Contains variables for storing error statistics used in the BER and PER calculations.
typedef  struct AVL_ErrorStats
{
    u16 usLFSRSynced;    // Indicates whether the receiver is synchronized with the transmitter generating the BER test pattern.
    u16 usLostLock;      // Indicates whether the receiver has lost lock since the BER/PER measurement was started.
    AVLuint64 stSwCntNumBits;     // A software counter which stores the number of bits which have been received.
    AVLuint64 stSwCntBitErrors;   // A software counter which stores the number of bit errors which have been detected.
    AVLuint64 stNumBits;          // The total number of bits which have been received.
    AVLuint64 stBitErrors;        // The total number of bit errors which have been detected.
    AVLuint64 stSwCntNumPkts;     // A software counter which stores the number of packets which have been received.
    AVLuint64 stSwCntPktErrors;   // A software counter which stores the number of packet errors which have been detected.
    AVLuint64 stNumPkts;          // The total number of packets which have been received.
    AVLuint64 stPktErrors;        // The total number of packet errors which have been detected.
    u32 uiBER;             // The bit error rate scaled by 1e9.
    u32 uiPER;             // The packet error rate scaled by 1e9.
}AVL_ErrorStats;

typedef enum AVL_Demod_Xtal
{
    Xtal_30M = 0,
    Xtal_16M,
    Xtal_24M,
    Xtal_27M
} AVL_Demod_Xtal;

typedef enum AVL_InputPath
{
    AVL_IF_I,
    AVL_IF_Q
} AVL_InputPath;

// Contains variables for storing error statistics used in the BER and PER calculations.
typedef  struct AVL_ErrorStatConfig
{
    AVL_ErrorStat_Mode      eErrorStatMode;           // indicates the error statistics mode. 
    AVL_AutoErrorStat_Type  eAutoErrorStatType;       // indicates the MPEG data sampling clock mode.
    u32              uiTimeThresholdMs;        // used to set time interval for auto error statistics.
    u32              uiNumberThresholdByte;    // used to set the received byte number threshold for auto error statistics.
}AVL_ErrorStatConfig;

// Contains variables for storing error statistics used in the BER and PER calculations.
typedef  struct AVL_BERConfig
{
    AVL_TestPattern         eBERTestPattern;         // indicates the pattern of LFSR.
    AVL_LFSR_FbBit          eBERFBInversion;         // indicates LFSR feedback bit inversion.
    u32              uiLFSRSynced;                // indicates the LFSR synchronization status.
    u32              uiLFSRStartPos;         //set LFSR start byte positon
}AVL_BERConfig;




/*
typedef struct AVL_StandardSpecificFunctions
{
    AVL_Func_Initialize fpRxInitializeFunc;
    AVL_Func_GetLockStatus fpGetLockStatus;
    AVL_Func_GetSNR fpGetSNR;
    AVL_Func_GetSQI fpGetSQI;
    AVL_Func_GetPrePostBER fpGetPrePostBER;
}AVL_StandardSpecificFunctions;
*/
typedef struct AVL_DVBTxPara
{
    AVL_InputPath eDVBTxInputPath;
    u32 uiDVBTxIFFreqHz;
    AVL_AGCPola eDVBTxAGCPola;
} AVL_DVBTxPara;

typedef enum AVL_Diseqc_WaveFormMode
{
    AVL_DWM_Normal = 0,                         // Normal waveform mode
    AVL_DWM_Envelope = 1                        // Envelope waveform mode
}AVL_Diseqc_WaveFormMode;

typedef struct AVL_DVBSxPara
{
    AVL_semaphore semDiseqc;
    AVL_DiseqcStatus eDiseqcStatus;
    AVL_AGCPola eDVBSxAGCPola;
    AVL_Diseqc_WaveFormMode e22KWaveForm;
}AVL_DVBSxPara;

typedef enum AVL_ISDBT_BandWidth
{
    AVL_ISDBT_BW_6M  =   0,
    AVL_ISDBT_BW_8M  =   1,
}AVL_ISDBT_BandWidth;

typedef struct AVL_ISDBTPara
{
    AVL_InputPath eISDBTInputPath;
    AVL_ISDBT_BandWidth eISDBTBandwidth;
    u32 uiISDBTIFFreqHz;
    AVL_AGCPola eISDBTAGCPola;
} AVL_ISDBTPara;

typedef struct AVL_DTMBPara
{
    AVL_InputPath eDTMBInputPath;
    u32 uiDTMBIFFreqHz;
    u32 uiDTMBSymbolRateHz;
    AVL_AGCPola eDTMBAGCPola;
} AVL_DTMBPara;

typedef enum AVL_DVBC_Standard
{
    AVL_DVBC_J83A    =   0,           //the J83A standard
    AVL_DVBC_J83B    =   1,           //the J83B standard
    AVL_DVBC_UNKNOWN =   2
}AVL_DVBC_Standard;

typedef struct AVL_DVBCPara
{
    AVL_InputPath eDVBCInputPath;
    u32 uiDVBCIFFreqHz;
    u32 uiDVBCSymbolRateSps;
    AVL_AGCPola eDVBCAGCPola;
    AVL_DVBC_Standard eDVBCStandard;
} AVL_DVBCPara;

/**************************************************/
typedef struct AVL_CommonConfig
{
    u16      usI2CAddr;
    AVL_Demod_Xtal  eDemodXtal;
    AVL_TSMode      eTSMode;
    AVL_TSClockEdge eClockEdge;
    AVL_TSClockMode eClockMode;
}AVL_CommonConfig;

typedef struct AVL_DVBTxConfig
{
    AVL_InputPath eDVBTxInputPath;
    u32  uiDVBTxIFFreqHz;
    AVL_AGCPola eDVBTxAGCPola;
} AVL_DVBTxConfig;

typedef struct AVL_DVBCConfig
{ 
    AVL_InputPath eDVBCInputPath;
    u32 uiDVBCIFFreqHz;
    u32 uiDVBCSymbolRateSps;
    AVL_AGCPola eDVBCAGCPola;
    AVL_DVBC_Standard eDVBCStandard;
} AVL_DVBCConfig;

typedef struct AVL_DVBSxConfig
{
    AVL_AGCPola eDVBSxAGCPola;
    AVL_Diseqc_WaveFormMode e22KWaveForm;
} AVL_DVBSxConfig;

typedef struct AVL_DTMBConfig
{
    AVL_InputPath eDTMBInputPath;
    u32 uiDTMBIFFreqHz;
    u32 uiDTMBSymbolRateHz;
    AVL_AGCPola eDTMBAGCPola;
} AVL_DTMBConfig;

typedef struct AVL_ISDBTConfig
{ 
    AVL_InputPath eISDBTInputPath;
    AVL_ISDBT_BandWidth eISDBTBandwidth;
    u32 uiISDBTIFFreqHz;
    AVL_AGCPola eISDBTAGCPola;
} AVL_ISDBTConfig;

typedef struct AVL_TSConfig
{
    AVL_TSMode eMode;
    AVL_TSClockEdge eClockEdge;
    AVL_TSClockMode eClockMode;
    AVL_TSSerialPin eSerialPin;
    AVL_TSSerialOrder eSerialOrder;
    AVL_TSSerialSyncPulse eSerialSyncPulse;
    AVL_TSErrorBit eErrorBit;
    AVL_TSErrorPolarity eErrorPolarity;
    AVL_TSValidPolarity eValidPolarity;
    AVL_TSPacketLen ePacketLen;
    AVL_TSParallelOrder eParallelOrder;
    u32 guiDVBTxSerialTSContinuousHz;
    u32 guiDVBSxSerialTSContinuousHz;
    u32 guiISDBTSerialTSContinuousHz;
    u32 guiDTMBSerialTSContinuousHz;
    u32 guiDVBCSerialTSContinuousHz;
}AVL_TSConfig;

typedef struct AVL_BaseAddressSet
{
    u32 hw_mcu_reset_base;
    u32 hw_mcu_system_reset_base;
    u32 hw_esm_base;
    u32 hw_tuner_i2c_base;
    u32 hw_gpio_control_base;
    u32 hw_gpio_debug_base;
    u32 hw_TS_tri_state_cntrl_base;
    u32 hw_AGC_tri_state_cntrl_base;
    u32 hw_diseqc_base;
    u32 hw_plp_list_base;
    u32 hw_blind_scan_info_base;
    u32 hw_member_ID_base;
    u32 hw_dma_sys_status_base;
    u32 hw_dma_sys_cmd_base;
    u32 fw_config_reg_base;
    u32 fw_status_reg_base;
    u32 fw_DVBTx_config_reg_base;
    u32 fw_DVBTx_status_reg_base;
    u32 fw_DVBT2_data_PLP_config_reg_base;
    u32 fw_DVBT2_common_PLP_config_reg_base;
    u32 fw_DVBT2_P1_reg_base;
    u32 fw_DVBT2_L1_pre_reg_base;
    u32 fw_DVBT2_L1_post_config_reg_base;
    u32 fw_DVBT_TPS_reg_base;
    u32 fw_DVBSx_config_reg_base;
    u32 fw_DVBSx_status_reg_base;
    u32 fw_ISDBT_config_reg_base;
    u32 fw_ISDBT_status_reg_base;
    u32 fw_DTMB_config_reg_base;
    u32 fw_DTMB_status_reg_base;
    u32 fw_DVBC_config_reg_base;
    u32 fw_DVBC_status_reg_base;
}AVL_BaseAddressSet;

typedef struct AVL_ChipInternal
{
    u16 usI2CAddr;
    u32 uiFamilyID;
    AVL_Demod_Xtal eDemodXtal;
    AVL_DemodMode eCurrentDemodMode;
    AVL_semaphore semRx;
    u32 uiCoreFrequencyHz;
    u32 uiFECFrequencyHz;
    u32 uiTSFrequencyHz;
    u32 uiADCFrequencyHz;
    u32 uiRefFrequencyHz;
    u32 uiDDCFrequencyHz;
    u32 uiSDRAMFrequencyHz;
    AVL_TSConfig stTSConfig;
    AVL_ErrorStatConfig stErrorStatConfig;
    AVL_DVBTxPara stDVBTxPara;
    AVL_DVBSxPara stDVBSxPara;
    AVL_ISDBTPara stISDBTPara;
    AVL_DTMBPara stDTMBPara;
    AVL_DVBCPara stDVBCPara;
//    AVL_StandardSpecificFunctions stStdSpecFunc;
    u8 ucCustomizeFwData;
    AVL_BaseAddressSet stBaseAddrSet;
    AVL_ErrorStats stAVLErrorStat;
    AVL_puchar DVBTxFwData;
    AVL_puchar DVBSxFwData;
    AVL_puchar ISDBTFwData;
    AVL_puchar DVBCFwData;
    AVL_puchar DTMBFwData;
	u32 variable_array[PATCH_VAR_ARRAY_SIZE];

    AVL_TSSerialPin eTSSerialPin;
    AVL_TSSerialOrder eTSSerialOrder;
    AVL_TSSerialSyncPulse eTSSerialSyncPulse;
    AVL_TSErrorBit eTSErrorBit;
    AVL_TSErrorPolarity eTSErrorPola;
    AVL_TSValidPolarity eTSValidPola;
    AVL_TSPacketLen eTSPacketLen;
    AVL_TSParallelOrder eTSParallelOrder;
    AVL_TSParallelPhase eTSParallelPhase;

    u8 ucDisableTCAGC;
    u8 ucDisableSAGC;
    u8 ucDisableTSOutput;
    
    u8 ucPin37Status; // 0 - InPut; 1- OutPut
    u8 ucPin38Status;
    u8 ucPin37Voltage; // 0 - High; 1- Low; 2 - High_Z
    u8 ucPin38Voltage;   

    u8 ucSleepFlag;  //0 - Wakeup, 1 - Sleep 

} AVL_ChipInternal;

// The Availink version structure.
typedef struct AVL_Version
{
    u8   ucMajor;                            // The major version number.
    u8   ucMinor;                            // The minor version number.
    u16  usBuild;                            // The build version number.
}AVL_Version;

// Stores AVLEM61 device version information.
typedef struct AVL_DemodVersion
{
    u32  uiChip;                             // Hardware version information. 0xYYYYMMDD
    AVL_Version stPatch;                            // The version of the internal patch.
} AVL_DemodVersion;


#define Diseqc_delay 20
 
#define AVL_EC_OK                   0           // There is no error. 
#define AVL_EC_WARNING              1           // There is warning. 
#define AVL_EC_GENERAL_FAIL         2           // A general failure has occurred. 
#define AVL_EC_I2C_FAIL             4           // An I2C operation failed during communication with the AVLEM61 through the BSP. 
#define AVL_EC_I2C_REPEATER_FAIL    8           // An error ocurred during communication between AVLEM61 I2C master and tuner. This error code is defined by enum AVLEM61_MessageType_II2C_Repeater_Status.                        	 
#define AVL_EC_RUNNING              16          // The AVLEM61 device is busy processing a previous receiver/i2c repeater command. 
#define AVL_EC_TIMEOUT              32          // Operation failed in a given time period 
#define AVL_EC_SLEEP                64          // Demod in sleep mode 
#define AVL_EC_NOT_SUPPORTED        128         // Specified operation isn't support in current senario 
#define AVL_EC_BSP_ERROR1           256         // BSP Error 1, if it's used, need to be customized 
#define AVL_EC_BSP_ERROR2           512         // BSP Error 2, if it's used, need to be customized 
 
#define AVL_CONSTANT_10_TO_THE_9TH      1000000000  //10e9 
 
 
 
#define AVL_min(x,y) (((x) < (y)) ? (x) : (y)) 
#define AVL_max(x,y) (((x) < (y)) ? (y) : (x)) 
#define AVL_floor(a) (((a) == (int)(a))? ((int)(a)) : (((a) < 0)? ((int)((a)-1)) : ((int)(a)))) 
#define AVL_ceil(a)  (((a) == (int)(a))? ((int)(a)) : (((a) < 0)? ((int)(a)) : ((int)((a)+1)))) 
#define AVL_abs(a)  (((a)>0) ? (a) : (-(a))) 








/* DVBS header file */
#define rc_DVBSx_rfagc_pol_iaddr_offset                                  0x00000000
#define rc_DVBSx_internal_decode_mode_iaddr_offset                        0x0000000c
#define rc_DVBSx_format_iaddr_offset                                      0x00000010
#define rc_DVBSx_input_iaddr_offset                                       0x00000014
#define rc_DVBSx_specinv_iaddr_offset                                     0x00000034
#define rc_DVBSx_dvbs_fec_coderate_iaddr_offset                           0x00000044
#define rc_DVBSx_int_sym_rate_MHz_iaddr_offset                            0x00000054
#define rc_DVBSx_aagc_acq_gain_saddr_offset                               0x000000fe
#define rc_DVBSx_Max_LowIF_sps_iaddr_offset                               0x000000a4
#define rc_DVBSx_int_dmd_clk_MHz_saddr_offset                             0x00000164
#define rc_DVBSx_int_mpeg_clk_MHz_saddr_offset                            0x00000168
#define rc_DVBSx_int_fec_clk_MHz_saddr_offset                             0x0000016a
#define rc_DVBSx_fec_bypass_coderate_saddr_offset                         0x0000019a
#define rc_DVBSx_tuner_frequency_100kHz_saddr_offset                      0x000001c0
#define rc_DVBSx_tuner_LPF_100kHz_saddr_offset                            0x000001c6
#define rc_DVBSx_blind_scan_start_freq_100kHz_saddr_offset                0x000001cc
#define rc_DVBSx_blind_scan_min_sym_rate_kHz_saddr_offset                 0x000001d0
#define rc_DVBSx_blind_scan_end_freq_100kHz_saddr_offset                  0x000001d2
#define rc_DVBSx_blind_scan_channel_info_offset_saddr_offset              0x000001d4
#define rc_DVBSx_blind_scan_max_sym_rate_kHz_saddr_offset                 0x000001d6
#define rc_DVBSx_decode_mode_saddr_offset                                 0x00000204
#define rc_DVBSx_iq_mode_saddr_offset                                     0x0000020a
#define rc_DVBSx_dishpoint_mode_saddr_offset                              0x0000020e
#define rc_DVBSx_blind_scan_reset_saddr_offset                            0x00000210
#define rc_DVBSx_functional_mode_saddr_offset                             0x00000212
#define rc_DVBSx_blind_scan_tuner_spectrum_inversion_saddr_offset         0x00000226
#define rc_DVBSx_IF_Offset_10kHz_saddr_offset                             0x00000234
#define rc_DVBSx_dvbs2_fec_coderate_caddr_offset                          0x0000023f
#define rc_DVBSx_adc_Q_chan_sel_caddr_offset                              0x00000246
#define rc_DVBSx_adc_I_chan_sel_caddr_offset                              0x00000247
#define rc_DVBSx_dvbs2_code_rate_saddr_offset                             0x00000258
#define rc_DVBSx_dvbs2_modulation_saddr_offset                            0x0000025a
#define rc_DVBSx_int_adc_clk_MHz_saddr_offset                             0x000002a0

#define rs_DVBSx_modulation_iaddr_offset                                  0x0000000c
#define rs_DVBSx_pilot_iaddr_offset                                       0x00000010
#define rs_DVBSx_int_SNR_dB_iaddr_offset                                  0x00000020
#define rs_DVBSx_blind_scan_channel_count_saddr_offset                    0x000000b0
#define rs_DVBSx_blind_scan_error_code_saddr_offset                       0x000000b4
#define rs_DVBSx_blind_scan_progress_saddr_offset                         0x000000b6
#define rs_DVBSx_post_viterbi_BER_estimate_x10M_iaddr_offset    		  0x000000c4
#define rs_DVBSx_post_LDPC_BER_estimate_x10M_iaddr_offset                 0x000000c8
#define rs_DVBSx_pre_LDPC_BER_estimate_x10M_iaddr_offset        		  0x000000cc
#define rs_DVBSx_detected_alpha_iaddr_offset                              0x000000d0
#define rs_DVBSx_int_carrier_freq_100kHz_saddr_offset                     0x00000078
#define rs_DVBSx_fec_lock_saddr_offset                                    0x0000009e


#define hw_diseqc_tx_cntrl_offset                                     0x0 
#define hw_diseqc_tone_frac_n_offset                                  0x4 
#define hw_diseqc_tone_frac_d_offset                                  0x8 
#define hw_diseqc_tx_st_offset                                        0xC 
#define hw_diseqc_rx_parity_offset                                    0x10
#define hw_diseqc_rx_msg_tim_offset                                   0x14
#define hw_diseqc_rx_st_offset                                        0x18
#define hw_diseqc_rx_cntrl_offset                                     0x1C
#define hw_diseqc_srst_offset                                         0x20
#define hw_diseqc_samp_frac_n_offset                                  0x28
#define hw_diseqc_samp_frac_d_offset                                  0x2C
#define hw_rx_fifo_map_offset                                         0x40
#define hw_tx_fifo_map_offset                                         0x80

/// Represents the code rate. The Availink device can automatically detect the code rate of the input signal.
typedef enum AVL_DVBS_CodeRate
{
    AVL_DVBS_CR_1_2  =   0, 
    AVL_DVBS_CR_2_3  =   1,
    AVL_DVBS_CR_3_4  =   2,
    AVL_DVBS_CR_5_6  =   3,
    AVL_DVBS_CR_6_7  =   4,
    AVL_DVBS_CR_7_8  =   5
}AVL_DVBS_CodeRate;


typedef enum AVL_DVBS2_CodeRate
{
    AVL_DVBS2_CR_1_4     =   0,
    AVL_DVBS2_CR_1_3     =   1,
    AVL_DVBS2_CR_2_5     =   2,
    AVL_DVBS2_CR_1_2     =   3,
    AVL_DVBS2_CR_3_5     =   4,
    AVL_DVBS2_CR_2_3     =   5,
    AVL_DVBS2_CR_3_4     =   6,
    AVL_DVBS2_CR_4_5     =   7,
    AVL_DVBS2_CR_5_6     =   8,
    AVL_DVBS2_CR_8_9     =   9,
    AVL_DVBS2_CR_9_10    =   10
}AVL_DVBS2_CodeRate;

typedef enum AVL_DVBSx_Pilot
{
    AVL_DVBSx_Pilot_OFF  =   0,                  // Pilot off
    AVL_DVBSx_Pilot_ON   =   1                   // Pilot on
}AVL_DVBSx_Pilot;

typedef enum AVL_DVBSx_ModulationMode
{
    AVL_DVBSx_QPSK   =   0,                      // QPSK
    AVL_DVBSx_8PSK   =   1,                      // 8-PSK
    AVL_DVBSx_16APSK =   2,                      // 16-APSK
    AVL_DVBSx_32APSK =   3                       // 32-APSK
}AVL_DVBSx_ModulationMode;


typedef enum AVL_DVBSx_RollOff
{
    AVL_DVBSx_RollOff_20 = 0,                    // Roll off is 0.20
    AVL_DVBSx_RollOff_25 = 1,                    // Roll off is 0.25
    AVL_DVBSx_RollOff_35 = 2                     // Roll off is 0.35
}AVL_DVBSx_RollOff;

typedef enum AVL_DVBSx_Standard
{
    AVL_DVBS     =   0,                          // DVBS standard
    AVL_DVBS2    =   1                           // DVBS2 standard
}AVL_DVBSx_Standard;    

// Defines the AVL device spectrum inversion mode
typedef enum AVL_SpectrumInversion
{
    AVL_SPECTRUM_NORMAL     =   0,                      // Signal spectrum in normal.
    AVL_SPECTRUM_INVERTED   =   1,                      // Signal spectrum in inverted.
    AVL_SPECTRUM_AUTO       =   2                       // Signal spectrum isn't known.
}AVL_SpectrumInversion;

// Defines the ON/OFF options for the AVLEM61 device.
typedef enum AVL_Switch
{
    AVL_ON  =   0,                              // switched on
    AVL_OFF =   1                               // switched off
}AVL_Switch;

// Defines the device functional mode.
typedef enum AVL_FunctionalMode
{
    AVL_FuncMode_Demod      =   0,              // The device is in demod mode.
    AVL_FuncMode_BlindScan  =   1               // The device is in blind scan mode.
}AVL_FunctionalMode;

typedef enum AVL_Diseqc_TxGap
{
    AVL_DTXG_15ms = 0,                          // The gap is 15 ms.
    AVL_DTXG_20ms = 1,                          // The gap is 20 ms.
    AVL_DTXG_25ms = 2,                          // The gap is 25 ms.
    AVL_DTXG_30ms = 3                           // The gap is 30 ms.
}AVL_Diseqc_TxGap;

typedef enum AVL_Diseqc_TxMode
{
    AVL_DTM_Modulation = 0,                     // Use modulation mode.
    AVL_DTM_Tone0 = 1,                          // Send out tone 0.
    AVL_DTM_Tone1 = 2,                          // Send out tone 1.
    AVL_DTM_Continuous = 3                      // Continuously send out pulses.
}AVL_Diseqc_TxMode;

typedef enum AVL_Diseqc_RxTime
{
    AVL_DRT_150ms = 0,                          // Wait 150 ms for receive data and then close the input FIFO.
    AVL_DRT_170ms = 1,                          // Wait 170 ms for receive data and then close the input FIFO.
    AVL_DRT_190ms = 2,                          // Wait 190 ms for receive data and then close the input FIFO.
    AVL_DRT_210ms = 3                           // Wait 210 ms for receive data and then close the input FIFO.
}AVL_Diseqc_RxTime;

// Stores blind scan info
typedef struct AVL_BSInfo
{
    u16 m_uiProgress;                        // The percentage completion of the blind scan procedure. A value of 100 indicates that the blind scan is finished.
    u16 m_uiChannelCount;                    // The number of channels detected thus far by the blind scan operation.  The Availink device can store up to 120 detected channels.
    u16 m_uiNextStartFreq_100kHz;            // The start frequency of the next scan in units of 100kHz.
    u16 m_uiResultCode;                      // The result of the blind scan operation.  Possible values are:  0 - blind scan operation normal; 1 -- more than 120 channels have been detected.
}AVL_BSInfo;

// Stores channel info
typedef struct AVL_ChannelInfo
{
    u32 m_uiFrequency_kHz;                   // The channel carrier frequency in units of kHz. 
    u32 m_uiSymbolRate_Hz;                   // The symbol rate in units of Hz. 
    u32 m_Flags;                             // Contains bit-mapped fields which store additional channel configuration information.
}AVL_ChannelInfo;

typedef struct AVL_DVBSxModulationInfo
{
    AVL_DVBSx_ModulationMode    eDVBSxModulationMode;
    AVL_DVBS_CodeRate           eDVBSCodeRate;
    AVL_DVBS2_CodeRate          eDVBS2CodeRate;
    AVL_DVBSx_Pilot             eDVBSxPilot;
    AVL_DVBSx_RollOff           eDVBSxRollOff;
    AVL_DVBSx_Standard          eDVBSxStandard;
}AVL_DVBSxModulationInfo;

typedef struct AVL_DVBSxManualLockInfo
{
    AVL_DVBSx_ModulationMode    eDVBSxModulationMode;
    AVL_DVBS_CodeRate           eDVBSCodeRate;
    AVL_DVBS2_CodeRate          eDVBS2CodeRate;
    AVL_DVBSx_Pilot             eDVBSxPilot;
    AVL_SpectrumInversion       eDVBSxSpecInversion;
    AVL_DVBSx_Standard          eDVBSxStandard;
    u32                  uiDVBSxSymbolRateSps;
}AVL_DVBSxManualLockInfo;

// Defines the device spectrum polarity setting. 
typedef enum AVL_BlindSanSpectrumPolarity
{
    AVL_Spectrum_Invert = 0,
    AVL_Spectrum_Normal = 1
}AVL_BlindSanSpectrumPolarity;

/// Stores the blind scan parameters which are passed to the blind scan function.
typedef struct AVL_BlindScanPara
{
    u16 m_uiStartFreq_100kHz;                // The start scan frequency in units of 100kHz. The minimum value depends on the tuner specification. 
    u16 m_uiStopFreq_100kHz;                 // The stop scan frequency in units of 100kHz. The maximum value depends on the tuner specification.
    u16 m_uiMinSymRate_kHz;                  // The minimum symbol rate to be scanned in units of kHz. The minimum value is 1000 kHz.
    u16 m_uiMaxSymRate_kHz;                  // The maximum symbol rate to be scanned in units of kHz. The maximum value is 45000 kHz.
    AVL_BlindSanSpectrumPolarity m_enumBSSpectrumPolarity;
}AVL_BlindScanPara;

// Stores DiSEqC operation parameters
typedef struct AVL_Diseqc_Para
{
    u16              uiToneFrequencyKHz;// The DiSEqC bus speed in units of kHz. Normally, it is 22kHz. 
    AVL_Diseqc_TxGap        eTXGap;            // Transmit gap
    AVL_Diseqc_WaveFormMode eTxWaveForm;       // Transmit waveform format
    AVL_Diseqc_RxTime       eRxTimeout;        // Receive time frame window
    AVL_Diseqc_WaveFormMode eRxWaveForm;       // Receive waveform format
}AVL_Diseqc_Para;

// Stores the DiSEqC transmitter status.
typedef struct AVL_Diseqc_TxStatus
{
    u8   m_TxDone;                           // Indicates whether the transmission is complete (1 - transmission is finished, 0 - transmission is still in progress).
    u8   m_TxFifoCount;                      // The number of bytes remaining in the transmit FIFO
}AVL_Diseqc_TxStatus;

// Stores the DiSEqC receiver status
typedef struct AVL_Diseqc_RxStatus
{
    u8   m_RxFifoCount;                      // The number of bytes in the DiSEqC receive FIFO.
    u8   m_RxFifoParChk;                     // The parity check result of the received data. This is a bit-mapped field in which each bit represents the parity check result for each each byte in the receive FIFO.  The upper bits without corresponding data are undefined. If a bit is 1, the corresponding byte in the FIFO has good parity. For example, if three bytes are in the FIFO, and the parity check value is 0x03 (value of bit 2 is zero), then the first and the second bytes in the receive FIFO are good. The third byte had bad parity. 
    u8   m_RxDone;                           // 1 if the receiver window is turned off, 0 if it is still in receiving state.
}AVL_Diseqc_RxStatus;






/* DVBC */
//DVBC config registers offset address
#define rc_DVBC_symbol_rate_Hz_iaddr_offset                     0x00000000
#define rc_DVBC_sample_rate_Hz_iaddr_offset                     0x00000004
#define rc_DVBC_dmd_clk_Hz_iaddr_offset                         0x00000008
#define rc_DVBC_j83b_mode_caddr_offset                          0x00000017
#define rc_DVBC_tuner_type_caddr_offset                         0x00000024
#define rc_DVBC_input_format_caddr_offset                       0x00000025
#define rc_DVBC_spectrum_invert_caddr_offset                    0x00000026
#define rc_DVBC_input_select_caddr_offset                       0x00000027
#define rc_DVBC_if_freq_Hz_iaddr_offset                         0x00000028
#define rc_DVBC_qam_mode_iaddr_offset                           0x0000002c
#define rc_DVBC_rfagc_pol_caddr_offset						    0x00000049
#define rc_DVBC_fec_clk_Hz_iaddr_offset                         0x00000050
#define rc_DVBC_get_btr_crl_iaddr_offset                        0x00000080
#define rc_DVBC_qam_mode_scan_control_iaddr_offset              0x00000090
#define rc_DVBC_adc_sel_caddr_offset							0x000001ef
#define rc_DVBC_adc_use_pll_clk_caddr_offset				    0x000001ee



//DVBC status registers offset address
#define rs_DVBC_mode_status_iaddr_offset                        0x00000004
#define rs_DVBC_snr_dB_x100_saddr_offset                        0x0000000e
#define rs_DVBC_j83b_il_mode_caddr_offset                       0x0000001d
#define rs_DVBC_post_viterbi_BER_estimate_x10M_iaddr_offset     0x0000004c

typedef enum AVL_DVBC_TunerType
{
    AVL_DVBC_IF          =       0,
    AVL_DVBC_BASEBAND    =       1
}AVL_DVBC_TunerType;

typedef enum AVL_DVBC_ChannelType
{
    AVL_DVBC_I_CHANNEL   =       0,
    AVL_DVBC_Q_CHANNEL   =       1
}AVL_DVBC_ChannelType;

typedef enum AVL_DVBCQAMMode
{
    AVL_DVBC_16QAM               =   0,      
    AVL_DVBC_32QAM               =   1,
    AVL_DVBC_64QAM               =   2,      
    AVL_DVBC_128QAM              =   3,
    AVL_DVBC_256QAM              =   4
}AVL_DVBCQAMMode;

// Defines the symbol interleave mode of the received DVBC signal, only used for J.83B.
typedef enum AVL_DVBCInterleaveMode
{
    AVL_DVBC_INTERLEAVE_128_1_0  =   0,
    AVL_DVBC_INTERLEAVE_128_1_1  =   1,
    AVL_DVBC_INTERLEAVE_128_2    =   2,
    AVL_DVBC_INTERLEAVE_64_2     =   3,
    AVL_DVBC_INTERLEAVE_128_3    =   4,
    AVL_DVBC_INTERLEAVE_32_4     =   5,
    AVL_DVBC_INTERLEAVE_128_4    =   6,
    AVL_DVBC_INTERLEAVE_16_8     =   7,
    AVL_DVBC_INTERLEAVE_128_5    =   8,
    AVL_DVBC_INTERLEAVE_8_16     =   9,
    AVL_DVBC_INTERLEAVE_128_6    =   10,
    AVL_DVBC_INTERLEAVE_128_7    =   12,
    AVL_DVBC_INTERLEAVE_128_8    =   14
}AVL_DVBCInterleaveMode;


typedef struct AVL_DVBCModulationInfo
{
	AVL_DVBCQAMMode         eQAMMode;
	AVL_DVBCInterleaveMode  eInterleaveMode;
}AVL_DVBCModulationInfo;
 
 typedef struct AVL_DVBC_SQI_CN_Table_Element
 {
	 AVL_DVBC_Standard eStandard;
	 AVL_DVBCQAMMode modulation;
	 u32 CN_Test_Result_x100_db;
 }AVL_DVBC_SQI_CN_Table_Element;


/* DVBT2 */

#define rc_DVBTx_fund_rate_Hz_iaddr_offset                       0x00000004
#define rc_DVBTx_sample_rate_Hz_iaddr_offset                     0x00000008
#define rc_DVBTx_rf_agc_pol_caddr_offset                         0x0000000f
#define rc_DVBTx_tuner_type_caddr_offset                         0x00000040
#define rc_DVBTx_input_format_caddr_offset                       0x00000041
#define rc_DVBTx_spectrum_invert_caddr_offset                    0x00000042
#define rc_DVBTx_input_select_caddr_offset                       0x00000043
#define rc_DVBTx_nom_carrier_freq_Hz_iaddr_offset                0x00000044
#define rc_DVBTx_l1_proc_only_caddr_offset                       0x00000054
#define rc_DVBTx_common_PLP_present_caddr_offset                 0x00000055
#define rc_DVBTx_common_PLP_ID_caddr_offset                      0x00000056
#define rc_DVBTx_data_PLP_ID_caddr_offset                        0x00000057
#define rc_DVBTx_dvbt_layer_select_caddr_offset                  0x0000006a
#define rc_DVBTx_acquire_mode_caddr_offset                       0x0000006b
#define rc_DVBTx_mpeg_clk_rate_Hz_iaddr_offset                   0x0000006c
#define rc_DVBTx_adc_sel_caddr_offset							 0x00000077
#define rc_DVBTx_adc_use_pll_clk_caddr_offset                    0x00000076


#define rs_DVBTx_rx_mode_caddr_offset                            0x000000d0
#define rs_DVBTx_fec_lock_caddr_offset                           0x000000d2
#define rs_DVBTx_snr_dB_x100_saddr_offset                        0x000000d6
#define rs_DVBTx_post_viterbi_BER_estimate_x10M_iaddr_offset     0x00000114
#define rs_DVBTx_post_LDPC_BER_estimate_x1B_iaddr_offset         0x00000118
#define rs_DVBTx_pre_LDPC_BER_estimate_x10M_iaddr_offset         0x0000011c
#define rs_DVBTx_plp_list_request_caddr_offset                   0x00000133

#define rs_DVBTx_data_PLP_ID_caddr_offset                   0x00000000
#define rs_DVBTx_data_PLP_TYPE_caddr_offset                 0x00000001
#define rs_DVBTx_data_PLP_COD_caddr_offset                  0x00000007
#define rs_DVBTx_data_PLP_MOD_caddr_offset                  0x00000008
#define rs_DVBTx_data_PLP_ROTATION_caddr_offset             0x00000009
#define rs_DVBTx_data_PLP_FEC_TYPE_caddr_offset             0x0000000b

#define rs_DVBTx_common_PLP_ID_caddr_offset                 0x00000000
#define rs_DVBTx_common_PLP_COD_caddr_offset                0x00000007
#define rs_DVBTx_common_PLP_MOD_caddr_offset                0x00000008
#define rs_DVBTx_common_PLP_ROTATION_caddr_offset           0x00000009
#define rs_DVBTx_common_PLP_FEC_TYPE_caddr_offset           0x0000000b

#define rs_DVBTx_P1_S2_field_2_caddr_offset                 0x00000003
#define rs_DVBTx_MISO_SISO_caddr_offset                     0x00000005
#define rs_DVBTx_T2_profile_caddr_offset                    0x00000006
#define rs_DVBTx_FFT_size_caddr_offset                      0x00000007

#define rs_DVBTx_NUM_PLP_caddr_offset                       0x00000002

#define rs_DVBTx_constellation_caddr_offset                 0x00000001
#define rs_DVBTx_hierarchy_caddr_offset                     0x00000002
#define rs_DVBTx_HP_code_rate_caddr_offset                  0x00000003
#define rs_DVBTx_LP_code_rate_caddr_offset                  0x00000004
#define rs_DVBTx_guard_interval_caddr_offset                0x00000005
#define rs_DVBTx_transmission_mode_caddr_offset             0x00000006

#define rs_DVBTx_BWT_EXT_caddr_offset                0x00000001
#define rs_DVBTx_GUARD_INTERVAL_caddr_offset         0x00000005
#define rs_DVBTx_PAPR_caddr_offset                   0x00000006
#define rs_DVBTx_L1_MOD_caddr_offset                 0x00000007
#define rs_DVBTx_PILOT_PATTERN_caddr_offset          0x00000014
#define rs_DVBTx_CELL_ID_saddr_offset                0x00000016
#define rs_DVBTx_NETWORK_ID_saddr_offset             0x00000018
#define rs_DVBTx_T2_SYSTEM_ID_saddr_offset           0x0000001a
#define rs_DVBTx_NUM_T2_FRAMES_caddr_offset          0x0000001d
#define rs_DVBTx_NUM_DATA_SYMBOLS_saddr_offset       0x0000001e

#define rs_DVBTx_Signal_Presence_iaddr_offset        0x00000150

typedef enum AVL_DVBTx_TunerType
{
    AVL_DVBTX_REAL_IF            =   0,
    AVL_DVBTX_COMPLEX_BASEBAND   =   1,
    AVL_DVBTX_REAL_IF_FROM_Q     =   2
}AVL_DVBTx_TunerType;

typedef enum AVL_DVBT2_PLP_Type
{
    AVL_DVBT2_SINGLE_PLP         =   0,
    AVL_DVBT2_MULTIPLE_PLP       =   1
}AVL_DVBT2_PLP_Type;

typedef enum AVL_DVBTxBandWidth
{
    AVL_DVBTx_BW_1M7 =   0,
    AVL_DVBTx_BW_5M  =   1,
    AVL_DVBTx_BW_6M  =   2,
    AVL_DVBTx_BW_7M  =   3,
    AVL_DVBTx_BW_8M  =   4
}AVL_DVBTxBandWidth;

typedef enum AVL_DVBTx_LockMode
{
    AVL_DVBTx_LockMode_T2BASE_T   =   0,
    AVL_DVBTx_LockMode_T2LITE_T   =   1,
    AVL_DVBTx_LockMode_T2BASE     =   2,
    AVL_DVBTx_LockMode_T2LITE     =   3,
    AVL_DVBTx_LockMode_T_ONLY     =   4,
	AVL_DVBTx_LockMode_ALL        =   5
}AVL_DVBTx_LockMode;

typedef enum AVL_DVBT_Layer
{
    AVL_DVBT_LAYER_LP    =   0,
    AVL_DVBT_LAYER_HP    =   1
}AVL_DVBT_Layer;

typedef enum AVL_DVBT_FFTSize
{
    AVL_DVBT_FFT_2K      =   0, 
    AVL_DVBT_FFT_8K      =   1,
    AVL_DVBT_FFT_UNKNOWN =   2
}AVL_DVBT_FFTSize;

typedef enum AVL_DVBT_GuardInterval
{
    AVL_DVBT_GUARD_1_32  =   0,
    AVL_DVBT_GUARD_1_16  =   1,
    AVL_DVBT_GUARD_1_8   =   2,
    AVL_DVBT_GUARD_1_4   =   3
}AVL_DVBT_GuardInterval;

typedef enum AVL_DVBT_ModulationMode
{
    AVL_DVBT_QPSK            =   0,
    AVL_DVBT_16QAM           =   1,
    AVL_DVBT_64QAM           =   2,
    AVL_DVBT_MOD_UNKNOWN     =   3
}AVL_DVBT_ModulationMode;

typedef enum AVL_DVBT_Hierarchy
{
    AVL_DVBT_HIER_NONE       =   0,
    AVL_DVBT_HIER_ALPHA_1    =   1,
    AVL_DVBT_HIER_ALPHA_2    =   2,
    AVL_DVBT_HIER_ALPHA_4    =   3
}AVL_DVBT_Hierarchy;

typedef enum AVL_DVBT_CodeRate
{
    AVL_DVBT_CR_1_2  =   0,
    AVL_DVBT_CR_2_3  =   1,
    AVL_DVBT_CR_3_4  =   2,
    AVL_DVBT_CR_5_6  =   3,
    AVL_DVBT_CR_7_8  =   4
}AVL_DVBT_CodeRate;

typedef enum AVL_DVBT2_FFTSize
{
    AVL_DVBT2_FFT_1K     =   0,
    AVL_DVBT2_FFT_2K     =   1,
    AVL_DVBT2_FFT_4K     =   2,
    AVL_DVBT2_FFT_8K     =   3,
    AVL_DVBT2_FFT_16K    =   4,
    AVL_DVBT2_FFT_32K    =   5
}AVL_DVBT2_FFTSize;

typedef enum AVL_DVBT2_MISO_SISO
{
    AVL_DVBT2_SISO   =   0,
    AVL_DVBT2_MISO   =   1
}AVL_DVBT2_MISO_SISO;


typedef enum AVL_DVBT2_PROFILE
{
    AVL_DVBT2_PROFILE_BASE    =  0,
    AVL_DVBT2_PROFILE_LITE    =  1,
    AVL_DVBT2_PROFILE_UNKNOWN =  2
}AVL_DVBT2_PROFILE;

typedef enum AVL_DVBT2_PILOT_PATTERN
{
    AVL_DVBT2_PP_PP1           = 0,
    AVL_DVBT2_PP_PP2           = 1,
    AVL_DVBT2_PP_PP3           = 2,
    AVL_DVBT2_PP_PP4           = 3,
    AVL_DVBT2_PP_PP5           = 4,
    AVL_DVBT2_PP_PP6           = 5,
    AVL_DVBT2_PP_PP7           = 6,
    AVL_DVBT2_PP_PP8           = 7,
    AVL_DVBT2_PP_DVBT          = 8,
    AVL_DVBT2_PP_DVBT_REVERSE  = 9,
    AVL_DVBT2_PP_UNKNOWN       = 10
}AVL_DVBT2_PILOT_PATTERN;

typedef enum AVL_DVBT2_DATA_PLP_TYPE
{
    AVL_DVBT2_DATA_PLP_TYPE1 =   1,
    AVL_DVBT2_DATA_PLP_TYPE2 =   2
}AVL_DVBT2_DATA_PLP_TYPE;

typedef enum AVL_DVBT2_CodeRate
{
    AVL_DVBT2_CR_1_2 = 0,
    AVL_DVBT2_CR_3_5 = 1,
    AVL_DVBT2_CR_2_3 = 2,
    AVL_DVBT2_CR_3_4 = 3,
    AVL_DVBT2_CR_4_5 = 4,
    AVL_DVBT2_CR_5_6 = 5,
    AVL_DVBT2_CR_1_3 = 6,
    AVL_DVBT2_CR_2_5 = 7
}AVL_DVBT2_CodeRate;

typedef enum AVL_DVBT2_PLP_ModulationMode
{
    AVL_DVBT2_QPSK   = 0,
    AVL_DVBT2_16QAM  = 1,
    AVL_DVBT2_64QAM  = 2, 
    AVL_DVBT2_256QAM = 3
}AVL_DVBT2_PLP_ModulationMode;

typedef enum AVL_DVBT2_L1_Modulation
{
    AVL_DVBT2_L1_BPSK = 0,
    AVL_DVBT2_L1_QPSK = 1,
	AVL_DVBT2_L1_16QAM = 2,
	AVL_DVBT2_L1_64QAM = 3
}AVL_DVBT2_L1_Modulation;

typedef enum AVL_DVBT2_PLP_Constellation_Rotation
{
    AVL_DVBT2_PLP_NOT_ROTATION   =   0,
    AVL_DVBT2_PLP_ROTATION       =   1
}AVL_DVBT2_PLP_Constellation_Rotation;

typedef enum AVL_DVBT2_PLP_FEC_Type
{
    AVL_DVBT2_FEC_LDPC16K    =   0,
    AVL_DVBT2_FEC_LDPC64K    =   1
}AVL_DVBT2_PLP_FEC_Type;

typedef enum AVL_DVBTx_Standard
{
    AVL_DVBTx_Standard_T     =   0,                  //the DVB-T standard
    AVL_DVBTx_Standard_T2    =   1                   //the DVB-T2 standard
}AVL_DVBTx_Standard;    

typedef enum AVL_DVBT2_PAPR
{
    AVL_DVBT2_PAPR_NONE       =     0,
    AVL_DVBT2_PAPR_ACE        =     1,
    AVL_DVBT2_PAPR_TR         =     2,
    AVL_DVBT2_PAPR_BOTH       =     3
}AVL_DVBT2_PAPR;

typedef enum AVL_DVBT2_GUARD_INTERVAL
{
    AVL_DVBT2_GI_1_32         =     0,
    AVL_DVBT2_GI_1_16         =     1,
    AVL_DVBT2_GI_1_8          =     2,
    AVL_DVBT2_GI_1_4          =     3,
    AVL_DVBT2_GI_1_128        =     4,
    AVL_DVBT2_GI_19_128       =     5,
    AVL_DVBT2_GI_19_256       =     6
}AVL_DVBT2_GUARD_INTERVAL;



typedef struct AVL_DVBT_RF_Table_Element
{
    AVL_DVBT_ModulationMode modulation;
    AVL_DVBT_CodeRate code_rate;
    AVL_int32 Nordig_RF_Ref_dbm;
}AVL_DVBT_RF_Table_Element;

typedef struct AVL_DVBT_BERSQI_List
{
    u32                 m_ber;
    u32                 m_ber_sqi;
}AVL_DVBT_BERSQI_List;

typedef struct AVL_DVBT_Non_Hierarchical_CN_Table_Element
{
    AVL_DVBT_ModulationMode modulation;
    AVL_DVBT_CodeRate hp_code_rate;
    AVL_int32 CN_NordigP1_x100_db;
}AVL_DVBT_Non_Hierarchical_CN_Table_Element;

typedef struct AVL_DVBT_Hierarchical_CN_Table_Element
{
    AVL_DVBT_Layer selected_layer;
    AVL_DVBT_ModulationMode modulation;
    AVL_DVBT_CodeRate code_rate;
    AVL_DVBT_Hierarchy hierarchy;
    AVL_int32 CN_NordigP1_x100_db;
}AVL_DVBT_Hierarchical_CN_Table_Element;

typedef struct DVBT2_CN_Table_Element
{
    AVL_DVBT2_PLP_ModulationMode modulation;
    AVL_DVBT2_CodeRate code_rate;
    AVL_int32 CN_NordigP1_x100_db;
}DVBT2_CN_Table_Element;

// DVBT2 pilot boosting correct CN table
typedef struct DVBT2_PBC_CN_Table_Element
{
    AVL_DVBT2_FFTSize fft_size;
    AVL_DVBT2_PILOT_PATTERN pilot_pattern;
    AVL_int32 PCB_CN;
}DVBT2_PBC_CN_Table_Element;
typedef struct AVL_DVBT2_RF_Table_Element
{
    AVL_DVBT2_PLP_ModulationMode modulation;
    AVL_DVBT2_CodeRate code_rate;
    AVL_int32 Nordig_RF_Ref_dbm;
}AVL_DVBT2_RF_Table_Element;





























AVL_DVBC_SQI_CN_Table_Element AVL_DVBC_CN_Table[]=
{
    //profile 1, AWGN
    {AVL_DVBC_J83A, AVL_DVBC_16QAM  , 1700}, 
    {AVL_DVBC_J83A, AVL_DVBC_32QAM  , 1980},
    {AVL_DVBC_J83A, AVL_DVBC_64QAM  , 2300}, 
    {AVL_DVBC_J83A, AVL_DVBC_128QAM , 2600}, 
    {AVL_DVBC_J83A, AVL_DVBC_256QAM , 2920},
    
    {AVL_DVBC_J83B, AVL_DVBC_64QAM ,  2180},
    {AVL_DVBC_J83B, AVL_DVBC_256QAM , 2810}
};

AVL_DVBT_Non_Hierarchical_CN_Table_Element AVL_DVBT_Non_Hierarchical_CN_Table[]=
{
    //profile 1, Gaussian
    {AVL_DVBT_QPSK, AVL_DVBT_CR_1_2, 510},
    {AVL_DVBT_QPSK, AVL_DVBT_CR_2_3, 690},
    {AVL_DVBT_QPSK, AVL_DVBT_CR_3_4, 790},
    {AVL_DVBT_QPSK, AVL_DVBT_CR_5_6, 890},
    {AVL_DVBT_QPSK, AVL_DVBT_CR_7_8, 970},

    {AVL_DVBT_16QAM, AVL_DVBT_CR_1_2, 1080},
    {AVL_DVBT_16QAM, AVL_DVBT_CR_2_3, 1310},
    {AVL_DVBT_16QAM, AVL_DVBT_CR_3_4, 1460},
    {AVL_DVBT_16QAM, AVL_DVBT_CR_5_6, 1560},
    {AVL_DVBT_16QAM, AVL_DVBT_CR_7_8, 1600},

    {AVL_DVBT_64QAM, AVL_DVBT_CR_1_2, 1650},
    {AVL_DVBT_64QAM, AVL_DVBT_CR_2_3, 1870},
    {AVL_DVBT_64QAM, AVL_DVBT_CR_3_4, 2020},
    {AVL_DVBT_64QAM, AVL_DVBT_CR_5_6, 2160},
    {AVL_DVBT_64QAM, AVL_DVBT_CR_7_8, 2250}
};



AVL_DVBT_Hierarchical_CN_Table_Element AVL_DVBT_Hierarchical_CN_Table[]=
{
    //profile 1, Gaussian
    //For HP, only QPSK is used

    //64QAM
    {AVL_DVBT_LAYER_HP, AVL_DVBT_64QAM, AVL_DVBT_CR_1_2,AVL_DVBT_HIER_ALPHA_1,1090},
    {AVL_DVBT_LAYER_LP, AVL_DVBT_64QAM, AVL_DVBT_CR_1_2,AVL_DVBT_HIER_ALPHA_1,1670},

    {AVL_DVBT_LAYER_HP, AVL_DVBT_64QAM, AVL_DVBT_CR_2_3,AVL_DVBT_HIER_ALPHA_1,1410},
    {AVL_DVBT_LAYER_LP, AVL_DVBT_64QAM, AVL_DVBT_CR_2_3,AVL_DVBT_HIER_ALPHA_1,1910},

    {AVL_DVBT_LAYER_HP, AVL_DVBT_64QAM, AVL_DVBT_CR_3_4,AVL_DVBT_HIER_ALPHA_1,1570},
    {AVL_DVBT_LAYER_LP, AVL_DVBT_64QAM, AVL_DVBT_CR_3_4,AVL_DVBT_HIER_ALPHA_1,2090},


    {AVL_DVBT_LAYER_HP, AVL_DVBT_64QAM, AVL_DVBT_CR_1_2,AVL_DVBT_HIER_ALPHA_2,850},
    {AVL_DVBT_LAYER_LP, AVL_DVBT_64QAM, AVL_DVBT_CR_1_2,AVL_DVBT_HIER_ALPHA_2,1850},

    {AVL_DVBT_LAYER_HP, AVL_DVBT_64QAM, AVL_DVBT_CR_2_3,AVL_DVBT_HIER_ALPHA_2,1100},
    {AVL_DVBT_LAYER_LP, AVL_DVBT_64QAM, AVL_DVBT_CR_2_3,AVL_DVBT_HIER_ALPHA_2,2120},

    {AVL_DVBT_LAYER_HP, AVL_DVBT_64QAM, AVL_DVBT_CR_3_4,AVL_DVBT_HIER_ALPHA_2,1280},
    {AVL_DVBT_LAYER_LP, AVL_DVBT_64QAM, AVL_DVBT_CR_3_4,AVL_DVBT_HIER_ALPHA_2,2360},

    //16QAM
    {AVL_DVBT_LAYER_HP, AVL_DVBT_16QAM, AVL_DVBT_CR_1_2,AVL_DVBT_HIER_ALPHA_2,680},
    {AVL_DVBT_LAYER_LP, AVL_DVBT_16QAM, AVL_DVBT_CR_1_2,AVL_DVBT_HIER_ALPHA_2,1500},

    {AVL_DVBT_LAYER_HP, AVL_DVBT_16QAM, AVL_DVBT_CR_2_3,AVL_DVBT_HIER_ALPHA_2,910},
    {AVL_DVBT_LAYER_LP, AVL_DVBT_16QAM, AVL_DVBT_CR_2_3,AVL_DVBT_HIER_ALPHA_2,1720},

    {AVL_DVBT_LAYER_HP, AVL_DVBT_16QAM, AVL_DVBT_CR_3_4,AVL_DVBT_HIER_ALPHA_2,1040},
    {AVL_DVBT_LAYER_LP, AVL_DVBT_16QAM, AVL_DVBT_CR_3_4,AVL_DVBT_HIER_ALPHA_2,1840},


    {AVL_DVBT_LAYER_HP, AVL_DVBT_16QAM, AVL_DVBT_CR_1_2,AVL_DVBT_HIER_ALPHA_4,580},
    {AVL_DVBT_LAYER_LP, AVL_DVBT_16QAM, AVL_DVBT_CR_1_2,AVL_DVBT_HIER_ALPHA_4,1950},

    {AVL_DVBT_LAYER_HP, AVL_DVBT_16QAM, AVL_DVBT_CR_2_3,AVL_DVBT_HIER_ALPHA_4,790},
    {AVL_DVBT_LAYER_LP, AVL_DVBT_16QAM, AVL_DVBT_CR_2_3,AVL_DVBT_HIER_ALPHA_4,2140},

    {AVL_DVBT_LAYER_HP, AVL_DVBT_16QAM, AVL_DVBT_CR_3_4,AVL_DVBT_HIER_ALPHA_4,910},
    {AVL_DVBT_LAYER_LP, AVL_DVBT_16QAM, AVL_DVBT_CR_3_4,AVL_DVBT_HIER_ALPHA_4,2250}
};


DVBT2_CN_Table_Element DVBT2_RAW_CN_Table[]=
{
    //profile 1, Gaussian
    {AVL_DVBT2_QPSK, AVL_DVBT2_CR_1_3, -120}, //from DVB-S2 std
    {AVL_DVBT2_QPSK, AVL_DVBT2_CR_2_5, -030}, //from DVB-S2 std
    {AVL_DVBT2_QPSK, AVL_DVBT2_CR_1_2, 100},
    {AVL_DVBT2_QPSK, AVL_DVBT2_CR_3_5, 220},
    {AVL_DVBT2_QPSK, AVL_DVBT2_CR_2_3, 310},
    {AVL_DVBT2_QPSK, AVL_DVBT2_CR_3_4, 410},
    {AVL_DVBT2_QPSK, AVL_DVBT2_CR_4_5, 470},
    {AVL_DVBT2_QPSK, AVL_DVBT2_CR_5_6, 520},

    {AVL_DVBT2_16QAM, AVL_DVBT2_CR_1_3, 370},
    {AVL_DVBT2_16QAM, AVL_DVBT2_CR_2_5, 490},
    {AVL_DVBT2_16QAM, AVL_DVBT2_CR_1_2, 620},
    {AVL_DVBT2_16QAM, AVL_DVBT2_CR_3_5, 760},
    {AVL_DVBT2_16QAM, AVL_DVBT2_CR_2_3, 890},
    {AVL_DVBT2_16QAM, AVL_DVBT2_CR_3_4, 1000},
    {AVL_DVBT2_16QAM, AVL_DVBT2_CR_4_5, 1080},
    {AVL_DVBT2_16QAM, AVL_DVBT2_CR_5_6, 1130},

    {AVL_DVBT2_64QAM, AVL_DVBT2_CR_1_3, 760},
    {AVL_DVBT2_64QAM, AVL_DVBT2_CR_2_5, 920},
    {AVL_DVBT2_64QAM, AVL_DVBT2_CR_1_2, 1050},
    {AVL_DVBT2_64QAM, AVL_DVBT2_CR_3_5, 1230},
    {AVL_DVBT2_64QAM, AVL_DVBT2_CR_2_3, 1360},
    {AVL_DVBT2_64QAM, AVL_DVBT2_CR_3_4, 1510},
    {AVL_DVBT2_64QAM, AVL_DVBT2_CR_4_5, 1610},
    {AVL_DVBT2_64QAM, AVL_DVBT2_CR_5_6, 1670},

    {AVL_DVBT2_256QAM, AVL_DVBT2_CR_1_3, 1110},
    {AVL_DVBT2_256QAM, AVL_DVBT2_CR_2_5, 1290},
    {AVL_DVBT2_256QAM, AVL_DVBT2_CR_1_2, 1440},
    {AVL_DVBT2_256QAM, AVL_DVBT2_CR_3_5, 1670},
    {AVL_DVBT2_256QAM, AVL_DVBT2_CR_2_3, 1810},
    {AVL_DVBT2_256QAM, AVL_DVBT2_CR_3_4, 2000},
    {AVL_DVBT2_256QAM, AVL_DVBT2_CR_4_5, 2130},
    {AVL_DVBT2_256QAM, AVL_DVBT2_CR_5_6, 2200}
};


DVBT2_PBC_CN_Table_Element DVBT2_PCB_CN_Table[]=
{
    {AVL_DVBT2_FFT_1K, AVL_DVBT2_PP_PP1, 34},
    {AVL_DVBT2_FFT_1K, AVL_DVBT2_PP_PP2, 32},
    {AVL_DVBT2_FFT_1K, AVL_DVBT2_PP_PP3, 44},
    {AVL_DVBT2_FFT_1K, AVL_DVBT2_PP_PP4, 42},
    {AVL_DVBT2_FFT_1K, AVL_DVBT2_PP_PP5, 48},
    {AVL_DVBT2_FFT_1K, AVL_DVBT2_PP_PP6, 0},
    {AVL_DVBT2_FFT_1K, AVL_DVBT2_PP_PP7, 29},
    {AVL_DVBT2_FFT_1K, AVL_DVBT2_PP_PP8, 0},

    {AVL_DVBT2_FFT_2K, AVL_DVBT2_PP_PP1, 35},
    {AVL_DVBT2_FFT_2K, AVL_DVBT2_PP_PP2, 33},
    {AVL_DVBT2_FFT_2K, AVL_DVBT2_PP_PP3, 43},
    {AVL_DVBT2_FFT_2K, AVL_DVBT2_PP_PP4, 42},
    {AVL_DVBT2_FFT_2K, AVL_DVBT2_PP_PP5, 47},
    {AVL_DVBT2_FFT_2K, AVL_DVBT2_PP_PP6, 0},
    {AVL_DVBT2_FFT_2K, AVL_DVBT2_PP_PP7, 29},
    {AVL_DVBT2_FFT_2K, AVL_DVBT2_PP_PP8, 0},

    {AVL_DVBT2_FFT_4K, AVL_DVBT2_PP_PP1, 39},
    {AVL_DVBT2_FFT_4K, AVL_DVBT2_PP_PP2, 37},
    {AVL_DVBT2_FFT_4K, AVL_DVBT2_PP_PP3, 47},
    {AVL_DVBT2_FFT_4K, AVL_DVBT2_PP_PP4, 45},
    {AVL_DVBT2_FFT_4K, AVL_DVBT2_PP_PP5, 51},
    {AVL_DVBT2_FFT_4K, AVL_DVBT2_PP_PP6, 0},
    {AVL_DVBT2_FFT_4K, AVL_DVBT2_PP_PP7, 34},
    {AVL_DVBT2_FFT_4K, AVL_DVBT2_PP_PP8, 0},

    {AVL_DVBT2_FFT_8K, AVL_DVBT2_PP_PP1, 41},
    {AVL_DVBT2_FFT_8K, AVL_DVBT2_PP_PP2, 39},
    {AVL_DVBT2_FFT_8K, AVL_DVBT2_PP_PP3, 49},
    {AVL_DVBT2_FFT_8K, AVL_DVBT2_PP_PP4, 48},
    {AVL_DVBT2_FFT_8K, AVL_DVBT2_PP_PP5, 53},
    {AVL_DVBT2_FFT_8K, AVL_DVBT2_PP_PP6, 0},
    {AVL_DVBT2_FFT_8K, AVL_DVBT2_PP_PP7, 37},
    {AVL_DVBT2_FFT_8K, AVL_DVBT2_PP_PP8, 37},

    {AVL_DVBT2_FFT_16K, AVL_DVBT2_PP_PP1, 41},
    {AVL_DVBT2_FFT_16K, AVL_DVBT2_PP_PP2, 38},
    {AVL_DVBT2_FFT_16K, AVL_DVBT2_PP_PP3, 49},
    {AVL_DVBT2_FFT_16K, AVL_DVBT2_PP_PP4, 47},
    {AVL_DVBT2_FFT_16K, AVL_DVBT2_PP_PP5, 52},
    {AVL_DVBT2_FFT_16K, AVL_DVBT2_PP_PP6, 49},
    {AVL_DVBT2_FFT_16K, AVL_DVBT2_PP_PP7, 33},
    {AVL_DVBT2_FFT_16K, AVL_DVBT2_PP_PP8, 35},

    {AVL_DVBT2_FFT_32K, AVL_DVBT2_PP_PP1, 0},
    {AVL_DVBT2_FFT_32K, AVL_DVBT2_PP_PP2, 37},
    {AVL_DVBT2_FFT_32K, AVL_DVBT2_PP_PP3, 48},
    {AVL_DVBT2_FFT_32K, AVL_DVBT2_PP_PP4, 45},
    {AVL_DVBT2_FFT_32K, AVL_DVBT2_PP_PP5, 0},
    {AVL_DVBT2_FFT_32K, AVL_DVBT2_PP_PP6, 48},
    {AVL_DVBT2_FFT_32K, AVL_DVBT2_PP_PP7, 33},
    {AVL_DVBT2_FFT_32K, AVL_DVBT2_PP_PP8, 35},
};

AVL_DVBT_BERSQI_List DVBT_BERSQI_Table[]=
{
    {100       ,    40  },
    {178       ,    45  },
    {316       ,    50  },
    {562       ,    55  },
    {1000      ,    60  },
    {1000      ,    60  },
    {1778      ,    65  },
    {3162      ,    70  },
    {5623      ,    75  },
    {10000     ,    80  },
    {17783     ,    85  },
    {31623     ,    90  },
    {56234     ,    95  },
    {100000    ,    100 },
    {177828    ,    105 },
    {316228    ,    110 },
    {562341    ,    115 },
    {1000000   ,    120 },
    {1778279   ,    125 },
    {3162278   ,    130 },
    {5623413   ,    135 },
    {10000000  ,    140 }
};



AVL_DVBT_RF_Table_Element AVL_DVBT_RF_TABLE[]=
{
    {AVL_DVBT_QPSK,AVL_DVBT_CR_1_2,-93},
    {AVL_DVBT_QPSK,AVL_DVBT_CR_2_3,-91},
    {AVL_DVBT_QPSK,AVL_DVBT_CR_3_4,-90},
    {AVL_DVBT_QPSK,AVL_DVBT_CR_5_6,-89},
    {AVL_DVBT_QPSK,AVL_DVBT_CR_7_8,-88},

    {AVL_DVBT_16QAM,AVL_DVBT_CR_1_2,-87},
    {AVL_DVBT_16QAM,AVL_DVBT_CR_2_3,-85},
    {AVL_DVBT_16QAM,AVL_DVBT_CR_3_4,-84},
    {AVL_DVBT_16QAM,AVL_DVBT_CR_5_6,-83},
    {AVL_DVBT_16QAM,AVL_DVBT_CR_7_8,-82},

    {AVL_DVBT_64QAM,AVL_DVBT_CR_1_2,-82},
    {AVL_DVBT_64QAM,AVL_DVBT_CR_2_3,-80},
    {AVL_DVBT_64QAM,AVL_DVBT_CR_3_4,-78},
    {AVL_DVBT_64QAM,AVL_DVBT_CR_5_6,-77},
    {AVL_DVBT_64QAM,AVL_DVBT_CR_7_8,-76}
};

AVL_DVBT2_RF_Table_Element AVL_DVBT2_RF_TABLE[]=
{
    {AVL_DVBT2_QPSK,AVL_DVBT2_CR_1_3,-101},
    {AVL_DVBT2_QPSK,AVL_DVBT2_CR_2_5,-100},
    {AVL_DVBT2_QPSK,AVL_DVBT2_CR_1_2,-96},
    {AVL_DVBT2_QPSK,AVL_DVBT2_CR_3_5,-95},
    {AVL_DVBT2_QPSK,AVL_DVBT2_CR_2_3,-94},
    {AVL_DVBT2_QPSK,AVL_DVBT2_CR_3_4,-93},
    {AVL_DVBT2_QPSK,AVL_DVBT2_CR_4_5,-92},
    {AVL_DVBT2_QPSK,AVL_DVBT2_CR_5_6,-92},

    {AVL_DVBT2_16QAM,AVL_DVBT2_CR_1_3,-96},
    {AVL_DVBT2_16QAM,AVL_DVBT2_CR_2_5,-95},
    {AVL_DVBT2_16QAM,AVL_DVBT2_CR_1_2,-91},
    {AVL_DVBT2_16QAM,AVL_DVBT2_CR_3_5,-89},
    {AVL_DVBT2_16QAM,AVL_DVBT2_CR_2_3,-88},
    {AVL_DVBT2_16QAM,AVL_DVBT2_CR_3_4,-87},
    {AVL_DVBT2_16QAM,AVL_DVBT2_CR_4_5,-86},
    {AVL_DVBT2_16QAM,AVL_DVBT2_CR_5_6,-86},

    {AVL_DVBT2_64QAM,AVL_DVBT2_CR_1_3,-93},
    {AVL_DVBT2_64QAM,AVL_DVBT2_CR_2_5,-92},
    {AVL_DVBT2_64QAM,AVL_DVBT2_CR_1_2,-86},
    {AVL_DVBT2_64QAM,AVL_DVBT2_CR_3_5,-85},
    {AVL_DVBT2_64QAM,AVL_DVBT2_CR_2_3,-83},
    {AVL_DVBT2_64QAM,AVL_DVBT2_CR_3_4,-82},
    {AVL_DVBT2_64QAM,AVL_DVBT2_CR_4_5,-81},
    {AVL_DVBT2_64QAM,AVL_DVBT2_CR_5_6,-80},

    {AVL_DVBT2_256QAM,AVL_DVBT2_CR_1_3,-89},
    {AVL_DVBT2_256QAM,AVL_DVBT2_CR_2_5,-88},
    {AVL_DVBT2_256QAM,AVL_DVBT2_CR_1_2,-82},
    {AVL_DVBT2_256QAM,AVL_DVBT2_CR_3_5,-80},
    {AVL_DVBT2_256QAM,AVL_DVBT2_CR_2_3,-78},
    {AVL_DVBT2_256QAM,AVL_DVBT2_CR_3_4,-76},
    {AVL_DVBT2_256QAM,AVL_DVBT2_CR_4_5,-75},
    {AVL_DVBT2_256QAM,AVL_DVBT2_CR_5_6,-74}
};





#endif

