
#ifndef _BT_HCI_LIB_H_
#define _BT_HCI_LIB_H_

// device signature
#define DEV_TI			1
#define DEV_SSV			2
#define DEV_CSR			3

#define _BT_HCI_W_INDICATOR_                1
#define _BT_HCI_W_ACL_DATA_CONN_HANDLE_     2
#define _BT_HCI_W_ACL_DATA_PB_FLAG_         2	//bit
#define _BT_HCI_W_ACL_DATA_PC_FALG_         2	//bit
#define _BT_HCI_W_ACL_DATA_LENGTH_          2
#define _BT_HCI_W_ACL_DATA_HEADER_          (_BT_HCI_W_INDICATOR_ + _BT_HCI_W_ACL_DATA_CONN_HANDLE_ + _BT_HCI_W_ACL_DATA_LENGTH_)
#define _BT_HCI_W_ACL_DATA_MAX_             251
#define _BT_HCI_W_EVENT_CODE_               1
#define _BT_HCI_W_EVENT_LENGTH_             1
#define _BT_HCI_W_EVENT_HEADER_             (_BT_HCI_W_INDICATOR_ + _BT_HCI_W_EVENT_CODE_ + _BT_HCI_W_EVENT_LENGTH_)
#define _BT_HCI_W_EVENT_NUM_OF_HANDLES	    1
#define _BT_HCI_W_EVENT_CONNECTION_HANDLE	2
#define _BT_HCI_W_EVENT_NUM_OF_PACKETS	    2

#define _BT_HCI_W_CMD_OPCODE_               2
#define _BT_HCI_W_CMD_LENGTH_               1
#define _BT_HCI_W_CMD_HEADER_               (_BT_HCI_W_INDICATOR_ + _BT_HCI_W_CMD_OPCODE_ + _BT_HCI_W_CMD_LENGTH_)

#define _BT_HCI_CMD_OPCODE_IDX_GROUP_       10
#define _BT_HCI_CMD_OPCODE_IDX_CMD_         0

#define _BT_HCI_IDX_INDICATOR_              0
#define _BT_HCI_IDX_EVENT_CODE_		    (_BT_HCI_IDX_INDICATOR_    + _BT_HCI_W_INDICATOR_   )
#define _BT_HCI_IDX_EVENT_LENGTH_		(_BT_HCI_IDX_EVENT_CODE_   + _BT_HCI_W_EVENT_CODE_  )
#define _BT_HCI_IDX_EVENT_PARAMETER_		(_BT_HCI_IDX_EVENT_LENGTH_ + _BT_HCI_W_EVENT_LENGTH_)

//# command complete event
#define _BT_HCI_IDX_EVENT_NUM_OF_HCI_COMMAND_PACKETS (_BT_HCI_IDX_EVENT_LENGTH_ + _BT_HCI_W_EVENT_LENGTH_)
#define _BT_HCI_IDX_EVENT_COMMAND_OP_		         (_BT_HCI_IDX_EVENT_NUM_OF_HCI_COMMAND_PACKETS +1 )

//# command status event
#define _BT_HCI_IDX_EVENT_STATUS                            (_BT_HCI_IDX_EVENT_LENGTH_ + _BT_HCI_W_EVENT_LENGTH_)
#define _BT_HCI_IDX_EVENT_STATUS_NUM_OF_HCI_COMMAND_PACKETS (_BT_HCI_IDX_EVENT_STATUS + 1)
#define _BT_HCI_IDX_EVENT_STATUS_COMMAND_OP_		        (_BT_HCI_IDX_EVENT_STATUS_NUM_OF_HCI_COMMAND_PACKETS +1 )

//# number of completed packets event
#define _BT_HCI_IDX_EVENT_NUM_OF_HANDLES	(_BT_HCI_IDX_EVENT_LENGTH_ + _BT_HCI_W_EVENT_LENGTH_)
#define _BT_HCI_IDX_EVENT_CONNECTION_HANDLE	(_BT_HCI_IDX_EVENT_NUM_OF_HANDLES + _BT_HCI_W_EVENT_NUM_OF_HANDLES)
#define _BT_HCI_IDX_EVENT_NUM_OF_PACKETS	(_BT_HCI_IDX_EVENT_CONNECTION_HANDLE + _BT_HCI_W_EVENT_CONNECTION_HANDLE)

#define _BT_HCI_IDX_INDICATOR_          0
#define _BT_HCI_IDX_CMD_OPCODE_         (_BT_HCI_IDX_INDICATOR_    + _BT_HCI_W_INDICATOR_ )
#define _BT_HCI_IDX_CMD_OPCODE_OCF      (_BT_HCI_IDX_INDICATOR_    + _BT_HCI_W_INDICATOR_ )
#define _BT_HCI_IDX_CMD_OPCODE_OGF      (_BT_HCI_IDX_INDICATOR_    + _BT_HCI_W_INDICATOR_ ) + 1
#define _BT_HCI_IDX_CMD_LENGTH_         (_BT_HCI_IDX_CMD_OPCODE_   + _BT_HCI_W_CMD_OPCODE_)
#define _BT_HCI_IDX_CMD_PARAMETER_      (_BT_HCI_IDX_CMD_LENGTH_   + _BT_HCI_W_CMD_LENGTH_)

