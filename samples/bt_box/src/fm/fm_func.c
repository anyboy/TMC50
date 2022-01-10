/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief fm func main.
 */

#include "fm.h"
#include "tts_manager.h"
#ifdef CONFIG_ESD_MANAGER
#include "esd_manager.h"
#endif

#ifdef CONFIG_PROPERTY
#include "property_manager.h"
#endif

#define FM_CHANNEL_SELECT CH_US_AREA
#define FM_TRY_LISTEN_TIME 2000  /*2s*/


const struct fm_radio_station_config radio_station_config[MAX_AREA] = {
	{8750, 10800, 10},
	{7600, 9000, 10},
	{8750, 10800, 5},
};

static void _fm_radio_update_mute_status(struct fm_app_t *fm, bool flag)
{
	fm->mute_flag = flag;

	fm_view_show_play_status(fm->mute_flag);

#ifdef CONFIG_ESD_MANAGER
	if (esd_manager_check_esd()) {
		fm_start_play();
		fm->playing = 1;
	}
#endif

}
int fm_radio_station_load(struct fm_app_t *fm)
{
#ifdef CONFIG_PROPERTY
	if (property_get("FM_STATION",
			(u8_t *)&fm->radio_station,
			sizeof(struct fm_radio_station_info)) <= 0) {
		SYS_LOG_ERR("failed\n");
	}
#endif

	fm_rx_set_freq(fm_radio_station_get_current(fm));
	fm_view_show_freq(fm->current_freq);
	_fm_radio_update_mute_status(fm, false);
	return 0;
}

int fm_radio_station_save(struct fm_app_t *fm)
{
#ifdef CONFIG_PROPERTY
	if (property_set("FM_STATION",
			(u8_t *)&fm->radio_station,
			sizeof(struct fm_radio_station_info)) < 0) {
		SYS_LOG_ERR("failed\n");
	}
#endif
	return 0;
}

u16_t fm_radio_station_get_current(struct fm_app_t *fm)
{
	fm->auto_scan_config = (struct fm_radio_station_config *)&radio_station_config[FM_CHANNEL_SELECT];

	if (fm->radio_station.default_freq != 0 &&
		fm->radio_station.default_freq <= fm->auto_scan_config->max_freq &&
		fm->radio_station.default_freq >= fm->auto_scan_config->min_freq) {
		fm->current_freq = fm->radio_station.default_freq;
		SYS_LOG_INF("default_freq %d current_freq %d\n", fm->radio_station.default_freq, fm->current_freq);
		return fm->current_freq;
	}

	if (fm->radio_station.station_num == 0) {
		fm->current_freq = fm->auto_scan_config->min_freq;
		fm->current_freq = 10300;
		SYS_LOG_INF("current_freq %d\n", fm->current_freq);
		return fm->current_freq;
	}

	if (fm->radio_station.current_station_index >= fm->radio_station.station_num) {
		fm->radio_station.current_station_index = fm->radio_station.station_num - 1;
	}

	if (fm->radio_station.current_station_index < 0) {
		fm->radio_station.current_station_index = 0;
	}

	fm->current_freq = fm->radio_station.history_station[fm->radio_station.current_station_index];

	return fm->current_freq;
}

u16_t fm_radio_station_get_next(struct fm_app_t *fm)
{
	if (fm->current_freq >= fm->auto_scan_config->max_freq) {
		return -1;
	}

	if (fm->radio_station.history_station[fm->radio_station.current_station_index] < fm->auto_scan_config->min_freq ||
		fm->radio_station.history_station[fm->radio_station.current_station_index] > fm->auto_scan_config->max_freq) {
		return -1;
	}

	fm->radio_station.current_station_index++;

	if (fm->radio_station.current_station_index >= fm->radio_station.station_num ||
		fm->radio_station.history_station[fm->radio_station.current_station_index] < fm->auto_scan_config->min_freq ||
		fm->radio_station.history_station[fm->radio_station.current_station_index] > fm->auto_scan_config->max_freq) {
		fm->radio_station.current_station_index = fm->radio_station.station_num - 1;
	}

	fm->current_freq = fm->radio_station.history_station[fm->radio_station.current_station_index];

	fm_view_show_station_num(fm->radio_station.current_station_index);

	fm_rx_set_freq(fm->current_freq);
	_fm_radio_update_mute_status(fm, false);

	fm->radio_station.default_freq = fm->current_freq;
	fm_radio_station_save(fm);

	return fm->current_freq;
}

