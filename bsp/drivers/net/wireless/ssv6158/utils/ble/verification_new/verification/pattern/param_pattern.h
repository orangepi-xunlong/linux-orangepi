#ifndef _PARAM_PATTERN_H_
#define _PARAM_PATTERN_H_

//#include "pattern2dut.h"

#define ADV_TYPE_MAX 4
#define ADV_CH_MAP_TBL_ROW 7
#define CH_NUM_MAX 3

#define ADV_AVG_DELAY_MS 5
#define CONN_INTVAL_ROUND_CHECK 10
#define CONN_SUPERTO_ROUND_CHECK 2


#define MS2US(VAL) (VAL*1000)
#define SCAN_INTVAL_UNIT_MS(VAL) (VAL / 0.625)
#define ADV_INTVAL_UNIT_MS(VAL) (VAL / 0.625)
#define CONN_INTVAL_UNIT_US(VAL) (VAL*1250)
#define CONN_SUPERTO_UNIT_US(VAL) (VAL*10000)


#define CALC_ADV_CNT(TIME , ADV_INT , RATE) \
    ((float)(TIME*1000) / (float)(ADV_INT * 0.625)) \
    * (float)RATE/100 \
    * ((float)adv_ch_map_tbl[adv_ch_map_cnt][1] / (float)CH_NUM_MAX)

#define CALC_ADV_DIRECT_CNT(ADV_INT , RATE) \
    ((float)(1280) / (float)(ADV_INT * 0.625)) \
    * (float)RATE/100 \
    * ((float)adv_ch_map_tbl[adv_ch_map_cnt][1] / (float)CH_NUM_MAX)

#define SCAN_CALC_ADV_CNT(TIME , SCAN_WIN , SCAN_INT , ADV_INT , RATE) \
    ((float)(TIME*1000)     / (float)(SCAN_INT * 0.625)) \
    * ((float)(SCAN_WIN)    / (float)(ADV_INT)) \
    * ((float)(SCAN_WIN)    / (float)(SCAN_INT)) \
    *  (float)RATE/100      \
    * ((float)adv_ch_map_tbl[adv_ch_map_cnt][1] / (float)CH_NUM_MAX)

#define SCAN_CALC_ADV_DIRECT_CNT(SCAN_WIN , SCAN_INT , ADV_INT , RATE) \
      ((float)(1280)        / (float)(ADV_INT*0.625)) \
    * ((float)(SCAN_WIN)    / (float)(SCAN_INT)) \
    *  (float)RATE/100      \
    * ((float)adv_ch_map_tbl[adv_ch_map_cnt][1] / (float)CH_NUM_MAX)

enum{
    CONFIG_DISABLE = 0 ,
    CONFIG_ENABLE,
    CONFIG_FORBID
};

typedef struct adv_test_param_st
{
	u8 adv_type;
	u16 adv_intval_min;
	u8 adv_ch_map;
	u8 adv_data_len;
	u8 own_addr_type;
	u8 dir_addr_type;
	u8 dir_addr;
	u8 addr_filter_policy;
}adv_test_param;


typedef struct str2var{
    const char* str;
    int *var;

}Str2var;


extern u8 adv_ch_map_tbl[][2];
extern adv_test_param adv_test_params[ADV_TYPE_MAX];

extern Str2var adv_param_time_indep_config_tbl[];
extern Str2var adv_param_time_dep_config_tbl[];
extern Str2var scan_param_time_indep_config_tbl[];
extern Str2var scan_param_time_dep_config_tbl[];
extern Str2var slave_param_time_indep_config_tbl[];
extern Str2var slave_param_time_dep_config_tbl[];
extern Str2var adv_stable_config_tbl[];
extern Str2var scan_stable_config_tbl[];
extern Str2var sla_stable_config_tbl[];
extern Str2var sla_stress_config_tbl[];
extern Str2var multirole_sla_perf_config_tbl[];

extern int g_adv_param_adv_type_step           ;
extern int g_adv_param_adv_data_len_step       ;
extern int g_adv_param_scn_rsp_data_len_step   ;
extern int g_adv_param_adv_filter_policy_step  ;
extern int g_adv_param_adv_collect_cnt         ;
extern int g_adv_param_scn_rsp_collect_cnt     ;
extern int g_adv_param_conn_req_collect_cnt    ;
extern int g_adv_param_time_out                ;
extern int g_adv_param_adv_intval              ;
extern int g_adv_param_adv_intval_step         ;

extern int g_adv_param_adv_ch_map_step         ;
extern int g_adv_param_loop_cnt                ;
extern int g_adv_param_adv_ideal_cnt           ;
extern int g_adv_param_query_time              ;
extern int g_adv_param_success_rate            ;
extern int g_adv_param_scn_intval              ;

extern int g_scan_param_loop_cnt               ;
extern int g_scan_param_adv_ideal_cnt          ;
extern int g_scan_param_query_time             ;
extern int g_scan_param_success_rate           ;
extern int g_scan_param_scn_intval             ;
extern int g_scan_param_scn_window             ;

extern int g_slave_param_loop_cnt              ;
extern int g_slave_param_acl_len_step          ;
extern int g_slave_param_ch_map_step           ;
extern int g_slave_param_conn_intval_step      ;
extern int g_slave_param_conn_slave_latency_step       ;
extern int g_slave_param_conn_supervision_timeout_step ;

extern int g_adv_stable_test_time              ;
extern int g_adv_stable_scan_intval            ;
extern int g_adv_stable_adv_intval             ;
extern int g_adv_stable_loop_cnt               ;

extern int g_scn_stable_test_time              ;
extern int g_scn_stable_adv_intval             ;
extern int g_scn_stable_scan_intval            ;
extern int g_scn_stable_scan_window            ;

extern int g_sla_stable_test_time              ;
extern int g_sla_stable_conn_intval            ;
extern int g_sla_stable_sla_latency            ;
extern int g_sla_stable_supervision_to         ;
extern int g_sla_stable_conn_event_change      ;

extern int g_sla_stress_test_time              ;
extern int g_sla_stress_conn_intval            ;
extern int g_sla_stress_sla_latency            ;
extern int g_sla_stress_supervision_to         ;
extern int g_sla_stress_duration          ;
extern int g_sla_stress_duration_max      ;
extern int g_sla_stress_conn_event_change      ;
extern int g_sla_stress_conn_event_change_max  ;
extern int g_sla_stress_ch_map_num          ;
extern int g_sla_stress_acl_date_len        ;

extern int g_multirole_sla_perf_test_time;
extern int g_multirole_sla_perf_adv_intval;
extern int g_multirole_sla_perf_scan_intval;
extern int g_multirole_sla_perf_scan_window;
extern int g_multirole_sla_perf_conn_intval;
extern int g_multirole_sla_perf_sla_latency;
extern int g_multirole_sla_perf_supervision_to;
extern int g_multirole_sla_perf_conn_event_change;

extern void param_config_read(const char * , Str2var *);
extern void calc_alive_time(u64 conn_intval , struct timeval * alive_time);
extern void shuffle(int *arr, int n, int low, int up) ;
#endif
