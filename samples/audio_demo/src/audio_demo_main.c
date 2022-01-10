/*
 *Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 *SPDX-License-Identifier: Apache-2.0
 *
 *@test for audio in and audio out
 */

#include <mem_manager.h>
#include <msg_manager.h>
#include <fw_version.h>
#include <sys_event.h>
#include "app_switch.h"
//#include "system_app.h"
#include <dvfs.h>
//#include "app_ui.h"
#include <bt_manager.h>
#include <stream.h>
#include <property_manager.h>
#include <audio_hal.h>
#include "input_dev.h"
#include "input_manager.h"
#include "srv_manager.h"
#include "app_manager.h"
#include "sys_manager.h"
#include "tts_manager.h"
#include "buffer_stream.h"
#include "ringbuff_stream.h"
#include "media_mem.h"
#include "media_player.h"
#include "audio_system.h"

struct audio_test_context_t {
	media_player_t *player;
	u8_t playering;
	u8_t ain_logic_src;
	u8_t aout_logic_src;
	u8_t media_opened;
	io_stream_t test_stream;
	os_work i2srx_cb_work;
	os_work spdifrx_cb_work;
	u8_t real_rate;
	u8_t setting_rate;
	audio_stream_type_e stream_type;
} ;
struct audio_test_context_t audio_test_context = {0};

extern void system_audio_policy_init(int aout_channel_type);
void audio_adda_media_test(audio_stream_type_e stream_type);

static void _i2srx_isr_event_callback_work(struct k_work *work)
{
	audio_adda_media_test(audio_test_context.stream_type);
}

static void _spdifrx_isr_event_callback_work(struct k_work *work)
{
	audio_adda_media_test(audio_test_context.stream_type);
}


int i2srx_callback(void *cb_data, u32_t cmd, void *param)
{
	u8_t tmp_rate = *(u8_t *)param;
	SYS_LOG_INF(" call back cmd :%d, real_rate :%d!! ", cmd, tmp_rate);
	if (cmd == I2SRX_SRD_FS_CHANGE && audio_test_context.setting_rate != tmp_rate) {
		//param return real rate
		audio_test_context.real_rate = *(u8_t *)param;
		os_work_submit(&audio_test_context.i2srx_cb_work);
	}

	return 0;
}

int spdifrx_callback(void *cb_data, u32_t cmd, void *param)
{
	u8_t tmp_rate = *(u8_t *)param;
	SYS_LOG_INF(" call back cmd :%d, real_rate :%d!! ", cmd, tmp_rate);
	if (cmd == SPDIFRX_SRD_FS_CHANGE && audio_test_context.setting_rate != tmp_rate) {
		//param return real rate
		audio_test_context.real_rate = tmp_rate;
		os_work_submit(&audio_test_context.spdifrx_cb_work);
	}

	return 0;
}

static io_stream_t _test_create_inputstream(audio_stream_type_e stream_type)
{
	int ret = 0;
	io_stream_t input_stream = NULL;

	input_stream = ringbuff_stream_create_ext(
			media_mem_get_cache_pool(INPUT_PLAYBACK, stream_type),
			media_mem_get_cache_pool_size(INPUT_PLAYBACK, stream_type));
	if (!input_stream) {
		goto exit;
	}

	ret = stream_open(input_stream, MODE_IN_OUT);
	if (ret) {
		stream_destroy(input_stream);
		input_stream = NULL;
		goto exit;
	}

exit:
	SYS_LOG_INF(" %p\n", input_stream);
	return	input_stream;
}