#define _BT_HCI_IDX_ACL_DATA_CONN_HANDLE_   (_BT_HCI_IDX_INDICATOR_    + _BT_HCI_W_INDICATOR_ )
#define _BT_HCI_IDX_ACL_DATA_LENGTH_        (_BT_HCI_IDX_ACL_DATA_CONN_HANDLE_   + _BT_HCI_W_ACL_DATA_CONN_HANDLE_)
#define _BT_HCI_IDX_ACL_DATA_PAYLOAD_       (_BT_HCI_IDX_ACL_DATA_LENGTH_   + _BT_HCI_W_ACL_DATA_LENGTH_)

#define _BT_HCI_IDX_CMD_STATUS	            3

#define _BT_HCI_INDICATOR_COMMAND_          0x01
#define _BT_HCI_INDICATOR_ACL_DATA_         0x02
#define _BT_HCI_INDICATOR_SCO_DATA_         0x03
#define _BT_HCI_INDICATOR_EVENT_            0x04
#define _BT_HCI_INDICATOR_QUERY_ACL_DATA_   0x05
#define _BT_HCI_INDICATOR_QUERY_EVENT_      0x06
#define _BT_HCI_INDICATOR_COMMAND_HCILL_GO_TO_SLEEP_IND 0x30
#define _BT_HCI_INDICATOR_COMMAND_HCILL_GO_TO_SLEEP_ACK 0x31
#define _BT_HCI_INDICATOR_COMMAND_HCILL_WAKE_UP_IND     0x32
#define _BT_HCI_INDICATOR_COMMAND_HCILL_WAKE_UP_ACK     0x33


#define _BT_HCI_W_PARAMTER_MAX_             255

#define _BT_HCI_CMD_OGF_LE_CONTROLLER_COMMANDS_     0x08
#define _BT_HCI_CMD_OGF_VENDOR_SPECIFIC_            0x3f

#define _BT_HCI_PARAMETER_PROP_DEF_         0x000000    //  default
#define _BT_HCI_PARAMETER_PROP_VLEN_        0x000001    //  variable length
#define _BT_HCI_PARAMETER_PROP_NTBL_        0x000080    //  next table


// command OGF
#define _BT_HCI_CMD_OGF_NOP_                        0x00
#define _BT_HCI_CMD_OGF_LINK_CONTROL_               0x01
#define _BT_HCI_CMD_OGF_LINK_POLICY_                0x02
#define _BT_HCI_CMD_OGF_CONTROL_BASEBAND_           0X03
#define _BT_HCI_CMD_OGF_INFORMATION_PARAMETERS_     0X04
#define _BT_HCI_CMD_OGF_STATUS_PARAMETERS_          0x05
#define _BT_HCI_CMD_OGF_TESTING_                    0x06
#define _BT_HCI_CMD_OGF_LE_CONTROLLER_              0x08
#define _BT_HCI_CMD_OGF_VENDOR_SPECIFIC_            0x3f

#define CMD_OPCODE(OGF, OCF)    ((OGF << _BT_HCI_CMD_OPCODE_IDX_GROUP_) | ( OCF << _BT_HCI_CMD_OPCODE_IDX_CMD_))

// nop commands
#define _BT_HCI_CMD_OCF_NOP_                        0x0000
// ----
#define _BT_HCI_CMD_OPCODE_NOP_    CMD_OPCODE(_HCI_CMD_OGF_NOP_, _BT_HCI_CMD_OCF_NOP_)

// link control commands
#define _BT_HCI_CMD_OCF_DISCONNECT_                        0x0006
#define _BT_HCI_CMD_OCF_READ_REMOTE_VERSION_INFORMATION_   0x001D

// link policy commands
// // none

// control baseband command
#define _BT_HCI_CMD_OCF_SET_EVENT_MASK_                        0x0001
#define _BT_HCI_CMD_OCF_RESET_                                 0x0003
#define _BT_HCI_CMD_OCF_READ_TRANSMIT_POWER_LEVEL_             0x002D
#define _BT_HCI_CMD_OCF_SET_CONTROLLER_TO_HOST_FLOW_CONTROL_   0x0031
#define _BT_HCI_CMD_OCF_HOST_BUFFER_SIZE_                      0x0033
#define _BT_HCI_CMD_OCF_HOST_NUMBER_OF_COMPLETED_PACKETS_      0x0035
#define _BT_HCI_CMD_OCF_READ_LE_HOST_SUPPORT_                  0x006C
#define _BT_HCI_CMD_OCF_WRITE_LE_HOST_SUPPORT_                 0x006D

// information parameters command
#define _BT_HCI_CMD_OCF_LOCAL_VERSION_INFORMATION_             0x0001
#define _BT_HCI_CMD_OCF_READ_LOCAL_SUPPORTED_COMMANDS_         0x0002
#define _BT_HCI_CMD_OCF_READ_LOCAL_SUPPORTED_FEATURES_         0x0003
#define _BT_HCI_CMD_OCF_READ_BUFFER_SIZE_                      0x0005
#define _BT_HCI_CMD_OCF_READ_BD_ADDR_                          0x0009

// status parameters command
#define _BT_HCI_CMD_OCF_READ_RSSI_                             0x0005

// testing command
// // none

