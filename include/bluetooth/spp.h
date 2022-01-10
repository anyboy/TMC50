/** @file
 *  @brief Bluetooth SPP handling
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_SPP_H
#define __BT_SPP_H

/**
 * @brief SPP
 * @defgroup SPP
 * @ingroup bluetooth
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "../../subsys/bluetooth/common/log.h"
#include <bluetooth/buf.h>
#include <bluetooth/conn.h>

/** @brief spp app call back structure. */
struct bt_spp_app_cb {
	void (*connect_failed)(struct bt_conn *conn, u8_t spp_id);
	void (*connected)(struct bt_conn *conn, u8_t spp_id);
	void (*disconnected)(struct bt_conn *conn, u8_t spp_id);
	void (*recv)(struct bt_conn *conn, u8_t spp_id, u8_t *data, u16_t len);
};

int bt_spp_send_data(u8_t channel, u8_t *data, u16_t len);
u8_t bt_spp_connect(struct bt_conn *conn, u8_t *uuid);
int bt_spp_disconnect(u8_t spp_id);
u8_t bt_spp_register_service(u8_t *uuid);
int bt_spp_register_cb(struct bt_spp_app_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#endif /* __BT_SPP_H */
