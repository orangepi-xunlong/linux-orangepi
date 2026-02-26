#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "task.h"
#include "types.h"

#include "../bench/cli/cli.h"
#include "../hci_tools/lib/ssv_lib.h"
#include "../hci_tools/lib_bt/bt_hci_cmd_declare.h"

#include "ctrl_bench.h"
#include "param_pattern.h"
#include "../log/log.h"


// ### ADV PARAM

// ## Time independ setting
// # 4 types adv   , min = 1 , max = 4 ;
int g_adv_param_adv_type_step           = 0 ;

// #               , min = 1 , max = 31;
int g_adv_param_adv_data_len_step       = 0 ;

// #               , min = 1 , max = 31;
int g_adv_param_scn_rsp_data_len_step   = 0 ;

// # 4 types       , min = 1 , max = 4 ;
int g_adv_param_adv_filter_policy_step  = 0 ;

// # the adv ind cnt that want to collect ; u16
int g_adv_param_adv_collect_cnt         = 0 ;

// # the scn rsp cnt that want to collect ; u16
int g_adv_param_scn_rsp_collect_cnt     = 0 ;

// # the conn req cnt that want to collect ; u16
int g_adv_param_conn_req_collect_cnt    = 0 ;

// # the stop time for query adv/scn event ; u16
int g_adv_param_time_out                = 0 ;


// ## Time depend setting
// # per step 0.625 ms , max = 0x4000 = 10240 ms
int g_adv_param_adv_intval_step         = 0 ;

// # 7 types       , min = 1 , max = 7 ;
int g_adv_param_adv_ch_map_step         = 0 ;

// # the testing loop cnt that want to check ; u16
int g_adv_param_loop_cnt                = 0 ;


// * means only for adv_direct_ind testing

// # *the adv ind cnt that want to check
int g_adv_param_adv_ideal_cnt           = 0 ;

// # *the testing time ; unit sec, u16
int g_adv_param_query_time              = 0 ;

// # *unit is %
int g_adv_param_success_rate            = 0 ;

// # per step 0.625 ms , max = 0x4000 = 10240 ms
int g_adv_param_scn_intval              = 0 ;

// # per step 0.625 ms , max = 0x4000 = 10240 ms
int g_adv_param_adv_intval              = 0 ;


// ### SCAN PARAM

// ## Time independ setting
// # 2 types scn   , min = 0 , max = 1 ;
int g_scn_param_adv_type_step           = 0 ;

// # the testing loop cnt that want to check ; u16
int g_scan_param_loop_cnt                = 0 ;

// # *the adv ind cnt that want to check
int g_scan_param_adv_ideal_cnt           = 0 ;

// # *the testing time ; unit sec, u16
int g_scan_param_query_time              = 0 ;

// # *unit is %
int g_scan_param_success_rate            = 0 ;

// # per step 0.625 ms , max = 0x4000 = 10240 ms
int g_scan_param_scn_intval              = 0 ;

// # per step 0.625 ms , max = 0x4000 = 10240 ms
int g_scan_param_scn_window              = 0 ;

// ### SLAVE PARAM

int g_slave_param_loop_cnt               = 0 ;

int g_slave_param_acl_len_step           = 0 ;

int g_slave_param_ch_map_step            = 0 ;

int g_slave_param_conn_intval_step       = 0 ;

int g_slave_param_conn_slave_latency_step           = 0 ;

int g_slave_param_conn_supervision_timeout_step     = 0 ;

// ### ADV STABLE

int g_adv_stable_test_time               = 0;

// # per step 0.625 ms , max = 0x4000 = 10240 ms
int g_adv_stable_adv_intval              = 0;

// # per step 0.625 ms , max = 0x4000 = 10240 ms
int g_adv_stable_scan_intval             = 0;

// # per step 0.625 ms , max = 0x4000 = 10240 ms
int g_adv_stable_scan_window             = 0;

// # the testing loop cnt that want to check ; u16
int g_adv_stable_loop_cnt                = 0;


// ### SCAN STABLE

int g_scn_stable_test_time               = 0;

// # per step 0.625 ms , max = 0x4000 = 10240 ms
int g_scn_stable_adv_intval              = 0;

// # per step 0.625 ms , max = 0x4000 = 10240 ms
int g_scn_stable_scan_intval             = 0;

// # per step 0.625 ms , max = 0x4000 = 10240 ms
int g_scn_stable_scan_window             = 0;


