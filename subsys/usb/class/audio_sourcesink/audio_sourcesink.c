/*
 * USB Audio Class -- Audio Source Sink driver
 *
 * Copyright (C) 2020 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * NOTE: The audio source is consists of one audio control interface and
 * one audio streaming interface which implements UAC version 1.0 and
 * works as full-speed.
 *
 * Todolist:
 * # support multi sample rate and multi channels.
 * # add buffer and timing statistics.
 * # more flexible configuration.
 * # support feature unit.
 * # support high-speed.
 * # support version 2.0.
 */

#define SYS_LOG_DOMAIN "usb/audio"
#define SYS_LOG_LEVEL  CONFIG_SYS_LOG_USB_DEVICE_LEVEL
#include <logging/sys_log.h>

#include <string.h>
#include <misc/byteorder.h>
#include <usb_common.h>
#include <class/usb_audio.h>
#include <usb/class/usb_hid.h>

#ifdef CONFIG_NVRAM_CONFIG
#include <string.h>
#include <nvram_config.h>
#include <property_manager.h>
#endif

#include <usb/usb_device.h>
#include "usb_audio_sourcesink_desc.h"

#define GET_SAMPLERATE		0xA2
#define SET_ENDPOINT_CONTROL	0x22
#define SAMPLING_FREQ_CONTROL	0x0100

#define FEATURE_UNIT_INDEX1	0x0901
#define FEATURE_UNIT_INDEX2	0x0501

#define AUDIO_STREAM_INTER2	2
#define AUDIO_STREAM_INTER3	3

static u16_t get_volume_val;
static int chx_cur_vol_dat;
static int ch0_cur_vol_val;
static int ch1_cur_vol_val;
static int ch2_cur_vol_val;

static u32_t g_cur_sample_rate = CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD;

static usb_ep_callback in_ep_cb;
static usb_ep_callback out_ep_cb;
static usb_audio_start audio_sink_start_cb;
static usb_audio_start audio_source_start_cb;
static usb_audio_pm audio_device_pm_cb;

usb_audio_volume_sync audio_sink_vol_ctrl_cb;
usb_audio_volume_sync audio_source_vol_ctrl_cb;

static bool audio_download_streaming_enabled;
static bool audio_upload_streaming_enabled;

static const struct usb_if_descriptor usb_audio_if  = {
	.bLength = sizeof(struct usb_if_descriptor),
	.bDescriptorType = USB_INTERFACE_DESC,
	.bInterfaceNumber = CONFIG_USB_AUDIO_DEVICE_IF_NUM,
	.bNumEndpoints = 0,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_SUBCLASS_AUDIOCONTROL,
};

static void audio_status_cb(enum usb_dc_status_code status, u8_t *param)
{
	static u8_t alt_setting;
	u8_t iface;

	/* Check the USB status and do needed action if required */
	switch (status) {
	case USB_DC_INTERFACE:
		/* iface: the higher byte, alt_setting: the lower byte */
		iface = *(u16_t *)param >> 8;
		alt_setting = *(u16_t *)param & 0xff;
		SYS_LOG_DBG("Set_Int: %d, alt_setting: %d", iface, alt_setting);
		switch (iface) {
		case AUDIO_STREAM_INTER2:
			if (!alt_setting) {
				audio_download_streaming_enabled = false;
			} else {
				audio_download_streaming_enabled = true;
			}

			if (audio_sink_start_cb) {
				audio_sink_start_cb(audio_download_streaming_enabled);
			}
			break;

		case AUDIO_STREAM_INTER3:
			if (!alt_setting) {
				audio_upload_streaming_enabled = false;
			} else {
				audio_upload_streaming_enabled = true;
			}

			if (audio_source_start_cb) {
				audio_source_start_cb(audio_upload_streaming_enabled);
			}
			break;

		default:
			SYS_LOG_WRN("Unavailable interface number");
			break;
		}
		break;

	case USB_DC_ALTSETTING:
		/* iface: the higher byte, alt_setting: the lower byte */
		iface = *(u16_t *)param >> 8;
		*(u16_t *)param = (iface << 8) + alt_setting;
		SYS_LOG_DBG("Get_Int: %d, alt_setting: %d", iface, alt_setting);
		break;

	case USB_DC_ERROR:
		SYS_LOG_DBG("USB device error");
		break;

	case USB_DC_RESET:
		audio_upload_streaming_enabled = false;
		if (audio_source_start_cb) {
			audio_source_start_cb(audio_upload_streaming_enabled);
		}

		audio_download_streaming_enabled = false;
		if (audio_sink_start_cb) {
			audio_sink_start_cb(audio_download_streaming_enabled);
		}
		SYS_LOG_DBG("USB device reset detected");
		break;

	case USB_DC_CONNECTED:
		SYS_LOG_DBG("USB device connected");
		break;

	case USB_DC_CONFIGURED:
		SYS_LOG_DBG("USB configuration done");
		break;

	case USB_DC_DISCONNECTED:
		audio_upload_streaming_enabled = false;
		if (audio_source_start_cb) {
			audio_source_start_cb(audio_upload_streaming_enabled);
		}

		audio_download_streaming_enabled = false;
		if (audio_sink_start_cb) {
			audio_sink_start_cb(audio_download_streaming_enabled);
		}
		SYS_LOG_DBG("USB device disconnected");
		break;

	case USB_DC_SUSPEND:
		audio_upload_streaming_enabled = false;
		if (audio_source_start_cb) {
			audio_source_start_cb(audio_upload_streaming_enabled);
		}

		audio_download_streaming_enabled = false;
		if (audio_sink_start_cb) {
			audio_sink_start_cb(audio_download_streaming_enabled);
		}

		if (audio_device_pm_cb) {
			audio_device_pm_cb(true);
		}
		SYS_LOG_DBG("USB device suspended");
		break;

	case USB_DC_RESUME:
		SYS_LOG_DBG("USB device resumed");
		if (audio_device_pm_cb) {
			audio_device_pm_cb(false);
		}
		break;

	case USB_DC_HIGHSPEED:
		SYS_LOG_DBG("USB device stack work in high-speed mode");
		break;

	case USB_DC_UNKNOWN:
		break;

	default:
		SYS_LOG_DBG("USB unknown state");
		break;
	}
}

