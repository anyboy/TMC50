/** @file
 *  @brief Bluetooth device id profile handling
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_DEVICE_ID_H
#define __BT_DEVICE_ID_H

/**
 * @brief SPP
 * @defgroup SPP
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <bluetooth/sdp.h>

struct device_id_info{
	u16_t vendor_id;
	u16_t product_id;
	u16_t id_source;
};

int bt_did_register_sdp(struct device_id_info *device_id);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* __BT_DEVICE_ID_H */
