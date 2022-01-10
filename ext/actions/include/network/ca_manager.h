/*
 * Copyright (c) 2018 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file ca manager interface 
 */

#ifndef __CA_MANGER_H__
#define __CA_MANGER_H__


/**
 * @defgroup ca_manager_apis ca manager APIs
 * @ingroup lib_ca_manager_apis
 * @{
 */

struct customer_info
{
	char *customer_id;
	char *token;
	char *project_name;
	char *ic_type;
	char *device_id;
	char *server_addr;
	char *server_port;
};

int register_customer_device(const struct customer_info *customer);

int get_service_lisence(const struct customer_info * customer, char * service_type, char * lisence);

int device_authentication(void);

/**
 * @} end defgroup ca_manager_apis
 */
#endif