static inline int handle_set_sample_rate(struct usb_setup_packet *setup,
				s32_t *len, u8_t *buf)
{
	switch (setup->bRequest) {
	case UAC_SET_CUR:
	case UAC_SET_MIN:
	case UAC_SET_MAX:
	case UAC_SET_RES:
		*len = 3;
		SYS_LOG_DBG("buf[0]: 0x%02x", buf[0]);
		SYS_LOG_DBG("buf[1]: 0x%02x", buf[1]);
		SYS_LOG_DBG("buf[2]: 0x%02x", buf[2]);
		g_cur_sample_rate = (buf[2] << 16) | (buf[1] << 8) | buf[0];
		SYS_LOG_DBG("g_cur_sample_rate:%d ", g_cur_sample_rate);
		return 0;
	default:
		break;
	}

	return -ENOTSUP;
}

static inline int handle_get_sample_rate(struct usb_setup_packet *setup,
				s32_t *len, u8_t *buf)
{
	switch (setup->bRequest) {
	case UAC_GET_CUR:
	case UAC_GET_MIN:
	case UAC_GET_MAX:
	case UAC_GET_RES:
		buf[0] = (u8_t)CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD;
		buf[1] = (u8_t)(CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD >> 8);
		buf[2] = (u8_t)(CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD >> 16);
		*len = 3;
		return 0;
	default:
		break;
	}
	return -ENOTSUP;
}

int usb_audio_source_inep_flush(void)
{
	return usb_dc_ep_flush(CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR);
}

int usb_audio_sink_outep_flush(void)
{
	return usb_dc_ep_flush(CONFIG_USB_AUDIO_SINK_OUT_EP_ADDR);
}

