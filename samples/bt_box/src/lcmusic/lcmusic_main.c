/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief lcmusic app main.
 */

#include "lcmusic.h"
#include "tts_manager.h"
#include "fs_manager.h"
#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif

static struct lcmusic_app_t *p_local_music = NULL;
static u8_t sd_music_init_state = LCMUSIC_STATUS_PLAYING;
static u8_t uhost_music_init_state = LCMUSIC_STATUS_PLAYING;
static u8_t nor_music_init_state = LCMUSIC_STATUS_PLAYING;

int lcmusic_get_status(void)
{
	if (!p_local_music)
		return LCMUSIC_STATUS_NULL;
	return p_local_music->music_state;
}
u8_t lcmusic_get_play_state(const char *dir)
{
	if (memcmp(dir, "SD:", strlen("SD:")) == 0)
		return sd_music_init_state;
	else if (memcmp(dir, "USB:", strlen("USB:")) == 0)
		return uhost_music_init_state;
	else if (memcmp(dir, "NOR:", strlen("NOR:")) == 0)
		return nor_music_init_state;
	else {
		SYS_LOG_INF("dir :%s error\n", dir);
		return LCMUSIC_STATUS_PLAYING;
	}
}
void lcmusic_store_play_state(const char *dir)
{
	if (memcmp(dir, "SD:", strlen("SD:")) == 0)
		sd_music_init_state = p_local_music->music_state;
	else if (memcmp(dir, "USB:", strlen("USB:")) == 0)
		uhost_music_init_state = p_local_music->music_state;
	else if (memcmp(dir, "NOR:", strlen("NOR:")) == 0)
		nor_music_init_state = p_local_music->music_state;
	else {
		SYS_LOG_INF("dir :%s error\n", dir);
	}
#ifdef CONFIG_ESD_MANAGER
	u8_t playing = p_local_music->music_state;

	esd_manager_save_scene(TAG_PLAY_STATE, &playing, 1);
#endif

}
void _lcmusic_restore_play_state(void)
{
	p_local_music->is_init = 1;

	thread_timer_start(&p_local_music->monitor_timer, 1000, 0);
}
static int _lcmusic_init(const char *dir)
{
	if (p_local_music)
		return 0;

	p_local_music = app_mem_malloc(sizeof(struct lcmusic_app_t));
	if (!p_local_music) {
		SYS_LOG_ERR("malloc failed!\n");
		return -ENOMEM;
	}
#ifdef CONFIG_TWS
#ifndef CONFIG_TWS_BACKGROUND_BT
	bt_manager_halt_phone();
#else
	if (system_check_low_latencey_mode()) {
		bt_manager_halt_phone();
	}
#endif
#endif

	memset(p_local_music, 0, sizeof(struct lcmusic_app_t));

	bt_manager_set_stream_type(AUDIO_STREAM_LOCAL_MUSIC);

	snprintf(p_local_music->cur_dir, sizeof(p_local_music->cur_dir), "%s", dir);

	/* resume track_no */
	_lcmusic_bpinfo_resume(p_local_music->cur_dir, &p_local_music->music_bp_info);

	lcmusic_view_init(p_local_music);

	lcmusic_thread_timer_init(p_local_music);

	SYS_LOG_INF("init ok\n");
	return 0;
}
void _lcmusic_exit(const char *dir)
{
	if (!p_local_music)
		goto exit;

	lcmusic_bp_info_save(p_local_music);

	lcmusic_stop_play(p_local_music, false);

	lcmusic_exit_iterator();

	lcmusic_view_deinit();

	app_mem_free(p_local_music);

	p_local_music = NULL;

#ifdef CONFIG_PROPERTY
	property_flush_req(NULL);
#endif

exit:
	app_manager_thread_exit(app_manager_get_current_app());

	SYS_LOG_INF("exit ok\n");
}

static void _lcmusic_main_prc(const char *dir)
{
	struct app_msg msg = { 0 };

	bool terminated = false;

	if (_lcmusic_init(dir)) {
		SYS_LOG_ERR("init failed");
		_lcmusic_exit(dir);
		return;
	}
	_lcmusic_restore_play_state();
	while (!terminated) {
		if (receive_msg(&msg, thread_timer_next_timeout())) {
			SYS_LOG_INF("type %d, value %d\n", msg.type, msg.value);
			switch (msg.type) {

			case MSG_INPUT_EVENT:
				lcmusic_input_event_proc(&msg);
				break;
			case MSG_LCMUSIC_EVENT:
				lcmusic_event_proc(&msg);
				break;
			case MSG_TTS_EVENT:
				lcmusic_tts_event_proc(&msg);
				break;
			case MSG_BT_EVENT:
				lcmusic_bt_event_proc(&msg);
				break;
			case MSG_EXIT_APP:
				_lcmusic_exit(dir);
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
struct lcmusic_app_t *lcmusic_get_app(void)
{
	return p_local_music;
}

static void sd_mplayer_main_loop(void *parama1, void *parama2, void *parama3)
{
	SYS_LOG_INF("sd Enter\n");

#ifdef CONFIG_FS_MANAGER
	fs_manager_sdcard_enter_high_speed();
#endif

	sys_event_notify(SYS_EVENT_ENTER_SDMPLAYER);

	_lcmusic_main_prc("SD:");

#ifdef CONFIG_FS_MANAGER
	fs_manager_sdcard_exit_high_speed();
#endif

	SYS_LOG_INF("sd Exit\n");
}

APP_DEFINE(sd_mplayer, share_stack_area, sizeof(share_stack_area),
	   CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
	   sd_mplayer_main_loop, NULL);


static void uhost_mplayer_main_loop(void *parama1, void *parama2, void *parama3)
{
	SYS_LOG_INF("uhost Enter\n");

	sys_event_notify(SYS_EVENT_ENTER_UMPLAYER);

	_lcmusic_main_prc("USB:");

	SYS_LOG_INF("uhost Exit\n");


}

APP_DEFINE(uhost_mplayer, share_stack_area, sizeof(share_stack_area),
			  CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
			  uhost_mplayer_main_loop, NULL);

#ifdef CONFIG_DISK_ACCESS_NOR
static void nor_mplayer_main_loop(void *parama1, void *parama2, void *parama3)
{
	SYS_LOG_INF("nor Enter\n");

	sys_event_notify(SYS_EVENT_ENTER_NORMPLAYER);

	_lcmusic_main_prc("NOR:");

	SYS_LOG_INF("nor Exit\n");
}
APP_DEFINE(nor_mplayer, share_stack_area, sizeof(share_stack_area),
			CONFIG_APP_PRIORITY, FOREGROUND_APP, NULL, NULL, NULL,
			nor_mplayer_main_loop, NULL);
#endif

