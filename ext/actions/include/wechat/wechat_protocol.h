#ifndef __WECHAT_PROTOCOL_H__
#define __WECHAT_PROTOCOL_H__

#define CMD_NULL		0
#define CMD_AUTH 		1
#define CMD_INIT 		2
#define CMD_SENDDAT		3

#define DEVICE_TYPE		"gh_f716795d7c6b"

#define DEVICE_ID		"gh_f716795d7c6b_2dc366b1844b914e"

#define PROTO_VERSION		0x010004
#define AUTH_PROTO		1

#define MAC_ADDRESS_LENGTH	6

#define CHALLENAGE_LENGTH	4

//#define EAM_md5AndNoEnrypt	1
//#define EAM_md5AndAesEnrypt	1
#define EAM_macNoEncrypt	2

#ifdef EAM_macNoEncrypt
#define AUTH_METHOD		EAM_macNoEncrypt
#define MD5_TYPE_AND_ID_LENGTH	0
#define CIPHER_TEXT_LENGTH	0
#endif

#ifdef EAM_md5AndAesEnrypt
#define AUTH_METHOD		EAM_md5AndAesEnrypt
#define MD5_TYPE_AND_ID_LENGTH	16
#define CIPHER_TEXT_LENGTH	16
#endif

#ifdef EAM_md5AndNoEnrypt
#define AUTH_METHOD		EAM_md5AndNoEnrypt
#define MD5_TYPE_AND_ID_LENGTH	16
#define CIPHER_TEXT_LENGTH	0
#endif

typedef struct {
	unsigned char		bMagicNumber;
	unsigned char		bVer;
	unsigned short		nLength;
	unsigned short		nCmdId;
	unsigned short		nSeq;
} cmd_fix_head_t;

typedef struct {
	char			*str;
	int			len;
} CString_t;

typedef struct {
	int			cmd;
	CString_t		send_msg;
} cmd_parameter_t;

typedef struct {
	bool			wechats_switch_state;
	bool			indication_state;
	bool			auth_state;
	bool			init_state;
	bool			auth_send;
	bool			init_send;
	unsigned short		send_data_seq;
	unsigned short		push_data_seq;
	unsigned short		seq;
} wxbt_state_t;

extern void wx_set_mac_address(uint8_t *mac_addr);

/*
 * return value:
 *	< 0 : error, check the error code for detail
 *	0   : data is not enough, need more data
 *	> 0 : parsed a complete pakcet,
 *	      return the consumed data in bytes.
 */
extern int data_consume_func_rec(uint8_t *data, uint32_t len,
				 uint8_t **raw_data, uint32_t *raw_len);

extern void data_produce_func_send(cmd_parameter_t *param, uint8_t **data,
				   uint32_t *len);
#endif

