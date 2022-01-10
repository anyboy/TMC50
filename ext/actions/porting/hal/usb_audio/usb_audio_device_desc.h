/*
 * Copyright (c) 2020 Actions Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief descriptors of composite device( HID + UAC )
 */

#ifndef __USB_AUDIO_DEVICE_DESC_H__
#define __USB_AUDIO_DEVICE_DESC_H__

#include <usb/usb_common.h>
#include <usb/class/usb_audio.h>
#include <usb/class/usb_hid.h>

#define SAMPLE_FREQ_TYPE	CONFIG_USB_AUDIO_SINK_SAMPLE_FREQ_TYPE
#define SAMPLE_RATE		CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD

/* Max Download/Upload Packet for USB Audio Device*/
#define RESOLUTION	CONFIG_USB_AUDIO_SINK_RESOLUTION
#define SUB_FRAME_SIZE	(RESOLUTION >> 3)

#define MAX_DOWNLOAD_PACKET	(ceiling_fraction(CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD, 1000) * SUB_FRAME_SIZE * CONFIG_USB_AUDIO_DOWNLOAD_CHANNEL_NUM)
#define MAX_UPLOAD_PACKET	(ceiling_fraction(CONFIG_USB_AUDIO_SOURCE_SAM_FREQ_UPLOAD, 1000) * SUB_FRAME_SIZE * CONFIG_USB_AUDIO_UPLOAD_CHANNEL_NUM)

/* Support HD(96000KHz) audio playback */
#ifdef CONFIG_SUPPORT_HD_AUDIO_PLAY
#define HD_FORMAT_RESOLUTION		CONFIG_USB_AUDIO_SINK_HD_RESOLUTION
#define HD_FORMAT_SUB_FRAME_SIZE	(HD_FORMAT_RESOLUTION >> 3)
#endif

#ifdef CONFIG_SUPPORT_HD_AUDIO_PLAY
#define HD_FORMAT_MAX_DOWNLOAD_PACKET	(ceiling_fraction(CONFIG_USB_AUDIO_SINK_HD_SAM_FREQ_DOWNLOAD, 1000) * HD_FORMAT_SUB_FRAME_SIZE * CONFIG_USB_AUDIO_DOWNLOAD_CHANNEL_NUM)
#endif

#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
#define AUDIO_NUM_AS_INTERFACES		2
#else
#define AUDIO_NUM_AS_INTERFACES		1
#endif

#define AUDIO_AS_INTERFACE02_NUM	2
#define AUDIO_AS_INTERFACE03_NUM	3

#define UAC_DT_AC_HEADER_LENGTH UAC_DT_AC_HEADER_SIZE(AUDIO_NUM_AS_INTERFACES)

#if defined(CONFIG_USB_AUDIO_SINK_LOGICALCH01_CONFIG) && defined(CONFIG_USB_AUDIO_SINK_LOGICALCH02_CONFIG)
#define	SINK_BM_CTRL_CHNUM	3
#define SINK_BNR_CHAN		2
#else
#define	SINK_BM_CTRL_CHNUM	1
#define SINK_BNR_CHAN		1
#endif

#if defined(CONFIG_USB_AUDIO_SOURCE_LOGICALCH01_CONFIG) && defined(CONFIG_USB_AUDIO_SOURCE_LOGICALCH02_CONFIG)
#define	SOURCE_BM_CTRL_CHNUM	3
#define SOURCE_BNR_CHAN		2
#else
#define	SOURCE_BM_CTRL_CHNUM	1
#define SOURCE_BNR_CHAN		1
#endif

#ifdef CONFIG_USB_AUDIO_CONTROL_UNIT
#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
#define UAC_DT_TOTAL_LENGTH (UAC_DT_AC_HEADER_LENGTH \
	+ (UAC_DT_INPUT_TERMINAL_SIZE * AUDIO_NUM_AS_INTERFACES) + (UAC_DT_OUTPUT_TERMINAL_SIZE * AUDIO_NUM_AS_INTERFACES) \
	+ UAC_DT_FEATURE_UNIT_SIZE(SINK_BM_CTRL_CHNUM, CONFIG_USB_AUDIO_BCONTROLSIZE) \
	+ UAC_DT_FEATURE_UNIT_SIZE(SOURCE_BM_CTRL_CHNUM, CONFIG_USB_AUDIO_BCONTROLSIZE))

#else
#define UAC_DT_TOTAL_LENGTH (UAC_DT_AC_HEADER_LENGTH \
	+ (UAC_DT_INPUT_TERMINAL_SIZE * AUDIO_NUM_AS_INTERFACES) + (UAC_DT_OUTPUT_TERMINAL_SIZE * AUDIO_NUM_AS_INTERFACES) \
	+ UAC_DT_FEATURE_UNIT_SIZE(SINK_BM_CTRL_CHNUM, CONFIG_USB_AUDIO_BCONTROLSIZE))

#endif
#else
#define UAC_DT_TOTAL_LENGTH (UAC_DT_AC_HEADER_LENGTH \
	+ (UAC_DT_INPUT_TERMINAL_SIZE * AUDIO_NUM_AS_INTERFACES) + (UAC_DT_OUTPUT_TERMINAL_SIZE * AUDIO_NUM_AS_INTERFACES))
#endif

#ifdef CONFIG_HID_INTERRUPT_OUT
#define HID_INTERRUPT_EP_NUM	2
#else
#define HID_INTERRUPT_EP_NUM	1
#endif

#define INPUT_TERMINAL1_ID		1
#define OUTPUT_TERMINAL3_ID		3
#define INPUT_TERMINAL2_ID		2
#define OUTPUT_TERMINAL4_ID		4

