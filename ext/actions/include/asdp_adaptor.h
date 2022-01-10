#ifndef _USP_ADAPTOR_H_
#define _USP_ADAPTOR_H_

#include <usp_protocol.h>
#include <lib_asdp.h>

int usp_phy_init(usp_phy_interface_t* phy);
int usp_phy_deinit(void);


int asdp_sppble_get_data_len(void);
int asdp_sppble_read_data(u8_t *buf, int size);
int asdp_sppble_write_data(u8_t *buf, int size);
int asdp_ota_spp_init(void);
int asdp_ota_spp_deinit(void);
#ifdef OTA_SPP_TEST
int asdp_ota_test(void);
#endif

int asdp_get_system_battery_capacity(void);
void asdp_set_system_battery_capacity(int percents);
void asdp_bt_contol(bt_manager_cmd_e c);

#endif


