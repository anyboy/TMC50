/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <os_common_api.h>
#include <mem_manager.h>

#ifdef CONFIG_MQTT_LIB
#include <net/mqtt.h>
#include <net/net_context.h>
#include <misc/printk.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "mqtt_api.h"
#include "mqtt_wrapper.h"
#include "mqtt_service.h"
#include "dns_client.h"
#include "system_config.h"

#define MQTT_DEBUG	1

#ifdef MQTT_DEBUG
#define MQTT_LOG(fmt, ...) SYS_LOG_DBG(fmt,  ##__VA_ARGS__)
#else
#define MQTT_LOG(fmt, ...)
#endif

#define MQTT_WRN(fmt, ...) SYS_LOG_WRN(fmt,  ##__VA_ARGS__)
#define MQTT_ERR(fmt, ...) SYS_LOG_ERR(fmt,  ##__VA_ARGS__)

#define MAX_MQTT_TOPIC_LEN		(MAX_SN_INFO_LEN + 32)
#define MAX_MQTT_PUBLISH_MSG_LEN		1460

/**
 * @brief mqtt_client_ctx	Container of some structures used by the
 *				publisher app.
 */
struct mqtt_client_ctx {
	/**
	 * The connect message structure is only used during the connect
	 * stage. Developers must set some msg properties before calling the
	 * mqtt_tx_connect routine. See below.
	 */
	struct mqtt_connect_msg connect_msg;
	/**
	 * This is the message that will be received by the server
	 * (MQTT broker).
	 */
	struct mqtt_publish_msg pub_msg;

	/**
	 * This is the MQTT application context variable.
	 */
	struct mqtt_ctx mqtt_ctx;

	enum mqtt_qos qos;

	u8_t mqtt_keep_work_runing;
	u8_t mqtt_ping_work_runing;
	/* for keep mqtt alive */
	os_delayed_work mqtt_keep_timeout;

	/* ping timeout */
	os_delayed_work mqtt_ping_timeout;

	char client_id[MAX_SN_INFO_LEN];
	char pub_topic[MAX_MQTT_TOPIC_LEN];
	char sub_topic[MAX_MQTT_TOPIC_LEN];
	char topic_head[MAX_MQTT_TOPIC_LEN - MAX_SN_INFO_LEN];
	char mqtt_server_addr[NET_IPV4_ADDR_LEN + 1];

	/** For MQTT recombine */
	struct MsgInfo *rxMsg;
};

/* The mqtt client struct */
static struct mqtt_client_ctx client_ctx;
static char mqtt_timeout_cnt = 0;

/*================================================================*/

#define OFFLINE    "{\"onlineStatus\":\"abnormal\"}"

int async_publish(char *msg, int len)
{
	int rc;
	struct mqtt_publish_msg *pub_msg = &client_ctx.pub_msg;

	if (!client_ctx.mqtt_ctx.net_app_ctx.is_init || !client_ctx.mqtt_ctx.connected) {
		MQTT_ERR("mqtt close or not connected!");
		return -EIO;
	}

	pub_msg->msg = msg;
	pub_msg->msg_len = len;
	pub_msg->qos = client_ctx.qos;
	pub_msg->topic = client_ctx.pub_topic;
	pub_msg->topic_len = strlen(client_ctx.pub_topic);
	pub_msg->pkt_id += 1;
	MQTT_LOG("mqtt_publish id:%d,msg:%s", pub_msg->pkt_id, msg);

	rc = mqtt_tx_publish(&client_ctx.mqtt_ctx, &client_ctx.pub_msg);
	if (rc < 0) {
		MQTT_ERR("mqtt_tx_publish failed:%d", rc);
	} else {
		/* when send mqtt message, restart send ping message */
		os_delayed_work_cancel(&client_ctx.mqtt_ping_timeout);
		os_delayed_work_cancel(&client_ctx.mqtt_keep_timeout);
		os_delayed_work_submit(&client_ctx.mqtt_keep_timeout, (MQTT_INTER_PINT_TIME * MSEC_PER_SEC));
	}

	return rc;
}

static
void connect_cb(struct mqtt_ctx *mqtt_ctx)
{
	struct mqtt_client_ctx *client_ctx;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);

	MQTT_LOG("mqtt connected:%d, %s!", mqtt_ctx->connected,
			(mqtt_ctx->connected)? "successful" : "failed");

	if (mqtt_ctx->connected) {
		struct app_msg  new_msg = {0};
		new_msg.type = MSG_MQTT_MESSAGE_CONNECT;
		send_async_msg(MQTT_SERVICE_NAME, &new_msg);
	}
}