// ### SLAVE STABLE

int g_sla_stable_test_time               = 0;

// # per step 1.25 ms , max = 0x0C80 = 4s
int g_sla_stable_conn_intval             = 0;

// # per step 1.25 ms , max = 0x01F3 = 499
int g_sla_stable_sla_latency             = 0;

// # per step 10 ms , max = 0x0C80 = 32s
int g_sla_stable_supervision_to          = 0;

//
int g_sla_stable_conn_event_change       = 0;


// ### SLAVE STRESS

int g_sla_stress_test_time               = 0;

// # per step 1.25 ms , max = 0x0C80 = 4s
int g_sla_stress_conn_intval             = 0;

// # per step 1.25 ms , max = 0x01F3 = 499
int g_sla_stress_sla_latency             = 0;

// # per step 10 ms , max = 0x0C80 = 32s
int g_sla_stress_supervision_to          = 0;

int g_sla_stress_duration           = 0;

int g_sla_stress_duration_max       = 0;

int g_sla_stress_conn_event_change       = 0;

int g_sla_stress_conn_event_change_max   = 0;

int g_sla_stress_ch_map_num         = 0;

int g_sla_stress_acl_date_len       = 0;

// ### MULTIROLE SLAVE PERF

int g_multirole_sla_perf_test_time = 0;

int g_multirole_sla_perf_adv_intval = 0;

int g_multirole_sla_perf_scan_intval = 0;

int g_multirole_sla_perf_scan_window = 0;

int g_multirole_sla_perf_conn_intval = 0;

int g_multirole_sla_perf_sla_latency = 0;

int g_multirole_sla_perf_supervision_to = 0;

int g_multirole_sla_perf_conn_event_change = 0;

adv_test_param adv_test_params[]={
        // #
        {.adv_type = HCI_CMD_PARAM_ADV_IND,
         .adv_intval_min = HCI_CMD_PARAM_ADVERTISING_INTERVAL_MIN,
        },
        // #
        {.adv_type = HCI_CMD_PARAM_ADV_DIRECT_IND,
         .adv_intval_min = HCI_CMD_PARAM_ADVERTISING_INTERVAL_MIN_ADV_DIRECT_IND,
        },
        // #
        {.adv_type = HCI_CMD_PARAM_ADV_SCAN_IND,
         .adv_intval_min = HCI_CMD_PARAM_ADVERTISING_INTERVAL_MIN_ADV_SCAN_OR_NONCONN_IND,
        },
        // #
        {.adv_type = HCI_CMD_PARAM_ADV_NONCONN_IND,
         .adv_intval_min = HCI_CMD_PARAM_ADVERTISING_INTERVAL_MIN_ADV_SCAN_OR_NONCONN_IND,
        },
};

u8 adv_ch_map_tbl[ADV_CH_MAP_TBL_ROW][2] = {
        {HCI_CMD_PARAM_ENABLE_CHANNEL_37 ,                                   1}, //time slot 1
        {HCI_CMD_PARAM_ENABLE_CHANNEL_38 ,                                   1}, //time slot 2
        {HCI_CMD_PARAM_ENABLE_CHANNEL_37 | HCI_CMD_PARAM_ENABLE_CHANNEL_38 , 2},
        {HCI_CMD_PARAM_ENABLE_CHANNEL_39 ,                                   1}, //time slot 3
        {HCI_CMD_PARAM_ENABLE_CHANNEL_37 | HCI_CMD_PARAM_ENABLE_CHANNEL_39 , 2},
        {HCI_CMD_PARAM_ENABLE_CHANNEL_38 | HCI_CMD_PARAM_ENABLE_CHANNEL_39 , 2},
        {HCI_CMD_PARAM_ENABLE_CHANNEL_37 | HCI_CMD_PARAM_ENABLE_CHANNEL_38 | HCI_CMD_PARAM_ENABLE_CHANNEL_39 , 3},
};


Str2var adv_param_time_indep_config_tbl[]={
    {.str="adv_type_step"         , .var=&g_adv_param_adv_type_step            },
    {.str="adv_data_len_step"     , .var=&g_adv_param_adv_data_len_step        },
    {.str="scn_rsp_data_len_step" , .var=&g_adv_param_scn_rsp_data_len_step    },
    {.str="adv_filter_policy_step", .var=&g_adv_param_adv_filter_policy_step   },
    {.str="adv_collect_cnt"       , .var=&g_adv_param_adv_collect_cnt          },
    {.str="scn_rsp_collect_cnt"   , .var=&g_adv_param_scn_rsp_collect_cnt      },
    {.str="conn_req_collect_cnt"  , .var=&g_adv_param_conn_req_collect_cnt     },
    {.str="time_out"              , .var=&g_adv_param_time_out                 },
};

