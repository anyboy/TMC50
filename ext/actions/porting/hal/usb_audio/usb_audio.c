/*
 * Copyright (c) 2020 Actions Semi Co., Ltd.
 *
 * usb audio support
 */
#define SYS_LOG_DOMAIN "usound hal"
#include <logging/sys_log.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <os_common_api.h>
#include <stdio.h>
#include <soc.h>
#include <stream.h>
#include <acts_ringbuf.h>
#include <audio_system.h>
#include <zephyr.h>
#include <init.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <usb/usb_device.h>
#include <usb/class/usb_hid.h>
#include <usb/class/usb_audio.h>
#include <misc/byteorder.h>
#include <usb_hid_inner.h>
#include <usb_audio_inner.h>
#include <usb_audio_hal.h>
#include <energy_statistics.h>
#ifdef CONFIG_PROPERTY
#include <property_manager.h>
#endif

#include "usb_audio_device_desc.h"

/* USB max packet size */
#define UAC_TX_UNIT_SIZE	MAX_UPLOAD_PACKET
#define UAC_TX_DUMMY_SIZE	MAX_UPLOAD_PACKET

struct usb_audio_info {
	os_work call_back_work;
	os_delayed_work call_back_stream_work;
	int down_irq_time;
	int up_irq_time;
	int usb_audio_diff;
	u32_t dowm_irq_cnt;
	u32_t up_irq_cnt;
	u16_t play_state:1;
	u16_t player_start:1;
	u16_t upload_state:1;
	u16_t zero_frame_cnt:8;
	signed int pre_volume_db;
	u16_t call_back_type;
	u16_t call_back_param;
	usb_audio_event_callback cb;
	io_stream_t usound_download_stream;
	u8_t *usb_audio_play_load;
};

static struct usb_audio_info *usb_audio;

static int usb_audio_tx_unit(const u8_t *buf)
{
	u32_t wrote;

	return usb_write(CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR, buf,
					UAC_TX_UNIT_SIZE, &wrote);
}

static int usb_audio_tx_dummy(void)
{
	u32_t wrote;

	memset(usb_audio->usb_audio_play_load, 0, MAX_UPLOAD_PACKET);

	return usb_write(CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR,
			usb_audio->usb_audio_play_load,
			MAX_UPLOAD_PACKET, &wrote);
}

static void _usb_audio_stream_state_notify(u8_t stream_event)
{
	usb_audio->call_back_type = stream_event;
	//os_work_submit(&usb_audio->call_back_work);
	if (stream_event == USOUND_STREAM_START || USOUND_OPEN_UPLOAD_CHANNEL) {
		os_work_submit(&usb_audio->call_back_work);
	} else {
		os_delayed_work_submit(&usb_audio->call_back_stream_work, OS_MSEC(200));
	}
}

static os_delayed_work usb_audio_dat_detect_work;
static u32_t usb_audio_cur_cnt, usb_audio_pre_cnt;

/* Work item process function */
static void usb_audio_handle_dat_detect(struct k_work *item)
{
	if (usb_audio_pre_cnt < usb_audio_cur_cnt) {
		usb_audio_pre_cnt = usb_audio_cur_cnt;
		os_delayed_work_submit(&usb_audio_dat_detect_work, OS_MSEC(50));
	} else if (usb_audio_pre_cnt == usb_audio_cur_cnt) {
		if (usb_audio->play_state) {
			usb_audio->play_state = 0;
			if (!usb_audio->upload_state) {
				_usb_audio_stream_state_notify(USOUND_STREAM_STOP);
			}
		}
	}
}

static int _usb_audio_check_stream(short *pcm_data, u32_t samples)
{
	assert(usb_audio);

	if (energy_statistics(pcm_data, samples) == 0) {
		usb_audio->zero_frame_cnt++;
		if (usb_audio->play_state && usb_audio->zero_frame_cnt > 50) {
			usb_audio->play_state = 0;
			usb_audio->zero_frame_cnt = 0;
			if (!usb_audio->upload_state) {
				_usb_audio_stream_state_notify(USOUND_STREAM_STOP);
			}
		}
	} else {
		usb_audio->zero_frame_cnt = 0;
		if (!usb_audio->play_state) {
			usb_audio->play_state = 1;
			if (!usb_audio->upload_state) {
				_usb_audio_stream_state_notify(USOUND_STREAM_START);
				os_delayed_work_submit(&usb_audio_dat_detect_work, OS_MSEC(50));
			}
			usb_audio_cur_cnt = 0;
			usb_audio_pre_cnt = 0;
		}
		usb_audio_cur_cnt++;
	}
	return 0;
}