#define FEATURE_UNIT1_ID		0x09
#define FEATURE_UNIT2_ID		0x05

#define HID_INTER_0			0
#define HID_INTER_0_ALT			0
#define HID_CHILD_DESC_NUM		1
#define HID_COUNTRYCODE_NUM		0

#define AUDIO_CTRL_INTER1		1

#define AUDIO_STRE_INTER2		2
#define AUDIO_STRE_INTER2_ALT0		0
#define AUDIO_STRE_INTER2_ALT1		1
#define AUDIO_STRE_INTER2_ALT2		2

#define AUDIO_STRE_INTER3		3
#define AUDIO_STRE_INTER3_ALT0		0
#define AUDIO_STRE_INTER3_ALT1		1

#define LOW_BYTE(x)  ((x) & 0xFF)
#define HIGH_BYTE(x) ((x) >> 8)

#define	SAM_LOW_BYTE(x)	(u8_t)(x)
#define	SAM_MIDDLE_BYTE(x)	(u8_t)(x >> 8)
#define	SAM_HIGH_BYTE(x)	(u8_t)(x >> 16)

static const u8_t usb_audio_dev_fs_desc[] = {
	/* Device Descriptor */
	USB_DEVICE_DESC_SIZE,		/* bLength */
	USB_DEVICE_DESC,		/* bDescriptorType */
	LOW_BYTE(USB_2_0),	/* bcdUSB */
	HIGH_BYTE(USB_2_0),
	0x00,				/* bDeviceClass */
	0x00,				/* bDeviceSubClass */
	0x00,				/* bDeviceProtocol */
	MAX_PACKET_SIZE0,		/* bMaxPacketSize0: 64 Bytes */
	LOW_BYTE(CONFIG_USB_APP_AUDIO_DEVICE_VID),	/* idVendor */
	HIGH_BYTE(CONFIG_USB_APP_AUDIO_DEVICE_VID),
	LOW_BYTE(CONFIG_USB_APP_AUDIO_DEVICE_PID),	/* idProduct */
	HIGH_BYTE(CONFIG_USB_APP_AUDIO_DEVICE_PID),
	LOW_BYTE(BCDDEVICE_RELNUM),			/* bcdDevice */
	HIGH_BYTE(BCDDEVICE_RELNUM),
	0x01,				/* iManufacturer */
	0x02,				/* iProduct */
	0x03,				/* iSerialNumber */
	0x01,				/* bNumConfigurations */

	/* Configuration Descriptor */
	USB_CONFIGURATION_DESC_SIZE,	/* bLength */
	USB_CONFIGURATION_DESC,		/* bDescriptorType */
	#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
	LOW_BYTE(0x00DC),		/* wTotalLength */
	HIGH_BYTE(0x00DC),
	0x04,				/* bNumInterfaces */
	#else
	LOW_BYTE(0x008C),		/* wTotalLength */
	HIGH_BYTE(0x008C),
	0x03,				/* bNumInterfaces */
	#endif
	0x01,				/* bConfigurationValue */
	0x00,				/* iConfiguration */
	USB_CONFIGURATION_ATTRIBUTES,	/* bmAttributes: Bus Powered */
	MAX_LOW_POWER,			/* MaxPower */

	/* Interface_0 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	HID_INTER_0,			/* bInterfaceNumber */
	HID_INTER_0_ALT,		/* bAlternateSetting */
	HID_INTERRUPT_EP_NUM,		/* bNumEndpoints */
	HID_CLASS,			/* bInterfaceClass: HID Interface Class */
	0x00,				/* bInterfaceSubClass */
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* HID Descriptor */
	sizeof(struct usb_hid_descriptor),	/* bLength */
	USB_HID_DESC,				/* bDescriptorType */
	LOW_BYTE(USB_1_1),			/* bcdHID */
	HIGH_BYTE(USB_1_1),
	HID_COUNTRYCODE_NUM,			/* bCountryCode */
	HID_CHILD_DESC_NUM,			/* bNumDescriptors */
	USB_HID_REPORT_DESC,			/* bDescriptorType */
	LOW_BYTE(CONFIG_HID_REPORT_DESC_SIZE),			/* wDescriptorLength */
	HIGH_BYTE(CONFIG_HID_REPORT_DESC_SIZE),

	/* Endpoint Descriptor */
	USB_ENDPOINT_DESC_SIZE,			/* bLength */
	USB_ENDPOINT_DESC,			/* bDescriptorType */
	/* bEndpointAddress: Direction: IN - EndpointID: n */
	CONFIG_HID_INTERRUPT_IN_EP_ADDR,
	USB_DC_EP_INTERRUPT,			/* bmAttributes: Interrupt Transfer Type */
	LOW_BYTE(CONFIG_HID_INTERRUPT_IN_EP_MPS),	/* wMaxPacketSize: n Bytes */
	HIGH_BYTE(CONFIG_HID_INTERRUPT_IN_EP_MPS),
	CONFIG_HID_INTERRUPT_EP_INTERVAL_FS,	/* bInterval */

#ifdef CONFIG_HID_INTERRUPT_OUT
	/* Endpoint Descriptor */
	USB_ENDPOINT_DESC_SIZE,			/* bLength */
	USB_ENDPOINT_DESC,			/* bDescriptorType */
	/* bEndpointAddress: Direction: OUT - EndpointID: n */
	CONFIG_HID_INTERRUPT_OUT_EP_ADDR,
	USB_DC_EP_INTERRUPT,			/* bmAttributes: Interrupt Transfer Type */
	LOW_BYTE(CONFIG_HID_INTERRUPT_OUT_EP_MPS),	/* wMaxPacketSize: n Bytes */
	HIGH_BYTE(CONFIG_HID_INTERRUPT_OUT_EP_MPS),
	CONFIG_HID_INTERRUPT_EP_INTERVAL_FS,		/* bInterval */
#endif

	/* Interface_01 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	AUDIO_CTRL_INTER1,		/* bInterfaceNumber */
	0x00,				/* bAlternateSetting */
	0x00,				/* bNumEndpoints */
	USB_CLASS_AUDIO,		/* bInterfaceClass: Audio Interface Class */
	USB_SUBCLASS_AUDIOCONTROL,	/* bInterfaceSubClass */
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* Audio Control Interface Header Descriptor */
	UAC_DT_AC_HEADER_LENGTH,	/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_HEADER,			/* bDescriptorSubtype */
	LOW_BYTE(0x0100),		/* bcdADC */
	HIGH_BYTE(0x0100),
	LOW_BYTE(UAC_DT_TOTAL_LENGTH),	/* wTotalLength */
	HIGH_BYTE(UAC_DT_TOTAL_LENGTH),
	AUDIO_NUM_AS_INTERFACES,	/* bInCollection */
	#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
	AUDIO_AS_INTERFACE02_NUM,	/* baInterfaceNr[1] */
	AUDIO_AS_INTERFACE03_NUM,	/* baInterfaceNr[2] */
	#else
	AUDIO_AS_INTERFACE02_NUM,	/* baInterfaceNr[1] */
	#endif

	/* Audio Control Input Terminal_01 Descriptor */
	UAC_DT_INPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_INPUT_TERMINAL,		/* bDescriptorSubtype */
	INPUT_TERMINAL1_ID,		/* bTerminalID */
	LOW_BYTE(UAC_TERMINAL_STREAMING),	/* wTerminalType: USB streaming */
	HIGH_BYTE(UAC_TERMINAL_STREAMING),
	0x00,									/* bAssocTerminal */
	SINK_BNR_CHAN,				/* bNrChannels */
	LOW_BYTE(0x0003),			/* wChannelConfig */
	HIGH_BYTE(0x0003),
	0x00,									/* iChannelNames */
	0x00,									/* iTerminal */

	/* Audio Control Output Terminal_03 Descriptor */
	UAC_DT_OUTPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_OUTPUT_TERMINAL,		/* bDescriptorSubtype */
	OUTPUT_TERMINAL3_ID,		/* bTerminalID */
	LOW_BYTE(UAC_OUTPUT_TERMINAL_SPEAKER),	/* wTerminalType: Speaker */
	HIGH_BYTE(UAC_OUTPUT_TERMINAL_SPEAKER),
	0x00,									/* bAssocTerminal */
	FEATURE_UNIT1_ID,		/* bSourceID */
	0x00,									/* iTerminal */

	#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
	/* Audio Control Input Terminal_02 Descriptor */
	UAC_DT_INPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_INPUT_TERMINAL,		/* bDescriptorSubtype */
	INPUT_TERMINAL2_ID,		/* bTerminalID */
	LOW_BYTE(UAC_INPUT_TERMINAL_MICROPHONE),	/* wTerminalType: Microphone */
	HIGH_BYTE(UAC_INPUT_TERMINAL_MICROPHONE),
	0x00,										/* bAssocTerminal */
	SOURCE_BNR_CHAN,				/* bNrChannels */
	LOW_BYTE(0x0000),		/* wChannelConfig */
	HIGH_BYTE(0x0000),
	0x00,				/* iChannelNames */
	0x00,				/* iTerminal */

	/* Audio Control Output Terminal_04 Descriptor */
	UAC_DT_OUTPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_OUTPUT_TERMINAL,		/* bDescriptorSubtype */
	OUTPUT_TERMINAL4_ID,		/* bTerminalID */
	LOW_BYTE(UAC_TERMINAL_STREAMING),	/* wTerminalType: USB streaming */
	HIGH_BYTE(UAC_TERMINAL_STREAMING),
	0x00,				/* bAssocTerminal */
	FEATURE_UNIT2_ID,		/* bSourceID */
	0x00,				/* iTerminal */
	#endif

	#ifdef CONFIG_USB_AUDIO_CONTROL_UNIT
	/* Audio Control Feature Unit_09 Descriptor */
	UAC_DT_FEATURE_UNIT_SIZE(SINK_BM_CTRL_CHNUM, CONFIG_USB_AUDIO_BCONTROLSIZE),
	CS_INTERFACE,			/* bDescriptorType */
	UAC_FEATURE_UNIT,		/* bDescriptorSubtype */
	FEATURE_UNIT1_ID,		/* bUnitID */
	INPUT_TERMINAL1_ID,		/* bSourceID */
	CONFIG_USB_AUDIO_BCONTROLSIZE,	/* bControlSize */
	CONFIG_USB_AUDIO_SINK_MAINCH_CONFIG,		/* bmaControls[0] */
	CONFIG_USB_AUDIO_SINK_LOGICALCH01_CONFIG,	/* bmaControls[1] */
	CONFIG_USB_AUDIO_SINK_LOGICALCH02_CONFIG,	/* bmaControls[2] */
	0x00,				/* iFeature */

	#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
	/* Audio Control Feature Unit_05 Descriptor */
	UAC_DT_FEATURE_UNIT_SIZE(SOURCE_BM_CTRL_CHNUM, CONFIG_USB_AUDIO_BCONTROLSIZE),	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_FEATURE_UNIT,	/* bDescriptorSubtype */
	FEATURE_UNIT2_ID,	/* bUnitID */
	INPUT_TERMINAL2_ID,	/* bSourceID */
	CONFIG_USB_AUDIO_BCONTROLSIZE,	/* bControlSize */
	0x03,	/* bmaControls[0] */
	0x00,	/* iFeature */
	#endif
	#endif

	/* Interface_02 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	AUDIO_STRE_INTER2,		/* bInterfaceNumber */
	AUDIO_STRE_INTER2_ALT0,		/* bAlternateSetting */
	0x00,				/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* Interface_02 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	AUDIO_STRE_INTER2,		/* bInterfaceNumber */
	AUDIO_STRE_INTER2_ALT1,		/* bAlternateSetting */
	0x01,				/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* Audio Streaming Class Specific Interface Descriptor */
	UAC_DT_AS_HEADER_SIZE,		/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_AS_GENERAL,			/* bDescriptorSubtype */
	INPUT_TERMINAL1_ID,		/* bTerminalLink */
	0x01,				/* bDelay */
	LOW_BYTE(UAC_FORMAT_TYPE_I_PCM),	/* wFormatTag: PCM */
	HIGH_BYTE(UAC_FORMAT_TYPE_I_PCM),

	/* Audio Streaming Format Type Descriptor */
	0x0B,				/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_FORMAT_TYPE,		/* bDescriptorSubtype */
	UAC_FORMAT_TYPE_I,		/* bFormatType */
	SINK_BNR_CHAN,			/* bNrChannels */
	SUB_FRAME_SIZE,			/* bSubframeSize */
	RESOLUTION,			/* bBitResolution */
	SAMPLE_FREQ_TYPE,		/* bSamFreqType */
	SAM_LOW_BYTE(CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD),	/* tSamFreq[n] */
	SAM_MIDDLE_BYTE(CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD),
	SAM_HIGH_BYTE(CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD),

	/* Endpoint Descriptor */
	USB_ENDPOINT_DESC_SIZE,		/* bLength */
	USB_ENDPOINT_DESC,		/* bDescriptorType */
	/* bEndpointAddress: Direction: OUT - EndpointID: n */
	CONFIG_USB_AUDIO_SINK_OUT_EP_ADDR,
	0x09,				/* bmAttributes */
	LOW_BYTE(MAX_DOWNLOAD_PACKET),	/* wMaxPacketSize: n byte */
	HIGH_BYTE(MAX_DOWNLOAD_PACKET),
	0x01,				/* bInterval: 1ms */

	/* Audio Streaming Class Specific Audio Data Endpoint Descriptor */
	UAC_ISO_ENDPOINT_DESC_SIZE,	/* bLength */
	CS_ENDPOINT,			/* bDescriptorType */
	UAC_EP_GENERAL,			/* bDescriptorSubtype */
	0x01,				/* bmAttributes */
	0x01,				/* bLockDelayUnits */
	LOW_BYTE(0x0001),		/* wLockDelay */
	HIGH_BYTE(0x0001),

	#ifdef CONFIG_SUPPORT_HD_AUDIO_PLAY
	/* Interface_02 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	AUDIO_STRE_INTER2,		/* bInterfaceNumber */
	AUDIO_STRE_INTER2_ALT2,		/* bAlternateSetting */
	0x01,				/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* Audio Streaming Class Specific Interface Descriptor */
	UAC_DT_AS_HEADER_SIZE,		/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_AS_GENERAL,			/* bDescriptorSubtype */
	INPUT_TERMINAL1_ID,		/* bTerminalLink */
	0x01,				/* bDelay */
	LOW_BYTE(UAC_FORMAT_TYPE_I_PCM),	/* wFormatTag: PCM */
	HIGH_BYTE(UAC_FORMAT_TYPE_I_PCM),

	/* Audio Streaming Format Type Descriptor */
	0x0B,				/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_FORMAT_TYPE,		/* bDescriptorSubtype */
	UAC_FORMAT_TYPE_I,		/* bFormatType */
	SINK_BNR_CHAN,			/* bNrChannels */
	HD_FORMAT_SUB_FRAME_SIZE,	/* bSubframeSize */
	HD_FORMAT_RESOLUTION,		/* bBitResolution */
	SAMPLE_FREQ_TYPE,		/* bSamFreqType */
	SAM_LOW_BYTE(CONFIG_USB_AUDIO_SINK_HD_SAM_FREQ_DOWNLOAD),	/* tSamFreq[n] */
	SAM_MIDDLE_BYTE(CONFIG_USB_AUDIO_SINK_HD_SAM_FREQ_DOWNLOAD),
	SAM_HIGH_BYTE(CONFIG_USB_AUDIO_SINK_HD_SAM_FREQ_DOWNLOAD),

	/* Endpoint Descriptor */
	USB_ENDPOINT_DESC_SIZE,		/* bLength */
	USB_ENDPOINT_DESC,		/* bDescriptorType */
	/* bEndpointAddress:Direction: OUT - EndpointID: n */
	CONFIG_USB_AUDIO_SINK_OUT_EP_ADDR,
	0x09,				/* bmAttributes:Isochronous */
	/* wMaxPacketSize: 576 Byte = 0x0240 */
	LOW_BYTE(HD_FORMAT_MAX_DOWNLOAD_PACKET),
	HIGH_BYTE(HD_FORMAT_MAX_DOWNLOAD_PACKET),
	0x01,				/* bInterval: 1ms */

	/* Audio Streaming Class Specific Audio Data Endpoint Descriptor */
	UAC_ISO_ENDPOINT_DESC_SIZE,	/* bLength */
	CS_ENDPOINT,			/* bDescriptorType */
	UAC_EP_GENERAL,			/* bDescriptorSubtype */
	0x01,				/* bmAttributes */
	0x01,				/* bLockDelayUnits */
	LOW_BYTE(0x0001),		/* wLockDelay */
	HIGH_BYTE(0x0001),
	#endif

	#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
	/* Interface_03 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	AUDIO_STRE_INTER3,		/* bInterfaceNumber */
	AUDIO_STRE_INTER3_ALT0,		/* bAlternateSetting */
	0x00,				/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* Interface_03 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	AUDIO_STRE_INTER3,		/* bInterfaceNumber */
	AUDIO_STRE_INTER3_ALT1,		/* bAlternateSetting */
	0x01,				/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* Audio Streaming Class Specific Interface Descriptor */
	UAC_DT_AS_HEADER_SIZE,		/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_AS_GENERAL,			/* bDescriptorSubtype */
	OUTPUT_TERMINAL4_ID,		/* bTerminalLink */
	0x01,				/* bDelay*/
	LOW_BYTE(UAC_FORMAT_TYPE_I_PCM),	/* wFormatTag: PCM */
	HIGH_BYTE(UAC_FORMAT_TYPE_I_PCM),

	/* Audio Streaming Format Type Descriptor */
	0x0B,				/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_FORMAT_TYPE,		/* bDescriptorSubtype */
	UAC_FORMAT_TYPE_I,		/* bFormatType */
	CONFIG_USB_AUDIO_UPLOAD_CHANNEL_NUM,	/* bNrChannels */
	SUB_FRAME_SIZE,			/* bSubframeSize */
	RESOLUTION,			/* bBitResolution */
	0x01,				/* bSamFreqType */
	SAM_LOW_BYTE(CONFIG_USB_AUDIO_SOURCE_SAM_FREQ_UPLOAD),	/* tSamFreq[n] */
	SAM_MIDDLE_BYTE(CONFIG_USB_AUDIO_SOURCE_SAM_FREQ_UPLOAD),
	SAM_HIGH_BYTE(CONFIG_USB_AUDIO_SOURCE_SAM_FREQ_UPLOAD),

	/* Endpoint Descriptor */
	USB_ENDPOINT_DESC_SIZE,		/* bLength */
	USB_ENDPOINT_DESC,		/* bDescriptorType */
	/* bEndpointAddress: Direction: IN - EndpointID: n */
	CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR,
	0x05,				/* bmAttributes */
	LOW_BYTE(MAX_UPLOAD_PACKET),	/* wMaxPacketSize: n Byte */
	HIGH_BYTE(MAX_UPLOAD_PACKET),
	0x01,				/* bInterval */

	/* Audio Streaming Class Specific Audio Data Endpoint Descriptor */
	UAC_ISO_ENDPOINT_DESC_SIZE,	/* bLength */
	CS_ENDPOINT,			/* bDescriptorType */
	UAC_EP_GENERAL,			/* bDescriptorSubtype */
	0x01,				/* bmAttributes */
	0x01,				/* bLockDelayUnits */
	LOW_BYTE(0x0001),		/* wLockDelay */
	HIGH_BYTE(0x0001),
	#endif
};

