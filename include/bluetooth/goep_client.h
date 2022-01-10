/** @file
 *  @brief Bluetooth GOEP client profile
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_GOEP_CLIENT_H
#define __BT_GOEP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
	GOEP_SEND_HEADER_CONNECTING_ID	= (0x1 << 0),
	GOEP_SEND_HEADER_NAME 			= (0x1 << 1),
	GOEP_SEND_HEADER_TYPE 			= (0x1 << 2),
	GOEP_SEND_HEADER_APP_PARAM 		= (0x1 << 3),
	GOEP_SEND_HEADER_FLAGS 		    = (0x1 << 4),
};

enum {
	GOEP_RX_HEADER_APP_PARAM		= (0x1 << 0),
	GOEP_RX_HEADER_BODY				= (0x1 << 1),
	GOEP_RX_HEADER_END_OF_BODY		= (0x1 << 2),
	GOEP_RX_CONTINUE_WITHOUT_BODY	= (0x1 << 3),
};

struct bt_goep_header_param {
	u16_t header_bit;
	u16_t param_len;
	void *param;
};

struct bt_goep_client_cb {
	void (*connected)(struct bt_conn *conn, u8_t goep_id);
	void (*aborted)(struct bt_conn *conn, u8_t goep_id);
	void (*disconnected)(struct bt_conn *conn, u8_t goep_id);
	void (*recv)(struct bt_conn *conn, u8_t goep_id, u16_t header_bit, u8_t *data, u16_t len);
	void (*setpath)(struct bt_conn *conn, u8_t goep_id);
};

u16_t bt_goep_client_connect(struct bt_conn *conn, u8_t *target_uuid, u8_t uuid_len, u8_t server_channel,
									u16_t connect_psm, struct bt_goep_client_cb *cb);
int bt_goep_client_get(struct bt_conn *conn, u8_t goep_id, u16_t headers,
					struct bt_goep_header_param *param, u8_t array_size);
int bt_goep_client_disconnect(struct bt_conn *conn, u8_t goep_id, u16_t headers);
int bt_goep_client_aboot(struct bt_conn *conn, u8_t goep_id, u16_t headers);
int bt_goep_client_setpath(struct bt_conn *conn, u8_t goep_id, u16_t headers,
					struct bt_goep_header_param *param, u8_t array_size);
#ifdef __cplusplus
}
#endif

#endif /* __BT_GOEP_CLIENT_H */