Str2var adv_param_time_dep_config_tbl[]={
    {.str="adv_intval_step"       , .var=&g_adv_param_adv_intval_step          },
    {.str="adv_ch_map_step"       , .var=&g_adv_param_adv_ch_map_step          },
    {.str="loop_cnt"              , .var=&g_adv_param_loop_cnt                 },
    {.str="adv_ideal_cnt"         , .var=&g_adv_param_adv_ideal_cnt            },
    {.str="query_time"            , .var=&g_adv_param_query_time               },
    {.str="success_rate"          , .var=&g_adv_param_success_rate             },
    {.str="scn_intval"            , .var=&g_adv_param_scn_intval               },
};

Str2var scan_param_time_indep_config_tbl[]={
    {.str="scan_type_step"        , .var=&g_adv_param_adv_type_step           },
    {.str="adv_data_len_step"     , .var=&g_adv_param_adv_data_len_step        },
    {.str="scn_rsp_data_len_step" , .var=&g_adv_param_scn_rsp_data_len_step    },
    {.str="adv_filter_policy_step", .var=&g_adv_param_adv_filter_policy_step   },
    {.str="adv_collect_cnt"       , .var=&g_adv_param_adv_collect_cnt          },
    {.str="scn_rsp_collect_cnt"   , .var=&g_adv_param_scn_rsp_collect_cnt      },
    {.str="conn_req_collect_cnt"  , .var=&g_adv_param_conn_req_collect_cnt     },
    {.str="time_out"              , .var=&g_adv_param_time_out                 },
};

Str2var scan_param_time_dep_config_tbl[]={
    {.str="loop_cnt"              , .var=&g_scan_param_loop_cnt                 },
    {.str="adv_ideal_cnt"         , .var=&g_scan_param_adv_ideal_cnt            },
    {.str="query_time"            , .var=&g_scan_param_query_time               },
    {.str="success_rate"          , .var=&g_scan_param_success_rate             },
    {.str="scn_intval"            , .var=&g_scan_param_scn_intval               },
    {.str="scn_window"            , .var=&g_scan_param_scn_window               },
};

Str2var slave_param_time_indep_config_tbl[]={
    {.str="loop_cnt"                , .var=&g_slave_param_loop_cnt                },
    {.str="acl_len_step"            , .var=&g_slave_param_acl_len_step            },
    {.str="ch_map_step"             , .var=&g_slave_param_ch_map_step             },
    {.str="conn_intval_step"        , .var=&g_slave_param_conn_intval_step        },
    {.str="conn_slave_latency_step" , .var=&g_slave_param_conn_slave_latency_step },
    {.str="conn_supervision_timeout_step" , .var=&g_slave_param_conn_supervision_timeout_step },

};

Str2var slave_param_time_dep_config_tbl[]={
    {.str="loop_cnt"                , .var=&g_slave_param_loop_cnt                },
    {.str="acl_len_step"            , .var=&g_slave_param_acl_len_step            },
    {.str="ch_map_step"             , .var=&g_slave_param_ch_map_step             },
    {.str="conn_intval_step"        , .var=&g_slave_param_conn_intval_step        },
    {.str="conn_slave_latency_step" , .var=&g_slave_param_conn_slave_latency_step },
    {.str="conn_supervision_timeout_step" , .var=&g_slave_param_conn_supervision_timeout_step },

};

Str2var adv_stable_config_tbl[]={
    {.str="test_time"               , .var=&g_adv_stable_test_time           },
    {.str="adv_intval"              , .var=&g_adv_stable_adv_intval          },
    {.str="scn_intval"              , .var=&g_adv_stable_scan_intval         },
    {.str="loop_cnt"                , .var=&g_adv_stable_loop_cnt            },
};

Str2var scan_stable_config_tbl[]={
    {.str="test_time"               , .var=&g_scn_stable_test_time           },
    {.str="adv_intval"              , .var=&g_scn_stable_adv_intval          },
    {.str="scn_intval"              , .var=&g_scn_stable_scan_intval         },
    {.str="scn_window"              , .var=&g_scn_stable_scan_window         },
};

