/*
 * Copyright (c) 2018 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __USB_SOUND_H__
#define __USB_SOUND_H__
#include <usb/class/usb_audio.h>
#include <stream.h>

typedef void (*usb_audio_event_callback)(u8_t event_type, u16_t event_param);

enum UAC_STATUS
{
	USB_STATUS_DISCONNECTED			= 0x0001,
	USB_STATUS_CONNECTED			= 0x0002,
	USB_STATUS_UACINITED			= 0x0004,
	USB_STATUS_UACRECORDING			= 0x0008,
	USB_STATUS_UACPLAYING			= 0x0010,
};


int usb_hid_control_pause_play(void);
int usb_hid_control_volume_dec(void);
int usb_hid_control_volume_inc(void);
int usb_hid_control_play_next(void);
int usb_hid_control_play_prev(void);
int usb_host_sync_volume_to_device(int host_volume);
int usb_hid_control_mic_mute(void);
int usb_hid_control_volume_mute(void);





int usb_audio_init(usb_audio_event_callback cb);
int usb_audio_deinit(void);
int usb_audio_set_stream(io_stream_t stream);

int usound_get_uac_status(void);
int usound_set_uac_status(u8_t uac_status);

io_stream_t usb_audio_upload_stream_create(void);

#endif
