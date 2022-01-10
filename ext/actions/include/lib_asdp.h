#ifndef __LIB_ASDP_H
#define __LIB_ASDP_H

typedef int (*asdp_port_cmd_handler_t)(u8_t* tlv_list, u16_t len);
typedef int (*asdp_port_srv_handler_t)(u8_t* asdp_frame, u32_t len);

struct tlv {
	u8_t type;
	u16_t len;
	u8_t *val;
};

enum ASDP_SERVICE_ID {
	ASDP_DEVICE_MANAGER_SERVICE_ID = 0x01,
	ASDP_BT_MANAGER_SERVICE_ID = 0x02,
	ASDP_OTA_SERVICE_ID = 0x09,
};

struct asdp_port_head {
	u8_t svc_id;
	u8_t cmd;
	u8_t param_type;
	u16_t param_len;
} __attribute__((packed));

struct asdp_port_cmd {
	u8_t cmd;
	asdp_port_cmd_handler_t handler;
};

struct asdp_port_srv {
	u8_t srv_id;
	asdp_port_srv_handler_t handler;
};

int asdp_service_open(void);
int adsp_service_close(void);
int asdp_register_service_handle(const struct asdp_port_srv *srv_proc);
int asdp_unregister_service_handle(u8_t service_id);

int asdp_send_cmd(u8_t srv, u8_t cmd, u16_t len);

typedef enum
{
	BT_ERROR_CODE_NO_ERROR = 100000,
}bt_error_code_e;

typedef enum
{
	ASDP_DM_CONNECT_NEGOTIATION = 0x01,
	ASDP_DM_SERVICE_SUPPORT_LIST = 0x02,
	ASDP_DM_CMD_SUPPORT_LIST = 0x03,
	ASDP_DM_SET_TIME_FORMAT = 0x04,
	ASDP_DM_SET_DATE_TIME = 0x05,
	ASDP_DM_GET_DATE_TIME = 0x06,
	ASDP_DM_GET_FIRMWARE_VERSION = 0x07,
	ASDP_DM_GET_BATTERY_CAPACITY = 0x08,
	ASDP_DM_FACTORY_RESET = 0x0d,
	ASDP_DM_SET_CONNECT_PARAM = 0x11,
	ASDP_DM_SET_BATTERY_CAPACITY = 0x27,//4B, 0x1:1:1; resp: 7B, 0x7f:4:errcode
	ASDP_DM_VOLUME_ADD = 0x80,//4B, 0x1:1:1; resp: 7B, 0x7f:4:errcode
	ASDP_DM_VOLUME_SUB = 0x81,//4B, 0x1:1:1; resp: 7B, 0x7f:4:errcode
	ASDP_DM_SET_VOLUME = 0x82,
	ASDP_DM_POWER_OFF = 0x83,//4B, 0x1:1:1; resp: 12B, 0x7f:4:errcode, 0x02:2:delay
	ASDP_DM_GET_BT_STATUS = 0x84,//14B, 0x1:4:BT_STATUS, 0x7:4:ota
	ASDP_DM_GET_OTA_DATA = 0x85,
}device_manager_cmd_e;

typedef enum
{
	ASDP_BM_PLAY = 0x01,//4B, 0x1:1:1; resp: 7B, 0x7f:4:errcode
	ASDP_BM_PAUSE = 0x02,//4B, 0x1:1:1; resp: 7B, 0x7f:4:errcode
	ASDP_BM_PREV = 0x03,//4B, 0x1:1:1; resp: 7B, 0x7f:4:errcode
	ASDP_BM_NEXT = 0x04,//4B, 0x1:1:1; resp: 7B, 0x7f:4:errcode
	ASDP_BM_CALLING_ACCEPT = 0x10,
	ASDP_BM_CALLING_HANGUP = 0x11,
	ASDP_BM_CALLING_REJECT = 0x12,
	ASDP_BM_READ_SPPBLE_DATA = 0x20,
	ASDP_BM_WRITE_SPPBLE_DATA = 0x21,

	ASDP_BM_SET_VISIBLE = 0x80,
	ASDP_BM_SET_CONNECTABLE = 0x81,
	ASDP_BM_DISCONNECT_MOBILE = 0x82,
	ASDP_BM_DISCONNECT_TWS = 0x83,
	ASDP_BM_DISCONNECT_ALL_DEVICE = 0x84,
	ASDP_BM_SET_BT_NAME = 0x90,
	ASDP_BM_SET_BLE_NAME = 0x91,
	ASDP_BM_SET_BT_MAC = 0x92,
	ASDP_BM_GET_BT_NAME = 0x93,
	ASDP_BM_GET_BLE_NAME = 0x94,
	ASDP_BM_GET_BT_MAC = 0x95,
	ASDP_BM_VOLUME_ADD = 0x96,
	ASDP_BM_VOLUME_SUB = 0x97,
}bt_manager_cmd_e;


enum SS_BT_STATUS
{
	SS_BT_STATUS_NONE                  = 0x0001,    //
	SS_BT_STATUS_WAIT_PAIR             = 0x0002,    //
	SS_BT_STATUS_MASTER_WAIT_PAIR      = 0x0004,    //
	SS_BT_STATUS_PAUSED                = 0x0008,    // <"Connected">
	SS_BT_STATUS_PLAYING               = 0x0010,    //
	SS_BT_STATUS_INCOMING              = 0x0020,    //
	SS_BT_STATUS_OUTGOING              = 0x0040,    //
	SS_BT_STATUS_ONGOING               = 0x0080,    // <"in bt call">
	SS_BT_STATUS_MULTIPARTY            = 0x0100,    // <"3 sides calling">
	SS_BT_STATUS_SIRI                  = 0x0200,    // <"Voice identifying">
	SS_BT_STATUS_3WAYIN                = 0x0400,    // <"3 sides call incoming">
};

//Device Managment -> System Status (0x84)
#define TLV_TYPE_DM_SS_BT	0x01
#define TLV_TYPE_DM_SS_A2DP 	0x02
#define TLV_TYPE_DM_SS_HFP	0x03
#define TLV_TYPE_DM_SS_SCN	0x04
#define TLV_TYPE_DM_SS_CCN	0x05
#define TLV_TYPE_DM_SS_CALLRING	0x06
#define TLV_TYPE_DM_SS_OTA	0x07

#define TLV_TYPE_DM_BATTERY_CAPACITY    (0x01)
#define TLV_TYPE_DM_VOLUME    (0x01)

#define TLV_TYPE_DM_GET_VOLUME		(0xa1)
#define TLV_TYPE_DM_USB_STATUS		(0xa2)
#define TLV_TYPE_DM_GET_SAMPLERATE		(0xa3)


//Device Managment -> System Power off (0x83)
#define TLV_TYPE_DM_POWER_OFF_DELAY    (0x02)

//Bluetooth management
#define TLV_TYPE_BM_BT_NAME    (0x01)
#define TLV_TYPE_BM_OTA_DATA (0x01)
#define TLV_TYPE_BM_OTA_DATA_MAX_LEN (0x02)

#define TLV_TYPE_OTA_BATTERY_THREADHOLD		(0x04)
#define TLV_TYPE_OTA_SUPPORT_FEATURES		(0x09)


#endif