static int audio_class_handle_req(struct usb_setup_packet *psetup,
	s32_t *len, u8_t **data)
{

	int ret = -ENOTSUP;
	u8_t *temp_data = *data;

	switch (psetup->bmRequestType) {
		case SPECIFIC_REQUEST_OUT:
		if (psetup->wIndex == FEATURE_UNIT_INDEX1) {
			if (psetup->bRequest == UAC_SET_CUR) {
				/* process cmd: set mute and unmute */
				if (psetup->wValue == ((MUTE_CONTROL << 8) | MAIN_CHANNEL_NUMBER0)) {
					SYS_LOG_DBG("sink set channel0 mute/unmute: %d %d\n", ch0_cur_vol_val, temp_data[0]);
					if (ch0_cur_vol_val != temp_data[0]) {
						ch0_cur_vol_val = temp_data[0];
						if (audio_sink_vol_ctrl_cb) {
							if (ch0_cur_vol_val == 1) {
								audio_sink_vol_ctrl_cb(&ch0_cur_vol_val, USOUND_SYNC_HOST_MUTE);
							} else {
								audio_sink_vol_ctrl_cb(&ch0_cur_vol_val, USOUND_SYNC_HOST_UNMUTE);
							}
						}
					}
				}
				/* process cmd: set current volume */
				else if (psetup->wValue == ((VOLUME_CONTROL << 8) | MAIN_CHANNEL_NUMBER1)) {
					ch1_cur_vol_val = (temp_data[1] << 8) | (temp_data[0]);
					SYS_LOG_DBG("sink set channel1 vol:0x%04x", ch1_cur_vol_val);
					if (ch1_cur_vol_val == 0x8000) {
						ch1_cur_vol_val = MINIMUM_VOLUME;
					}
					if (ch1_cur_vol_val == 0) {
						ch1_cur_vol_val = 65536;
					}
					if (audio_sink_vol_ctrl_cb) {
						audio_sink_vol_ctrl_cb(&ch1_cur_vol_val, USOUND_SYNC_HOST_VOL_TYPE);
					}
				}
			}
		}
#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
		else if (psetup->wIndex == FEATURE_UNIT_INDEX2) {
			if (psetup->bRequest == UAC_SET_CUR) {
				if (HIGH_BYTE(psetup->wValue) == MUTE_CONTROL) {
					SYS_LOG_DBG("source set channel0 mute/unmute: %d %d\n", ch0_cur_vol_val, temp_data[0]);
					if (ch0_cur_vol_val != temp_data[0]) {
						ch0_cur_vol_val = temp_data[0];
						if (audio_source_vol_ctrl_cb) {
							if (ch0_cur_vol_val == 1) {
								audio_source_vol_ctrl_cb(&ch0_cur_vol_val, UMIC_SYNC_HOST_MUTE);
							} else {
								audio_source_vol_ctrl_cb(&ch0_cur_vol_val, UMIC_SYNC_HOST_UNMUTE);
							}
						}
					}
				} else if (HIGH_BYTE(psetup->wValue) == VOLUME_CONTROL) {
					ch0_cur_vol_val = (temp_data[1] << 8) | (temp_data[0]);
					SYS_LOG_DBG("source set channel0 vol:0x%04x", ch0_cur_vol_val);
					if (ch0_cur_vol_val == 0x8000) {
						ch0_cur_vol_val = MINIMUM_VOLUME;
					}
					if (ch0_cur_vol_val == 0) {
						ch0_cur_vol_val = 65536;
					}
					if (audio_source_vol_ctrl_cb) {
						audio_source_vol_ctrl_cb(&ch0_cur_vol_val, UMIC_SYNC_HOST_VOL_TYPE);
					}
				}
			}
		}
#endif
		ret = 0;
		break;

		case SPECIFIC_REQUEST_IN:
		if (psetup->wIndex == FEATURE_UNIT_INDEX1) {
			if (psetup->bRequest == UAC_GET_CUR) {
				if (psetup->wValue == ((MUTE_CONTROL << 8) | MAIN_CHANNEL_NUMBER0)) {
					SYS_LOG_DBG("sink get_channel0_volume");
					ch0_cur_vol_val = MUTE_VOLUME;
					*data = (u8_t *) &ch0_cur_vol_val;
					*len = MUTE_LENGTH;
				} else if (psetup->wValue == ((VOLUME_CONTROL << 8) | MAIN_CHANNEL_NUMBER1)) {
					SYS_LOG_DBG("sink get_channel1_volume");
					ch1_cur_vol_val = chx_cur_vol_dat;
					*data = (u8_t *) &ch1_cur_vol_val;
					*len = VOLUME_LENGTH;
				} else if (psetup->wValue == ((VOLUME_CONTROL << 8) | MAIN_CHANNEL_NUMBER2)) {
					SYS_LOG_DBG("sink get_channel2_volume");
					ch2_cur_vol_val = chx_cur_vol_dat;
					*data = (u8_t *) &ch2_cur_vol_val;
					*len = VOLUME_LENGTH;
				}
			} else if (psetup->bRequest == UAC_GET_MIN) {
				get_volume_val = MINIMUM_VOLUME;
				*data = (u8_t *) &get_volume_val;
				*len = VOLUME_LENGTH;
			} else if (psetup->bRequest == UAC_GET_MAX) {
				get_volume_val = MAXIMUM_VOLUME;
				*data = (u8_t *) &get_volume_val;
				*len = VOLUME_LENGTH;
			} else if (psetup->bRequest == UAC_GET_RES) {
				get_volume_val = RESOTION_VOLUME;
				*data = (u8_t *) &get_volume_val;
				*len = VOLUME_LENGTH;
			}
		}
#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
		else if (psetup->wIndex == FEATURE_UNIT_INDEX2) {
			if (psetup->bRequest == UAC_GET_CUR) {
				if (HIGH_BYTE(psetup->wValue) == MUTE_CONTROL) {
					SYS_LOG_DBG("source get_channel0_mute");
					ch0_cur_vol_val = MUTE_VOLUME;
					*data = (u8_t *)&ch0_cur_vol_val;
					*len  = MUTE_LENGTH;
				} else if(HIGH_BYTE(psetup->wValue) == VOLUME_CONTROL) {
					SYS_LOG_DBG("source get_channel0_volume");
					*data = (u8_t *) &chx_cur_vol_dat;
					*len  = VOLUME_LENGTH;
				}
			} else if (psetup->bRequest == UAC_GET_MIN) {
				if (HIGH_BYTE(psetup->wValue) == VOLUME_CONTROL) {
					SYS_LOG_DBG("source get_min_vol_channel0");
					get_volume_val = MINIMUM_VOLUME;
					*data = (u8_t *) &get_volume_val;
					*len  = VOLUME_LENGTH;
				}
			} else if (psetup->bRequest == UAC_GET_MAX) {
				if (HIGH_BYTE(psetup->wValue) == VOLUME_CONTROL) {
					SYS_LOG_DBG("source get_max_vol_channel0");
					get_volume_val = MAXIMUM_VOLUME;
					*data = (u8_t *) &get_volume_val;
					*len  = VOLUME_LENGTH;
				}

			} else if(psetup->bRequest == UAC_GET_RES) {
				if (HIGH_BYTE(psetup->wValue) == VOLUME_CONTROL) {
					SYS_LOG_DBG("source get_vol_res_channel0");
					get_volume_val = RESOTION_VOLUME;
					*data = (u8_t *) &get_volume_val;
					*len  = VOLUME_LENGTH;
				}
			}
		}
#endif
		ret = 0;
		break;

		case SET_ENDPOINT_CONTROL:
			if (psetup->wValue == SAMPLING_FREQ_CONTROL) {
				ret = handle_set_sample_rate(psetup, len, *data);
			}
		break;

		case GET_SAMPLERATE:
			if (psetup->wValue == SAMPLING_FREQ_CONTROL) {
				ret = handle_get_sample_rate(psetup, len, *data);
			}
		break;

		default:
		break;
	}

	return ret;
}