/*
 * Interrupt Context
 */
static u8_t usb_audio_play_load[MAX_UPLOAD_PACKET];
static u8_t usb_audio_set0_buf[MAX_DOWNLOAD_PACKET];
extern io_stream_t usb_audio_upload_stream;

extern int usb_audio_stream_read(io_stream_t handle, unsigned char *buf, int len);

static void _usb_audio_in_ep_complete(u8_t ep,
	enum usb_dc_ep_cb_status_code cb_status)
{
	/* In transaction request on this EP, Send recording data to PC */
	if (USB_EP_DIR_IS_IN(ep) && usb_audio_upload_stream) {
		if (usb_audio_stream_read(usb_audio_upload_stream, usb_audio_play_load, UAC_TX_UNIT_SIZE) == UAC_TX_UNIT_SIZE) {
			usb_audio_tx_unit(usb_audio_play_load);
		} else {
			usb_audio_tx_dummy();
		}
	} else {
		SYS_LOG_INF("complete: ep = 0x%02x	ep_status_code = %d, stream:%p", ep, cb_status, usb_audio_upload_stream);
		usb_audio_tx_dummy();
	}

	if (usb_audio->dowm_irq_cnt != 0 && usb_audio->up_irq_cnt == 0) {
		usb_audio->dowm_irq_cnt = 0;
	}

	int diff_cnt = usb_audio->up_irq_cnt > usb_audio->dowm_irq_cnt?
					(usb_audio->up_irq_cnt -usb_audio->dowm_irq_cnt) :
					(usb_audio->dowm_irq_cnt - usb_audio->up_irq_cnt);

	if (diff_cnt > 3 || (usb_audio->up_irq_cnt == 1 && usb_audio->dowm_irq_cnt == 0)) {
		//printk("err down:%d, up:%d ,diff:%d\n", usb_audio->dowm_irq_cnt, usb_audio->up_irq_cnt, diff_cnt);
		if (usb_audio->usound_download_stream) {
			stream_write(usb_audio->usound_download_stream, usb_audio_set0_buf, MAX_DOWNLOAD_PACKET);
		}

		usb_audio->dowm_irq_cnt = 0;
		usb_audio->up_irq_cnt = 0;
		usb_audio->play_state = 1;
	}

	usb_audio->up_irq_cnt++;

	if (usb_audio->usound_download_stream && !usb_audio->play_state) {
		//printk("play state:%d, set %d zero, st:%p, sp:%d\n", usb_audio->play_state, MAX_DOWNLOAD_PACKET,
		//usb_audio->usound_download_stream, stream_get_space(usb_audio->usound_download_stream));
		stream_write(usb_audio->usound_download_stream, usb_audio_set0_buf, MAX_DOWNLOAD_PACKET);
	}
}

/*
 * Interrupt Context
 */
static void _usb_audio_out_ep_complete(u8_t ep,
	enum usb_dc_ep_cb_status_code cb_status)
{
	u32_t read_byte, res;

	u8_t ISOC_out_Buf[MAX_DOWNLOAD_PACKET];

	usb_audio->dowm_irq_cnt++;

	/* Out transaction on this EP, data is available for read */
	if (USB_EP_DIR_IS_OUT(ep) && cb_status == USB_DC_EP_DATA_OUT) {
		res = usb_audio_device_ep_read(ISOC_out_Buf,
				sizeof(ISOC_out_Buf), &read_byte);
		if (!res && read_byte != 0) {
			_usb_audio_check_stream((short *)ISOC_out_Buf, read_byte / 2);
			if (usb_audio->usound_download_stream && usb_audio->play_state) {
				res = stream_write(usb_audio->usound_download_stream, ISOC_out_Buf, read_byte);
				if (res != read_byte) {
					SYS_LOG_ERR("stream_write fail, ret:%d!, st:%p, space:%d", res, usb_audio->usound_download_stream, stream_get_space(usb_audio->usound_download_stream));
				}
			}
		}
	}
}