static
void disconnect_cb(struct mqtt_ctx *mqtt_ctx)
{
	MQTT_LOG("mqtt disconnect");
}

static
int publish_tx_cb(struct mqtt_ctx *mqtt_ctx, uint16_t pkt_id,
	       enum mqtt_packet type)
{
	MQTT_LOG("type:%d packet id: %u", type, pkt_id);
	return 0;
}

static
int publish_rx_cb(struct mqtt_ctx *mqtt_ctx, struct mqtt_publish_msg *msg,
		  uint16_t pkt_id, enum mqtt_packet type)
{
	struct mqtt_client_ctx *client_ctx;
	struct app_msg notiyf_msg = {0};
	struct MsgInfo * mqtt_msg;
	uint32_t func;

	client_ctx = CONTAINER_OF(mqtt_ctx, struct mqtt_client_ctx, mqtt_ctx);
	MQTT_LOG("type:%d, packet id: %u", type, pkt_id);

	if (!client_ctx->mqtt_ctx.net_app_ctx.is_init || !client_ctx->mqtt_ctx.connected) {
		MQTT_ERR("mqtt close or not connected!");
		return -EIO;
	}

	/* receive mqtt message, cancal timer and restart keep timer */
	mqtt_timeout_cnt = 0;
	os_delayed_work_cancel(&client_ctx->mqtt_ping_timeout);
	os_delayed_work_cancel(&client_ctx->mqtt_keep_timeout);
	os_delayed_work_submit(&client_ctx->mqtt_keep_timeout, (MQTT_INTER_PINT_TIME * MSEC_PER_SEC));

	if (msg->new_publish) {
		if (msg->wait_rx_len) {
			if (client_ctx->rxMsg == NULL) {
				client_ctx->rxMsg = mem_malloc(sizeof(struct MsgInfo) + MAX_MQTT_PUBLISH_MSG_LEN + 1);
			}

			if (client_ctx->rxMsg == NULL) {
				return 0;
			}

			memcpy(client_ctx->rxMsg->msg, msg->msg, msg->msg_len);
			client_ctx->rxMsg->msg_len = msg->msg_len;
			return 0;
		} else {
			if (client_ctx->rxMsg) {
				mem_free(client_ctx->rxMsg);
				client_ctx->rxMsg = NULL;
			}
		}
	} else {
		if (client_ctx->rxMsg == NULL) {
			return 0;
		}

		if ((client_ctx->rxMsg->msg_len + msg->msg_len) > MAX_MQTT_PUBLISH_MSG_LEN) {
			mem_free(client_ctx->rxMsg);
			client_ctx->rxMsg = NULL;
			return 0;
		}

		memcpy(&client_ctx->rxMsg->msg[client_ctx->rxMsg->msg_len], msg->msg, msg->msg_len);
		client_ctx->rxMsg->msg_len += msg->msg_len;

		if (msg->wait_rx_len) {
			return 0;
		}
	}

	if((msg_manager_get_free_msg_num() > (CONFIG_NUM_MBOX_ASYNC_MSGS / 2)))
	{
		if (msg->new_publish) {
			mqtt_msg = (struct MsgInfo *)mem_malloc(sizeof(struct MsgInfo) + msg->msg_len + 1);
			if(mqtt_msg == NULL)
			{
			    MQTT_ERR("mem_malloc failed");
				return 0;
			}

			mqtt_msg->msg_len = msg->msg_len;
			memcpy(mqtt_msg->msg, msg->msg, msg->msg_len);
		} else {
			mqtt_msg = client_ctx->rxMsg;
			client_ctx->rxMsg = NULL;
		}

		MQTT_LOG("msg_len:%d, msg: %s ", mqtt_msg->msg_len, mqtt_msg->msg);
		notiyf_msg.ptr = mqtt_msg;
		notiyf_msg.type = MSG_MQTT_MESSAGE_RECEIVED;

		if (type == MQTT_PUBLISH) {
			func = pkt_id | (msg->qos << 16);
			notiyf_msg.callback = (MSG_CALLBAK)func;
		} else {
			notiyf_msg.callback = NULL;
			func = 0;
		}

		if (send_async_msg(MQTT_SERVICE_NAME,&notiyf_msg)) {
			if(func != 0) {
				/*
				 * Send msg success, let msg process send publish ack
				 * return EPERM, mqtt.c will not send publish ack
				 * other all return 0, mqtt.c have chance to send ack to confirm the message
				 */
				return EPERM;
			}
		} else {
			mem_free(mqtt_msg);
			MQTT_WRN("send_async_msg failed");
		}
	} else {
		if (client_ctx->rxMsg) {
			mem_free(client_ctx->rxMsg);
			client_ctx->rxMsg = NULL;
		}

		msg->msg[msg->msg_len - 1] = 0;
		MQTT_LOG("msg_len:%d, msg: %s ", msg->msg_len, msg->msg);
		MQTT_WRN("drop mqtt message ...");
	}

	return 0;
}