/*
 * NOTICE: In composite device case, this function will never be called,
 * the reason is same as class request as above.
 */
static int audio_custom_handle_req(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	SYS_LOG_DBG("Custom Request: 0x%x 0x%x %d",
		    setup->bRequest, setup->bmRequestType, *len);
	return -ENOTSUP;
}

static int audio_vendor_handle_req(struct usb_setup_packet *setup,
				s32_t *len, u8_t **data)
{
	SYS_LOG_DBG("Custom Request: 0x%x 0x%x %d",
		    setup->bRequest, setup->bmRequestType, *len);
	return -ENOTSUP;
}

static void audio_isoc_in(u8_t ep, enum usb_dc_ep_cb_status_code cb_status)
{

	SYS_LOG_DBG("**isoc_in_cb!**\n");
	if (in_ep_cb) {
		in_ep_cb(ep, cb_status);
	}
}

static void audio_isoc_out(u8_t ep, enum usb_dc_ep_cb_status_code cb_status)
{
	SYS_LOG_DBG("**isoc_out_cb!**\n");
	if (out_ep_cb) {
		out_ep_cb(ep, cb_status);
	}
}

/* Describe Endpoints configuration */
static const struct usb_ep_cfg_data audio_ep_data[] = {
	{
		.ep_cb = audio_isoc_out,
		.ep_addr = CONFIG_USB_AUDIO_SINK_OUT_EP_ADDR,
	},

	{
		.ep_cb = audio_isoc_in,
		.ep_addr = CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR,
	}
};

static const struct usb_cfg_data audio_config = {
	.usb_device_description = NULL,
	.interface_descriptor = &usb_audio_if,
	.cb_usb_status = audio_status_cb,
	.interface = {
		.class_handler = audio_class_handle_req,
		.custom_handler = audio_custom_handle_req,
		.vendor_handler = audio_vendor_handle_req,
	},
	.num_endpoints = ARRAY_SIZE(audio_ep_data),
	.endpoint = audio_ep_data,
};