Str2var sla_stable_config_tbl[]={
    {.str="test_time"               , .var=&g_sla_stable_test_time           },
    {.str="conn_intval"             , .var=&g_sla_stable_conn_intval         },
    {.str="sla_latency"             , .var=&g_sla_stable_sla_latency         },
    {.str="supervision_to"          , .var=&g_sla_stable_supervision_to      },
    {.str="conn_event"              , .var=&g_sla_stable_conn_event_change   },
};

Str2var sla_stress_config_tbl[]={
    {.str="test_time"               , .var=&g_sla_stress_test_time              },
    {.str="conn_intval"             , .var=&g_sla_stress_conn_intval            },
    {.str="sla_latency"             , .var=&g_sla_stress_sla_latency            },
    {.str="supervision_to"          , .var=&g_sla_stress_supervision_to         },
    {.str="duration"                , .var=&g_sla_stress_duration               },
    {.str="duration_max"            , .var=&g_sla_stress_duration_max           },
    {.str="ch_map_num"              , .var=&g_sla_stress_ch_map_num             },
    {.str="acl_date_len"            , .var=&g_sla_stress_acl_date_len           },
    //{.str="conn_event_change"       , .var=&g_sla_stress_conn_event_change      },
    //{.str="conn_event_change_max"   , .var=&g_sla_stress_conn_event_change_max  },
};

Str2var multirole_sla_perf_config_tbl[]={
    {.str="test_time"               , .var=&g_multirole_sla_perf_test_time           },
    {.str="adv_intval"              , .var=&g_multirole_sla_perf_adv_intval          },
    {.str="scn_intval"              , .var=&g_multirole_sla_perf_scan_intval         },
    {.str="scn_window"              , .var=&g_multirole_sla_perf_scan_window         },
    {.str="conn_intval"             , .var=&g_multirole_sla_perf_conn_intval         },
    {.str="sla_latency"             , .var=&g_multirole_sla_perf_sla_latency         },
    {.str="supervision_to"          , .var=&g_multirole_sla_perf_supervision_to      },
    {.str="conn_event"              , .var=&g_multirole_sla_perf_conn_event_change   },
};

void param_config_read(const char * func_name , Str2var *tbl){
	FILE *cfg_f;
	u8 cfg[1025],*pt ;

    char file_str[256]={0};
    //memcpy (file_str , func_name , strlen(func_name));
    sprintf(file_str , "./pattern/config/%s.cfg" , func_name);

    if ((cfg_f = fopen(file_str,"r")) == NULL){
        printf("cant't open config file\n");
        while(1){};
        return ;
    }

    int cnt = 0 ;
    int val = 0 ;

    {
        char now[256];
        log_time(now);
        LOG_INFO("\n[%s] : %s \n" , func_name , now);
    }

    while(1){
        fgets((char*)cfg,1024,cfg_f);

        if(cfg[0] == '^'){
            break;
        }

        pt=cfg;

        if (memcmp(cfg , (tbl + cnt)->str
                       , strlen ((tbl + cnt)->str )    ) == 0 ){
            while(*pt++ != '='){};

            val = atoi((char*)pt);

            LOG_INFO("# %s = %d #\n"    , tbl[cnt].str
                                        , val);

            *(tbl[cnt].var) = val;

            cnt ++ ;
        }

    }
    printf("\n");
    fclose(cfg_f);

}

void calc_alive_time(u64 time , struct timeval * alive_time){

    if(time >= 1000000){
        alive_time->tv_sec  = time / 1000000 ;
        alive_time->tv_usec = time % 1000000 ;

    }else{
        alive_time->tv_sec  = 0 ;
        alive_time->tv_usec = time ;
    }

    return ;
}

void shuffle(int *arr, int n, int low, int up)
{
    int i, pos1, pos2, tmp;
    int size = up-low + 1;

    int * poker = (int*)malloc(sizeof(int) * size);

    for(i=0 ; i<size; ++i)
        poker[i] = i+low;

    for(i=0; i<size; ++i){
        // # get random pos
        pos1 = (int)(rand() / (RAND_MAX+1.0) * size);
        pos2 = (int)(rand() / (RAND_MAX+1.0) * size);

        // # change
        tmp = poker[pos1];
        poker[pos1] = poker[pos2];
        poker[pos2]=tmp;
    }

    for(i=0; i<n; ++i)
        arr[i] = poker[i];

    free(poker);

}