static void _usb_audio_start_stop(bool start)
{
	if (!start) {
		usb_audio_source_inep_flush();
		usb_audio->upload_state = 0;
		_usb_audio_stream_state_notify(USOUND_CLOSE_UPLOAD_CHANNEL);

		/*
		 * Don't cleanup in "Set Alt Setting 0" case, Linux may send
		 * this before going "Set Alt Setting 1".
		 */
		/* uac_cleanup(); */
	} else {
		/*
		 * First packet is all-zero in case payload be flushed (Linux
		 * may send "Set Alt Setting" several times).
		 */
		SYS_IRQ_FLAGS flags;
		_usb_audio_stream_state_notify(USOUND_OPEN_UPLOAD_CHANNEL);
		usb_audio->upload_state = 1;
		usb_audio->up_irq_cnt = 0;
		sys_irq_lock(&flags);

		if (usb_audio_upload_stream) {
			/**drop stream data before */
			stream_flush(usb_audio_upload_stream);
		}
		sys_irq_unlock(&flags);
		usb_audio_tx_dummy();
	}
}

void usb_audio_tx_flush(void)
{
	usb_audio_source_inep_flush();
}

bool usb_audio_tx_enabled(void)
{
	return usb_audio_device_enabled();
}

int usb_audio_set_stream(io_stream_t stream)
{
	SYS_IRQ_FLAGS flags;

	sys_irq_lock(&flags);
	usb_audio->usound_download_stream = stream;
	usb_audio->dowm_irq_cnt = 0;
	sys_irq_unlock(&flags);
	SYS_LOG_INF("stream %p\n", stream);
	return 0;
}

int usb_host_sync_volume_to_device(int host_volume)
{
	int vol_db;

	if (host_volume == 0x0000) {
		vol_db = 0;
	} else {
		vol_db = (int)((host_volume - 65536)*10 / 256.0);
	}
	return vol_db;
}

static void _usb_mic_vol_changed_notify(int *pstore_info, u8_t info_type)
{
	int volume_db;

	if (info_type == UMIC_SYNC_HOST_VOL_TYPE) {
		volume_db = usb_host_sync_volume_to_device(*pstore_info);
		SYS_LOG_DBG("Microphone vol:%d", volume_db);
	} else {
		if (info_type == UMIC_SYNC_HOST_MUTE) {
			SYS_LOG_DBG("Microphone mute");
		} else if(info_type == UMIC_SYNC_HOST_UNMUTE) {
			SYS_LOG_DBG("Microphone unmute");
		}
	}
}

static void _usb_sounder_vol_changed_notify(int *pstore_info, u8_t info_type)
{
	int volume_db = usb_host_sync_volume_to_device(*pstore_info);

	if (usb_audio  && volume_db != usb_audio->pre_volume_db) {
		usb_audio->pre_volume_db = volume_db;
		usb_audio->call_back_type = info_type;
		usb_audio->call_back_param = *pstore_info;
		os_work_submit(&usb_audio->call_back_work);
	}
}

static void _usb_audio_callback(struct k_work *work)
{
	if (usb_audio && usb_audio->cb) {
		usb_audio->cb(usb_audio->call_back_type, usb_audio->call_back_param);
	}
}

static void _usb_audio_call_status_cb(u32_t status_rec)
{
	SYS_LOG_DBG("status_rec: 0x%04x", status_rec);
}

#ifdef CONFIG_NVRAM_CONFIG
#include <string.h>
#include <nvram_config.h>
#include <property_manager.h>
#endif

int usb_device_composite_fix_dev_sn(void)
{
	static u8_t mac_str[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];
	int ret;
	int read_len;
#ifdef CONFIG_NVRAM_CONFIG
	read_len = nvram_config_get(CFG_BT_MAC, mac_str, CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN);
	if (read_len < 0) {
		SYS_LOG_DBG("no sn data in nvram: %d", read_len);
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_APP_AUDIO_DEVICE_SN, strlen(CONFIG_USB_APP_AUDIO_DEVICE_SN));
		if (ret)
			return ret;
	} else {
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, mac_str, read_len);
		if (ret)
			return ret;
	}