static int usb_audio_fix_dev_sn(void)
{
	static u8_t mac_str[CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN];
	int ret;
	int read_len;
#ifdef CONFIG_NVRAM_CONFIG
	read_len = nvram_config_get(CFG_BT_MAC, mac_str, CONFIG_USB_DEVICE_STRING_DESC_MAX_LEN);
	if (read_len < 0) {
		SYS_LOG_DBG("no sn data in nvram: %d", read_len);
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_AUDIO_DEVICE_SN, strlen(CONFIG_USB_AUDIO_DEVICE_SN));
		if (ret)
			return ret;
	} else {
		ret = usb_device_register_string_descriptor(DEV_SN_DESC, mac_str, read_len);
		if (ret)
			return ret;
	}
#else
	ret = usb_device_register_string_descriptor(DEV_SN_DESC, CONFIG_USB_AUDIO_DEVICE_SN, strlen(CONFIG_USB_AUDIO_DEVICE_SN));
		if (ret)
			return ret;
#endif
	return 0;
}

/*
 * API: initialize USB audio dev
 */
int usb_audio_device_init(void)
{

	int ret;
	/* Register string descriptor */
	ret = usb_device_register_string_descriptor(MANUFACTURE_STR_DESC, CONFIG_USB_AUDIO_DEVICE_MANUFACTURER, strlen(CONFIG_USB_AUDIO_DEVICE_MANUFACTURER));
	if (ret) {
		return ret;
	}
	ret = usb_device_register_string_descriptor(PRODUCT_STR_DESC, CONFIG_USB_AUDIO_DEVICE_PRODUCT, strlen(CONFIG_USB_AUDIO_DEVICE_PRODUCT));
	if (ret) {
		return ret;
	}
	ret = usb_audio_fix_dev_sn();
	if (ret) {
		return ret;
	}

	/* Register device descriptors */
	usb_device_register_descriptors(usb_audio_sourcesink_fs_desc, usb_audio_sourcesink_hs_desc);

	/* Initialize the USB driver with the right configuration */
	ret = usb_set_config(&audio_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to config USB");
		return ret;
	}

	/* Enable USB driver */
	ret = usb_enable(&audio_config);
	if (ret < 0) {
		SYS_LOG_ERR("Failed to enable USB");
		return ret;
	}

	return 0;
}

/*
 * API: deinitialize USB audio dev
 */
int usb_audio_device_exit(void)
{
	int ret;

	ret = usb_disable();
	if (ret) {
		SYS_LOG_ERR("Failed to disable USB: %d", ret);
		return ret;
	}
	usb_deconfig();
	return 0;
}

/*
 * API: Initialize USB audio composite devie
 */
int usb_audio_composite_dev_init(void)
{
	return usb_decice_composite_set_config(&audio_config);
}

/*
 * Check if the audio streaming interface is enabled before send data.
 * NOTE: It is 100% sure that send() will succeed, it is only for
 * optimization.
 */
bool usb_audio_device_enabled(void)
{
	return audio_upload_streaming_enabled;
}

void usb_audio_device_register_inter_in_ep_cb(usb_ep_callback cb)
{
	in_ep_cb = cb;
}

void usb_audio_device_register_inter_out_ep_cb(usb_ep_callback cb)
{
	out_ep_cb = cb;
}

void usb_audio_device_register_pm_cb(usb_audio_pm cb)
{
	audio_device_pm_cb = cb;
}

void usb_audio_sink_register_start_cb(usb_audio_start cb)
{
	audio_sink_start_cb = cb;
}

void usb_audio_source_register_start_cb(usb_audio_start cb)
{
	audio_source_start_cb = cb;
}

void usb_audio_source_register_volume_sync_cb(usb_audio_volume_sync cb)
{
	audio_source_vol_ctrl_cb = cb;
}

void usb_audio_sink_register_volume_sync_cb(usb_audio_volume_sync cb)
{
	audio_sink_vol_ctrl_cb = cb;
}

void usb_audio_sink_set_cur_vol(int vol_dat)
{
	chx_cur_vol_dat = vol_dat;
}

u32_t usb_audio_get_cur_sample_rate(void)
{
	return g_cur_sample_rate;
}

int usb_audio_device_ep_write(const u8_t *data, u32_t data_len, u32_t *bytes_ret)
{
	return usb_write(CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR,
				data, data_len, bytes_ret);
}

int usb_audio_device_ep_read(u8_t *data, u32_t data_len, u32_t *bytes_ret)
{
	return usb_read(CONFIG_USB_AUDIO_SINK_OUT_EP_ADDR,
				data, data_len, bytes_ret);
}
