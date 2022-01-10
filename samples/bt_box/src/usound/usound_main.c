/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief usound app main.
 */

#include "usound.h"
#include "tts_manager.h"
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif
#ifdef CONFIG_USB_UART_CONSOLE
#include <drivers/console/uart_usb.h>
#endif

#ifdef CONFIG_TOOL
#include "tool_app.h"
#endif
#include <ui_manager.h>

static struct usound_app_t *p_usound;

int usound_get_uac_status(void)
{
	if (!p_usound)
		return USB_STATUS_DISCONNECTED;

	return p_usound->uac_status;
}

int usound_set_uac_status(u8_t uac_status)
{
	if (!p_usound)
		return USB_STATUS_DISCONNECTED;

	p_usound->uac_status = uac_status;
	return 0;
}

#if 0
void _usound_delay_resume(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	SYS_LOG_INF("playing %d\n", p_usound->playing);
#ifdef CONFIG_USOUND_SPEAKER
	if (!p_usound->playing) {
		usb_hid_control_pause_play();
	}
#endif
	usound_start_play();

}

static void _usound_restore_play_state(void)
{
	u8_t init_play_state = USOUND_STATUS_PLAYING;
#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()) {
		esd_manager_restore_scene(TAG_PLAY_STATE, &init_play_state, 1);
	}
#endif

	if (init_play_state == USOUND_STATUS_PLAYING) {
		if (thread_timer_is_running(&p_usound->monitor_timer)) {
			thread_timer_stop(&p_usound->monitor_timer);
		}
		thread_timer_init(&p_usound->monitor_timer, _usound_delay_resume, NULL);
		thread_timer_start(&p_usound->monitor_timer, 2000, 0);
		usound_show_play_status(true);
	} else {
		usound_show_play_status(false);
	}

	SYS_LOG_INF("%d\n", init_play_state);
}
#endif

static void _usb_audio_event_callback_handle(u8_t info_type, u16_t pstore_info)
{
	struct app_msg msg = { 0 };

	switch (info_type) {
	case USOUND_SYNC_HOST_MUTE:
		SYS_LOG_INF("Host Set Mute");
		audio_system_set_stream_mute(AUDIO_STREAM_USOUND, 1);
	#ifdef CONFIG_TWS
		bt_manager_tws_sync_volume_to_slave(AUDIO_STREAM_USOUND, 0);
	#endif
	break;

	case USOUND_SYNC_HOST_UNMUTE:
		SYS_LOG_INF("Host Set UnMute");
		audio_system_set_stream_mute(AUDIO_STREAM_USOUND, 0);
	#ifdef CONFIG_TWS
		bt_manager_tws_sync_volume_to_slave(AUDIO_STREAM_USOUND, audio_system_get_stream_volume(AUDIO_STREAM_USOUND));
	#endif
	break;

	case USOUND_SYNC_HOST_VOL_TYPE:
	{
		int volume_db = usb_host_sync_volume_to_device(pstore_info);
		u8_t volume_level = audio_policy_get_volume_level_by_db(AUDIO_STREAM_USOUND, volume_db);
		SYS_LOG_INF("volume_level(db): %d level %d volume_req_level %d \n", volume_db, volume_level, p_usound->volume_req_level);
		if (p_usound->volume_req_type) {
			if (p_usound->volume_req_level == volume_level) {
				system_volume_set(AUDIO_STREAM_USOUND, volume_level, false);
				p_usound->volume_req_type = USOUND_VOLUME_NONE;
			} else {
				if (p_usound->volume_req_type == USOUND_VOLUME_INC) {
					if(p_usound->volume_req_level > volume_level) {
						usb_hid_control_volume_inc();
					} else if(p_usound->volume_req_level > volume_level - 2) {
						system_volume_set(AUDIO_STREAM_USOUND, volume_level, false);
						p_usound->volume_req_type = USOUND_VOLUME_NONE;
					}
				} else if(p_usound->volume_req_type == USOUND_VOLUME_DEC) {
					if(p_usound->volume_req_level < volume_level) {
						usb_hid_control_volume_dec();
					} else if(p_usound->volume_req_level < volume_level + 2) {
						system_volume_set(AUDIO_STREAM_USOUND, volume_level, false);
						p_usound->volume_req_type = USOUND_VOLUME_NONE;
					}
				}
			}
		} else {
			system_volume_set(AUDIO_STREAM_USOUND, volume_level, false);
			p_usound->current_volume_level = volume_level;
			p_usound->volume_req_type = USOUND_VOLUME_NONE;
		}
		break;
	}
	case UMIC_SYNC_HOST_VOL_TYPE:
	{
		//audio_system_set_microphone_volume(AUDIO_STREAM_DEFAULT,pstore_info);
		SYS_LOG_INF("mic volume %d \n",pstore_info);
		break;
	}
	case UMIC_SYNC_HOST_MUTE:
	{
		audio_system_mute_microphone(1);
		SYS_LOG_INF("mic mute\n");
		break;
	}
	case UMIC_SYNC_HOST_UNMUTE:
	{
		audio_system_mute_microphone(0);
		SYS_LOG_INF("mic unmute\n");
		break;
	}
	case USOUND_STREAM_STOP:
		SYS_LOG_INF("stream stop\n");
		msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_STREAM_STOP;
		send_async_msg(APP_ID_USOUND, &msg);
	break;
	case USOUND_STREAM_START:
		SYS_LOG_INF("stream start\n");
		msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_STREAM_START;
		send_async_msg(APP_ID_USOUND, &msg);
		break;

	case USOUND_OPEN_UPLOAD_CHANNEL:
		SYS_LOG_INF("mic upload stream open\n");
		p_usound->mic_upload = 1;
		msg.type = MSG_USOUND_EVENT;
		msg.cmd = MSG_USOUND_STREAM_RESTART;
		send_async_msg(APP_ID_USOUND, &msg);

		break;

	case USOUND_CLOSE_UPLOAD_CHANNEL:
		SYS_LOG_INF("mic upload stream close\n");
		if (p_usound->mic_upload) {
			p_usound->mic_upload = 0;
			msg.type = MSG_USOUND_EVENT;
			msg.cmd = MSG_USOUND_STREAM_RESTART;
			send_async_msg(APP_ID_USOUND, &msg);
		}
		break;
	default:
		break;
	}
}