static int subscribe_cb(struct mqtt_ctx *ctx, uint16_t pkt_id,
			 uint8_t items, enum mqtt_qos qos[])
{
	return 0;
}

static
void malformed_cb(struct mqtt_ctx *mqtt_ctx, uint16_t pkt_type)
{
	MQTT_LOG("type:%d\n", pkt_type);
}

static
void ping_respond_cb(struct mqtt_ctx *mqtt_ctx, uint16_t result)
{
	MQTT_LOG("Ping ok");
	mqtt_timeout_cnt = 0;
	os_delayed_work_cancel(&client_ctx.mqtt_ping_timeout);
}


static
int try_to_connect(struct mqtt_client_ctx *client_ctx)
{
	int rc, i = 0;

	while (i++ < APP_CONNECT_TRIES) {
		rc = mqtt_tx_connect(&client_ctx->mqtt_ctx,
				     &client_ctx->connect_msg);
		if (rc != 0) {
			MQTT_ERR("mqtt_tx_connect retry rc:%d", rc);
			os_sleep(APP_SLEEP_MSECS);
			continue;
		} else {
			break;
		}
	}

	if (rc != 0) {
		return -EINVAL;
	}

	for (i=0; i<50; i++) {
		if (client_ctx->mqtt_ctx.connected) {
			return 0;
		}
		os_sleep(100);
	}

	return -EINVAL;
}

/* send ping request to keep mqtt alive */
static void mqtt_keep_timeout(os_work *work)
{
	struct app_msg  new_msg = {0};
	struct mqtt_client_ctx *client_ctx = CONTAINER_OF(work, struct mqtt_client_ctx,
											mqtt_keep_timeout);

	if (!client_ctx->mqtt_ctx.net_app_ctx.is_init || !client_ctx->mqtt_ctx.connected) {
		MQTT_ERR("mqtt close or not connected!");
		return;
	}

	client_ctx->mqtt_keep_work_runing = 1;

	new_msg.type = MSG_MQTT_SEND_PINGREQ;
	send_async_msg(MQTT_SERVICE_NAME, &new_msg);

	os_delayed_work_submit(&client_ctx->mqtt_ping_timeout, (MQTT_PINT_TIMEOUT * MSEC_PER_SEC));
	os_delayed_work_submit(&client_ctx->mqtt_keep_timeout, (MQTT_INTER_PINT_TIME * MSEC_PER_SEC));

	client_ctx->mqtt_keep_work_runing = 0;
}

static void mqtt_ping_timeout(struct k_work *work)
{
	struct mqtt_client_ctx *client_ctx = CONTAINER_OF(work, struct mqtt_client_ctx,
											mqtt_ping_timeout);

	if (!client_ctx->mqtt_ctx.net_app_ctx.is_init || !client_ctx->mqtt_ctx.connected) {
		MQTT_ERR("mqtt close or not connected!");
		return;
	}

	client_ctx->mqtt_ping_work_runing = 1;

	if (++mqtt_timeout_cnt >= MQTT_LOST_KEEP_CNT) {
		mqtt_timeout_cnt = 0;
		struct app_msg  new_msg = {0};
		new_msg.type = MSG_MQTT_SERVICE_RESTART;
		send_async_msg(MQTT_SERVICE_NAME, &new_msg);
	}

	client_ctx->mqtt_ping_work_runing = 0;
}

void mqtt_on_connect(void)
{
	const char *topics[1];
	enum mqtt_qos qos[1];
	int rc;

	if (!client_ctx.mqtt_ctx.net_app_ctx.is_init || !client_ctx.mqtt_ctx.connected) {
		MQTT_ERR("mqtt close or not connected!");
		return;
	}

	topics[0] = client_ctx.sub_topic;
	qos[0] = client_ctx.qos;
	client_ctx.pub_msg.pkt_id += 1;
	rc = mqtt_tx_subscribe(&client_ctx.mqtt_ctx, client_ctx.pub_msg.pkt_id, 1, topics, qos);
	if (rc < 0) {
		MQTT_ERR("mqtt_tx_subscribe failed:%d", rc);
	}
}

