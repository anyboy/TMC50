/** @file
 *  @brief Internal APIs for Bluetooth Handsfree profile handling.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <bluetooth/rfcomm.h>
#include "at.h"

#define BT_HFP_MAX_MTU           140
#define BT_HF_CLIENT_MAX_PDU     BT_HFP_MAX_MTU

/* HFP AG Features */
#define BT_HFP_AG_FEATURE_3WAY_CALL     0x00000001 /* Three-way calling */
#define BT_HFP_AG_FEATURE_ECNR          0x00000002 /* EC and/or NR function */
#define BT_HFP_AG_FEATURE_VOICE_RECG    0x00000004 /* Voice recognition */
#define BT_HFP_AG_INBAND_RING_TONE      0x00000008 /* In-band ring capability */
#define BT_HFP_AG_VOICE_TAG             0x00000010 /* Attach no. to voice tag */
#define BT_HFP_AG_FEATURE_REJECT_CALL   0x00000020 /* Ability to reject call */
#define BT_HFP_AG_FEATURE_ECS           0x00000040 /* Enhanced call status */
#define BT_HFP_AG_FEATURE_ECC           0x00000080 /* Enhanced call control */
#define BT_HFP_AG_FEATURE_EXT_ERR       0x00000100 /* Extented error codes */
#define BT_HFP_AG_FEATURE_CODEC_NEG     0x00000200 /* Codec negotiation */
#define BT_HFP_AG_FEATURE_HF_IND        0x00000400 /* HF Indicators */
#define BT_HFP_AG_FEATURE_ESCO_S4       0x00000800 /* eSCO S4 Settings */
/* User external support feature */
#define BT_HFP_AG_FEATURE_BAT_REPORT    0x10000000 /* AT+IPHONEACCEV battery report*/

/* HFP HF Features */
#define BT_HFP_HF_FEATURE_ECNR          0x00000001 /* EC and/or NR */
#define BT_HFP_HF_FEATURE_3WAY_CALL     0x00000002 /* Three-way calling */
#define BT_HFP_HF_FEATURE_CLI           0x00000004 /* CLI presentation */
#define BT_HFP_HF_FEATURE_VOICE_RECG    0x00000008 /* Voice recognition */
#define BT_HFP_HF_FEATURE_VOLUME        0x00000010 /* Remote volume control */
#define BT_HFP_HF_FEATURE_ECS           0x00000020 /* Enhanced call status */
#define BT_HFP_HF_FEATURE_ECC           0x00000040 /* Enhanced call control */
#define BT_HFP_HF_FEATURE_CODEC_NEG     0x00000080 /* CODEC Negotiation */
#define BT_HFP_HF_FEATURE_HF_IND        0x00000100 /* HF Indicators */
#define BT_HFP_HF_FEATURE_ESCO_S4       0x00000200 /* eSCO S4 Settings */

#ifdef CONFIG_BT_HFP_HF_VOL_SYNC
#define BT_CONFIG_HFP_HF_FEATURE_VOLUME		BT_HFP_HF_FEATURE_VOLUME
#else
#define BT_CONFIG_HFP_HF_FEATURE_VOLUME		0
#endif

#ifdef CONFIG_BT_HFP_MSBC
#define BT_CONFIG_HFP_HF_FEATURE_CODEC_NEG	BT_HFP_HF_FEATURE_CODEC_NEG
#define BT_CONFIG_HFP_AG_FEATURE_CODEC_NEG	BT_HFP_AG_FEATURE_CODEC_NEG
#else
#define BT_CONFIG_HFP_HF_FEATURE_CODEC_NEG	0
#define BT_CONFIG_HFP_AG_FEATURE_CODEC_NEG	0
#endif

/* HFP HF Supported features */
#define BT_HFP_HF_SUPPORTED_FEATURES    (BT_HFP_HF_FEATURE_ECNR | \
						BT_HFP_HF_FEATURE_3WAY_CALL | \
						BT_HFP_HF_FEATURE_CLI | \
						BT_HFP_HF_FEATURE_VOICE_RECG | \
						BT_CONFIG_HFP_HF_FEATURE_VOLUME | \
						BT_HFP_HF_FEATURE_ECS | \
						BT_HFP_HF_FEATURE_ECC | \
						BT_CONFIG_HFP_HF_FEATURE_CODEC_NEG)

/* HFP AG Supported features */
#define BT_HFP_AG_SUPPORTED_FEATURES    (BT_HFP_AG_FEATURE_3WAY_CALL | \
						BT_HFP_AG_FEATURE_ECNR | \
						BT_HFP_AG_FEATURE_VOICE_RECG | \
						BT_HFP_AG_FEATURE_REJECT_CALL | \
						BT_HFP_HF_FEATURE_ECS | \
						BT_HFP_AG_FEATURE_EXT_ERR | \
						BT_CONFIG_HFP_AG_FEATURE_CODEC_NEG)

#define HF_MAX_BUF_LEN                  BT_HF_CLIENT_MAX_PDU
#define HF_MAX_AG_INDICATORS            20

struct bt_hfp_hf {
	struct bt_rfcomm_dlc rfcomm_dlc;
	char hf_buffer[HF_MAX_BUF_LEN];
	struct net_buf *cache_buf;
	char at_buffer[32];
	u8_t has_cmd;
	struct at_client at;
	u32_t hf_features;
	u32_t ag_features;
	u8_t codec_id;
	u8_t slc_step;
	s8_t ind_table[HF_MAX_AG_INDICATORS];
};

enum hfp_hf_ag_indicators {
	HFP_SERVICE_IND,
	HFP_CALL_IND,
	HFP_CALL_SETUP_IND,
	HFP_CALL_HELD_IND,
	HFP_SINGNAL_IND,
	HFP_ROAM_IND,
	HFP_BATTERY_IND,
	HFP_MAX_IND,
};

enum hfp_slc_establish_step {
	HF_SLC_START,
	HF_SLC_BRSF,
	HF_SLC_BAC,
	HF_SLC_CIND_CMD,
	HF_SLC_CIND_STATE,
	HF_SLC_CMER,
	HF_SLC_CHLD,
	HF_SLC_COPS,
	HF_SLC_CMEE,
	HF_SLC_BTRH,
	HF_SLC_CLIP,
	HF_SLC_CCWA,
	HF_SLC_CGMI,
	HF_SLC_CGMM,
	HF_SLC_FINISH,
};

/* Initialize hfp environment */
void bt_hfp_hf_init(void);

/***********************   HFP AG   ******************************/
struct bt_hfp_ag {
	struct bt_rfcomm_dlc rfcomm_dlc;
	struct k_delayed_work monitor_work;
	u32_t hf_features;
	u32_t ag_features;
	u8_t hf_codec_bit;
	u8_t codec_id;
	u8_t state;
	u8_t slc_established:1;
	u8_t ind_event_report:1;
};

void bt_hfp_ag_init(void);
