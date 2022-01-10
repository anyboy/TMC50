/** @file
 *  @brief Bluetooth PBAP client profile
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_PBAP_CLIENT_H
#define __BT_PBAP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/parse_vcard.h>

enum {
	PBAP_PARAM_ID_MAX_LIST_COUNT = 0x04,
	PBAP_PARAM_ID_FILTER = 0x06,
	PBAP_PARAM_ID_FORMAT = 0x07,
	PBAP_PARAM_ID_PHONEBOOK_SIZE = 0x08,
};

enum {
	PBAP_VCARD_VERSION_2_1 = 0x00,		/* Version 2.1 */
};

enum {
	PBAP_FILTER_VCARD_VERSION = (0x1 << 0),
	PBAP_FILTER_FORMAT_NAME = (0x1 << 1),
	PBAP_FILTER_SP_NAME = (0x1 << 2),		/* Structured Presentation of Name */
	PBAP_FILTER_AI_PHOTO = (0x1 << 3),		/* Associated Image or Photo */
	PBAP_FILTER_BIRTHDAY = (0x1 << 4),
	PBAP_FILTER_DELIVERY_ADDRESS = (0x1 << 5),
	PBAP_FILTER_DELIVERY = (0x1 << 6),
	PBAP_FILTER_TELEPHONE_NUMBER = (0x1 << 7),
	PBAP_FILTER_EMAIL_ADDRESS = (0x1 << 8),
	PBAP_FILTER_EMAIL = (0x1 << 9),
	PBAP_FILTER_TIME_ZONE = (0x1 << 10),
	PBAP_FILTER_GEOGRAPHIC_POSITION = (0x1 << 11),
	PBAP_FILTER_JOB = (0x1 << 12),
	PBAP_FILTER_ROLE_WITH_ORG = (0x1 << 13),
	PBAP_FILTER_ORG_LOGO = (0x1 << 14),
	PBAP_FILTER_VCARD_OF_PERSON = (0x1 << 15),
	PBAP_FILTER_NAME_OF_ORG = (0x1 << 16),
	PBAP_FILTER_COMMENTS = (0x1 << 17),
	PBAP_FILTER_REVISION = (0x1 << 18),
	PBAP_FILTER_PRON_OF_NAME = (0x1 << 19),
	PBAP_FILTER_UNIFORM_LOCATOR = (0x1 << 20),
	PBAP_FILTER_UNIQUE_ID = (0x1 << 21),
	PBAP_FILTER_PUBLIC_ENCRY_KEY = (0x1 << 22),
	PBAP_FILTER_NICKNAME = (0x1 << 23),
	PBAP_FILTER_CATEGORIES = (0x1 << 24),
	PBAP_FILTER_PRODUCT_ID = (0x1 << 25),
	PBAP_FILTER_CLASS_INFO = (0x1 << 26),
	PBAP_FILTER_STRING_FOR_OPT = (0x1 << 27),
	PBAP_FILTER_TIMESTAMP = (0x1 << 28),
};

#define PBAP_GET_VALUE_16(addr)		sys_be16_to_cpu(UNALIGNED_GET((u16_t *)(addr)))
#define PBAP_PUT_VALUE_16(value, addr)	UNALIGNED_PUT(sys_cpu_to_be16(value), (u16_t *)addr)
#define PBAP_GET_VALUE_32(addr)		sys_be32_to_cpu(UNALIGNED_GET((u32_t *)(addr)))
#define PBAP_PUT_VALUE_32(value, addr)	UNALIGNED_PUT(sys_cpu_to_be32(value), (u32_t *)addr)

#define PBAP_SET_FILTER		(PBAP_FILTER_VCARD_VERSION | \
							PBAP_FILTER_FORMAT_NAME | \
							PBAP_FILTER_SP_NAME| \
							PBAP_FILTER_TELEPHONE_NUMBER)

struct pbap_common_param {
	u8_t id;
	u8_t len;
	u8_t data[0];
} __packed;

struct bt_pbap_client_user_cb {
	void (*connect_failed)(struct bt_conn *conn, u8_t user_id);
	void (*connected)(struct bt_conn *conn, u8_t user_id);
	void (*disconnected)(struct bt_conn *conn, u8_t user_id);
	void (*recv)(struct bt_conn *conn, u8_t user_id, struct parse_vcard_result *result, u8_t array_size);
};

u8_t bt_pbap_client_get_phonebook(struct bt_conn *conn, char *path, struct bt_pbap_client_user_cb *cb);
int bt_pbap_client_abort_get(struct bt_conn *conn, u8_t user_id);

/* Function interface for pts test, only for pts test, not for normal used */
int bt_pts_pbap_client_get_phonebook_size(struct bt_conn *conn, u8_t user_id);
int bt_pts_pbap_client_get_vcard(struct bt_conn *conn, u8_t user_id);
int bt_pts_pbap_client_get_final(struct bt_conn *conn, u8_t user_id);
int bt_pts_pbap_client_abort(struct bt_conn *conn, u8_t user_id);
int bt_pts_pbap_client_disconnect(struct bt_conn *conn, u8_t user_id);

#ifdef __cplusplus
}
#endif
#endif /* __BT_PBAP_CLIENT_H */
