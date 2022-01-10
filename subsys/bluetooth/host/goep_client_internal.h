/** @file
 *  @brief Bluetooth GOEP client profile internal headfile
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_GOEP_CLIENT_INTERNAL_H
#define __BT_GOEP_CLIENT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

#define GOEP_CREATE_HEADER_ID(encoding, meaning)		((((encoding)&0x03) << 6) | ((meaning)&0x3F))
#define GOEP_GET_VALUE_16(addr)		sys_be16_to_cpu(UNALIGNED_GET((u16_t *)(addr)))
#define GOEP_PUT_VALUE_16(value, addr)	UNALIGNED_PUT(sys_cpu_to_be16(value), (u16_t *)addr)
#define GOEP_GET_VALUE_32(addr)		sys_be32_to_cpu(UNALIGNED_GET((u32_t *)(addr)))
#define GOEP_PUT_VALUE_32(value, addr)	UNALIGNED_PUT(sys_cpu_to_be32(value), (u32_t *)addr)

enum {
	GOEP_OPCODE_REQ_CONNECT = 0x00,
	GOEP_OPCODE_REQ_DISCONNECT = 0x01,
	GOEP_OPCODE_REQ_GET = 0x03,
	GOEP_OPCODE_REQ_SETPATH = 0x05,
	GOEP_OPCODE_REQ_ABORT = 0x7F,

	GOEP_OPCODE_RSP_CONTINUE = 0x10,
	GOEP_OPCODE_RSP_SUCCESS = 0x20,
	GOEP_OPCODE_RSP_CREATED = 0x21,
	GOEP_OPCODE_RSP_ACCEPTED = 0x22,
};

enum {
	GOEP_HEADER_ENCODING_UNICODE_TEXT = 0,
	GOEP_HEADER_ENCODING_BYTE_SEQUENCE = 1,
	GOEP_HEADER_ENCODING_1_BYTE_QUANTITY = 2,
	GOEP_HEADER_ENCODING_4_BYTE_QUANTITY = 3,
};

enum {
	GOEP_HEADER_MEANING_NAME = 0x01,
	GOEP_HEADER_MEANING_TYPE = 0x02,
	GOEP_HEADER_MEANING_TARGET = 0x06,
	GOEP_HEADER_MEANING_BODY = 0x08,
	GOEP_HEADER_MEANING_END_OF_BODY = 0x09,
	GOEP_HEADER_MEANING_WHO = 0x0A,
	GOEP_HEADER_MEANING_CONNECTION_ID = 0x0B,
	GOEP_HEADER_MEANING_APP_PARAM = 0x0C,
	GOEP_HEADER_MEANING_AUTH_CHAL = 0x0D,
	GOEP_HEADER_MEANING_AUTH_RSP = 0x0E,
};

struct bt_goep_field_head {
	u8_t opcode:7;
	u8_t final_flag:1;
	u8_t pkt_len[2];
} __packed;

struct bt_goep_connect_field {
	u8_t opcode:7;
	u8_t final_flag:1;
	u8_t pkt_len[2];
	u8_t version;
	u8_t flags;
	u8_t max_pkt_len[2];
} __packed;


struct bt_goep_get_field {
	u8_t opcode:7;
	u8_t final_flag:1;
	u8_t pkt_len[2];
} __packed;

struct bt_goep_header {
	u8_t meaning:6;
	u8_t encoding:2;
	u8_t data[0];
} __packed;

struct bt_goep_header_len {
	u8_t meaning:6;
	u8_t encoding:2;
	u8_t len[2];
} __packed;

struct bt_goep_header_body {
	u8_t meaning:6;
	u8_t encoding:2;
	u8_t len[2];
	u8_t data[0];
} __packed;

struct bt_goep_header_who {
	u8_t meaning:6;
	u8_t encoding:2;
	u8_t len[2];
	u8_t data[0];
} __packed;

struct bt_goep_header_connection_id {
	u8_t meaning:6;
	u8_t encoding:2;
	u8_t id[4];
} __packed;

struct bt_goep_header_app_param {
	u8_t meaning:6;
	u8_t encoding:2;
	u8_t len[2];
	u8_t data[0];
} __packed;

#ifdef __cplusplus
}
#endif

#endif /* __BT_GOEP_CLIENT_INTERNAL_H */
