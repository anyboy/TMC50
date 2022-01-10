/*
 * Copyright (c) 2018 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <string.h>
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_USB_DEVICE_LEVEL
#define SYS_LOG_DOMAIN "usb/hal"
#include <logging/sys_log.h>
#include <usb/class/usb_audio.h>
#include <usb/class/usb_hid.h>

#include <usb_hid_inner.h>
#include <usb_audio_inner.h>
#include <usb_audio_hal.h>

System_call_status_flag call_status_cb;

static struct k_sem hid_write_sem;

static u32_t g_recv_report;

void usb_audio_register_call_status_cb(System_call_status_flag cb)
{
	call_status_cb = cb;
}

static void _hid_inter_out_ready_cb(void)
{
	u32_t bytes_to_read;
	u8_t ret;
	u8_t usb_hid_out_buf[64];
	/* Get all bytes were received */
	ret = hid_int_ep_read(usb_hid_out_buf, sizeof(usb_hid_out_buf),
			&bytes_to_read);
	if (!ret && bytes_to_read != 0) {
		for (u8_t i = 0; i < bytes_to_read; i++) {
			SYS_LOG_DBG("usb_hid_out_buf[%d]: 0x%02x", i,
			usb_hid_out_buf[i]);
		}

	} else {
		SYS_LOG_INF("Read Self-defined data fail");
	}
}

static void _hid_inter_in_ready_cb(void)
{
	SYS_LOG_DBG("Self-defined data has been sent");
	k_sem_give(&hid_write_sem);
}

static int _usb_hid_tx(const u8_t *buf, u16_t len)
{
	u32_t wrote, timeout = 0;
	u8_t speed_mode = usb_device_speed();

	/* wait one interval at most, unit: 125us */
	if (speed_mode == USB_SPEED_HIGH) {
		timeout = (1 << (CONFIG_HID_INTERRUPT_EP_INTERVAL_HS - 1))/8 + 1;
	} else if (speed_mode == USB_SPEED_FULL || speed_mode == USB_SPEED_LOW) {
		timeout = CONFIG_HID_INTERRUPT_EP_INTERVAL_FS + 1;
	}

	k_sem_take(&hid_write_sem, MSEC(timeout));

	return hid_int_ep_write(buf, len, &wrote);
}

static int _usb_hid_tx_command(u8_t hid_cmd)
{
	u8_t data[HID_SIZE_CONSUMER], ret;

	memset(data, 0, sizeof(data));
	data[0] = REPORT_ID_4;
	data[1] = hid_cmd;
	ret = _usb_hid_tx(data, sizeof(data));
	if (ret == 0) {
		memset(data, 0, sizeof(data));
		data[0] = REPORT_ID_4;
		ret = _usb_hid_tx(data, sizeof(data));
		SYS_LOG_DBG("test_ret: %d\n", ret);
	}

	return ret;
}

static int _usb_hid_ctrl_speed(const u8_t key_id)
{
	int ret = 0;
	u8_t data[HID_SIZE_KEYBOARD];

	memset(data, 0, sizeof(data));
	data[0] = REPORT_ID_2;
	data[1] = 1<<(HID_SHIFT_LEFT-1)|HID_CTRL_LEFT;
	data[3] = key_id;
	ret = _usb_hid_tx(data, sizeof(data));
	if (ret == 0) {
		memset(data, 0, sizeof(data));
		data[0] = REPORT_ID_2;
		return _usb_hid_tx(data, sizeof(data));
	}

	return ENOTSUP;
}

static int _usb_hid_debug_cb(struct usb_setup_packet *setup, s32_t *len,
	     u8_t **data)
{
	SYS_LOG_DBG("Debug callback");

	return -ENOTSUP;
}

int usb_hid_control_pause_play(void)
{
	return _usb_hid_tx_command(PLAYING_AND_PAUSE);
}

int usb_hid_control_volume_dec(void)
{
	return _usb_hid_tx_command(PLAYING_VOLUME_DEC);
}

int usb_hid_control_volume_inc(void)
{
	return _usb_hid_tx_command(PLAYING_VOLUME_INC);
}

int usb_hid_control_volume_mute(void)
{
	return _usb_hid_tx_command(PLAYING_VOLUME_MUTE);
}

