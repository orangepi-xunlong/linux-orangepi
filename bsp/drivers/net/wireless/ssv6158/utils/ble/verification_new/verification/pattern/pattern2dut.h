#ifndef _PATTERN_2_DUT_H_
#define _PATTERN_2_DUT_H_

#define CMD_SUCCESS 0x00
#define EQUAL	    0x00
#define ADDR_LENGTH 6
#define QUERY_EVT_HDR_LENGTH 2
#define QUERY_EVT_LENGTH 0
#define QUERY_LE_EVT_LENGTH 1
#define QUERY_ACL_LENGTH 0


#define SLEEP_MS 300*1000

enum test_type{
    ADV_PARAM_TIME_INDEP = 1  ,
    ADV_PARAM_TIME_DEP        ,
    SCAN_PARAM_TIME_INDEP     ,
    SCAN_PARAM_TIME_DEP       ,
    SLAVE_PARAM_TIME_INDEP    ,
    SLAVE_PARAM_TIME_DEP      ,
    ADV_STABLE                ,
    SCAN_STABLE               ,
    SLAVE_STABLE              ,
    SLAVE_STRESS              ,
    MULTIROLE_SLA_PERF        ,
};


typedef struct buff_size
{
    u16 HCI_LE_ACL_Data_Packet_Length   ;
    u8  HCI_Total_Num_LE_ACL_Data_Packets;
}Buff_size;

typedef struct adv_data_parameter
{
	u8 Advertising_Data_Length[1];
	u8 Advertising_Data [31];
}Adv_data_param;

typedef struct adv_parameter
{
	u8 Advertising_Interval_Min[2];
	u8 Advertising_Interval_Max[2];
	u8 Advertising_Type[1];
	u8 Own_Address_Type[1];
	u8 Direct_Address_Type[1];
	u8 Direct_Address[6];
	u8 Advertising_Channel_Map[1];
	u8 Advertising_Filter_Policy[1];
}Adv_param;

typedef struct scn_parameter
{
	u8 LE_Scan_Type[1];
	u8 LE_Scan_Interval[2];
	u8 LE_Scan_Window[2];
	u8 Own_Address_Type[1];
	u8 Scanning_Filter_Policy[1];
}Scn_param;

typedef struct scn_rsp_parameter
{
	u8 Scan_Response_Data_Length[1];
	u8 Scan_Response_Data [31];
}Scn_rsp_param;

typedef struct create_conn_parameter
{
	u8 LE_Scan_Interval[2];
	u8 LE_Scan_Window[2];
	u8 Initiator_Filter_Policy[1];
	u8 Peer_Address_Type[1];
	u8 Peer_Address[6];
	u8 Own_Address_Type[1];
	u8 Conn_Interval_Min[2];
	u8 Conn_Interval_Max[2];
	u8 Conn_Latency[2];
	u8 Supervision_Timeout[2];
	u8 Minimum_CE_Length[2];
	u8 Maximum_CE_Length[2];
}Create_conn_param;


typedef struct conn_update_parameter
{
	u8 Connection_Handle[2];
	u8 Conn_Interval_Min[2];
	u8 Conn_Interval_Max[2];
	u8 Conn_Latency[2];
	u8 Supervision_Timeout[2];
	u8 Minimum_CE_Length[2];
	u8 Maximum_CE_Length[2];
}Conn_update_param;


typedef struct disconnect_conn_parameter
{
	u8 Connection_Handle[2];
	u8 Reason[6];
}Disconn_param;

typedef struct local_version_information_parameter
{
	u8 HCI_Version[1];
	u8 HCI_Revision[2];
	u8 LMP_PAL_Version[1];
	u8 Manufacturer_Name[2];
	u8 LMP_PAL_Subversion[2];
}Local_version_information_param;

typedef struct local_supported_features_parameter
{
	//u64 LE_Features[1];
}Local_supported_features_param;

typedef struct local_supported_states_parameter
{
	//u64 LE_States[1];
}Local_supported_states_param;

typedef struct remote_version_information_parameter
{
	u8 Connection_Handle[2];
}Remote_version_information_param;

typedef struct remote_used_features_parameter
{
	u8 Connection_Handle[2];
}Remote_used_features_param;

typedef struct white_list_parameter
{
	u8 Address_Type[1];
	u8 Address[6];
    u8 Size[1];
}White_list_param;

typedef struct encryption_parameter
{
	u8 Connection_Handle[2];
	u8 Random_Number[8];
	u8 Encrypted_Diversifier[2];
	u8 Long_Term_Key[16];
}Encryption_parameter;

typedef struct ltk_req_rep_parameter
{
	u8 Connection_Handle[2];
	u8 Long_Term_Key[16];
}Ltk_req_rep_parameter;


typedef struct ltk_req_negative_rep_parameter
{
	u8 Connection_Handle[2];
}Ltk_req_negative_rep_parameter;

typedef struct fail_reason{
    u16 no_conn_req       ;
    u16 no_create_conn    ;
    u16 no_num_pkt_comp   ;
    u16 fail_to_establish ;
    u16 connection_timeout;
    u16 error_pld_acl     ;
    u16 error_no_acl      ;
    u16 instant_passed    ;
    u16 no_encrypt_change_master ;
    u16 no_encrypt_change_slave  ;
}Fail_reason;

typedef struct pattern_parameter
{
    Buff_size buff_size_param;
    u8 random_address[6];
    u8 ir[16];
    u8 plaintext[16];
    Adv_param	adv_param;
    Adv_data_param adv_data_param;
    u8 Advertising_Enable[1];
    u8 Filter_Duplicates[1];
    Scn_param	scn_param;
    Scn_rsp_param scn_rsp_param;
    u8 Scan_Enable[1];
    u8 LE_Event_Mask[8];
    u8 Event_Mask[8];
    u8 LE_Channel_Map[5];
    u8 LE_Connection_Handle[2];
	Create_conn_param create_conn_param;
    Conn_update_param conn_update_param;
	Disconn_param disconn_param;
	Local_version_information_param local_version_information_param;
	Local_supported_features_param local_features_param;
	Local_supported_states_param local_states_param;
	Remote_version_information_param remote_version_information_param;
	Remote_used_features_param remote_features_param;
	White_list_param white_list_param;
	Encryption_parameter encryption_parameter;
	Ltk_req_rep_parameter ltk_req_rep_parameter;
	Ltk_req_negative_rep_parameter ltk_req_negative_rep_parameter;

	//int LMP_Features[8];
	//int LE_Features[8];
}Pattern_param;

enum
{
	PASS = 0,
	FAIL = 255 //avdoid 0x00 ~ 0x3f, but it is not greater than 255 because it was delivered by u8 somewhere.
};

typedef int verdict;

typedef enum {
    ADVERTISING_CHANNEL_FAVOR_ADVERTISER = 0,
    ADVERTISING_CHANNEL_FAVOR_SCANNER_INITIATOR,
}   ADVERTISING_CHANNEL_PRIORITY;


verdict dut_reset_with_mask_en( u8 dut_fd );

void	read_le_event_enable();
void	read_le_event_disable();
int	read_le_event_get();
int		QUERY_LE_Event(u8 dut_fd ,u8 * buf ,u8 sub_event ,u8 adv_type);
verdict COMM_Send_ACL_Data (u8 dut_fd , u8 * conn_handle , u8 pb ,u8 bc ,u16 data_len,u8* data);
verdict COMM_Reset(u8 dut_fd);

#endif