//LE advertising report event field
#define _BT_HCI_EVT_ADV_RPT_SUB_CODE			_BT_HCI_IDX_EVENT_PARAMETER_
#define _BT_HCI_EVT_ADV_RPT_NUM_RPT			    _BT_HCI_EVT_ADV_RPT_SUB_CODE	+	1
#define _BT_HCI_EVT_ADV_RPT_EVT_TYPE			_BT_HCI_EVT_ADV_RPT_NUM_RPT	+	1
#define _BT_HCI_EVT_ADV_RPT_ADDR_TYPE			_BT_HCI_EVT_ADV_RPT_EVT_TYPE	+	1
#define _BT_HCI_EVT_ADV_RPT_ADDR0				_BT_HCI_EVT_ADV_RPT_ADDR_TYPE	+	1
#define _BT_HCI_EVT_ADV_RPT_ADDR1				_BT_HCI_EVT_ADV_RPT_ADDR0	    +	1
#define _BT_HCI_EVT_ADV_RPT_ADDR2				_BT_HCI_EVT_ADV_RPT_ADDR1	    +	1
#define _BT_HCI_EVT_ADV_RPT_ADDR3				_BT_HCI_EVT_ADV_RPT_ADDR2	    +	1
#define _BT_HCI_EVT_ADV_RPT_ADDR4				_BT_HCI_EVT_ADV_RPT_ADDR3	    +	1
#define _BT_HCI_EVT_ADV_RPT_ADDR5				_BT_HCI_EVT_ADV_RPT_ADDR4	    +	1
#define _BT_HCI_EVT_ADV_RPT_DATA_LEN			_BT_HCI_EVT_ADV_RPT_ADDR5		+	1
#define _BT_HCI_EVT_ADV_RPT_DATA				_BT_HCI_EVT_ADV_RPT_DATA_LEN	+	1
#define _BT_HCI_EVT_ADV_RPT_RSSI(buf)			_BT_HCI_EVT_ADV_RPT_DATA		+	buf[_BT_HCI_EVT_ADV_RPT_DATA_LEN]

//LE conneciton complete event field
#define _BT_HCI_EVT_CON_COMP_SUB_CODE			_BT_HCI_IDX_EVENT_PARAMETER_
#define _BT_HCI_EVT_CON_COMP_STATUS			    _BT_HCI_EVT_CON_COMP_SUB_CODE	    +	1
#define _BT_HCI_EVT_CON_COMP_CONN_HANDLE		_BT_HCI_EVT_CON_COMP_STATUS	    +	1
#define _BT_HCI_EVT_CON_COMP_ROLE				_BT_HCI_EVT_CON_COMP_CONN_HANDLE    +	2
#define _BT_HCI_EVT_CON_COMP_PEER_ADDR_TYPE	    _BT_HCI_EVT_CON_COMP_ROLE		    +	1
#define _BT_HCI_EVT_CON_COMP_PEER_ADDR0		    _BT_HCI_EVT_CON_COMP_PEER_ADDR_TYPE	+	1
#define _BT_HCI_EVT_CON_COMP_PEER_ADDR1		    _BT_HCI_EVT_CON_COMP_PEER_ADDR0	    +	1
#define _BT_HCI_EVT_CON_COMP_PEER_ADDR2		    _BT_HCI_EVT_CON_COMP_PEER_ADDR1	    +	1
#define _BT_HCI_EVT_CON_COMP_PEER_ADDR3		    _BT_HCI_EVT_CON_COMP_PEER_ADDR2	    +	1
#define _BT_HCI_EVT_CON_COMP_PEER_ADDR4		    _BT_HCI_EVT_CON_COMP_PEER_ADDR3	    +	1
#define _BT_HCI_EVT_CON_COMP_PEER_ADDR5		    _BT_HCI_EVT_CON_COMP_PEER_ADDR4	    +	1
#define _BT_HCI_EVT_CON_COMP_CONN_INT			_BT_HCI_EVT_CON_COMP_PEER_ADDR5	    +	1
#define _BT_HCI_EVT_CON_COMP_CONN_LATENCY		_BT_HCI_EVT_CON_COMP_CONN_INT		+	2
#define _BT_HCI_EVT_CON_COMP_SUPER_TO			_BT_HCI_EVT_CON_COMP_CONN_LATENCY   +	2
#define _BT_HCI_EVT_CON_COMP_MASTER_CLK_ACCURACY	_BT_HCI_EVT_CON_COMP_SUPER_TO	+	2
//LE Long Term Key Request Event field
#define _BT_HCI_EVT_LTK_REQ_SUB_CODE			_BT_HCI_IDX_EVENT_PARAMETER_
#define _BT_HCI_EVT_LTK_REQ_CONN_HANDLES		_BT_HCI_EVT_LTK_REQ_SUB_CODE		+	1
#define _BT_HCI_EVT_LTK_REQ_RANDOM_NUM		    _BT_HCI_EVT_LTK_REQ_CONN_HANDLES	+	2
#define _BT_HCI_EVT_LTK_REQ_EDIV				_BT_HCI_EVT_LTK_REQ_RANDOM_NUM	    +	8
//status Error Codes
#define _BT_HCI_EVT_CMD_COMP_STATUS_UNKNOWN_CONNECTION_IDENTIFIER			0x02
#define _BT_HCI_EVT_CMD_COMP_STATUS_PIN_OR_KEY_MISSING			            0x06
#define _BT_HCI_EVT_CMD_COMP_STATUS_CONNECTION_TIMEOUT			            0x08
#define _BT_HCI_EVT_CMD_COMP_STATUS_COMMAND_DISALLOWED			            0x0C
#define _BT_HCI_EVT_CMD_COMP_STATUS_UNSUPPROTED_REMOTE_FEATURE_LMP          0x1A
#define _BT_HCI_EVT_CMD_COMP_STATUS_RESPONSE_TIMEOUT			            0x22
#define _BT_HCI_EVT_CMD_COMP_STATUS_DIRECTED_ADVERTISING_TIMEOUT	        0x3C
#define _BT_HCI_EVT_CMD_COMP_STATUS_CONNECTION_FAILED_TO_BE_ESTABLISHED		0x3E

