// $Module: isp_wdma $
// $RegisterBank Version: V 1.0.00 $
// $Author: brian $
// $Date: Sun, 26 Sep 2021 04:20:44 PM $
//

#ifndef __REG_ISP_WDMA_STRUCT_H__
#define __REG_ISP_WDMA_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*Shadow reg read select
		0 : read active reg
		1 : read shadow reg;*/
		uint32_t reg_shadow_rd_sel:1;
		/*;*/
		uint32_t reg_abort_mode:1;
	};
	uint32_t val;
} isp_wdma_shadow_rd_sel_c;
// typedef union {
// 	struct {
// 		/*Disable IP;*/
// 		uint32_t reg_ip_disable:32;
// 	};
// 	uint32_t val;
// } ISP_WDMA_IP_DISABLE_C;
typedef union {
	struct {
		/*Disable seglen function and use ip_wlast to finish write dma;*/
		uint32_t reg_seglen_disable:32;
	};
	uint32_t val;
} isp_wdma_disable_seglen_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_up_ring_base:32;
	};
	uint32_t val;
} isp_wdma_up_ring_base_c;
typedef union {
	struct {
		/*Abort done flag;*/
		uint32_t reg_abort_done:1;
		uint32_t rsv_1_3:3;
		/*Error flag : AXI response error;*/
		uint32_t reg_error_axi:1;
		/*Error flag : DMI transfer size mismatch;*/
		uint32_t reg_error_dmi:1;
		/*Warning flag : WDMA Slot buffer full;*/
		uint32_t reg_slot_full:1;
		uint32_t rsv_7_7:1;
		/*Error client ID;*/
		uint32_t reg_error_id:5;
		uint32_t rsv_13_15:3;
		/*Date of latest update;*/
		uint32_t reg_dma_version:16;
	};
	uint32_t val;
} isp_wdma_norm_status0_c;
typedef union {
	struct {
		/*reg_id_idle[id] : done status of client id
		(WDMA has received all data from the client);*/
		uint32_t reg_id_idle:32;
	};
	uint32_t val;
} isp_wdma_norm_status1_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_ring_enable:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_en_c;
typedef union {
	struct {
		/*Bandwidth limiter window size;*/
		uint32_t reg_bwlwin:10;
		/*Bandwidth limiter transaction number;*/
		uint32_t reg_bwltxn:6;
		/*QoS only mode threshold;*/
		uint32_t reg_qoso_th:4;
		/*QoS only mode enable;*/
		uint32_t reg_qoso_en:1;
	};
	uint32_t val;
} isp_wdma_norm_perf_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_ring_patch_enable:32;
	};
	uint32_t val;
} isp_wdma_ring_patch_enable_c;
typedef union {
	struct {
		/*it's work if reg_ring_patch_enable = 1;*/
		uint32_t reg_set_ring_base:32;
	};
	uint32_t val;
} isp_wdma_set_ring_base_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_ring_base_l:32;
	};
	uint32_t val;
} isp_wdma_ring_base_addr_l_c;
typedef union {
	struct {
		/*;*/
		uint32_t reg_ring_base_h:8;
	};
	uint32_t val;
} isp_wdma_ring_base_addr_h_c;
typedef union {
	struct {
		/*ring buffer size for client0;*/
		uint32_t reg_rbuf_size0:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size0_c;
typedef union {
	struct {
		/*ring buffer size for client1;*/
		uint32_t reg_rbuf_size1:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size1_c;
typedef union {
	struct {
		/*ring buffer size for client2;*/
		uint32_t reg_rbuf_size2:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size2_c;
typedef union {
	struct {
		/*ring buffer size for client3;*/
		uint32_t reg_rbuf_size3:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size3_c;
typedef union {
	struct {
		/*ring buffer size for client4;*/
		uint32_t reg_rbuf_size4:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size4_c;
typedef union {
	struct {
		/*ring buffer size for client5;*/
		uint32_t reg_rbuf_size5:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size5_c;
typedef union {
	struct {
		/*ring buffer size for client6;*/
		uint32_t reg_rbuf_size6:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size6_c;
typedef union {
	struct {
		/*ring buffer size for client7;*/
		uint32_t reg_rbuf_size7:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size7_c;
typedef union {
	struct {
		/*ring buffer size for client8;*/
		uint32_t reg_rbuf_size8:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size8_c;
typedef union {
	struct {
		/*ring buffer size for client9;*/
		uint32_t reg_rbuf_size9:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size9_c;
typedef union {
	struct {
		/*ring buffer size for client10;*/
		uint32_t reg_rbuf_size10:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size10_c;
typedef union {
	struct {
		/*ring buffer size for client11;*/
		uint32_t reg_rbuf_size11:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size11_c;
typedef union {
	struct {
		/*ring buffer size for client12;*/
		uint32_t reg_rbuf_size12:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size12_c;
typedef union {
	struct {
		/*ring buffer size for client13;*/
		uint32_t reg_rbuf_size13:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size13_c;
typedef union {
	struct {
		/*ring buffer size for client14;*/
		uint32_t reg_rbuf_size14:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size14_c;
typedef union {
	struct {
		/*ring buffer size for client15;*/
		uint32_t reg_rbuf_size15:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size15_c;
typedef union {
	struct {
		/*ring buffer size for client16;*/
		uint32_t reg_rbuf_size16:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size16_c;
typedef union {
	struct {
		/*ring buffer size for client17;*/
		uint32_t reg_rbuf_size17:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size17_c;
typedef union {
	struct {
		/*ring buffer size for client18;*/
		uint32_t reg_rbuf_size18:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size18_c;
typedef union {
	struct {
		/*ring buffer size for client19;*/
		uint32_t reg_rbuf_size19:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size19_c;
typedef union {
	struct {
		/*ring buffer size for client20;*/
		uint32_t reg_rbuf_size20:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size20_c;
typedef union {
	struct {
		/*ring buffer size for client21;*/
		uint32_t reg_rbuf_size21:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size21_c;
typedef union {
	struct {
		/*ring buffer size for client22;*/
		uint32_t reg_rbuf_size22:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size22_c;
typedef union {
	struct {
		/*ring buffer size for client23;*/
		uint32_t reg_rbuf_size23:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size23_c;
typedef union {
	struct {
		/*ring buffer size for client24;*/
		uint32_t reg_rbuf_size24:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size24_c;
typedef union {
	struct {
		/*ring buffer size for client25;*/
		uint32_t reg_rbuf_size25:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size25_c;
typedef union {
	struct {
		/*ring buffer size for client26;*/
		uint32_t reg_rbuf_size26:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size26_c;
typedef union {
	struct {
		/*ring buffer size for client27;*/
		uint32_t reg_rbuf_size27:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size27_c;
typedef union {
	struct {
		/*ring buffer size for client28;*/
		uint32_t reg_rbuf_size28:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size28_c;
typedef union {
	struct {
		/*ring buffer size for client29;*/
		uint32_t reg_rbuf_size29:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size29_c;
typedef union {
	struct {
		/*ring buffer size for client30;*/
		uint32_t reg_rbuf_size30:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size30_c;
typedef union {
	struct {
		/*ring buffer size for client31;*/
		uint32_t reg_rbuf_size31:32;
	};
	uint32_t val;
} isp_wdma_ring_buffer_size31_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr0:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts0_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr1:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts1_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr2:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts2_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr3:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts3_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr4:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts4_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr5:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts5_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr6:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts6_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr7:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts7_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr8:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts8_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr9:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts9_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr10:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts10_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr11:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts11_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr12:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts12_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr13:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts13_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr14:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts14_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr15:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts15_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr16:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts16_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr17:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts17_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr18:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts18_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr19:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts19_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr20:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts20_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr21:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts21_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr22:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts22_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr23:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts23_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr24:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts24_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr25:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts25_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr26:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts26_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr27:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts27_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr28:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts28_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr29:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts29_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr30:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts30_c;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr31:32;
	};
	uint32_t val;
} isp_wdma_next_dma_addr_sts31_c;
typedef struct {
	volatile isp_wdma_shadow_rd_sel_c shadow_rd_sel;
	volatile uint32_t _disable_seglen; // 0x04
	volatile isp_wdma_disable_seglen_c disable_seglen;
	volatile isp_wdma_up_ring_base_c up_ring_base;
	volatile isp_wdma_norm_status0_c norm_status0;
	volatile isp_wdma_norm_status1_c norm_status1;
	volatile isp_wdma_ring_buffer_en_c ring_buffer_en;
	volatile isp_wdma_norm_perf_c norm_perf;
	volatile isp_wdma_ring_patch_enable_c ring_patch_enable;
	volatile isp_wdma_set_ring_base_c set_ring_base;
	volatile isp_wdma_ring_base_addr_l_c ring_base_addr_l;
	volatile isp_wdma_ring_base_addr_h_c ring_base_addr_h;
	volatile uint32_t _ring_buffer_size0; // 0x38
	volatile uint32_t _ring_buffer_size1; // 0x3c
	volatile uint32_t _ring_buffer_size2; // 0x40
	volatile uint32_t _ring_buffer_size3; // 0x44
	volatile uint32_t _ring_buffer_size4; // 0x48
	volatile uint32_t _ring_buffer_size5; // 0x4c
	volatile uint32_t _ring_buffer_size6; // 0x50
	volatile uint32_t _ring_buffer_size7; // 0x54
	volatile uint32_t _ring_buffer_size8; // 0x58
	volatile uint32_t _ring_buffer_size9; // 0x5c
	volatile uint32_t _ring_buffer_size10; // 0x60
	volatile uint32_t _ring_buffer_size11; // 0x64
	volatile uint32_t _ring_buffer_size12; // 0x68
	volatile uint32_t _ring_buffer_size13; // 0x6c
	volatile uint32_t _ring_buffer_size14; // 0x70
	volatile uint32_t _ring_buffer_size15; // 0x74
	volatile uint32_t _ring_buffer_size16; // 0x78
	volatile uint32_t _ring_buffer_size17; // 0x7c
	volatile isp_wdma_ring_buffer_size0_c ring_buffer_size0;
	volatile isp_wdma_ring_buffer_size1_c ring_buffer_size1;
	volatile isp_wdma_ring_buffer_size2_c ring_buffer_size2;
	volatile isp_wdma_ring_buffer_size3_c ring_buffer_size3;
	volatile isp_wdma_ring_buffer_size4_c ring_buffer_size4;
	volatile isp_wdma_ring_buffer_size5_c ring_buffer_size5;
	volatile isp_wdma_ring_buffer_size6_c ring_buffer_size6;
	volatile isp_wdma_ring_buffer_size7_c ring_buffer_size7;
	volatile isp_wdma_ring_buffer_size8_c ring_buffer_size8;
	volatile isp_wdma_ring_buffer_size9_c ring_buffer_size9;
	volatile isp_wdma_ring_buffer_size10_c ring_buffer_size10;
	volatile isp_wdma_ring_buffer_size11_c ring_buffer_size11;
	volatile isp_wdma_ring_buffer_size12_c ring_buffer_size12;
	volatile isp_wdma_ring_buffer_size13_c ring_buffer_size13;
	volatile isp_wdma_ring_buffer_size14_c ring_buffer_size14;
	volatile isp_wdma_ring_buffer_size15_c ring_buffer_size15;
	volatile isp_wdma_ring_buffer_size16_c ring_buffer_size16;
	volatile isp_wdma_ring_buffer_size17_c ring_buffer_size17;
	volatile isp_wdma_ring_buffer_size18_c ring_buffer_size18;
	volatile isp_wdma_ring_buffer_size19_c ring_buffer_size19;
	volatile isp_wdma_ring_buffer_size20_c ring_buffer_size20;
	volatile isp_wdma_ring_buffer_size21_c ring_buffer_size21;
	volatile isp_wdma_ring_buffer_size22_c ring_buffer_size22;
	volatile isp_wdma_ring_buffer_size23_c ring_buffer_size23;
	volatile isp_wdma_ring_buffer_size24_c ring_buffer_size24;
	volatile isp_wdma_ring_buffer_size25_c ring_buffer_size25;
	volatile isp_wdma_ring_buffer_size26_c ring_buffer_size26;
	volatile isp_wdma_ring_buffer_size27_c ring_buffer_size27;
	volatile isp_wdma_ring_buffer_size28_c ring_buffer_size28;
	volatile isp_wdma_ring_buffer_size29_c ring_buffer_size29;
	volatile isp_wdma_ring_buffer_size30_c ring_buffer_size30;
	volatile isp_wdma_ring_buffer_size31_c ring_buffer_size31;
	volatile isp_wdma_next_dma_addr_sts0_c next_dma_addr_sts0;
	volatile isp_wdma_next_dma_addr_sts1_c next_dma_addr_sts1;
	volatile isp_wdma_next_dma_addr_sts2_c next_dma_addr_sts2;
	volatile isp_wdma_next_dma_addr_sts3_c next_dma_addr_sts3;
	volatile isp_wdma_next_dma_addr_sts4_c next_dma_addr_sts4;
	volatile isp_wdma_next_dma_addr_sts5_c next_dma_addr_sts5;
	volatile isp_wdma_next_dma_addr_sts6_c next_dma_addr_sts6;
	volatile isp_wdma_next_dma_addr_sts7_c next_dma_addr_sts7;
	volatile isp_wdma_next_dma_addr_sts8_c next_dma_addr_sts8;
	volatile isp_wdma_next_dma_addr_sts9_c next_dma_addr_sts9;
	volatile isp_wdma_next_dma_addr_sts10_c next_dma_addr_sts10;
	volatile isp_wdma_next_dma_addr_sts11_c next_dma_addr_sts11;
	volatile isp_wdma_next_dma_addr_sts12_c next_dma_addr_sts12;
	volatile isp_wdma_next_dma_addr_sts13_c next_dma_addr_sts13;
	volatile isp_wdma_next_dma_addr_sts14_c next_dma_addr_sts14;
	volatile isp_wdma_next_dma_addr_sts15_c next_dma_addr_sts15;
	volatile isp_wdma_next_dma_addr_sts16_c next_dma_addr_sts16;
	volatile isp_wdma_next_dma_addr_sts17_c next_dma_addr_sts17;
	volatile isp_wdma_next_dma_addr_sts18_c next_dma_addr_sts18;
	volatile isp_wdma_next_dma_addr_sts19_c next_dma_addr_sts19;
	volatile isp_wdma_next_dma_addr_sts20_c next_dma_addr_sts20;
	volatile isp_wdma_next_dma_addr_sts21_c next_dma_addr_sts21;
	volatile isp_wdma_next_dma_addr_sts22_c next_dma_addr_sts22;
	volatile isp_wdma_next_dma_addr_sts23_c next_dma_addr_sts23;
	volatile isp_wdma_next_dma_addr_sts24_c next_dma_addr_sts24;
	volatile isp_wdma_next_dma_addr_sts25_c next_dma_addr_sts25;
	volatile isp_wdma_next_dma_addr_sts26_c next_dma_addr_sts26;
	volatile isp_wdma_next_dma_addr_sts27_c next_dma_addr_sts27;
	volatile isp_wdma_next_dma_addr_sts28_c next_dma_addr_sts28;
	volatile isp_wdma_next_dma_addr_sts29_c next_dma_addr_sts29;
	volatile isp_wdma_next_dma_addr_sts30_c next_dma_addr_sts30;
	volatile isp_wdma_next_dma_addr_sts31_c next_dma_addr_sts31;
} isp_wdma_c;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void isp_wdma_dump_ini(FILE* fp, isp_wdma_c* p) {
	fprintf(fp, "reg_shadow_rd_sel = 0x%x\n",p->shadow_rd_sel.reg_shadow_rd_sel);
	fprintf(fp, "reg_abort_mode = 0x%x\n",p->shadow_rd_sel.reg_abort_mode);
	// fprintf(fp, "reg_ip_disable = 0x%x\n",p->IP_DISABLE.reg_ip_disable);
	fprintf(fp, "reg_seglen_disable = 0x%x\n",p->disable_seglen.reg_seglen_disable);
	fprintf(fp, "reg_up_ring_base = 0x%x\n",p->up_ring_base.reg_up_ring_base);
	fprintf(fp, "reg_abort_done = 0x%x\n",p->norm_status0.reg_abort_done);
	fprintf(fp, "reg_error_axi = 0x%x\n",p->norm_status0.reg_error_axi);
	fprintf(fp, "reg_error_dmi = 0x%x\n",p->norm_status0.reg_error_dmi);
	fprintf(fp, "reg_slot_full = 0x%x\n",p->norm_status0.reg_slot_full);
	fprintf(fp, "reg_error_id = 0x%x\n",p->norm_status0.reg_error_id);
	fprintf(fp, "reg_dma_version = 0x%x\n",p->norm_status0.reg_dma_version);
	fprintf(fp, "reg_id_idle = 0x%x\n",p->norm_status1.reg_id_idle);
	fprintf(fp, "reg_ring_enable = 0x%x\n",p->ring_buffer_en.reg_ring_enable);
	fprintf(fp, "reg_bwlwin = 0x%x\n",p->norm_perf.reg_bwlwin);
	fprintf(fp, "reg_bwltxn = 0x%x\n",p->norm_perf.reg_bwltxn);
	fprintf(fp, "reg_qoso_th = 0x%x\n",p->norm_perf.reg_qoso_th);
	fprintf(fp, "reg_qoso_en = 0x%x\n",p->norm_perf.reg_qoso_en);
	fprintf(fp, "reg_ring_patch_enable = 0x%x\n",p->ring_patch_enable.reg_ring_patch_enable);
	fprintf(fp, "reg_set_ring_base = 0x%x\n",p->set_ring_base.reg_set_ring_base);
	fprintf(fp, "reg_ring_base_l = 0x%x\n",p->ring_base_addr_l.reg_ring_base_l);
	fprintf(fp, "reg_ring_base_h = 0x%x\n",p->ring_base_addr_h.reg_ring_base_h);
	fprintf(fp, "reg_rbuf_size0 = 0x%x\n",p->ring_buffer_size0.reg_rbuf_size0);
	fprintf(fp, "reg_rbuf_size1 = 0x%x\n",p->ring_buffer_size1.reg_rbuf_size1);
	fprintf(fp, "reg_rbuf_size2 = 0x%x\n",p->ring_buffer_size2.reg_rbuf_size2);
	fprintf(fp, "reg_rbuf_size3 = 0x%x\n",p->ring_buffer_size3.reg_rbuf_size3);
	fprintf(fp, "reg_rbuf_size4 = 0x%x\n",p->ring_buffer_size4.reg_rbuf_size4);
	fprintf(fp, "reg_rbuf_size5 = 0x%x\n",p->ring_buffer_size5.reg_rbuf_size5);
	fprintf(fp, "reg_rbuf_size6 = 0x%x\n",p->ring_buffer_size6.reg_rbuf_size6);
	fprintf(fp, "reg_rbuf_size7 = 0x%x\n",p->ring_buffer_size7.reg_rbuf_size7);
	fprintf(fp, "reg_rbuf_size8 = 0x%x\n",p->ring_buffer_size8.reg_rbuf_size8);
	fprintf(fp, "reg_rbuf_size9 = 0x%x\n",p->ring_buffer_size9.reg_rbuf_size9);
	fprintf(fp, "reg_rbuf_size10 = 0x%x\n",p->ring_buffer_size10.reg_rbuf_size10);
	fprintf(fp, "reg_rbuf_size11 = 0x%x\n",p->ring_buffer_size11.reg_rbuf_size11);
	fprintf(fp, "reg_rbuf_size12 = 0x%x\n",p->ring_buffer_size12.reg_rbuf_size12);
	fprintf(fp, "reg_rbuf_size13 = 0x%x\n",p->ring_buffer_size13.reg_rbuf_size13);
	fprintf(fp, "reg_rbuf_size14 = 0x%x\n",p->ring_buffer_size14.reg_rbuf_size14);
	fprintf(fp, "reg_rbuf_size15 = 0x%x\n",p->ring_buffer_size15.reg_rbuf_size15);
	fprintf(fp, "reg_rbuf_size16 = 0x%x\n",p->ring_buffer_size16.reg_rbuf_size16);
	fprintf(fp, "reg_rbuf_size17 = 0x%x\n",p->ring_buffer_size17.reg_rbuf_size17);
	fprintf(fp, "reg_rbuf_size18 = 0x%x\n",p->ring_buffer_size18.reg_rbuf_size18);
	fprintf(fp, "reg_rbuf_size19 = 0x%x\n",p->ring_buffer_size19.reg_rbuf_size19);
	fprintf(fp, "reg_rbuf_size20 = 0x%x\n",p->ring_buffer_size20.reg_rbuf_size20);
	fprintf(fp, "reg_rbuf_size21 = 0x%x\n",p->ring_buffer_size21.reg_rbuf_size21);
	fprintf(fp, "reg_rbuf_size22 = 0x%x\n",p->ring_buffer_size22.reg_rbuf_size22);
	fprintf(fp, "reg_rbuf_size23 = 0x%x\n",p->ring_buffer_size23.reg_rbuf_size23);
	fprintf(fp, "reg_rbuf_size24 = 0x%x\n",p->ring_buffer_size24.reg_rbuf_size24);
	fprintf(fp, "reg_rbuf_size25 = 0x%x\n",p->ring_buffer_size25.reg_rbuf_size25);
	fprintf(fp, "reg_rbuf_size26 = 0x%x\n",p->ring_buffer_size26.reg_rbuf_size26);
	fprintf(fp, "reg_rbuf_size27 = 0x%x\n",p->ring_buffer_size27.reg_rbuf_size27);
	fprintf(fp, "reg_rbuf_size28 = 0x%x\n",p->ring_buffer_size28.reg_rbuf_size28);
	fprintf(fp, "reg_rbuf_size29 = 0x%x\n",p->ring_buffer_size29.reg_rbuf_size29);
	fprintf(fp, "reg_rbuf_size30 = 0x%x\n",p->ring_buffer_size30.reg_rbuf_size30);
	fprintf(fp, "reg_rbuf_size31 = 0x%x\n",p->ring_buffer_size31.reg_rbuf_size31);
	fprintf(fp, "reg_next_dma_addr0 = 0x%x\n",p->next_dma_addr_sts0.reg_next_dma_addr0);
	fprintf(fp, "reg_next_dma_addr1 = 0x%x\n",p->next_dma_addr_sts1.reg_next_dma_addr1);
	fprintf(fp, "reg_next_dma_addr2 = 0x%x\n",p->next_dma_addr_sts2.reg_next_dma_addr2);
	fprintf(fp, "reg_next_dma_addr3 = 0x%x\n",p->next_dma_addr_sts3.reg_next_dma_addr3);
	fprintf(fp, "reg_next_dma_addr4 = 0x%x\n",p->next_dma_addr_sts4.reg_next_dma_addr4);
	fprintf(fp, "reg_next_dma_addr5 = 0x%x\n",p->next_dma_addr_sts5.reg_next_dma_addr5);
	fprintf(fp, "reg_next_dma_addr6 = 0x%x\n",p->next_dma_addr_sts6.reg_next_dma_addr6);
	fprintf(fp, "reg_next_dma_addr7 = 0x%x\n",p->next_dma_addr_sts7.reg_next_dma_addr7);
	fprintf(fp, "reg_next_dma_addr8 = 0x%x\n",p->next_dma_addr_sts8.reg_next_dma_addr8);
	fprintf(fp, "reg_next_dma_addr9 = 0x%x\n",p->next_dma_addr_sts9.reg_next_dma_addr9);
	fprintf(fp, "reg_next_dma_addr10 = 0x%x\n",p->next_dma_addr_sts10.reg_next_dma_addr10);
	fprintf(fp, "reg_next_dma_addr11 = 0x%x\n",p->next_dma_addr_sts11.reg_next_dma_addr11);
	fprintf(fp, "reg_next_dma_addr12 = 0x%x\n",p->next_dma_addr_sts12.reg_next_dma_addr12);
	fprintf(fp, "reg_next_dma_addr13 = 0x%x\n",p->next_dma_addr_sts13.reg_next_dma_addr13);
	fprintf(fp, "reg_next_dma_addr14 = 0x%x\n",p->next_dma_addr_sts14.reg_next_dma_addr14);
	fprintf(fp, "reg_next_dma_addr15 = 0x%x\n",p->next_dma_addr_sts15.reg_next_dma_addr15);
	fprintf(fp, "reg_next_dma_addr16 = 0x%x\n",p->next_dma_addr_sts16.reg_next_dma_addr16);
	fprintf(fp, "reg_next_dma_addr17 = 0x%x\n",p->next_dma_addr_sts17.reg_next_dma_addr17);
	fprintf(fp, "reg_next_dma_addr18 = 0x%x\n",p->next_dma_addr_sts18.reg_next_dma_addr18);
	fprintf(fp, "reg_next_dma_addr19 = 0x%x\n",p->next_dma_addr_sts19.reg_next_dma_addr19);
	fprintf(fp, "reg_next_dma_addr20 = 0x%x\n",p->next_dma_addr_sts20.reg_next_dma_addr20);
	fprintf(fp, "reg_next_dma_addr21 = 0x%x\n",p->next_dma_addr_sts21.reg_next_dma_addr21);
	fprintf(fp, "reg_next_dma_addr22 = 0x%x\n",p->next_dma_addr_sts22.reg_next_dma_addr22);
	fprintf(fp, "reg_next_dma_addr23 = 0x%x\n",p->next_dma_addr_sts23.reg_next_dma_addr23);
	fprintf(fp, "reg_next_dma_addr24 = 0x%x\n",p->next_dma_addr_sts24.reg_next_dma_addr24);
	fprintf(fp, "reg_next_dma_addr25 = 0x%x\n",p->next_dma_addr_sts25.reg_next_dma_addr25);
	fprintf(fp, "reg_next_dma_addr26 = 0x%x\n",p->next_dma_addr_sts26.reg_next_dma_addr26);
	fprintf(fp, "reg_next_dma_addr27 = 0x%x\n",p->next_dma_addr_sts27.reg_next_dma_addr27);
	fprintf(fp, "reg_next_dma_addr28 = 0x%x\n",p->next_dma_addr_sts28.reg_next_dma_addr28);
	fprintf(fp, "reg_next_dma_addr29 = 0x%x\n",p->next_dma_addr_sts29.reg_next_dma_addr29);
	fprintf(fp, "reg_next_dma_addr30 = 0x%x\n",p->next_dma_addr_sts30.reg_next_dma_addr30);
	fprintf(fp, "reg_next_dma_addr31 = 0x%x\n",p->next_dma_addr_sts31.reg_next_dma_addr31);
}
static void isp_wdma_print(isp_wdma_c* p) {
    fprintf(stderr, "isp_wdma\n");
	fprintf(stderr, "\tSHADOW_RD_SEL.reg_shadow_rd_sel = 0x%x\n", p->shadow_rd_sel.reg_shadow_rd_sel);
	fprintf(stderr, "\tSHADOW_RD_SEL.reg_abort_mode = 0x%x\n", p->shadow_rd_sel.reg_abort_mode);
	// fprintf(stderr, "\tIP_DISABLE.reg_ip_disable = 0x%x\n", p->IP_DISABLE.reg_ip_disable);
	fprintf(stderr, "\tDISABLE_SEGLEN.reg_seglen_disable = 0x%x\n", p->disable_seglen.reg_seglen_disable);
	fprintf(stderr, "\tUP_RING_BASE.reg_up_ring_base = 0x%x\n", p->up_ring_base.reg_up_ring_base);
	fprintf(stderr, "\tNORM_STATUS0.reg_abort_done = 0x%x\n", p->norm_status0.reg_abort_done);
	fprintf(stderr, "\tNORM_STATUS0.reg_error_axi = 0x%x\n", p->norm_status0.reg_error_axi);
	fprintf(stderr, "\tNORM_STATUS0.reg_error_dmi = 0x%x\n", p->norm_status0.reg_error_dmi);
	fprintf(stderr, "\tNORM_STATUS0.reg_slot_full = 0x%x\n", p->norm_status0.reg_slot_full);
	fprintf(stderr, "\tNORM_STATUS0.reg_error_id = 0x%x\n", p->norm_status0.reg_error_id);
	fprintf(stderr, "\tNORM_STATUS0.reg_dma_version = 0x%x\n", p->norm_status0.reg_dma_version);
	fprintf(stderr, "\tNORM_STATUS1.reg_id_idle = 0x%x\n", p->norm_status1.reg_id_idle);
	fprintf(stderr, "\tRING_BUFFER_EN.reg_ring_enable = 0x%x\n", p->ring_buffer_en.reg_ring_enable);
	fprintf(stderr, "\tNORM_PERF.reg_bwlwin = 0x%x\n", p->norm_perf.reg_bwlwin);
	fprintf(stderr, "\tNORM_PERF.reg_bwltxn = 0x%x\n", p->norm_perf.reg_bwltxn);
	fprintf(stderr, "\tNORM_PERF.reg_qoso_th = 0x%x\n", p->norm_perf.reg_qoso_th);
	fprintf(stderr, "\tNORM_PERF.reg_qoso_en = 0x%x\n", p->norm_perf.reg_qoso_en);
	fprintf(stderr, "\tRING_PATCH_ENABLE.reg_ring_patch_enable = 0x%x\n", p->ring_patch_enable.reg_ring_patch_enable);
	fprintf(stderr, "\tSET_RING_BASE.reg_set_ring_base = 0x%x\n", p->set_ring_base.reg_set_ring_base);
	fprintf(stderr, "\tRING_BASE_ADDR_L.reg_ring_base_l = 0x%x\n", p->ring_base_addr_l.reg_ring_base_l);
	fprintf(stderr, "\tRING_BASE_ADDR_H.reg_ring_base_h = 0x%x\n", p->ring_base_addr_h.reg_ring_base_h);
	fprintf(stderr, "\tRING_BUFFER_SIZE0.reg_rbuf_size0 = 0x%x\n", p->ring_buffer_size0.reg_rbuf_size0);
	fprintf(stderr, "\tRING_BUFFER_SIZE1.reg_rbuf_size1 = 0x%x\n", p->ring_buffer_size1.reg_rbuf_size1);
	fprintf(stderr, "\tRING_BUFFER_SIZE2.reg_rbuf_size2 = 0x%x\n", p->ring_buffer_size2.reg_rbuf_size2);
	fprintf(stderr, "\tRING_BUFFER_SIZE3.reg_rbuf_size3 = 0x%x\n", p->ring_buffer_size3.reg_rbuf_size3);
	fprintf(stderr, "\tRING_BUFFER_SIZE4.reg_rbuf_size4 = 0x%x\n", p->ring_buffer_size4.reg_rbuf_size4);
	fprintf(stderr, "\tRING_BUFFER_SIZE5.reg_rbuf_size5 = 0x%x\n", p->ring_buffer_size5.reg_rbuf_size5);
	fprintf(stderr, "\tRING_BUFFER_SIZE6.reg_rbuf_size6 = 0x%x\n", p->ring_buffer_size6.reg_rbuf_size6);
	fprintf(stderr, "\tRING_BUFFER_SIZE7.reg_rbuf_size7 = 0x%x\n", p->ring_buffer_size7.reg_rbuf_size7);
	fprintf(stderr, "\tRING_BUFFER_SIZE8.reg_rbuf_size8 = 0x%x\n", p->ring_buffer_size8.reg_rbuf_size8);
	fprintf(stderr, "\tRING_BUFFER_SIZE9.reg_rbuf_size9 = 0x%x\n", p->ring_buffer_size9.reg_rbuf_size9);
	fprintf(stderr, "\tRING_BUFFER_SIZE10.reg_rbuf_size10 = 0x%x\n", p->ring_buffer_size10.reg_rbuf_size10);
	fprintf(stderr, "\tRING_BUFFER_SIZE11.reg_rbuf_size11 = 0x%x\n", p->ring_buffer_size11.reg_rbuf_size11);
	fprintf(stderr, "\tRING_BUFFER_SIZE12.reg_rbuf_size12 = 0x%x\n", p->ring_buffer_size12.reg_rbuf_size12);
	fprintf(stderr, "\tRING_BUFFER_SIZE13.reg_rbuf_size13 = 0x%x\n", p->ring_buffer_size13.reg_rbuf_size13);
	fprintf(stderr, "\tRING_BUFFER_SIZE14.reg_rbuf_size14 = 0x%x\n", p->ring_buffer_size14.reg_rbuf_size14);
	fprintf(stderr, "\tRING_BUFFER_SIZE15.reg_rbuf_size15 = 0x%x\n", p->ring_buffer_size15.reg_rbuf_size15);
	fprintf(stderr, "\tRING_BUFFER_SIZE16.reg_rbuf_size16 = 0x%x\n", p->ring_buffer_size16.reg_rbuf_size16);
	fprintf(stderr, "\tRING_BUFFER_SIZE17.reg_rbuf_size17 = 0x%x\n", p->ring_buffer_size17.reg_rbuf_size17);
	fprintf(stderr, "\tRING_BUFFER_SIZE18.reg_rbuf_size18 = 0x%x\n", p->ring_buffer_size18.reg_rbuf_size18);
	fprintf(stderr, "\tRING_BUFFER_SIZE19.reg_rbuf_size19 = 0x%x\n", p->ring_buffer_size19.reg_rbuf_size19);
	fprintf(stderr, "\tRING_BUFFER_SIZE20.reg_rbuf_size20 = 0x%x\n", p->ring_buffer_size20.reg_rbuf_size20);
	fprintf(stderr, "\tRING_BUFFER_SIZE21.reg_rbuf_size21 = 0x%x\n", p->ring_buffer_size21.reg_rbuf_size21);
	fprintf(stderr, "\tRING_BUFFER_SIZE22.reg_rbuf_size22 = 0x%x\n", p->ring_buffer_size22.reg_rbuf_size22);
	fprintf(stderr, "\tRING_BUFFER_SIZE23.reg_rbuf_size23 = 0x%x\n", p->ring_buffer_size23.reg_rbuf_size23);
	fprintf(stderr, "\tRING_BUFFER_SIZE24.reg_rbuf_size24 = 0x%x\n", p->ring_buffer_size24.reg_rbuf_size24);
	fprintf(stderr, "\tRING_BUFFER_SIZE25.reg_rbuf_size25 = 0x%x\n", p->ring_buffer_size25.reg_rbuf_size25);
	fprintf(stderr, "\tRING_BUFFER_SIZE26.reg_rbuf_size26 = 0x%x\n", p->ring_buffer_size26.reg_rbuf_size26);
	fprintf(stderr, "\tRING_BUFFER_SIZE27.reg_rbuf_size27 = 0x%x\n", p->ring_buffer_size27.reg_rbuf_size27);
	fprintf(stderr, "\tRING_BUFFER_SIZE28.reg_rbuf_size28 = 0x%x\n", p->ring_buffer_size28.reg_rbuf_size28);
	fprintf(stderr, "\tRING_BUFFER_SIZE29.reg_rbuf_size29 = 0x%x\n", p->ring_buffer_size29.reg_rbuf_size29);
	fprintf(stderr, "\tRING_BUFFER_SIZE30.reg_rbuf_size30 = 0x%x\n", p->ring_buffer_size30.reg_rbuf_size30);
	fprintf(stderr, "\tRING_BUFFER_SIZE31.reg_rbuf_size31 = 0x%x\n", p->ring_buffer_size31.reg_rbuf_size31);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS0.reg_next_dma_addr0 = 0x%x\n", p->next_dma_addr_sts0.reg_next_dma_addr0);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS1.reg_next_dma_addr1 = 0x%x\n", p->next_dma_addr_sts1.reg_next_dma_addr1);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS2.reg_next_dma_addr2 = 0x%x\n", p->next_dma_addr_sts2.reg_next_dma_addr2);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS3.reg_next_dma_addr3 = 0x%x\n", p->next_dma_addr_sts3.reg_next_dma_addr3);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS4.reg_next_dma_addr4 = 0x%x\n", p->next_dma_addr_sts4.reg_next_dma_addr4);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS5.reg_next_dma_addr5 = 0x%x\n", p->next_dma_addr_sts5.reg_next_dma_addr5);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS6.reg_next_dma_addr6 = 0x%x\n", p->next_dma_addr_sts6.reg_next_dma_addr6);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS7.reg_next_dma_addr7 = 0x%x\n", p->next_dma_addr_sts7.reg_next_dma_addr7);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS8.reg_next_dma_addr8 = 0x%x\n", p->next_dma_addr_sts8.reg_next_dma_addr8);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS9.reg_next_dma_addr9 = 0x%x\n", p->next_dma_addr_sts9.reg_next_dma_addr9);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS10.reg_next_dma_addr10 = 0x%x\n", p->next_dma_addr_sts10.reg_next_dma_addr10);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS11.reg_next_dma_addr11 = 0x%x\n", p->next_dma_addr_sts11.reg_next_dma_addr11);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS12.reg_next_dma_addr12 = 0x%x\n", p->next_dma_addr_sts12.reg_next_dma_addr12);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS13.reg_next_dma_addr13 = 0x%x\n", p->next_dma_addr_sts13.reg_next_dma_addr13);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS14.reg_next_dma_addr14 = 0x%x\n", p->next_dma_addr_sts14.reg_next_dma_addr14);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS15.reg_next_dma_addr15 = 0x%x\n", p->next_dma_addr_sts15.reg_next_dma_addr15);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS16.reg_next_dma_addr16 = 0x%x\n", p->next_dma_addr_sts16.reg_next_dma_addr16);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS17.reg_next_dma_addr17 = 0x%x\n", p->next_dma_addr_sts17.reg_next_dma_addr17);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS18.reg_next_dma_addr18 = 0x%x\n", p->next_dma_addr_sts18.reg_next_dma_addr18);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS19.reg_next_dma_addr19 = 0x%x\n", p->next_dma_addr_sts19.reg_next_dma_addr19);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS20.reg_next_dma_addr20 = 0x%x\n", p->next_dma_addr_sts20.reg_next_dma_addr20);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS21.reg_next_dma_addr21 = 0x%x\n", p->next_dma_addr_sts21.reg_next_dma_addr21);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS22.reg_next_dma_addr22 = 0x%x\n", p->next_dma_addr_sts22.reg_next_dma_addr22);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS23.reg_next_dma_addr23 = 0x%x\n", p->next_dma_addr_sts23.reg_next_dma_addr23);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS24.reg_next_dma_addr24 = 0x%x\n", p->next_dma_addr_sts24.reg_next_dma_addr24);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS25.reg_next_dma_addr25 = 0x%x\n", p->next_dma_addr_sts25.reg_next_dma_addr25);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS26.reg_next_dma_addr26 = 0x%x\n", p->next_dma_addr_sts26.reg_next_dma_addr26);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS27.reg_next_dma_addr27 = 0x%x\n", p->next_dma_addr_sts27.reg_next_dma_addr27);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS28.reg_next_dma_addr28 = 0x%x\n", p->next_dma_addr_sts28.reg_next_dma_addr28);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS29.reg_next_dma_addr29 = 0x%x\n", p->next_dma_addr_sts29.reg_next_dma_addr29);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS30.reg_next_dma_addr30 = 0x%x\n", p->next_dma_addr_sts30.reg_next_dma_addr30);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS31.reg_next_dma_addr31 = 0x%x\n", p->next_dma_addr_sts31.reg_next_dma_addr31);

}
#pragma GCC diagnostic pop
#define DEFINE_ISP_WDMA_C(X) \
	 isp_wdma_c X = \
{\
	{	/*.shadow_rd_sel.reg_shadow_rd_sel = */0x1,\
	/*.shadow_rd_sel.reg_abort_mode = */0x0,\
	},\
	{	/*.disable_seglen.reg_seglen_disable = */0x0,\
	},\
	{	/*.up_ring_base.reg_up_ring_base = */0x0,\
	},\
	{	/*.norm_status0.reg_abort_done = */0x0,\
	/*uint32_t rsv_1_3:3;=*/0,\
	/*.norm_status0.reg_error_axi = */0x0,\
	/*.norm_status0.reg_error_dmi = */0x0,\
	/*.norm_status0.reg_slot_full = */0x0,\
	/*uint32_t rsv_7_7:1;=*/0,\
	/*.norm_status0.reg_error_id = */0x1f,\
	/*uint32_t rsv_13_15:3;=*/0,\
	/*.norm_status0.reg_dma_version = */0x0703,\
	},\
	{	/*.norm_status1.reg_id_idle = */0x0,\
	},\
	{	/*.ring_buffer_en.reg_ring_enable = */0x0,\
	},\
	{	/*.norm_perf.reg_bwlwin = */0x0,\
	/*.norm_perf.reg_bwltxn = */0x0,\
	/*.norm_perf.reg_qoso_th = */0x0,\
	/*.norm_perf.reg_qoso_en = */0x0,\
	},\
	{	/*.ring_patch_enable.reg_ring_patch_enable = */0x0,\
	},\
	{	/*.set_ring_base.reg_set_ring_base = */0x0,\
	},\
	{	/*.ring_base_addr_l.reg_ring_base_l = */0x0,\
	},\
	{	/*.ring_base_addr_h.reg_ring_base_h = */0x0,\
	},\
	{	/*.ring_buffer_size0.reg_rbuf_size0 = */0x0,\
	},\
	{	/*.ring_buffer_size1.reg_rbuf_size1 = */0x0,\
	},\
	{	/*.ring_buffer_size2.reg_rbuf_size2 = */0x0,\
	},\
	{	/*.ring_buffer_size3.reg_rbuf_size3 = */0x0,\
	},\
	{	/*.ring_buffer_size4.reg_rbuf_size4 = */0x0,\
	},\
	{	/*.ring_buffer_size5.reg_rbuf_size5 = */0x0,\
	},\
	{	/*.ring_buffer_size6.reg_rbuf_size6 = */0x0,\
	},\
	{	/*.ring_buffer_size7.reg_rbuf_size7 = */0x0,\
	},\
	{	/*.ring_buffer_size8.reg_rbuf_size8 = */0x0,\
	},\
	{	/*.ring_buffer_size9.reg_rbuf_size9 = */0x0,\
	},\
	{	/*.ring_buffer_size10.reg_rbuf_size10 = */0x0,\
	},\
	{	/*.ring_buffer_size11.reg_rbuf_size11 = */0x0,\
	},\
	{	/*.ring_buffer_size12.reg_rbuf_size12 = */0x0,\
	},\
	{	/*.ring_buffer_size13.reg_rbuf_size13 = */0x0,\
	},\
	{	/*.ring_buffer_size14.reg_rbuf_size14 = */0x0,\
	},\
	{	/*.ring_buffer_size15.reg_rbuf_size15 = */0x0,\
	},\
	{	/*.ring_buffer_size16.reg_rbuf_size16 = */0x0,\
	},\
	{	/*.ring_buffer_size17.reg_rbuf_size17 = */0x0,\
	},\
	{	/*.ring_buffer_size18.reg_rbuf_size18 = */0x0,\
	},\
	{	/*.ring_buffer_size19.reg_rbuf_size19 = */0x0,\
	},\
	{	/*.ring_buffer_size20.reg_rbuf_size20 = */0x0,\
	},\
	{	/*.ring_buffer_size21.reg_rbuf_size21 = */0x0,\
	},\
	{	/*.ring_buffer_size22.reg_rbuf_size22 = */0x0,\
	},\
	{	/*.ring_buffer_size23.reg_rbuf_size23 = */0x0,\
	},\
	{	/*.ring_buffer_size24.reg_rbuf_size24 = */0x0,\
	},\
	{	/*.ring_buffer_size25.reg_rbuf_size25 = */0x0,\
	},\
	{	/*.ring_buffer_size26.reg_rbuf_size26 = */0x0,\
	},\
	{	/*.ring_buffer_size27.reg_rbuf_size27 = */0x0,\
	},\
	{	/*.ring_buffer_size28.reg_rbuf_size28 = */0x0,\
	},\
	{	/*.ring_buffer_size29.reg_rbuf_size29 = */0x0,\
	},\
	{	/*.ring_buffer_size30.reg_rbuf_size30 = */0x0,\
	},\
	{	/*.ring_buffer_size31.reg_rbuf_size31 = */0x0,\
	},\
	{	/*.next_dma_addr_sts0.reg_next_dma_addr0 = */0x0,\
	},\
	{	/*.next_dma_addr_sts1.reg_next_dma_addr1 = */0x0,\
	},\
	{	/*.next_dma_addr_sts2.reg_next_dma_addr2 = */0x0,\
	},\
	{	/*.next_dma_addr_sts3.reg_next_dma_addr3 = */0x0,\
	},\
	{	/*.next_dma_addr_sts4.reg_next_dma_addr4 = */0x0,\
	},\
	{	/*.next_dma_addr_sts5.reg_next_dma_addr5 = */0x0,\
	},\
	{	/*.next_dma_addr_sts6.reg_next_dma_addr6 = */0x0,\
	},\
	{	/*.next_dma_addr_sts7.reg_next_dma_addr7 = */0x0,\
	},\
	{	/*.next_dma_addr_sts8.reg_next_dma_addr8 = */0x0,\
	},\
	{	/*.next_dma_addr_sts9.reg_next_dma_addr9 = */0x0,\
	},\
	{	/*.next_dma_addr_sts10.reg_next_dma_addr10 = */0x0,\
	},\
	{	/*.next_dma_addr_sts11.reg_next_dma_addr11 = */0x0,\
	},\
	{	/*.next_dma_addr_sts12.reg_next_dma_addr12 = */0x0,\
	},\
	{	/*.next_dma_addr_sts13.reg_next_dma_addr13 = */0x0,\
	},\
	{	/*.next_dma_addr_sts14.reg_next_dma_addr14 = */0x0,\
	},\
	{	/*.next_dma_addr_sts15.reg_next_dma_addr15 = */0x0,\
	},\
	{	/*.next_dma_addr_sts16.reg_next_dma_addr16 = */0x0,\
	},\
	{	/*.next_dma_addr_sts17.reg_next_dma_addr17 = */0x0,\
	},\
	{	/*.next_dma_addr_sts18.reg_next_dma_addr18 = */0x0,\
	},\
	{	/*.next_dma_addr_sts19.reg_next_dma_addr19 = */0x0,\
	},\
	{	/*.next_dma_addr_sts20.reg_next_dma_addr20 = */0x0,\
	},\
	{	/*.next_dma_addr_sts21.reg_next_dma_addr21 = */0x0,\
	},\
	{	/*.next_dma_addr_sts22.reg_next_dma_addr22 = */0x0,\
	},\
	{	/*.next_dma_addr_sts23.reg_next_dma_addr23 = */0x0,\
	},\
	{	/*.next_dma_addr_sts24.reg_next_dma_addr24 = */0x0,\
	},\
	{	/*.next_dma_addr_sts25.reg_next_dma_addr25 = */0x0,\
	},\
	{	/*.next_dma_addr_sts26.reg_next_dma_addr26 = */0x0,\
	},\
	{	/*.next_dma_addr_sts27.reg_next_dma_addr27 = */0x0,\
	},\
	{	/*.next_dma_addr_sts28.reg_next_dma_addr28 = */0x0,\
	},\
	{	/*.next_dma_addr_sts29.reg_next_dma_addr29 = */0x0,\
	},\
	{	/*.next_dma_addr_sts30.reg_next_dma_addr30 = */0x0,\
	},\
	{	/*.next_dma_addr_sts31.reg_next_dma_addr31 = */0x0,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_ISP_WDMA_C \
{\
	.shadow_rd_sel.reg_shadow_rd_sel = 0x1,\
	.shadow_rd_sel.reg_abort_mode = 0x0,\
	.disable_seglen.reg_seglen_disable = 0x0,\
	.up_ring_base.reg_up_ring_base = 0x0,\
	.norm_status0.reg_abort_done = 0x0,\
	.norm_status0.reg_error_axi = 0x0,\
	.norm_status0.reg_error_dmi = 0x0,\
	.norm_status0.reg_slot_full = 0x0,\
	.norm_status0.reg_error_id = 0x1f,\
	.norm_status0.reg_dma_version = 0x0703,\
	.norm_status1.reg_id_idle = 0x0,\
	.ring_buffer_en.reg_ring_enable = 0x0,\
	.norm_perf.reg_bwlwin = 0x0,\
	.norm_perf.reg_bwltxn = 0x0,\
	.norm_perf.reg_qoso_th = 0x0,\
	.norm_perf.reg_qoso_en = 0x0,\
	.ring_patch_enable.reg_ring_patch_enable = 0x0,\
	.set_ring_base.reg_set_ring_base = 0x0,\
	.ring_base_addr_l.reg_ring_base_l = 0x0,\
	.ring_base_addr_h.reg_ring_base_h = 0x0,\
	.ring_buffer_size0.reg_rbuf_size0 = 0x0,\
	.ring_buffer_size1.reg_rbuf_size1 = 0x0,\
	.ring_buffer_size2.reg_rbuf_size2 = 0x0,\
	.ring_buffer_size3.reg_rbuf_size3 = 0x0,\
	.ring_buffer_size4.reg_rbuf_size4 = 0x0,\
	.ring_buffer_size5.reg_rbuf_size5 = 0x0,\
	.ring_buffer_size6.reg_rbuf_size6 = 0x0,\
	.ring_buffer_size7.reg_rbuf_size7 = 0x0,\
	.ring_buffer_size8.reg_rbuf_size8 = 0x0,\
	.ring_buffer_size9.reg_rbuf_size9 = 0x0,\
	.ring_buffer_size10.reg_rbuf_size10 = 0x0,\
	.ring_buffer_size11.reg_rbuf_size11 = 0x0,\
	.ring_buffer_size12.reg_rbuf_size12 = 0x0,\
	.ring_buffer_size13.reg_rbuf_size13 = 0x0,\
	.ring_buffer_size14.reg_rbuf_size14 = 0x0,\
	.ring_buffer_size15.reg_rbuf_size15 = 0x0,\
	.ring_buffer_size16.reg_rbuf_size16 = 0x0,\
	.ring_buffer_size17.reg_rbuf_size17 = 0x0,\
	.ring_buffer_size18.reg_rbuf_size18 = 0x0,\
	.ring_buffer_size19.reg_rbuf_size19 = 0x0,\
	.ring_buffer_size20.reg_rbuf_size20 = 0x0,\
	.ring_buffer_size21.reg_rbuf_size21 = 0x0,\
	.ring_buffer_size22.reg_rbuf_size22 = 0x0,\
	.ring_buffer_size23.reg_rbuf_size23 = 0x0,\
	.ring_buffer_size24.reg_rbuf_size24 = 0x0,\
	.ring_buffer_size25.reg_rbuf_size25 = 0x0,\
	.ring_buffer_size26.reg_rbuf_size26 = 0x0,\
	.ring_buffer_size27.reg_rbuf_size27 = 0x0,\
	.ring_buffer_size28.reg_rbuf_size28 = 0x0,\
	.ring_buffer_size29.reg_rbuf_size29 = 0x0,\
	.ring_buffer_size30.reg_rbuf_size30 = 0x0,\
	.ring_buffer_size31.reg_rbuf_size31 = 0x0,\
	.next_dma_addr_sts0.reg_next_dma_addr0 = 0x0,\
	.next_dma_addr_sts1.reg_next_dma_addr1 = 0x0,\
	.next_dma_addr_sts2.reg_next_dma_addr2 = 0x0,\
	.next_dma_addr_sts3.reg_next_dma_addr3 = 0x0,\
	.next_dma_addr_sts4.reg_next_dma_addr4 = 0x0,\
	.next_dma_addr_sts5.reg_next_dma_addr5 = 0x0,\
	.next_dma_addr_sts6.reg_next_dma_addr6 = 0x0,\
	.next_dma_addr_sts7.reg_next_dma_addr7 = 0x0,\
	.next_dma_addr_sts8.reg_next_dma_addr8 = 0x0,\
	.next_dma_addr_sts9.reg_next_dma_addr9 = 0x0,\
	.next_dma_addr_sts10.reg_next_dma_addr10 = 0x0,\
	.next_dma_addr_sts11.reg_next_dma_addr11 = 0x0,\
	.next_dma_addr_sts12.reg_next_dma_addr12 = 0x0,\
	.next_dma_addr_sts13.reg_next_dma_addr13 = 0x0,\
	.next_dma_addr_sts14.reg_next_dma_addr14 = 0x0,\
	.next_dma_addr_sts15.reg_next_dma_addr15 = 0x0,\
	.next_dma_addr_sts16.reg_next_dma_addr16 = 0x0,\
	.next_dma_addr_sts17.reg_next_dma_addr17 = 0x0,\
	.next_dma_addr_sts18.reg_next_dma_addr18 = 0x0,\
	.next_dma_addr_sts19.reg_next_dma_addr19 = 0x0,\
	.next_dma_addr_sts20.reg_next_dma_addr20 = 0x0,\
	.next_dma_addr_sts21.reg_next_dma_addr21 = 0x0,\
	.next_dma_addr_sts22.reg_next_dma_addr22 = 0x0,\
	.next_dma_addr_sts23.reg_next_dma_addr23 = 0x0,\
	.next_dma_addr_sts24.reg_next_dma_addr24 = 0x0,\
	.next_dma_addr_sts25.reg_next_dma_addr25 = 0x0,\
	.next_dma_addr_sts26.reg_next_dma_addr26 = 0x0,\
	.next_dma_addr_sts27.reg_next_dma_addr27 = 0x0,\
	.next_dma_addr_sts28.reg_next_dma_addr28 = 0x0,\
	.next_dma_addr_sts29.reg_next_dma_addr29 = 0x0,\
	.next_dma_addr_sts30.reg_next_dma_addr30 = 0x0,\
	.next_dma_addr_sts31.reg_next_dma_addr31 = 0x0,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_ISP_WDMA_STRUCT_H__
