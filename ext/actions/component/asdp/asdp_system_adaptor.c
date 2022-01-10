#include <sys_manager.h>
#include <msg_manager.h>
#include <app_manager.h>
#include <bt_manager.h>
#include <lib_asdp.h>
#include <logging/sys_log.h>
#include <power_manager.h>
#include <ui_manager.h>
#include <input_dev.h>
#include <input_manager.h>

#define ASDPA_INF SYS_LOG_INF

#ifdef CONFIG_BT_DONGLE_SERVICE
static int battery_capacity = 100;

int asdp_get_system_battery_capacity(void)
{
	return battery_capacity;
}

void asdp_set_system_battery_capacity(int percents)
{
	battery_capacity = percents;
	bt_manager_hfp_battery_report(BT_BATTERY_REPORT_VAL, power_manager_get_battery_capacity());
}


#if 0
void asdp_report_key_event(u8_t key)
{
	struct app_msg  msg = {0};

	msg.type = MSG_KEY_INPUT;
	msg.value = KEY_TYPE_SHORT_UP|key;
	msg.sender = MSG_KEY_INPUT;
	send_async_msg("main", &msg);
}

static int asdp_send_app_message(char* app, u8_t msg)
{
	int result =  0;
	struct app_msg new_msg;

	if(app != NULL) {
		memset((void*)&new_msg, 0, sizeof(struct app_msg));
		new_msg.type = MSG_INPUT_EVENT;
		new_msg.cmd = msg;
		result = send_async_msg(app, &new_msg);
	}

	return result;
}
#endif

static int asdp_send_active_app_message(u32_t key_event)
{
	return ui_manager_dispatch_key_event(key_event);
}

void asdp_bt_contol(bt_manager_cmd_e c)
{
	ASDPA_INF("cmd=%d", c);
	switch (c) {
		case ASDP_BM_PLAY:
			asdp_send_active_app_message(KEY_POWER | KEY_TYPE_SHORT_UP);
			break;
		case ASDP_BM_PAUSE:
			asdp_send_active_app_message(KEY_POWER | KEY_TYPE_SHORT_UP);
			break;
		case ASDP_BM_PREV:
			asdp_send_active_app_message(KEY_PREVIOUSSONG | KEY_TYPE_SHORT_UP);
			break;
		case ASDP_BM_NEXT:
			asdp_send_active_app_message(KEY_NEXTSONG | KEY_TYPE_SHORT_UP);
			break;
		case ASDP_BM_CALLING_ACCEPT:
			asdp_send_active_app_message(KEY_POWER | KEY_TYPE_SHORT_UP);
			break;
		case ASDP_BM_CALLING_HANGUP:
			asdp_send_active_app_message(KEY_POWER | KEY_TYPE_SHORT_UP);
			break;
		case ASDP_BM_CALLING_REJECT:
			asdp_send_active_app_message(KEY_MENU | KEY_TYPE_SHORT_UP);
			break;
		case ASDP_BM_DISCONNECT_ALL_DEVICE:
			//bt_manager_dev_disconnect();
			bt_manager_clear_list(BTSRV_DEVICE_ALL);
			break;
		case ASDP_BM_SET_CONNECTABLE :
			//asdp_send_app_message("main", MSG_ENTER_PAIRING_MODE);
			break;
		case ASDP_BM_VOLUME_ADD:
			asdp_send_active_app_message(KEY_VOLUMEUP | KEY_TYPE_SHORT_UP);
			break;

		case ASDP_BM_VOLUME_SUB:
			asdp_send_active_app_message(KEY_VOLUMEDOWN | KEY_TYPE_SHORT_UP);
			break;
		default:
			break;

	}

}
#endif