u16_t fm_radio_station_get_prev(struct fm_app_t *fm)
{
	if (fm->current_freq <= fm->auto_scan_config->min_freq) {
		return -1;
	}

	if (fm->radio_station.history_station[fm->radio_station.current_station_index] < fm->auto_scan_config->min_freq ||
		fm->radio_station.history_station[fm->radio_station.current_station_index] > fm->auto_scan_config->max_freq) {
		return -1;
	}

	fm->radio_station.current_station_index--;

	if (fm->radio_station.current_station_index < 0) {
		fm->radio_station.current_station_index = 0;
	}

	fm->current_freq = fm->radio_station.history_station[fm->radio_station.current_station_index];

	fm_view_show_station_num(fm->radio_station.current_station_index);

	fm_rx_set_freq(fm->current_freq);
	_fm_radio_update_mute_status(fm, false);

	fm->radio_station.default_freq = fm->current_freq;
	fm_radio_station_save(fm);

	return fm->current_freq;
}

int fm_radio_station_add_freq(struct fm_app_t *fm, u16_t freq)
{
	int ret = 0;
	bool new_freq = true;

	for (int i = 0 ; i < fm->radio_station.station_num; i++) {
		if (fm->radio_station.history_station[i] == freq) {
			new_freq = false;
			ret = -EEXIST;
			break;
		}
	}

	if (new_freq) {
		if (fm->radio_station.station_num >= MAX_RADIO_STATION_NUM) {
			ret = -ENOSPC;
			goto exit;
		}
		fm->radio_station.history_station[fm->radio_station.station_num] = freq;
		fm->radio_station.station_num++;
	}

exit:
	return ret;
}

static void fm_radio_station_auto_scan_handle(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct fm_app_t *fm = fm_get_app();

	if (fm->try_listening) {
		if(fm->try_listening_count >= FM_TRY_LISTEN_TIME) {
			fm->try_listening = 0;
			fm->try_listening_count = 0;
		}

		fm->try_listening_count += 100;
		fm_view_show_try_listen();
		return;
	}

	SYS_LOG_INF("current_freq %d\n", fm->current_freq);
	fm_view_show_freq(fm->current_freq);
	if (!fm_rx_check_freq(fm->current_freq)) {
		if (fm_radio_station_add_freq(fm, fm->current_freq) == -ENOSPC) {
			goto stop_exit;
		}
		fm_rx_set_freq(fm->current_freq);
		_fm_radio_update_mute_status(fm, false);
		fm->try_listening = 1;
	}

	fm->current_freq += fm->auto_scan_config->step;

	if (fm->current_freq > fm->auto_scan_config->max_freq) {
		goto stop_exit;
	}

	return;

stop_exit:
	fm->scan_mode = NONE_SCAN;
	thread_timer_stop(&fm->monitor_timer);

	//play first freq
	fm->radio_station.current_station_index = 0;
	fm->current_freq = fm->radio_station.history_station[fm->radio_station.current_station_index];
	if(fm->current_freq < fm->auto_scan_config->min_freq ||
		fm->current_freq > fm->auto_scan_config->max_freq) {
		fm->current_freq = fm->auto_scan_config->min_freq;
	}

	fm->radio_station.default_freq = fm->current_freq;
	fm_radio_station_save(fm);

	fm_view_show_station_num(fm->radio_station.current_station_index);

	fm_rx_set_freq(fm->current_freq);
	_fm_radio_update_mute_status(fm, false);


	return;

}