int mqtt_send_ping_request(void)
{
	if (!client_ctx.mqtt_ctx.net_app_ctx.is_init || !client_ctx.mqtt_ctx.connected) {
		MQTT_ERR("mqtt close or not connected!");
		return -EIO;
	}

	return mqtt_tx_pingreq(&client_ctx.mqtt_ctx);
}

int mqtt_send_publish_ack(uint32_t pkt_id)
{
	int rc = 0;
	uint16_t id;
	uint16_t qos;

	if (!client_ctx.mqtt_ctx.net_app_ctx.is_init || !client_ctx.mqtt_ctx.connected) {
		MQTT_ERR("mqtt close or not connected!");
		return -EIO;
	}

	id = pkt_id&0xFFFF;
	qos = (pkt_id >> 16)&0xFFFF;

	switch (qos) {
		case MQTT_QoS2:
			rc = mqtt_tx_pubrec(&client_ctx.mqtt_ctx, id);
			break;
		case MQTT_QoS1:
			rc = mqtt_tx_puback(&client_ctx.mqtt_ctx, id);
			break;
		case MQTT_QoS0:
			break;
		default:
			rc = -EINVAL;
	}

	return rc;
}

int mqtt_send_power_off(void)
{
	if (!client_ctx.mqtt_ctx.net_app_ctx.is_init || !client_ctx.mqtt_ctx.connected) {
		return -1;
	}
	return 0;
}

int mqtt_wrapper_open(char *client_id, char *srv_addr, uint16_t server_port)
{
	int rc, i, prio;
	struct in_addr  in_addr_t = { { { 0 } } };

	SYS_LOG_INF("enter");
	memset(&client_ctx, 0x00, sizeof(client_ctx));
	client_ctx.pub_msg.pkt_id = sys_rand32_get()&0xFFFF;

	rc = net_addr_pton(AF_INET, srv_addr, &in_addr_t);
	if (rc < 0)
	{
		rc = net_dns_resolve(srv_addr,&in_addr_t);
		if (rc == 0) {
				net_addr_ntop(AF_INET, &in_addr_t,
					client_ctx.mqtt_server_addr, NET_IPV4_ADDR_LEN);
		}
		else
		{
			MQTT_ERR("mqtt dns error :%d", rc);
			return rc;
		}
	} else {
		memcpy(client_ctx.mqtt_server_addr, srv_addr, strlen(srv_addr));
	}

	get_mqtt_topic_head(client_ctx.topic_head);
	memcpy(client_ctx.client_id, client_id, strlen(client_id));
	sprintf(client_ctx.sub_topic, "%s/%s%s",client_ctx.topic_head, client_id, "/server/#");
	sprintf(client_ctx.pub_topic, "%s/%s%s",client_ctx.topic_head, client_id, "/client");

	/* The network context is the only field that must be set BEFORE
	 * calling the mqtt_init routine.
	 */
	client_ctx.mqtt_ctx.net_init_timeout = APP_NET_INIT_TIMEOUT;
	client_ctx.mqtt_ctx.net_timeout = APP_TX_RX_TIMEOUT;
	client_ctx.mqtt_ctx.peer_addr_str = client_ctx.mqtt_server_addr;
	client_ctx.mqtt_ctx.peer_port = server_port;

	/* connect, disconnect and malformed may be set to NULL */
	client_ctx.mqtt_ctx.connect = connect_cb;
	client_ctx.mqtt_ctx.disconnect = disconnect_cb;
	client_ctx.mqtt_ctx.malformed = malformed_cb;
	client_ctx.mqtt_ctx.pingresp = ping_respond_cb;

	/* Publisher apps TX/TX the MQTT PUBLISH msg */
	client_ctx.mqtt_ctx.publish_tx = publish_tx_cb;
	client_ctx.mqtt_ctx.publish_rx = publish_rx_cb;
	client_ctx.mqtt_ctx.subscribe = subscribe_cb;

	/* The connect message will be sent to the MQTT server (broker).
	 * If clean_session here is 0, the mqtt_ctx clean_session variable
	 * will be set to 0 also. Please don't do that, set always to 1.
	 * Clean session = 0 is not yet supported.
	 */
	client_ctx.connect_msg.client_id = client_ctx.client_id;
	client_ctx.connect_msg.client_id_len = strlen(client_ctx.client_id);
	client_ctx.connect_msg.clean_session = 0;
	client_ctx.connect_msg.keep_alive = MQTT_KEEP_ALIVE_TIME;
	client_ctx.connect_msg.will_qos = 1;
	client_ctx.connect_msg.will_flag = 1;
	client_ctx.connect_msg.will_topic = client_ctx.pub_topic;
	client_ctx.connect_msg.will_topic_len = strlen(client_ctx.pub_topic);
	client_ctx.connect_msg.will_msg = OFFLINE;
	client_ctx.connect_msg.will_msg_len = strlen(OFFLINE);
	client_ctx.qos = MQTT_QoS1;

	rc = mqtt_init(&client_ctx.mqtt_ctx, MQTT_APP_PUBLISHER_SUBSCRIBER);
	if (rc != 0) {
		MQTT_ERR("mqtt_init failed rc:%d", rc);
		goto exit_mqtt;
	}

	prio = k_thread_priority_get(k_current_get());
	if (prio >= 0)
		k_thread_priority_set(k_current_get(), -1);

	for (i = 0; i < APP_CONNECT_TRIES; i++) {
		rc = mqtt_connect(&client_ctx.mqtt_ctx);
		if (!rc) {
			break;
		}
	}

	if (prio >= 0)
		k_thread_priority_set(k_current_get(), prio);

	if (rc) {
		MQTT_ERR("mqtt_connect:%d", rc);
		goto exit_mqtt;
	}

	mqtt_timeout_cnt = 0;
	os_delayed_work_init(&client_ctx.mqtt_ping_timeout, mqtt_ping_timeout);
	os_delayed_work_init(&client_ctx.mqtt_keep_timeout, mqtt_keep_timeout);

	rc = try_to_connect(&client_ctx);
	if (rc != 0) {
		os_delayed_work_cancel(&client_ctx.mqtt_ping_timeout);
		os_delayed_work_cancel(&client_ctx.mqtt_keep_timeout);
		MQTT_ERR("try_to_connect failed rc:%d", rc);
		goto exit_mqtt;
	}

	os_delayed_work_cancel(&client_ctx.mqtt_keep_timeout);
	os_delayed_work_submit(&client_ctx.mqtt_keep_timeout, (MQTT_INTER_PINT_TIME * MSEC_PER_SEC));

	SYS_LOG_INF("finish");
	return rc;

exit_mqtt:
	mqtt_close(&client_ctx.mqtt_ctx);
	return rc;
}