#define _BT_HCI_EVT_COMP_STATUS			        _BT_HCI_IDX_EVENT_PARAMETER_        +   1
#define _BT_HCI_EVT_COMP_HANDLE		            _BT_HCI_EVT_COMP_STATUS             +	1

//LE disconnection complete event field
#define _BT_HCI_EVT_DISCONN_COMP_STATUS		    _BT_HCI_IDX_EVENT_PARAMETER_
#define _BT_HCI_EVT_DISCONN_COMP_HANDLE		    _BT_HCI_EVT_DISCONN_COMP_STATUS     +	1
#define _BT_HCI_EVT_DISCONN_COMP_REASON		    _BT_HCI_EVT_DISCONN_COMP_HANDLE     +	2

//read le buff size complete
#define _BT_HCI_EVT_LE_READ_BUFF_SIZE_STATUS               _BT_HCI_IDX_EVENT_PARAMETER_                     +   3
#define _BT_HCI_EVT_LE_READ_BUFF_SIZE_LE_DATA_PKT_LEN      _BT_HCI_EVT_LE_READ_BUFF_SIZE_STATUS             +   1
#define _BT_HCI_EVT_LE_READ_BUFF_SIZE_LE_DATA_PKT_NUM      _BT_HCI_EVT_LE_READ_BUFF_SIZE_LE_DATA_PKT_LEN    +   2

//read white list size
#define _BT_HCI_EVT_LE_READ_WHITE_LIST_SIZE_STATUS          _BT_HCI_IDX_EVENT_PARAMETER_                     +   3
#define _BT_HCI_EVT_LE_READ_WHITE_LIST_SIZE                 _BT_HCI_EVT_LE_READ_WHITE_LIST_SIZE_STATUS       +   1

//read local  version information complete
#define _BT_HCI_EVT_LOCAL_VER_INFO_STATUS				    _BT_HCI_IDX_EVENT_PARAMETER_
#define _BT_HCI_EVT_LOCAL_VER_INFO_HCI_VER		            _BT_HCI_EVT_LOCAL_VER_INFO_STATUS              +	1
#define _BT_HCI_EVT_LOCAL_VER_INFO_HCI_REV					_BT_HCI_EVT_LOCAL_VER_INFO_HCI_VER             +	1
#define _BT_HCI_EVT_LOCAL_VER_INFO_LMP_VER		            _BT_HCI_EVT_LOCAL_VER_INFO_HCI_REV             +	2
#define _BT_HCI_EVT_LOCAL_VER_INFO_MANU_NAME				_BT_HCI_EVT_LOCAL_VER_INFO_LMP_VER             +	1
#define _BT_HCI_EVT_LOCAL_VER_INFO_LMP_SUBVER				_BT_HCI_EVT_LOCAL_VER_INFO_MANU_NAME           +	2

//read remote version information complete
#define _BT_HCI_EVT_REMOTE_VER_INFO_STATUS				    _BT_HCI_IDX_EVENT_PARAMETER_
#define _BT_HCI_EVT_REMOTE_VER_INFO_CONN_HANDLE		        _BT_HCI_EVT_REMOTE_VER_INFO_STATUS              +	1
#define _BT_HCI_EVT_REMOTE_VER_INFO_VER					    _BT_HCI_EVT_REMOTE_VER_INFO_CONN_HANDLE         +	2
#define _BT_HCI_EVT_REMOTE_VER_INFO_MANU_NAME		        _BT_HCI_EVT_REMOTE_VER_INFO_VER                 +	1
#define _BT_HCI_EVT_REMOTE_VER_INFO_SUBVER				    _BT_HCI_EVT_REMOTE_VER_INFO_MANU_NAME           +	2

//read remote used features complete
#define _BT_HCI_EVT_REMOTE_USED_FEATURES_SUB_CODE		    _BT_HCI_IDX_EVENT_PARAMETER_
#define _BT_HCI_EVT_REMOTE_USED_FEATURES_STATUS				_BT_HCI_EVT_REMOTE_USED_FEATURES_SUB_CODE       +	1
#define _BT_HCI_EVT_REMOTE_USED_FEATURES_HANDLE				_BT_HCI_EVT_REMOTE_USED_FEATURES_STATUS         +	1
#define _BT_HCI_EVT_REMOTE_USED_FEATURES_LE_FEATURES		_BT_HCI_EVT_REMOTE_USED_FEATURES_HANDLE         +	2

//read channel map complete
#define _BT_HCI_EVT_LE_READ_CHANNEL_MAP_STATUS		        _BT_HCI_IDX_EVENT_PARAMETER_
#define _BT_HCI_EVT_LE_READ_CHANNEL_MAP_CONN_HANDLE		    _BT_HCI_EVT_LE_READ_CHANNEL_MAP_STATUS          +	1
#define _BT_HCI_EVT_LE_READ_CHANNEL_MAP_CHANNEL_MAP		    _BT_HCI_EVT_LE_READ_CHANNEL_MAP_CONN_HANDLE     +	2