int fm_radio_station_auto_scan(struct fm_app_t *fm)
{
#ifdef CONFIG_PLAYTTS
	tts_manager_remove_event_lisener(NULL);
	tts_manager_wait_finished(true);
#endif

	fm->auto_scan_config = (struct fm_radio_station_config *)&radio_station_config[FM_CHANNEL_SELECT];
	fm->current_freq = fm->auto_scan_config->min_freq;
	fm->scan_mode = AUTO_SCAN;

	if (thread_timer_is_running(&fm->monitor_timer)) {
		thread_timer_stop(&fm->monitor_timer);
	}

	//reset default freq
	memset(&fm->radio_station, 0, sizeof(struct fm_radio_station_info));
	fm->radio_station.default_freq = fm->current_freq;
	fm_radio_station_save(fm);

	thread_timer_init(&fm->monitor_timer, fm_radio_station_auto_scan_handle, NULL);

	thread_timer_start(&fm->monitor_timer, 100, 100);

	return 0;
}

static void fm_radio_station_manual_scan_handle(struct thread_timer *ttimer, void *expiry_fn_arg)
{
	struct fm_app_t *fm = fm_get_app();

	SYS_LOG_INF("current_freq %d\n", fm->current_freq);
	fm_view_show_freq(fm->current_freq);
	if (!fm_rx_check_freq(fm->current_freq)) {
		fm_rx_set_freq(fm->current_freq);
		_fm_radio_update_mute_status(fm, false);
		goto stop_exit;
	}

	if (fm->scan_mode == NEXT_STATION) {
		fm->current_freq += fm->auto_scan_config->step;
	} else {
		fm->current_freq -= fm->auto_scan_config->step;
	}

	if (fm->current_freq > fm->auto_scan_config->max_freq) {
		fm->current_freq = fm->auto_scan_config->min_freq;
	}

	if (fm->current_freq < fm->auto_scan_config->min_freq) {
		fm->current_freq = fm->auto_scan_config->max_freq;
	}

	if (fm->manual_freq != 0 && fm->manual_freq == fm->current_freq ) {
		goto stop_exit;
	}
	return;

stop_exit:
	fm->scan_mode = NONE_SCAN;
	thread_timer_stop(&fm->monitor_timer);
	fm->radio_station.default_freq = fm->current_freq;
	fm_radio_station_save(fm);
	fm->manual_freq = 0;
}

int fm_radio_station_manual_search(struct fm_app_t *fm, int mode)
{
#ifdef CONFIG_PLAYTTS
	tts_manager_remove_event_lisener(NULL);
	tts_manager_wait_finished(true);
#endif

	fm->auto_scan_config = (struct fm_radio_station_config *)&radio_station_config[FM_CHANNEL_SELECT];
	fm->scan_mode = mode;
	fm->manual_freq = fm->current_freq;

	if (mode == NEXT_STATION) {
		fm->current_freq += fm->auto_scan_config->step;
	} else {
		fm->current_freq -= fm->auto_scan_config->step;
	}

	if (thread_timer_is_running(&fm->monitor_timer)) {
		thread_timer_stop(&fm->monitor_timer);
	}

	thread_timer_init(&fm->monitor_timer, fm_radio_station_manual_scan_handle, NULL);
	thread_timer_start(&fm->monitor_timer, 100, 100);

	return 0;
}

int fm_radio_station_cancel_scan(struct fm_app_t *fm)
{
	thread_timer_stop(&fm->monitor_timer);
	return 0;
}

int fm_radio_station_set_mute(struct fm_app_t *fm)
{
	fm_rx_set_mute(fm->mute_flag);
	return 0;
}

int fm_function_init(void)
{
	struct fm_init_para_t fm_init_para = {0};
	fm_init_para.channel_config = FM_CHANNEL_SELECT;

	int ret = fm_rx_init(&fm_init_para);
	return ret;
}