void mqtt_wrapper_close(void)
{
	int rc, prio;

	SYS_LOG_INF("enter");
	if (!client_ctx.mqtt_ctx.net_app_ctx.is_init)
	{
		SYS_LOG_WRN("mqtt not open");
		return;
	}

	prio = k_thread_priority_get(k_current_get());
	if (prio >= 0)
		k_thread_priority_set(k_current_get(), -1);

	rc = mqtt_tx_disconnect(&client_ctx.mqtt_ctx);
	if (rc < 0) {
		client_ctx.mqtt_ctx.connected = 0;
	}

	/* After mqtt disconnect, cancal delay work */
	os_delayed_work_cancel(&client_ctx.mqtt_keep_timeout);
	while (client_ctx.mqtt_keep_work_runing) {
		os_sleep(10);
	}

	os_delayed_work_cancel(&client_ctx.mqtt_ping_timeout);
	while (client_ctx.mqtt_ping_work_runing) {
		os_sleep(10);
	}

	mqtt_close(&client_ctx.mqtt_ctx);

	if (client_ctx.rxMsg) {
		mem_free(client_ctx.rxMsg);
		client_ctx.rxMsg = NULL;
	}

	if (prio >= 0)
		k_thread_priority_set(k_current_get(), prio);

	SYS_LOG_INF("finish");
}
#else
int async_publish(char *msg, int len)
{
	SYS_LOG_WRN("this function not support");
}

void mqtt_on_connect(void)
{
	SYS_LOG_WRN("this function not support");
}
int mqtt_send_ping_request(void)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}
int mqtt_send_publish_ack(uint32_t pkt_id)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}
int mqtt_send_power_off(void)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}
int mqtt_wrapper_open(char *client_id, char *srv_addr, uint16_t server_port)
{
	SYS_LOG_WRN("this function not support");
	return 0;
}
void mqtt_wrapper_close(void)
{
	SYS_LOG_WRN("this function not support");
}
void on_mqtt_connect(void)
{
	SYS_LOG_WRN("this function not support");
}
#endif