static int _usound_init(void)
{
	if (p_usound)
		return 0;

	p_usound = app_mem_malloc(sizeof(struct usound_app_t));
	if (!p_usound) {
		SYS_LOG_ERR("malloc failed!\n");
		return -ENOMEM;
	}

#ifdef CONFIG_USB_UART_CONSOLE
	if (uart_usb_is_enabled()) {
		uart_usb_uninit();
	}
#endif

#ifdef CONFIG_TWS
#ifndef CONFIG_TWS_BACKGROUND_BT
	bt_manager_halt_phone();
#else
	if (system_check_low_latencey_mode()) {
		bt_manager_halt_phone();
	}
#endif
#endif

	memset(p_usound, 0, sizeof(struct usound_app_t));

	bt_manager_set_stream_type(AUDIO_STREAM_USOUND);

	usound_view_init();

	usb_audio_init(_usb_audio_event_callback_handle);

	p_usound->current_volume_level = audio_system_get_stream_volume(AUDIO_STREAM_USOUND);

	usound_set_uac_status(USB_STATUS_CONNECTED);

#ifdef CONFIG_USP
	bt_manager_set_connectable(0);
#endif

	SYS_LOG_INF("init ok\n");
	return 0;
}

void _usound_exit(void)
{
	if (!p_usound)
		goto exit;

	if (p_usound->playing) {
		usb_hid_control_pause_play();
	}

	usound_stop_play();

	usound_set_uac_status(USB_STATUS_DISCONNECTED);

	usb_audio_deinit();

	usound_view_deinit();

	app_mem_free(p_usound);

	p_usound = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

#ifdef CONFIG_USB_UART_CONSOLE
	if (uart_usb_is_enabled()) {
		uart_usb_init();
	}
#endif

#ifdef CONFIG_USP
	bt_manager_set_connectable(1);
#endif

exit:
	app_manager_thread_exit(app_manager_get_current_app());

	SYS_LOG_INF("exit ok\n");
}

struct usound_app_t *usound_get_app(void)
{
	return p_usound;
}

static void _usound_main_loop(void *parama1, void *parama2, void *parama3)
{
	struct app_msg msg = { 0 };

	bool terminated = false;

	if (_usound_init()) {
		SYS_LOG_ERR("init failed");
		_usound_exit();
		return;
	}

	//_usound_restore_play_state();

	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d\n", msg.type, msg.value);
			switch (msg.type) {
			case MSG_INPUT_EVENT:
				usound_input_event_proc(&msg);
				break;

			case MSG_TTS_EVENT:
				usound_tts_event_proc(&msg);
				break;

			case MSG_USOUND_EVENT:
				usound_event_proc(&msg);
				break;

			case MSG_BT_EVENT:
				usound_bt_event_proc(&msg);
				break;

			case MSG_EXIT_APP:
				_usound_exit();
				terminated = true;
				break;
			default:
				break;
			}
			if (msg.callback)
				msg.callback(&msg, 0, NULL);
		}
		if (!terminated)
			thread_timer_handle_expired();
	}
}

APP_DEFINE(usound, share_stack_area, sizeof(share_stack_area),
	CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
	_usound_main_loop, NULL);