void audio_adda_media_test(audio_stream_type_e stream_type)
{
	media_init_param_t init_param;
	io_stream_t input_stream = NULL;

	if (audio_test_context.player) {

		media_player_fade_out(audio_test_context.player, 60);

		/** reserve time to fade out*/
		os_sleep(60);

		if (audio_test_context.test_stream)
			stream_close(audio_test_context.test_stream);

		media_player_stop(audio_test_context.player);

		media_player_close(audio_test_context.player);
		SYS_LOG_INF(" player close success!! ");
	}

	memset(&init_param, 0, sizeof(media_init_param_t));

	input_stream = _test_create_inputstream(stream_type);

	init_param.type = MEDIA_SRV_TYPE_PLAYBACK_AND_CAPTURE;
	init_param.stream_type = stream_type;
	init_param.efx_stream_type = init_param.stream_type;
	init_param.format = PCM_TYPE;
	init_param.input_stream = input_stream;
	init_param.output_stream = NULL;
	init_param.event_notify_handle = NULL;
	init_param.dumpable = 1;

	init_param.capture_format = PCM_TYPE;
	init_param.capture_input_stream = NULL;
	init_param.capture_output_stream = input_stream;

	if (stream_type == AUDIO_STREAM_MIC_IN) {
		init_param.channels = 1;
		init_param.sample_rate = 16;
		init_param.capture_channels = 1;
		init_param.capture_sample_rate_input = 16;
		init_param.capture_sample_rate_output = 16;
	} else if (stream_type == AUDIO_STREAM_I2SRX_IN) {
		init_param.channels = 2;
		init_param.capture_channels = 2;
		if (audio_test_context.real_rate) {
			init_param.sample_rate = audio_test_context.real_rate;
			init_param.capture_sample_rate_input = audio_test_context.real_rate;
			init_param.capture_sample_rate_output = audio_test_context.real_rate;
		} else {
			init_param.sample_rate = 48;
			init_param.capture_sample_rate_input = 48;
			init_param.capture_sample_rate_output = 48;
		}

		hal_ain_set_contrl_callback(AUDIO_CHANNEL_SPDIFRX, i2srx_callback);
	} else if (stream_type == AUDIO_STREAM_SPDIF_IN) {
			init_param.channels = 2;
			init_param.capture_channels = 2;
			if (audio_test_context.real_rate) {
				init_param.sample_rate = audio_test_context.real_rate;
				init_param.capture_sample_rate_input = audio_test_context.real_rate;
				init_param.capture_sample_rate_output = audio_test_context.real_rate;
			} else {
				init_param.sample_rate = 48;
				init_param.capture_sample_rate_input = 48;
				init_param.capture_sample_rate_output = 48;
			}

		hal_ain_set_contrl_callback(AUDIO_CHANNEL_SPDIFRX, spdifrx_callback);
	} else {
		init_param.channels = 2;
		init_param.sample_rate = 48;
		init_param.capture_channels = 2;
		init_param.capture_sample_rate_input = 48;
		init_param.capture_sample_rate_output = 48;
	}

	audio_test_context.player = media_player_open(&init_param);
	if (!audio_test_context.player) {
		SYS_LOG_ERR("player open failed\n");
		goto err_exit;
	}

	audio_test_context.test_stream = init_param.input_stream;

	media_player_fade_in(audio_test_context.player, 60);

	media_player_play(audio_test_context.player);

	audio_test_context.setting_rate = init_param.sample_rate;
	audio_test_context.real_rate = 0;

	SYS_LOG_INF("player open sucessed %p ", audio_test_context.player);
	return;
err_exit:
	if (input_stream) {
		stream_close(input_stream);
		stream_destroy(input_stream);
	}
}


