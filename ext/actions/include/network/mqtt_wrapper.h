#ifndef __MQTT_WRAPPER_H__
#define __MQTT_WRAPPER_H__

#define APP_SLEEP_MSECS		500
#define APP_NET_INIT_TIMEOUT    1000
#define APP_TX_RX_TIMEOUT	2000
#define APP_CONNECT_TRIES	5

#define MQTT_KEEP_ALIVE_TIME	20		/* mqtt keep alive time */
#define MQTT_INTER_PINT_TIME	(MQTT_KEEP_ALIVE_TIME/2)
#define MQTT_PINT_TIMEOUT		(MQTT_INTER_PINT_TIME*4/5)
#define MQTT_LOST_KEEP_CNT		2		/* mqtt service keep link 1.5 keep alive time */

int async_publish(char *msg, int len);

void mqtt_on_connect(void);

int mqtt_send_ping_request(void);

int mqtt_send_publish_ack(uint32_t pkt_id);

int mqtt_send_power_off(void);

int mqtt_wrapper_open(char *client_id, char *srv_addr, uint16_t server_port);

void mqtt_wrapper_close(void);

void on_mqtt_connect(void);

#endif //__MQTT_WRAPPER_H__