static const u8_t usb_audio_dev_hs_desc[] = {
	/* Device Descriptor */
	USB_DEVICE_DESC_SIZE,		/* bLength */
	USB_DEVICE_DESC,		/* bDescriptorType */
	LOW_BYTE(USB_2_0),		/* bcdUSB */
	HIGH_BYTE(USB_2_0),
	0x00,				/* bDeviceClass */
	0x00,				/* bDeviceSubClass */
	0x00,				/* bDeviceProtocol */
	MAX_PACKET_SIZE0,		/* bMaxPacketSize0: 64 Bytes */
	LOW_BYTE(CONFIG_USB_APP_AUDIO_DEVICE_VID),	/* idVendor */
	HIGH_BYTE(CONFIG_USB_APP_AUDIO_DEVICE_VID),
	LOW_BYTE(CONFIG_USB_APP_AUDIO_DEVICE_PID),	/* idProduct */
	HIGH_BYTE(CONFIG_USB_APP_AUDIO_DEVICE_PID),
	LOW_BYTE(BCDDEVICE_RELNUM),			/* bcdDevice */
	HIGH_BYTE(BCDDEVICE_RELNUM),
	0x01,				/* iManufacturer */
	0x02,				/* iProduct */
	0x03,				/* iSerialNumber */
	0x01,				/* bNumConfigurations */

	/* Configuration Descriptor */
	USB_CONFIGURATION_DESC_SIZE,	/* bLength */
	USB_CONFIGURATION_DESC,		/* bDescriptorType */
	#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
	LOW_BYTE(0x00DC),		/* wTotalLength */
	HIGH_BYTE(0x00DC),
	0x04,				/* bNumInterfaces */
	#else
	LOW_BYTE(0x008C),		/* wTotalLength */
	HIGH_BYTE(0x008C),
	0x03,				/* bNumInterfaces */
	#endif
	0x01,				/* bConfigurationValue */
	0x00,				/* iConfiguration */
	USB_CONFIGURATION_ATTRIBUTES,	/* bmAttributes: Bus Powered */
	MAX_LOW_POWER,			/* MaxPower */

	/* Interface_0 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	HID_INTER_0,			/* bInterfaceNumber */
	HID_INTER_0_ALT,		/* bAlternateSetting */
	HID_INTERRUPT_EP_NUM,		/* bNumEndpoints */
	HID_CLASS,			/* bInterfaceClass: HID Interface Class */
	0x00,				/* bInterfaceSubClass */
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* HID Descriptor */
	sizeof(struct usb_hid_descriptor),	/* bLength */
	USB_HID_DESC,				/* bDescriptorType */
	LOW_BYTE(USB_1_1),			/* bcdHID */
	HIGH_BYTE(USB_1_1),
	HID_COUNTRYCODE_NUM,			/* bCountryCode */
	HID_CHILD_DESC_NUM,			/* bNumDescriptors */
	USB_HID_REPORT_DESC,			/* bDescriptorType */
	LOW_BYTE(CONFIG_HID_REPORT_DESC_SIZE),			/* wDescriptorLength */
	HIGH_BYTE(CONFIG_HID_REPORT_DESC_SIZE),

	/* Endpoint Descriptor */
	USB_ENDPOINT_DESC_SIZE,			/* bLength */
	USB_ENDPOINT_DESC,			/* bDescriptorType */
	/* bEndpointAddress: Direction: IN - EndpointID: n */
	CONFIG_HID_INTERRUPT_IN_EP_ADDR,
	USB_DC_EP_INTERRUPT,			/* bmAttributes: Interrupt Transfer Type */
	LOW_BYTE(CONFIG_HID_INTERRUPT_IN_EP_MPS),	/* wMaxPacketSize: n Bytes */
	HIGH_BYTE(CONFIG_HID_INTERRUPT_IN_EP_MPS),
	CONFIG_HID_INTERRUPT_EP_INTERVAL_HS,	/* bInterval */

#ifdef CONFIG_HID_INTERRUPT_OUT
	/* Endpoint Descriptor */
	USB_ENDPOINT_DESC_SIZE,			/* bLength */
	USB_ENDPOINT_DESC,			/* bDescriptorType */
	/* bEndpointAddress: Direction: OUT - EndpointID: n */
	CONFIG_HID_INTERRUPT_OUT_EP_ADDR,
	USB_DC_EP_INTERRUPT,			/* bmAttributes: Interrupt Transfer Type */
	LOW_BYTE(CONFIG_HID_INTERRUPT_OUT_EP_MPS),	/* wMaxPacketSize: n Bytes */
	HIGH_BYTE(CONFIG_HID_INTERRUPT_OUT_EP_MPS),
	CONFIG_HID_INTERRUPT_EP_INTERVAL_HS,	/* bInterval */
#endif

	/* Interface_01 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	AUDIO_CTRL_INTER1,		/* bInterfaceNumber */
	0x00,				/* bAlternateSetting */
	0x00,				/* bNumEndpoints */
	USB_CLASS_AUDIO,		/* bInterfaceClass: Audio Interface Class */
	USB_SUBCLASS_AUDIOCONTROL,	/* bInterfaceSubClass */
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* Audio Control Interface Header Descriptor */
	UAC_DT_AC_HEADER_LENGTH,	/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_HEADER,			/* bDescriptorSubtype */
	LOW_BYTE(0x0100),		/* bcdADC */
	HIGH_BYTE(0x0100),
	LOW_BYTE(UAC_DT_TOTAL_LENGTH),	/* wTotalLength */
	HIGH_BYTE(UAC_DT_TOTAL_LENGTH),
	AUDIO_NUM_AS_INTERFACES,	/* bInCollection */
	#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
	AUDIO_AS_INTERFACE02_NUM,	/* baInterfaceNr[1] */
	AUDIO_AS_INTERFACE03_NUM,	/* baInterfaceNr[2] */
	#else
	AUDIO_AS_INTERFACE02_NUM,	/* baInterfaceNr[1] */
	#endif

	/* Audio Control Input Terminal_01 Descriptor */
	UAC_DT_INPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_INPUT_TERMINAL,		/* bDescriptorSubtype */
	INPUT_TERMINAL1_ID,		/* bTerminalID */
	LOW_BYTE(UAC_TERMINAL_STREAMING),	/* wTerminalType: USB streaming */
	HIGH_BYTE(UAC_TERMINAL_STREAMING),
	0x00,									/* bAssocTerminal */
	SINK_BNR_CHAN,				/* bNrChannels */
	LOW_BYTE(0x0003),			/* wChannelConfig */
	HIGH_BYTE(0x0003),
	0x00,									/* iChannelNames */
	0x00,									/* iTerminal */

	/* Audio Control Output Terminal_03 Descriptor */
	UAC_DT_OUTPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_OUTPUT_TERMINAL,		/* bDescriptorSubtype */
	OUTPUT_TERMINAL3_ID,		/* bTerminalID */
	LOW_BYTE(UAC_OUTPUT_TERMINAL_SPEAKER),	/* wTerminalType: Speaker */
	HIGH_BYTE(UAC_OUTPUT_TERMINAL_SPEAKER),
	0x00,									/* bAssocTerminal */
	FEATURE_UNIT1_ID,		/* bSourceID */
	0x00,									/* iTerminal */

	#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
	/* Audio Control Input Terminal_02 Descriptor */
	UAC_DT_INPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_INPUT_TERMINAL,		/* bDescriptorSubtype */
	INPUT_TERMINAL2_ID,		/* bTerminalID */
	LOW_BYTE(UAC_INPUT_TERMINAL_MICROPHONE),	/* wTerminalType: Microphone */
	HIGH_BYTE(UAC_INPUT_TERMINAL_MICROPHONE),
	0x00,									/* bAssocTerminal */
	SOURCE_BNR_CHAN,				/* bNrChannels */
	LOW_BYTE(0x0000),		/* wChannelConfig */
	HIGH_BYTE(0x0000),
	0x00,									/* iChannelNames */
	0x00,									/* iTerminal */

	/* Audio Control Output Terminal_04 Descriptor */
	UAC_DT_OUTPUT_TERMINAL_SIZE,	/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_OUTPUT_TERMINAL,		/* bDescriptorSubtype */
	OUTPUT_TERMINAL4_ID,		/* bTerminalID */
	LOW_BYTE(UAC_TERMINAL_STREAMING),	/* wTerminalType: USB streaming */
	HIGH_BYTE(UAC_TERMINAL_STREAMING),
	0x00,				/* bAssocTerminal */
	FEATURE_UNIT2_ID,		/* bSourceID */
	0x00,				/* iTerminal */
	#endif

	#ifdef CONFIG_USB_AUDIO_CONTROL_UNIT
	/* Audio Control Feature Unit_09 Descriptor */
	UAC_DT_FEATURE_UNIT_SIZE(SINK_BM_CTRL_CHNUM, CONFIG_USB_AUDIO_BCONTROLSIZE),
	CS_INTERFACE,			/* bDescriptorType */
	UAC_FEATURE_UNIT,		/* bDescriptorSubtype */
	FEATURE_UNIT1_ID,		/* bUnitID */
	INPUT_TERMINAL1_ID,		/* bSourceID */
	CONFIG_USB_AUDIO_BCONTROLSIZE,	/* bControlSize */
	CONFIG_USB_AUDIO_SINK_MAINCH_CONFIG,		/* bmaControls[0] */
	CONFIG_USB_AUDIO_SINK_LOGICALCH01_CONFIG,	/* bmaControls[1] */
	CONFIG_USB_AUDIO_SINK_LOGICALCH02_CONFIG,	/* bmaControls[2] */
	0x00,				/* iFeature */

	#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
	/* Audio Control Feature Unit_05 Descriptor */
	UAC_DT_FEATURE_UNIT_SIZE(SOURCE_BM_CTRL_CHNUM, CONFIG_USB_AUDIO_BCONTROLSIZE),	/* bLength */
	CS_INTERFACE,	/* bDescriptorType */
	UAC_FEATURE_UNIT,	/* bDescriptorSubtype */
	FEATURE_UNIT2_ID,	/* bUnitID */
	INPUT_TERMINAL2_ID,	/* bSourceID */
	CONFIG_USB_AUDIO_BCONTROLSIZE,	/* bControlSize */
	0X03,	/* bmaControls[0] */
	0x00,	/* iFeature */
	#endif
	#endif

	/* Interface_02 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	AUDIO_STRE_INTER2,		/* bInterfaceNumber */
	AUDIO_STRE_INTER2_ALT0,		/* bAlternateSetting */
	0x00,				/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* Interface_02 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	AUDIO_STRE_INTER2,		/* bInterfaceNumber */
	AUDIO_STRE_INTER2_ALT1,		/* bAlternateSetting */
	0x01,				/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* Audio Streaming Class Specific Interface Descriptor */
	UAC_DT_AS_HEADER_SIZE,		/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_AS_GENERAL,			/* bDescriptorSubtype */
	INPUT_TERMINAL1_ID,		/* bTerminalLink */
	0x01,				/* bDelay */
	LOW_BYTE(UAC_FORMAT_TYPE_I_PCM),	/* wFormatTag: PCM */
	HIGH_BYTE(UAC_FORMAT_TYPE_I_PCM),

	/* Audio Streaming Format Type Descriptor */
	0x0B,				/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_FORMAT_TYPE,		/* bDescriptorSubtype */
	UAC_FORMAT_TYPE_I,		/* bFormatType */
	SINK_BNR_CHAN,			/* bNrChannels */
	SUB_FRAME_SIZE,			/* bSubframeSize */
	RESOLUTION,			/* bBitResolution */
	SAMPLE_FREQ_TYPE,		/* bSamFreqType */
	SAM_LOW_BYTE(CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD),	/* tSamFreq[n] */
	SAM_MIDDLE_BYTE(CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD),
	SAM_HIGH_BYTE(CONFIG_USB_AUDIO_SINK_SAM_FREQ_DOWNLOAD),

	/* Endpoint Descriptor */
	USB_ENDPOINT_DESC_SIZE,		/* bLength */
	USB_ENDPOINT_DESC,		/* bDescriptorType */
	/* bEndpointAddress: Direction: OUT - EndpointID: n */
	CONFIG_USB_AUDIO_SINK_OUT_EP_ADDR,
	0x09,				/* bmAttributes */
	LOW_BYTE(MAX_DOWNLOAD_PACKET),	/* wMaxPacketSize: n byte */
	HIGH_BYTE(MAX_DOWNLOAD_PACKET),
	0x04,				/* bInterval: 4ms */

	/* Audio Streaming Class Specific Audio Data Endpoint Descriptor */
	UAC_ISO_ENDPOINT_DESC_SIZE,	/* bLength */
	CS_ENDPOINT,			/* bDescriptorType */
	UAC_EP_GENERAL,			/* bDescriptorSubtype */
	0x01,				/* bmAttributes */
	0x01,				/* bLockDelayUnits */
	LOW_BYTE(0x0001),		/* wLockDelay */
	HIGH_BYTE(0x0001),

	#ifdef CONFIG_SUPPORT_HD_AUDIO_PLAY
	/* Interface_02 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	AUDIO_STRE_INTER2,		/* bInterfaceNumber */
	AUDIO_STRE_INTER2_ALT2,		/* bAlternateSetting */
	0x01,				/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* Audio Streaming Class Specific Interface Descriptor */
	UAC_DT_AS_HEADER_SIZE,		/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_AS_GENERAL,			/* bDescriptorSubtype */
	INPUT_TERMINAL1_ID,		/* bTerminalLink */
	0x01,				/* bDelay */
	LOW_BYTE(UAC_FORMAT_TYPE_I_PCM),	/* wFormatTag: PCM */
	HIGH_BYTE(UAC_FORMAT_TYPE_I_PCM),

	/* Audio Streaming Format Type Descriptor */
	0x0B,				/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_FORMAT_TYPE,		/* bDescriptorSubtype */
	UAC_FORMAT_TYPE_I,		/* bFormatType */
	SINK_BNR_CHAN,			/* bNrChannels */
	HD_FORMAT_SUB_FRAME_SIZE,	/* bSubframeSize */
	HD_FORMAT_RESOLUTION,		/* bBitResolution */
	SAMPLE_FREQ_TYPE,		/* bSamFreqType */
	SAM_LOW_BYTE(CONFIG_USB_AUDIO_SINK_HD_SAM_FREQ_DOWNLOAD),	/* tSamFreq[n] */
	SAM_MIDDLE_BYTE(CONFIG_USB_AUDIO_SINK_HD_SAM_FREQ_DOWNLOAD),
	SAM_HIGH_BYTE(CONFIG_USB_AUDIO_SINK_HD_SAM_FREQ_DOWNLOAD),

	/* Endpoint Descriptor */
	USB_ENDPOINT_DESC_SIZE,		/* bLength */
	USB_ENDPOINT_DESC,		/* bDescriptorType */
	/* bEndpointAddress:Direction: OUT - EndpointID: n */
	CONFIG_USB_AUDIO_SINK_OUT_EP_ADDR,
	0x09,				/* bmAttributes:Isochronous */
	/* wMaxPacketSize: 576 Byte = 0x0240 */
	LOW_BYTE(HD_FORMAT_MAX_DOWNLOAD_PACKET),
	HIGH_BYTE(HD_FORMAT_MAX_DOWNLOAD_PACKET),
	0x04,				/* bInterval: 4ms */

	/* Audio Streaming Class Specific Audio Data Endpoint Descriptor */
	UAC_ISO_ENDPOINT_DESC_SIZE,	/* bLength */
	CS_ENDPOINT,			/* bDescriptorType */
	UAC_EP_GENERAL,			/* bDescriptorSubtype */
	0x01,				/* bmAttributes */
	0x01,				/* bLockDelayUnits */
	LOW_BYTE(0x0001),		/* wLockDelay */
	HIGH_BYTE(0x0001),
	#endif

	#ifdef CONFIG_SUPPORT_USB_AUDIO_SOURCE
	/* Interface_03 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	AUDIO_STRE_INTER3,		/* bInterfaceNumber */
	AUDIO_STRE_INTER3_ALT0,		/* bAlternateSetting */
	0x00,				/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* Interface_03 Descriptor */
	USB_INTERFACE_DESC_SIZE,	/* bLength */
	USB_INTERFACE_DESC,		/* bDescriptorType */
	AUDIO_STRE_INTER3,		/* bInterfaceNumber */
	AUDIO_STRE_INTER3_ALT1,		/* bAlternateSetting */
	0x01,				/* bNumEndpoints */
	/* bInterfaceClass: Audio Interface Class */
	USB_CLASS_AUDIO,
	/* bInterfaceSubClass: Audio Streaming Interface SubClass */
	USB_SUBCLASS_AUDIOSTREAMING,
	0x00,				/* bInterfaceProtocol */
	0x00,				/* iInterface */

	/* Audio Streaming Class Specific Interface Descriptor */
	UAC_DT_AS_HEADER_SIZE,		/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_AS_GENERAL,			/* bDescriptorSubtype */
	OUTPUT_TERMINAL4_ID,		/* bTerminalLink */
	0x01,				/* bDelay*/
	LOW_BYTE(UAC_FORMAT_TYPE_I_PCM),	/* wFormatTag: PCM */
	HIGH_BYTE(UAC_FORMAT_TYPE_I_PCM),

	/* Audio Streaming Format Type Descriptor */
	0x0B,				/* bLength */
	CS_INTERFACE,			/* bDescriptorType */
	UAC_FORMAT_TYPE,		/* bDescriptorSubtype */
	UAC_FORMAT_TYPE_I,		/* bFormatType */
	CONFIG_USB_AUDIO_UPLOAD_CHANNEL_NUM,	/* bNrChannels */
	SUB_FRAME_SIZE,			/* bSubframeSize */
	RESOLUTION,			/* bBitResolution */
	0x01,				/* bSamFreqType */
	SAM_LOW_BYTE(CONFIG_USB_AUDIO_SOURCE_SAM_FREQ_UPLOAD),	/* tSamFreq[n] */
	SAM_MIDDLE_BYTE(CONFIG_USB_AUDIO_SOURCE_SAM_FREQ_UPLOAD),
	SAM_HIGH_BYTE(CONFIG_USB_AUDIO_SOURCE_SAM_FREQ_UPLOAD),

	/* Endpoint Descriptor */
	USB_ENDPOINT_DESC_SIZE,		/* bLength */
	USB_ENDPOINT_DESC,		/* bDescriptorType */
	/* bEndpointAddress: Direction: IN - EndpointID: n */
	CONFIG_USB_AUDIO_SOURCE_IN_EP_ADDR,
	0x05,				/* bmAttributes */
	LOW_BYTE(MAX_UPLOAD_PACKET),	/* wMaxPacketSize: n Byte */
	HIGH_BYTE(MAX_UPLOAD_PACKET),
	0x04,				/* bInterval */

	/* Audio Streaming Class Specific Audio Data Endpoint Descriptor */
	UAC_ISO_ENDPOINT_DESC_SIZE,	/* bLength */
	CS_ENDPOINT,			/* bDescriptorType */
	UAC_EP_GENERAL,			/* bDescriptorSubtype */
	0x01,				/* bmAttributes */
	0x01,				/* bLockDelayUnits */
	LOW_BYTE(0x0001),		/* wLockDelay */
	HIGH_BYTE(0x0001),
	#endif
};

#endif/*__USB_AUDIO_DEVICE_DESC_H__*/
