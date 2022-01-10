#ifndef __WEBSOCKET_WRAPPER_H__
#define __WEBSOCKET_WRAPPER_H__

#include "websocket.h"

#define WEBSK_HOST_LEN						32

/* websocket time define */
#define WEBSOCKET_TCP_CONNECT_TRIES			10			/* tcp retry times: 10 */
#define WEBSOCKET_TCP_CONNECT_TIMEOUT		1000		/* tcp connect timeout 1000ms */
#define WEBSOCKET_CONNECT_TIMEOUT			1500		/* websocket connect timeout 1500ms */
#define WEBSOCKET_DISCONNECT_WAIT			1000		/* websocket disconnect wait timeout 1000ms */
#define WEBSOCKET_PING_TIMEOUT				10000		/* ping timeout 10s */

typedef int (*websocket_callback)(const void *usrdata, const void *message, int size);

/* websocket agency structure */
typedef struct websocket_agency {
	struct websocket_ctx client_ctx;
	struct k_sem connect_sem;
	struct k_sem disconnect_sem;
	struct k_delayed_work ping_timeout;
	websocket_callback cb;
	void *usrdata;
}websocket_agency_t;

/*
 * Open websocket
 *
 * @param [in] srv_addr websocket connect addr
 * @param [in] srv_port websocket connect port
 * @param [in] origin websocket server domain name or can be NULL
 *
 * @retval websocket_agency_t *, websocket_agency pointer or NULL
 */
websocket_agency_t *websocket_open(char *srv_addr, u16_t srv_port, char *pos, char *origin);


/*
 * Close websocket
 *
 * @param [in] websk websocket agency
 * @param [in] msg message to send, msg can be NULL
 * @param [in] len message length, length can be 0
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_close(websocket_agency_t *websk, u8_t *msg, u16_t len);

/*
 * send websocket fram
 *
 * @param [in] websk websocket agency
 * @param [in] msg message to send
 * @param [in] len message length
 * @param [in] type test or binary type
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_send_frame(websocket_agency_t *websk, u8_t *msg, u16_t len, u8_t type);

/*
 * Send websocket ping
 *
 * @param [in] websk websocket agency
 * @param [in] msg message to send, msg can be NULL
 * @param [in] len message length, length can be 0
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_send_ping(websocket_agency_t *websk, u8_t *msg, u16_t len);

/*
 * register websocket receive callback
 *
 * @param [in] websk websocket agency
 * @param [in] callback callback function
 * @param [in] usrdate callback parameter
 *
 * @retval 0 on success
 * @retval -xx, failed
 */
int websocket_register_rxcb(websocket_agency_t *websk, websocket_callback callback, void *usrdate);

#endif //__WEBSOCKET_WRAPPER_H__