// LE controller commands
#define _BT_HCI_CMD_OCF_LE_SET_EVENT_MASK_                         0x0001
#define _BT_HCI_CMD_OCF_LE_READ_BUFFER_SIZE_                       0x0002
#define _BT_HCI_CMD_OCF_LE_READ_LOCAL_SUPPORTED_FEATURES_          0x0003
#define _BT_HCI_CMD_OCF_LE_SET_RANDOM_ADDRESS_                     0x0005
#define _BT_HCI_CMD_OCF_LE_SET_ADVERTISING_PARAMETERS_             0x0006
#define _BT_HCI_CMD_OCF_LE_READ_ADVERTISING_CHANNEL_TX_POWER_      0x0007
#define _BT_HCI_CMD_OCF_LE_SET_ADVERTISING_DATA_                   0x0008
#define _BT_HCI_CMD_OCF_LE_SET_SCAN_RESPONSE_DATA_                 0x0009
#define _BT_HCI_CMD_OCF_LE_SET_ADVERTISE_ENABLE_                   0x000A
#define _BT_HCI_CMD_OCF_LE_SET_SCAN_PARAMETERS_                    0x000B
#define _BT_HCI_CMD_OCF_LE_SET_SCAN_ENABLE_                        0x000C
#define _BT_HCI_CMD_OCF_LE_CREATE_CONNECTION_                      0x000D
#define _BT_HCI_CMD_OCF_LE_CREATE_CONNECTION_CANCEL_               0x000E
#define _BT_HCI_CMD_OCF_LE_READ_WHITE_LIST_SIZE_                   0x000F
#define _BT_HCI_CMD_OCF_LE_CLEAR_WHITE_LIST_                       0x0010
#define _BT_HCI_CMD_OCF_LE_ADD_DEVICE_TO_WHITE_LIST_               0x0011
#define _BT_HCI_CMD_OCF_LE_REMOVE_DEVICE_FROM_WHITE_LIST_          0x0012
#define _BT_HCI_CMD_OCF_LE_CONNECTION_UPDATE_                      0x0013
#define _BT_HCI_CMD_OCF_LE_SET_HOST_CHANNEL_CLASSIFICATION_        0x0014
#define _BT_HCI_CMD_OCF_LE_READ_CHANNEL_MAP_                       0x0015
#define _BT_HCI_CMD_OCF_LE_READ_REMOTE_USED_FEATURES_              0x0016
#define _BT_HCI_CMD_OCF_LE_ENCRYPT_                                0x0017
#define _BT_HCI_CMD_OCF_LE_RAND_                                   0x0018
#define _BT_HCI_CMD_OCF_LE_START_ENCRYPTION_                       0x0019
#define _BT_HCI_CMD_OCF_LE_LONG_TERM_KEY_REQUEST_REPLY_            0x001A
#define _BT_HCI_CMD_OCF_LE_LONG_TERM_KEY_REQUEST_NEGATIVE_REPLY_   0x001B
#define _BT_HCI_CMD_OCF_LE_READ_SUPPORTED_STATES_                  0x001C
#define _BT_HCI_CMD_OCF_LE_RECEIVER_TEST_                          0x001D
#define _BT_HCI_CMD_OCF_LE_TRANSMITTER_TEST_                       0x001E
#define _BT_HCI_CMD_OCF_LE_TEST_END_                               0x001F

//LE encryption change event field
#define _BT_HCI_EVT_ENCRYPTION_CHANGE_STATUS			_BT_HCI_IDX_EVENT_PARAMETER_
#define _BT_HCI_EVT_ENCRYPTION_CHANGE_CONN_HANDLE	    _BT_HCI_EVT_ENCRYPTION_CHANGE_STATUS        +	1
#define _BT_HCI_EVT_ENCRYPTION_CHANGE_ENABLED			_BT_HCI_EVT_ENCRYPTION_CHANGE_CONN_HANDLE   +	2

//LE encryption key refresh complete field
#define _BT_HCI_EVT_ENCRYPTION_KEY_REFRESH_COMPLETE_STATUS		_BT_HCI_IDX_EVENT_PARAMETER_
#define _BT_HCI_EVT_ENCRYPTION_KEY_REFRESH_COMPLETE_CONN_HANDLE	\
        _BT_HCI_EVT_ENCRYPTION_KEY_REFRESH_COMPLETE_STATUS +    1

// TI-VS command
#define _BT_HCI_CMD_OCF_TI_SET_LE_TEST_MODE_PARAMETERS_     0x0177
#define _BT_HCI_CMD_OCF_TI_DRPB_RESET_                      0x0188
#define _BT_HCI_CMD_OCF_TI_READ_RSSI_                       0x01FC
#define _BT_HCI_CMD_OCF_TI_SLEEP_MODE_CONFIGURATIONS_       0x010C
#define _BT_HCI_CMD_OCF_TI_LE_OUTPUT_POWER_                 0x01DD
#define _BT_HCI_CMD_OCF_TI_GET_SYSTEM_STATUS_               0x021F
#define _BT_HCI_CMD_OCF_TI_UPDATE_UART_HCI_BAUDRATE_        0x0336
#define _BT_HCI_CMD_OCF_TI_WRITE_BD_ADDR_                   0x0006
#define _BT_HCI_CMD_OCF_TI_READ_BER_METER_RESULT_           0x0113
#define _BT_HCI_CMD_OCF_TI_CONT_TX_MODE_                    0x0184
#define _BT_HCI_CMD_OCF_TI_PKT_TX_RX_                       0x0185
#define _BT_HCI_CMD_OCF_TI_BER_METER_START_                 0x018B


