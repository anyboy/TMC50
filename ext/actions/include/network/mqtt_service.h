/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mqtt service interface 
 */

#ifndef __MQTT_SERVICE_H__
#define __MQTT_SERVICE_H__

enum
{
	MSG_MQTT_MESSAGE_RECEIVED = MSG_APP_MESSAGE_START,
	MSG_MQTT_MESSAGE_CONNECT,
	MSG_MQTT_SEND_PINGREQ,
	MSG_MQTT_SERVICE_RESTART,
};

struct mqtt_service_t
{
	bool running;
	bool first_init;
	bool connect_finished;
	bool lock;
	struct k_delayed_work mqtt_delay_restart;
};

int unpack_message(char* data, int length);

/**
 * @defgroup mqtt_service_apis Mqtt Service APIs
 * @ingroup mem_managers
 * @{
 */

/**
 * @brief start mqtt service.
 *
 * start mqtt services , mqtt thread will be created by this funcion
 * connect to mqtt server in the main loop of mqtt thread,
 * if mqtt connect failed, then will be reconnect later,
 *
 * @return -1  mqtt start failed
 * @return 0   mqtt start success
 */

int32_t mqtt_service_start(void);

/**
 * @brief stop mqtt service.
 *
 * stop mqtt services ,and disconnect mqtt server
 * mqtt thread will be exit after this function called.
 *
 * @return -1  mqtt stop failed
 * @return 0   mqtt stop success
 */

int32_t mqtt_service_stop(void);

/**
 * @brief get the mqtt service states
 *
 * some times , we need do something according mqtt services state,
 * used this function get mqtt services state
 *
 * @return true  mqtt services run and mqtt tcp connect ok
 * @return false  mqtt services not run or mqtt tcp not connected
 */

bool mqtt_services_is_online(void);

/**
 * @brief mqtt event lock
 *
 * this routine provides  lock the mqtt message event,  after this called
 * all mqtt messages will be drop
 *
 * @return true  mqtt event lock ok
 * @return false  mqtt event lock failed
 */

bool mqtt_event_lock(void);

/**
 * @brief mqtt event unlock
 *
 * this routine provides unlock the mqtt message event, after this called
 * mqtt message will delivered to application
 *
 * @return true  mqtt event lock ok
 * @return false  mqtt event lock failed
 */

bool mqtt_event_unlock(void);
/**
 * @} end defgroup mqtt_service_apis
 */

#endif   //__MQTT_SERVICE_H__

