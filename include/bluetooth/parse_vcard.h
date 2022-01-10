/** @file
 *  @brief Bluetooth Parse vcrad
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_PARSE_VCARD_H
#define __BT_PARSE_VCARD_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
	VCARD_TYPE_NAME,
	VCARD_TYPE_TEL,
};

struct parse_vcard_result {
	u8_t type;
	u16_t len;
	u8_t *data;
};

u16_t bt_parse_one_vcard(u8_t *data, u16_t len, struct parse_vcard_result *result, u8_t *result_size);

#ifdef __cplusplus
}
#endif
#endif /* __BT_PARSE_VCARD_H */