// SSV-VS command
#define _BT_HCI_CMD_OCF_SSV_REG_READ_                       0x0001
#define _BT_HCI_CMD_OCF_SSV_REG_WRITE_                      0x0002

#ifdef CTRL_V1_DTM
#define _BT_HCI_CMD_OCF_SSV_LE_TRANSMITTER_TEST_            0x0003//0x0010
#else
#define _BT_HCI_CMD_OCF_SSV_BLE_INIT_                         0x0003
#define _BT_HCI_CMD_OCF_SSV_SLAVE_SUBRATE_                    0x0004
#define _BT_HCI_CMD_OCF_SSV_SET_ADVERTISING_CHANNEL_PRIORITY_ 0x0005
#define _BT_HCI_CMD_OCF_SSV_ACL_EVT_TO_EXTERNAL_HOST_         0x0006

#define _BT_HCI_CMD_OCF_SSV_LE_TRANSMITTER_TEST_            0x00f0
#endif
#define _BT_HCI_CMD_OCF_DUT_CLEAN_BUFFER_                   0x0020
#define _BT_HCI_CMD_OCF_DUT_CHECK_PAYLOAD_                  0x0021
#define _BT_HCI_CMD_OCF_DUT_QUERY_ADV_CNT_                  0x0022
#define _BT_HCI_CMD_OCF_DUT_QUERY_NUM_OF_PACKETS_CNT_       0x0023
#define _BT_HCI_CMD_OCF_DUT_QUERY_DATA_BUFFER_OVERFLOW_CNT_ 0x0024
#define _BT_HCI_CMD_OCF_DUT_QUERY_RX_ACL_PACKETS_CNT_       0x0025

// CSR-VS command

// event OP-code
#define _BT_HCI_EVENT_OP_DISCONNECTION_COMPLETE_                        0x05
#define _BT_HCI_EVENT_OP_ENCRYPTION_CHANGE_                             0x08
#define _BT_HCI_EVENT_OP_READ_REMOTE_VERSION_INFORMATION_COMPLETE_      0x0c
#define _BT_HCI_EVENT_OP_COMMAND_COMPLETE_                              0x0e
#define _BT_HCI_EVENT_OP_COMMAND_STATUS_                                0x0f
#define _BT_HCI_EVENT_OP_HARDWARE_ERROR_                                0x10
#define _BT_HCI_EVENT_OP_NUMBER_OF_COMPLETED_PACKETS_                   0x13
#define _BT_HCI_EVENT_OP_DATA_BUFFER_OVERFLOW_                          0x1a
#define _BT_HCI_EVENT_OP_ENCRYPTION_KEY_REFRESH_COMPLETE_               0x30
#define _BT_HCI_EVENT_OP_LE_EVENT_                                      0x3e

#define _BT_HCI_LE_EVENT_SUB_OP_CONNECTION_COMPLETE_                    0x01
#define _BT_HCI_LE_EVENT_SUB_OP_ADVERTISING_REPORT_                     0x02
#define _BT_HCI_LE_EVENT_SUB_OP_CONNECTION_UPDATE_COMPLETE_             0x03
#define _BT_HCI_LE_EVENT_SUB_OP_READ_REMOTE_USED_FEATURES_COMPLETE_     0x04
#define _BT_HCI_LE_EVENT_SUB_OP_LONG_TERM_KEY_REQUEST_                  0x05


//subevent event type
#define HCI_SUBEVENT_ADV_RPT_EVENT_ADV_IND		                        0x00
#define HCI_SUBEVENT_ADV_RPT_EVENT_DIRECT_IND	                        0x01
#define HCI_SUBEVENT_ADV_RPT_EVENT_SCAN_IND	                            0x02
#define HCI_SUBEVENT_ADV_RPT_EVENT_NONCONN_IND	                        0x03
#define HCI_SUBEVENT_ADV_RPT_EVENT_SCAN_RSP	                            0x04


//HCI command parameters
//Advertising_Interval_Min / Max
#define HCI_CMD_PARAM_ADVERTISING_INTERVAL_MIN                          0x0020		//32*0.625ms=20ms
#define HCI_CMD_PARAM_ADVERTISING_INTERVAL_MAX                          0x4000
#define HCI_CMD_PARAM_ADVERTISING_INTERVAL_MIN_ADV_DIRECT_IND           0x06
#define HCI_CMD_PARAM_ADVERTISING_INTERVAL_MIN_ADV_SCAN_OR_NONCONN_IND  0x00A0

//Advertising_Type
#define HCI_CMD_PARAM_ADV_IND	                                        0x00
#define HCI_CMD_PARAM_ADV_DIRECT_IND	                                0x01
#define HCI_CMD_PARAM_ADV_SCAN_IND	                                    0x02
#define HCI_CMD_PARAM_ADV_NONCONN_IND	                                0x03