void audio_test_loop(void)
{
	struct app_msg msg = {0};
	int result = 0;
	u8_t ain_current_channel = 0;
	u8_t aout_current_channel = 0;
	u8_t ain_logic_src = 0;
	audio_stream_type_e stream_type;

	os_work_init(&audio_test_context.i2srx_cb_work, _i2srx_isr_event_callback_work);
	os_work_init(&audio_test_context.spdifrx_cb_work, _spdifrx_isr_event_callback_work);
	while(1) {
		if(receive_msg(&msg, thread_timer_next_timeout())) {
			printk("msg type: %d, value: %d\n", msg.type, msg.value);
			switch(msg.type){
			case MSG_KEY_INPUT:
				if(msg.value == (KEY_VOLUMEUP | KEY_TYPE_SHORT_UP)) {
					ain_current_channel++;
					printk("++++++++++ain_current_channel++ :%d\n", ain_current_channel);
					break;
				}

				if(msg.value == (KEY_VOLUMEDOWN | KEY_TYPE_SHORT_UP)) {
					ain_current_channel--;
					printk("++++++++++ain_current_channel-- :%d\n", ain_current_channel);
					break;
				}

				if(msg.value == (KEY_NEXTSONG | KEY_TYPE_SHORT_UP)) {
					aout_current_channel++;
					printk("++++++++++aout_current_channel++ :%d\n", aout_current_channel);
					break;
				}

				if(msg.value == (KEY_PREVIOUSSONG | KEY_TYPE_SHORT_UP)) {
					aout_current_channel--;
					printk("++++++++++aout_current_channel-- :%d\n", aout_current_channel);
					break;
				}

			default:
					SYS_LOG_ERR(" error message type \n");
					break;
			}

			switch(ain_current_channel) {
			case 0:
				ain_logic_src = AUDIO_ANALOG_MIC0;
				stream_type = AUDIO_STREAM_MIC_IN;
				SYS_LOG_INF("AUDIO_ANALOG_MIC0");
				break;

			case 1:
				ain_logic_src = AUDIO_LINE_IN1;
				stream_type = AUDIO_STREAM_LINEIN;
				SYS_LOG_INF("AUDIO_LINE_IN1");
				break;

			case 2:
				ain_logic_src = AUDIO_LINE_IN0;
				stream_type = AUDIO_STREAM_LINEIN;
				SYS_LOG_INF("AUDIO_LINE_IN0");
				break;

			case 3:
				ain_logic_src = AUDIO_DIGITAL_MIC0;
				stream_type = AUDIO_STREAM_MIC_IN;
				SYS_LOG_INF("AUDIO_DIGITAL_MIC0");
				break;

#ifdef CONFIG_AUDIO_IN_I2SRX0_SUPPORT
			case 4:
				ain_logic_src = AUDIO_STREAM_I2SRX_IN;
				stream_type = AUDIO_STREAM_I2SRX_IN;
				SYS_LOG_INF("AUDIO_STREAM_I2SRX_IN");
				break;
#endif

#ifdef CONFIG_AUDIO_IN_SPDIFRX_SUPPORT
			case 5:
				ain_logic_src = AUDIO_STREAM_SPDIF_IN;
				stream_type = AUDIO_STREAM_SPDIF_IN;
				SYS_LOG_INF("AUDIO_STREAM_SPDIF_IN");
				break;
#endif

			default:
				ain_current_channel = 0;
				ain_logic_src = AUDIO_ANALOG_MIC0;
				stream_type = AUDIO_STREAM_MIC_IN;
				SYS_LOG_INF("AUDIO_ANALOG_MIC0");
				break;
			}
			printk("----------ain_current_channel : %d\n", ain_current_channel);
			switch(aout_current_channel) {
			case 0 :
				audio_test_context.aout_logic_src = AUDIO_CHANNEL_DAC;
				system_audio_policy_init(AUDIO_CHANNEL_DAC);
				SYS_LOG_INF("AUDIO_CHANNEL_DAC");
				break;

#ifdef CONFIG_AUDIO_OUT_I2STX0_SUPPORT
			case 1:
				audio_test_context.aout_logic_src = AUDIO_CHANNEL_I2STX;
				system_audio_policy_init(AUDIO_CHANNEL_I2STX);
				SYS_LOG_INF("AUDIO_CHANNEL_I2STX");
				break;
#endif

#ifdef CONFIG_AUDIO_OUT_SPDIFTX_SUPPORT
			case 2:
				audio_test_context.aout_logic_src = AUDIO_CHANNEL_SPDIFTX;
				system_audio_policy_init(AUDIO_CHANNEL_SPDIFTX);
				SYS_LOG_INF("AUDIO_CHANNEL_SPDIFTX");
				break;
#endif

			default:
				audio_test_context.aout_logic_src = AUDIO_CHANNEL_DAC;
				system_audio_policy_init(AUDIO_CHANNEL_DAC);
				aout_current_channel = 0;
				break;
			}
			printk("----------aout_current_channel : %d\n", aout_current_channel);
			audio_test_context.stream_type = stream_type;
			audio_adda_media_test(stream_type);

			if(msg.callback != NULL) {
				msg.callback(&msg, result, NULL);
			}
		}
	}
}


void system_app_init(void)
{
	//init message manager
	msg_manager_init();

	//init service manager
	srv_manager_init();

	// init app manager
	app_manager_init();

    // init audio hal
    hal_audio_out_init();
	hal_audio_in_init();

}

void system_input_handle_init(void)
{
#ifdef CONFIG_INPUT_MANAGER
	input_manager_init(NULL);
	/** input init is locked ,so we must unlock*/
	input_manager_unlock();
#endif
}

void main(void)
{
	/*sys init*/
	system_init();

	system_app_init();

	system_input_handle_init();

	audio_test_loop();

}

extern char _main_stack[CONFIG_MAIN_STACK_SIZE];

APP_DEFINE(main, _main_stack, CONFIG_MAIN_STACK_SIZE,\
			APP_PRIORITY, RESIDENT_APP,\
			NULL, NULL, NULL,\
			NULL, NULL);