#else
	ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_APP_AUDIO_DEVICE_SN, strlen(CONFIG_USB_APP_AUDIO_DEVICE_SN));
		if (ret)
			return ret;
#endif
	return 0;
}

const int usb_audio_sink_pa_table[16] = {
       -59000, -44000, -38000, -33900,
       -29000, -24000, -22000, -19400,
       -17000, -14700, -12300, -10000,
       -7400,  -5000,  -3000,  0,
};

int usb_audio_init(usb_audio_event_callback cb)
{
	int ret = 0;
	int audio_cur_level, audio_cur_dat;

	usb_audio = mem_malloc(sizeof(struct usb_audio_info));
	if (!usb_audio)
		return -ENOMEM;

	usb_audio->usb_audio_play_load = mem_malloc(MAX_UPLOAD_PACKET);
	if (!usb_audio->usb_audio_play_load) {
		mem_free(usb_audio);
		return -ENOMEM;
	}

	os_work_init(&usb_audio->call_back_work, _usb_audio_callback);
	os_delayed_work_init(&usb_audio->call_back_stream_work, _usb_audio_callback);

	usb_audio->cb = cb;
	usb_audio->play_state = 0;
	usb_audio->zero_frame_cnt = 0;

	audio_cur_level = audio_system_get_current_volume(AUDIO_STREAM_USOUND);
	audio_cur_dat = (usb_audio_sink_pa_table[audio_cur_level]/1000) * 256 + 65536;
	SYS_LOG_DBG("audio_cur_level:%d, audio_cur_dat:0x%x", audio_cur_level, audio_cur_dat);
	usb_audio_sink_set_cur_vol(audio_cur_dat);

	/* Register composite device descriptor */
	usb_device_register_descriptors(usb_audio_dev_fs_desc, usb_audio_dev_hs_desc);

	/* Register composite device string descriptor */
	ret = usb_device_register_string_descriptor(MANUFACTURE_STR_DESC, CONFIG_USB_APP_AUDIO_DEVICE_MANUFACTURER, strlen(CONFIG_USB_APP_AUDIO_DEVICE_MANUFACTURER));
	if (ret) {
		goto exit;
	}
	ret = usb_device_register_string_descriptor(PRODUCT_STR_DESC, CONFIG_USB_APP_AUDIO_DEVICE_PRODUCT, strlen(CONFIG_USB_APP_AUDIO_DEVICE_PRODUCT));
	if (ret) {
		goto exit;
	}
	ret = usb_device_composite_fix_dev_sn();
	if (ret) {
		goto exit;
	}

	/* USB Hid Device Init*/
	ret = usb_hid_device_init();
	if (ret) {
		SYS_LOG_ERR("usb hid init failed");
		goto exit;
	}

	/* Register callbacks */
	usb_audio_source_register_start_cb(_usb_audio_start_stop);
	usb_audio_device_register_inter_in_ep_cb(_usb_audio_in_ep_complete);
	usb_audio_device_register_inter_out_ep_cb(_usb_audio_out_ep_complete);
	usb_audio_source_register_volume_sync_cb(_usb_mic_vol_changed_notify);
	usb_audio_sink_register_volume_sync_cb(_usb_sounder_vol_changed_notify);
	usb_audio_register_call_status_cb(_usb_audio_call_status_cb);

	/* USB Audio Source & Sink Initialize */
	ret = usb_audio_composite_dev_init();
	if (ret) {
		SYS_LOG_ERR("usb audio init failed");
		goto exit;
	}

	usb_device_composite_init(NULL);

	/* init system work item */
	os_delayed_work_init(&usb_audio_dat_detect_work, usb_audio_handle_dat_detect);
	SYS_LOG_INF("ok");
exit:
	return ret;
}

int usb_audio_deinit(void)
{
	os_delayed_work_cancel(&usb_audio->call_back_stream_work);
	usb_device_composite_exit();
	os_delayed_work_cancel(&usb_audio_dat_detect_work);

	if (usb_audio) {
		if (usb_audio->usb_audio_play_load)
			mem_free(usb_audio->usb_audio_play_load);

		mem_free(usb_audio);
		usb_audio = NULL;
	}

	return 0;
}