//Own_Address_Type , Direct_Address_Type ,Peer_Address_Type
#define HCI_CMD_PARAM_PUBLIC_DEVICE_ADDRESS                             0x00
#define HCI_CMD_PARAM_RANDOM_DEVICE_ADDRESS                             0x01
//Advertising_Channel_Map
#define HCI_CMD_PARAM_ENABLE_CHANNEL_37                                 0b00000001
#define HCI_CMD_PARAM_ENABLE_CHANNEL_38                                 0b00000010
#define HCI_CMD_PARAM_ENABLE_CHANNEL_39                                 0b00000100
//Advertising_Filter_Policy
#define HCI_CMD_PARAM_ALLOW_SCAN_REQ_AND_CONN_REQ_FROM_ANY                                  0x00
#define HCI_CMD_PARAM_ALLOW_SCAN_REQ_FROM_WHITELIST_ONLY_AND_CONN_REQ_FROM_ANY              0x01
#define HCI_CMD_PARAM_ALLOW_SCAN_REQ_FROM_ANY_AND_CONN_REQ_FROM_WHITELIST_ONLY              0x02
#define HCI_CMD_PARAM_ALLOW_SCAN_REQ_FROM_WHITELIST_ONLY_AND_CONN_REQ_FROM_WHITELIST_ONLY   0x03
//Advertising_Enable
#define HCI_CMD_PARAM_ADV_DISABLE           0x00
#define HCI_CMD_PARAM_ADV_ENABLE            0x01
//Advertising_Enable_Data_Length
#define HCI_CMD_PARAM_ADV_DATA_LEN_MAX      31
//Scan_Response_Data_Length
#define HCI_CMD_PARAM_SCAN_RSP_DATA_LEN_MAX 31
//LE_Scan_Type
#define HCI_CMD_PARAM_PASSIVE_SCAN          0x00
#define HCI_CMD_PARAM_ACTIVE_SCAN           0x01
//LE_Scan_Interval
#define HCI_CMD_PARAM_SCAN_INT_MIN          0x0004
#define HCI_CMD_PARAM_SCAN_INT_MAX          0x4000
//LE_Scan_Windows
#define HCI_CMD_PARAM_SCAN_WIN_MIN          0x0004
#define HCI_CMD_PARAM_SCAN_WIN_MAX          0x4000
//Scanning_Filter_Policy
#define HCI_CMD_PARAM_ACCEPT_ALL_ADV_PKTS   0x00
#define HCI_CMD_PARAM_IGNORE_ADV_PKTS_FROM_DEV_NOT_IN_WHITE_LIST_ONLY 0x01
//LE_Scan_Enable
#define HCI_CMD_PARAM_SCAN_DISABLE          0x00
#define HCI_CMD_PARAM_SCAN_ENABLE           0x01
//Filter_Duplicates
#define HCI_CMD_PARAM_FILTER_DUPLICATES_DISABLE	0x00
#define HCI_CMD_PARAM_FILTER_DUPLICATES_ENABLE	0x01
//LE_Event_Mask
#define HCI_CMD_PARAM_LE_EVENT_MASK_NO_SPECIFIED                    0x00
#define HCI_CMD_PARAM_LE_EVENT_MASK_CONN_COMP                       0x01
#define HCI_CMD_PARAM_LE_EVENT_MASK_ADV_RPT                         0x02
#define HCI_CMD_PARAM_LE_EVENT_MASK_CONN_UPDATE_COMP                0x04
#define HCI_CMD_PARAM_LE_EVENT_MASK_READ_REMOTE_USED_FEATURES_COMP  0x08
#define HCI_CMD_PARAM_LE_EVENT_MASK_LONG_TERM_KEY_REQ               0x10
#define HCI_CMD_PARAM_LE_EVENT_MASK_DEFAULT                         0x1F

//Event_Mask
#define HCI_CMD_PARAM_LE_META           0x20
//LE_Scan_Interval
#define HCI_CMD_PARAM_LE_SCAN_INT_MIN	0x0004
#define HCI_CMD_PARAM_LE_SCAN_INT_MAX	0x4000
//LE_Scan_Window
#define HCI_CMD_PARAM_LE_SCAN_WIN_MIN	0x0004
#define HCI_CMD_PARAM_LE_SCAN_WIN_MAX	0x4000
//Initiator_Filter_Policy
#define HCI_CMD_PARAM_FILTER_POLICY_WHITE_LIST_NOT_USED	0x00
#define HCI_CMD_PARAM_FILTER_POLICY_WHITE_LIST_USED	    0x01
//Conn_Interval_Min ,max
#define HCI_CMD_PARAM_CONN_INT_MIN	    0x0006
#define HCI_CMD_PARAM_CONN_INT_MAX	    0x0C80
//Conn_latency
// # at 4.0 ,change from 6 to 0x0C80 / 0x1F4
// # at 4.1 ,change from 0 to 0x01F3 ; < 500
#define HCI_CMD_PARAM_CONN_LATENCY_MIN	0x0000
#define HCI_CMD_PARAM_CONN_LATENCY_MAX	0x0C80

#define HCI_CMD_PARAM_CONN_LATENCY_MAX_V42 0x01F3

//Supervision_Timeout:
#define HCI_CMD_PARAM_SUPERVISION_TO_MIN	0x000A
#define HCI_CMD_PARAM_SUPERVISION_TO_MAX	0x0C80
//Minimum_CE_Length , Maximum_CE_Length:
#define HCI_CMD_PARAM_CE_LENGTH_MIN	    0x0000
#define HCI_CMD_PARAM_CE_LENGHT_MAX	    0xFFFF

#define HCI_CMD_PARAM_MAX_SLAVE_LATENCY_WITH_MAX_CONN_INT_AND_TO 6