int usb_hid_control_play_next(void)
{
	return _usb_hid_tx_command(PLAYING_NEXTSONG);
}

int usb_hid_control_play_prev(void)
{
	return _usb_hid_tx_command(PLAYING_PREVSONG);
}

int usb_hid_control_play_fast(void)
{
	return _usb_hid_ctrl_speed(KBD_RIGHT_ARROW);
}

int usb_hid_control_play_slow(void)
{
	return _usb_hid_ctrl_speed(KBD_LEFT_ARROW);
}

int usb_hid_control_mic_mute(void)
{
	int ret = 0;
	u8_t data[HID_SIZE_TELEPHONY];
	if (g_recv_report == STATUS_ON_CALL || g_recv_report == STATUS_MIC_MUTE){
		memset(data, 0, HID_SIZE_TELEPHONY);
		data[0] = REPORT_ID_5;
		data[1] = 1<<(HID_PHONE_MUTE-1)|HID_HOOK_SWITCH;

		ret = _usb_hid_tx(data, HID_SIZE_TELEPHONY);
		if (ret == 0) {
			memset(data, 0, HID_SIZE_TELEPHONY);
			data[0] = REPORT_ID_5;
			data[1] = HID_HOOK_SWITCH;
			_usb_hid_tx(data, HID_SIZE_TELEPHONY);
		}
	}

	return ret;
}

int usb_hid_telephone_drop(void)
{
	int ret = 0;
	u8_t data[HID_SIZE_TELEPHONY];
	if (g_recv_report == STATUS_ON_CALL || g_recv_report == STATUS_MIC_MUTE) {
		memset(data, 0, HID_SIZE_TELEPHONY);
		data[0] = REPORT_ID_5;
		data[1] = 1<<(HID_DROP-1)|HID_HOOK_SWITCH;
		ret = _usb_hid_tx(data, sizeof(data));
		if (ret == 0) {
			memset(data, 0, sizeof(data));
			data[0] = REPORT_ID_5;
			data[1] = 0x00;
			_usb_hid_tx(data, sizeof(data));
		}
	}

	return ret;
}

int usb_hid_telephone_answer(void)
{
	u8_t data[HID_SIZE_TELEPHONY];
	memset(data, 0, HID_SIZE_TELEPHONY);
	if (g_recv_report != STATUS_RING) {
		return 0;
	}
	data[0] = REPORT_ID_5;
	data[1] = HID_HOOK_SWITCH;
	data[2] = 0x00;
	return _usb_hid_tx(data, HID_SIZE_TELEPHONY);
}

/*
 * Self-defined protocol via set_report
 */
static int _usb_hid_set_report(struct usb_setup_packet *setup, s32_t *len,
				u8_t **data)
{
	u8_t *temp_data = *data;

	SYS_LOG_DBG("temp_data[0]: 0x%04x\n", temp_data[0]);
	SYS_LOG_DBG("temp_data[1]: 0x%04x\n", temp_data[1]);
	SYS_LOG_DBG("temp_data[2]: 0x%04x\n", temp_data[2]);

	g_recv_report = (temp_data[2] << 16) | (temp_data[1] << 8) | temp_data[0];

	SYS_LOG_DBG("g_recv_report:0x%04x\n", g_recv_report);

	if (call_status_cb) {
			call_status_cb(g_recv_report);
	}

	return 0;
}

u32_t usb_hid_get_call_status(void)
{
	return g_recv_report;
}

static const struct hid_ops ops = {
	.get_report = _usb_hid_debug_cb,
	.get_idle = _usb_hid_debug_cb,
	.get_protocol = _usb_hid_debug_cb,
	.set_report = _usb_hid_set_report,
	.set_idle = _usb_hid_debug_cb,
	.set_protocol = _usb_hid_debug_cb,
	.int_in_ready = _hid_inter_in_ready_cb,
	.int_out_ready = _hid_inter_out_ready_cb,
};

int usb_hid_device_init(void)
{
	/* Register hid report desc to HID core */
	usb_hid_register_device(hid_report_desc, sizeof(hid_report_desc), &ops);

	k_sem_init(&hid_write_sem, 1, 1);

	/* USB HID initialize */
	return usb_hid_composite_init();
}