//ERROR CODE DESCRIPTIONS
#define HCI_CMD_PARAM_CONNECTION_TIMEOUT							                0x08
#define HCI_CMD_PARAM_REMOTE_USER_TERMINATED_CONNECTION								0x13
#define HCI_CMD_PARAM_REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_LOW_RESOURCES	    0x14
#define HCI_CMD_PARAM_REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_POWER_OFF		    0x15
#define HCI_CMD_PARAM_CONNECTION_TERMINATED_BY_LOCAL_HOST							0x16
#define HCI_CMD_PARAM_LMP_LL_Response_Timeout                                       0x22
#define HCI_CMD_PARAM_INSTANT_PASSED                                                0x28

// property in HCI command structure
#define _BT_HCI_PARAMETER_PROP_DEFAULT_            0x00
#define _BT_HCI_PARAMETER_PROP_VARIABLE_LENGTH_    0x01

// macros
#define PRINT_DATA(type, cmd, length) {\
    int len = 0;\
    unsigned char *tmp = (unsigned char *)cmd;\
    if (tmp == NULL){\
        printf("input cmd is NULL !\n");\
        return ;\
    }\
    if (type == CMD_TYPE){\
        printf("[cmd]  type: %02x    opcode: %02x%02x    len: %02x    param:", cmd[0], cmd[1], cmd[2], cmd[3]);\
        while (len < cmd[3]){\
            printf("%02x",cmd[len + 4]);\
            len++;\
        }}\
    if (type == EVENT_TYPE){\
    printf("[event]type: %02x    opcode: %02x      len: %02x    param:", cmd[0], cmd[1], cmd[2]);\
    while (len < cmd[2]){\
        printf("%02x",cmd[len + 3]);\
        len++;\
    }}\
    printf("\n");};

#define COPY_AND_MOVE(dst, src, len) {memcpy(dst, src, len);dst += len; src += len;}

#define DECLARE_BT_HCI_CMD_OPCODE(_GROUP_, _CMD_) { \
            ((BUILD_BT_HCI_CMD_OPCODE(_GROUP_, _CMD_) & 0x00ff) >> 0),    \
            ((BUILD_BT_HCI_CMD_OPCODE(_GROUP_, _CMD_) & 0xff00) >> 8),    \
        }

#define BT_HCI_VS_PATCH_BYCHARRAY(fd, charray)  bt_hci_vs_patch(fd, sizeof(charray), charray)

enum {
    CMD_TYPE,
    EVENT_TYPE
}DATA_TYPE;

typedef struct bt_hci_parameter_entry_st {
    int     length;
    int     property;
    void*   value;
} bt_hci_parameter_entry;

typedef int (*FX_CMD_RET_HANDLE) (const int parameter_tbl_size,     bt_hci_parameter_entry*  const parameter_tbl,
                                  const int ret_parameter_tbl_size, bt_hci_parameter_entry*  const ret_parameter_tbl);
typedef int (*FX_EVENT_HANDLE)   (const int parameter_tbl_size,     bt_hci_parameter_entry*  const parameter_tbl    );

typedef struct bt_hci_acl_data_st {
	unsigned   conn_handle:12;
	unsigned   pb:2;
	unsigned   bc:2;
	u16		length;
	u8		*data;//[_BT_HCI_W_ACL_DATA_MAX_];
} bt_hci_acl_data;

typedef struct bt_hci_cmd_st {
    int                         opcode_group;
    int                         opcode;
    bt_hci_parameter_entry*     parameter_tbl;
    int                         parameter_tbl_size;
    bt_hci_parameter_entry*     ret_parameter_tbl;
    int                         ret_parameter_tbl_size;
    FX_CMD_RET_HANDLE           ret_handle;
    char*                       name;
} bt_hci_cmd;

typedef struct bt_hci_event_st {
    unsigned char               opcode;
    bt_hci_parameter_entry*     parameter_tbl;
    int                         parameter_tbl_size;
    FX_EVENT_HANDLE             handle;
    char*                       name;
} bt_hci_event;

typedef struct bt_hci_le_event_st {
    u32                         sub_opcode;
    bt_hci_parameter_entry*     parameter_tbl;
    u32                         parameter_tbl_size;
    FX_EVENT_HANDLE             handle;
    char*                       name;
} bt_hci_le_event;

void bt_hci_init_csr(void);
int bt_hci_read_csr(int fd, u8 *rxbuf, int buf_len);
int bt_hci_read_icmd(int fd, u8 *rxbuf);
int bt_hci_read(int fd, u8 *rxbuf);
int bt_hci_read_event(int fd, unsigned char *code, unsigned char *length, unsigned char *parameter);
int bt_hci_vs_patch  (int fd, int patch_size, unsigned char *patch);

int bt_hci_parameter_building (unsigned char parameter_tbl_size, bt_hci_parameter_entry *parameter_tbl,\
                               int length_max, unsigned char *parameter);

int bt_hci_event_parsing(unsigned char code, unsigned char length, unsigned char *parameter);
int bt_hci_write     (int fd, int buf_length,     unsigned char* buf);
// the csr version's bt_hci_write()
// before calling this function, plz make sure device is DEV_CSR.
int bt_hci_write_4csr(int fd, int buf_length,     unsigned char* buf);
int bt_hci_write_cmd(int fd, bt_hci_cmd* p_cmd);

char*	hci_error_code_str(u8 code);
char*	hci_event_code_str(u8 code);
void	hci_event_socket_dump(u8 *buf, int len);

#endif
