/*
 * Copyright (c) 2020 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Audio SPDIFTX physical implementation
 */

/*
 * Features
 *    - 32 level * 24bits FIFO
 *	- Support HDMI ARC
 *    - Support Sample Rate Change & Timeout Detect
 *    - Sample rate support up to 96KHz
 */

#include <kernel.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <soc.h>
#include <board.h>
#include <dvfs.h>
#include <audio_common.h>
#include "../phy_audio_common.h"
#include "../audio_acts_utils.h"
#include "audio_in.h"

#define SYS_LOG_DOMAIN "P_SPIDFRX"
#include <logging/sys_log.h>

/***************************************************************************************************
 * SPDIFRX_CTL0
 */
#define SPDIFR0_CTL0_ANA_RES                                  BIT(29) /* Enable or disable internal 2k ohm registor */
#define SPDIFR0_CTL0_ANA_HYSVOL                               BIT(28) /* Analog pin Hysteresis Voltage select 0: 110mv 1: 220mv */
#define SPDIFR0_CTL0_FCS                                      BIT(16) /* Filter Pulse Cycle Select */
#define SPDIFR0_CTL0_LOOPBACK_EN                              BIT(15) /* Enable internal loopback */
#define SPDIFR0_CTL0_VBM                                      BIT(14) /* Validity bit */
#define SPDIFR0_CTL0_DAMS                                     BIT(13) /* Data mask state. If sample rate changed, data will be keep as 0 and set this bit*/
#define SPDIFR0_CTL0_DAMEN                                    BIT(12) /* If sample rate changed, the data mask enable flag*/
#define SPDIFR0_CTL0_DELTAADD_SHIFT                           (8) /* Delta to add on configured or detected T width */
#define SPDIFR0_CTL0_DELTAADD_MASK                            (0xF << SPDIFR0_CTL0_DELTAADD_SHIFT)
#define SPDIFR0_CTL0_DELTAADD(x)                              ((x) << SPDIFR0_CTL0_DELTAADD_SHIFT)

#define SPDIFR0_CTL0_DELTAMIN_SHIFT                           (4) /* Delta to minus from configured or detected T width */
#define SPDIFR0_CTL0_DELTAMIN_MASK                            (0xF << SPDIFR0_CTL0_DELTAMIN_SHIFT)
#define SPDIFR0_CTL0_DELTAMIN(x)                              ((x) << SPDIFR0_CTL0_DELTAMIN_SHIFT)

#define SPDIFR0_CTL0_DELTA_MODE                               BIT(3) /* T Width delta mode select */
#define SPDIFR0_CTL0_CAL_MODE                                 BIT(2) /* T Width cal mode select */
#define SPDIFR0_CTL0_SPDIF_CKEDG                              BIT(1) /* Select of SPDIF input signal latch clock edge */
#define SPDIFR0_CTL0_SPDIF_RXEN                               BIT(0) /* SPDIF RX Enable */

/***************************************************************************************************
 * SPDIFRX_CTL1
 */
#define SPDIFR0_CTL1_WID2TCFG_SHIFT                           (16) /* 2T Width Config MAX:512 */
#define SPDIFR0_CTL1_WID2TCFG_MASK                            (0x1FF << SPDIFR0_CTL1_WID2TCFG_SHIFT)
#define SPDIFR0_CTL1_WID1P5TCFG_SHIFT                         (8) /* 1.5T Width Config MAX:156*/
#define SPDIFR0_CTL1_WID1P5TCFG_MASK                          (0xFF << SPDIFR0_CTL1_WID1P5TCFG_SHIFT)
#define SPDIFR0_CTL1_WID1TCFG_SHIFT                           (0) /* 1T Width Config MAX:156*/
#define SPDIFR0_CTL1_WID1TCFG_MASK                            (0xFF << SPDIFR0_CTL1_WID1TCFG_SHIFT)

/***************************************************************************************************
 * SPDIFRX_CTL2
 */
#define SPDIFR0_CTL2_WID4TCFG_SHIFT                           (18) /* 4T Width Config MAX:1024 */
#define SPDIFR0_CTL2_WID4TCFG_MASK                            (0x3FF << SPDIFR0_CTL2_WID4TCFG_SHIFT)
#define SPDIFR0_CTL2_WID3TCFG_SHIFT                           (9) /* 3T Width Config MAX:512 */
#define SPDIFR0_CTL2_WID3TCFG_MASK                            (0x1FF << SPDIFR0_CTL2_WID3TCFG_SHIFT)
#define SPDIFR0_CTL2_WID2P5TCFG_SHIFT                         (0) /* 2.5T Width Config MAX:512 */
#define SPDIFR0_CTL2_WID2P5TCFG_MASK                          (0x1FF << SPDIFR0_CTL2_WID2P5TCFG_SHIFT)

/***************************************************************************************************
 * SPDIFRX_PD
 */
#define SPDIFR0_PD_BL_HEADPD                                  BIT(16) /* Block head detect pending */
#define SPDIFR0_PD_SRTOPD                                     BIT(14) /* Sample rate detect timeout interrupt pending */
#define SPDIFR0_PD_CSSRUPPD                                   BIT(13) /* Channel state sample rate change IRQ pending */
#define SPDIFR0_PD_CSUPPD                                     BIT(12) /* Channel state update irq pending */
#define SPDIFR0_PD_SRCPD                                      BIT(11) /* Sample rate change pending */
#define SPDIFR0_PD_BMCERPD                                    BIT(10) /* BMC Decoder Err pending */
#define SPDIFR0_PD_SUBRCVPD                                   BIT(9) /* Sub Frame Receive Err pending */
#define SPDIFR0_PD_BLKRCVPD                                   BIT(8) /* Block received Err pending */
#define SPDIFR0_PD_SRTOEN                                     BIT(6) /* sample rate detect timeout IRQ enable */
#define SPDIFR0_PD_CSSRCIRQEN                                 BIT(5) /* channel state sample rate change irq enable */
#define SPDIFR0_PD_CSUPIRQEN                                  BIT(4) /* channel state update irq enable */
#define SPDIFR0_PD_SRCIRQEN                                   BIT(3) /* SPDIF RX sample rate change IRQ enable */
#define SPDIFR0_PD_BMCIRQEN                                   BIT(2) /* BMC Decoder Err IRQ enable */
#define SPDIFR0_PD_SUBIRQEN                                   BIT(1) /* Sub Frame receive Err IRQ enable */
#define SPDIFR0_PD_BLKIRQEN                                   BIT(0) /* Block Receive Err IRQ enable */

/***************************************************************************************************
 * SPDIFRX_DBG
 */
#define SPDIFR0_DBG_DBGSEL_SHIFT                              (17)
#define SPDIFR0_DBG_DBGSEL_MASK                               (0xF << SPDIFR0_DBG_DBGSEL_SHIFT)
#define SPDIFR0_DBG_SUBRCVFSM_SHIFT                           (14)
#define SPDIFR0_DBG_SUBRCVFSM_MASK                            (0x7 << SPDIFR0_DBG_SUBRCVFSM_SHIFT)
#define SPDIFR0_DBG_BMCRCVFSM_SHIFT                           (6)
#define SPDIFR0_DBG_BMCRCVFSM_MASK                            (0xFF << SPDIFR0_DBG_BMCRCVFSM_SHIFT)
#define SPDIFR0_DBG_BMCDECFSM_SHIFT                           (3)
#define SPDIFR0_DBG_BMCDECFSM_MASK                            (0x7 << SPDIFR0_DBG_BMCDECFSM_SHIFT)
#define SPDIFR0_DBG_HW_FSM_SHIFT                              (0)
#define SPDIFR0_DBG_HW_FSM_MASK                               (0x7 << SPDIFR0_DBG_HW_FSM_SHIFT)

/***************************************************************************************************
 * SPDIFRX_CNT
 */
#define SPDIFR0_CNT_DIN2_WIDTH_SHIFT                          (24) /* Din2 Width */
#define SPDIFR0_CNT_DIN2_WIDTH_MASK                           (0xFF << SPDIFR0_CNT_DIN1_WIDTH_SHIFT)
#define SPDIFR0_CNT_DIN1_WIDTH_SHIFT                          (16) /* Din1 Width */
#define SPDIFR0_CNT_DIN1_WIDTH_MASK                           (0xFF << SPDIFR0_CNT_DIN1_WIDTH_SHIFT)
#define SPDIFR0_CNT_DIN0_WIDTH_SHIFT                          (8) /* Din0 Width */
#define SPDIFR0_CNT_DIN0_WIDTH_MASK                           (0xFF << SPDIFR0_CNT_DIN0_WIDTH_SHIFT)
#define SPDIFR0_CNT_FRAMECNT_SHIFT                            (0) /* Audio Frame Counter */
#define SPDIFR0_CNT_FRAMECNT_MASK                             (0xFF << SPDIFR0_CNT_FRAMECNT_SHIFT)

/***************************************************************************************************
 * SPDIFRX_CSL
 */
#define SPDIFR0_CSL_SPDCSL_E                                  (31) /* SPDIFRX Channel State Low */
#define SPDIFR0_CSL_SPDCSL_SHIFT                              (0)
#define SPDIFR0_CSL_SPDCSL_MASK                               (0xFFFFFFFF << SPDIFR0_CSL_SPDCSL_SHIFT)

/***************************************************************************************************
 * SPDIFRX_CSH
 */
#define SPDIFR0_CSH_SPDCSH_SHIFT                              (0) /* SPDIFRX Channel State High */
#define SPDIFR0_CSH_SPDCSH_MASK                               (0xFFFF << SPDIFR0_CSH_SPDCSH_SHIFT)

/***************************************************************************************************
 * SPDIFRX_SAMP - Sample Rate Detect Register
 */
#define SPDIFR0_SAMP_SAMP_VALID                               BIT(28) /* sample rate valid flag */
#define SPDIFR0_SAMP_SAMP_CNT_SHIFT                           (16) /* SPDIFRX sample rate counter detect by 24M clock */
#define SPDIFR0_SAMP_SAMP_CNT_MASK                            (0xFFF << SPDIFR0_SAMP_SAMP_CNT_SHIFT)
#define SPDIFR0_SAMP_SAMP_DELTA_SHIFT                         (1) /* Delta is used by SAMP_CNT to detect sample rate change or not */
#define SPDIFR0_SAMP_SAMP_DELTA_MASK                          (0xF << SPDIFR0_SAMP_SAMP_DELTA_SHIFT)
#define SPDIFR0_SAMP_SAMP_DELTA(x)                            ((x) << SPDIFR0_SAMP_SAMP_DELTA_SHIFT)
#define SPDIFR0_SAMP_SAMP_EN                                  BIT(0) /* Sample rate detect enable */

/***************************************************************************************************
 * SPDIFRX_SRTO_THRES
 */
#define SPDIFR0_SRTO_THRES_SRTO_THRES_SHIFT                   (0) /* The threshold to generate sample rate detect timeout signal */
#define SPDIFR0_SRTO_THRES_SRTO_THRES_MASK                    (0xFFFFFF << 0)

/***************************************************************************************************
 * SPDIFRX_FIFOCTL
 */
#define SPDIFR0_FIFOCTL_FIFO_DMAWIDTH                         BIT(7) /* FIFO DMA transfer width configured */
#define SPDIFR0_FIFOCTL_FIFO_OS_SHIFT                         (4) /* FIFO output select */
#define SPDIFR0_FIFOCTL_FIFO_OS_MASK                          (0x3 << SPDIFR0_FIFOCTL_FIFO_OS_SHIFT)
#define SPDIFR0_FIFOCTL_FIFO_OS(x)                            ((x) << SPDIFR0_FIFOCTL_FIFO_OS_SHIFT)
#define SPDIFR0_FIFOCTL_FIFO_OS_CPU                           SPDIFR0_FIFOCTL_FIFO_OS(0)
#define SPDIFR0_FIFOCTL_FIFO_OS_DMA                           SPDIFR0_FIFOCTL_FIFO_OS(1)

#define SPDIFR0_FIFOCTL_FIFO_IEN                              BIT(2) /* FIFO Half filled IRQ enable */
#define SPDIFR0_FIFOCTL_FIFO_DEN                              BIT(1) /* FIFO Half filled DRQ enable */
#define SPDIFR0_FIFOCTL_FIFO_RST                              BIT(0) /* FIFO reset */

/***************************************************************************************************
 * SPDIFRX_FIFOSTA
 */
#define SPDIFR0_FIFOSTA_FIFO_ER                               BIT(8) /* FIFO error */
#define SPDIFR0_FIFOSTA_FIFO_IP                               BIT(7) /* FIFO Half Filled IRQ pending bit */
#define SPDIFR0_FIFOSTA_FIFO_EMPTY                            BIT(6) /* FIFO empty flag */
#define SPDIFR0_FIFOSTA_FIFO_STATUS_SHIFT                     (0) /* FIFO status */
#define SPDIFR0_FIFOSTA_FIFO_STATUS_MASK                      (0x3F << SPDIFR0_FIFOSTA_FIFO_STATUS_SHIFT)

/***************************************************************************************************
 * SPDIFRX_DAT_FIFO
 */
#define SPDIFR0_DAT_FIFO_RXDAT_SHIFT                          (8) /* SPDIFRX Data */
#define SPDIFR0_DAT_FIFO_RXDAT_MASK                           (0xFFFFFF << SPDIFR0_DAT_FIFO_RXDAT_SHIFT)

#define SPDIF_RX_INIT_DELTA                                   (4)
#define SPDIF_RX_DEFAULT_SAMPLE_DELTA                         (8)
#define SPDIF_RX_HIGH_SR_SAMPLE_DELTA                         (3)
#define SPDIF_RX_SR_DETECT_MS                                 (1)
#define SPDIF_RX_SRD_TIMEOUT_THRES                            (0xFFFFFF)

#define SPDIF_SAMPLE_RATE_FIXED

/*
 * enum a_spdifrx_srd_sts_e
 * @brief The SPDIFRX sample rate detect status
 */
typedef enum {
	RX_SR_STATUS_NOTGET = 0, /* Still not get the sample rate */
	RX_SR_STATUS_CHANGE, /* sample rate detect change happened */
	RX_SR_STATUS_GET, /* has gotten the sample rate */
	RX_SR_STATUS_TIMEOUT, /* connected timeout */
} a_spdifrx_srd_sts_e;

/* The SPDIF pin will consume about 200uA, and need to control independently */
static const struct acts_pin_config board_pin_spdifrx_config[] = {
#ifdef BOARD_SPDIFRX_MAP
	BOARD_SPDIFRX_MAP
#endif
};

/*
 * struct spdifrx_private_data
 * @brief Physical SPDIFRX device private data
 */
struct spdifrx_private_data {
	struct device *parent_dev;
	int (*srd_callback)(void *cb_data, u32_t cmd, void *param); /* sample rate detect callback */
	void *cb_data; /* callback user data */
	a_spdifrx_srd_sts_e srd_status; /* sample rate detect status */
	u8_t sample_rate;
};

/*
 * @struct acts_audio_spdifrx
 * @brief SPDIFRX controller hardware register
 */
struct acts_audio_spdifrx {
	volatile u32_t ctl0; /* SPDIFRX Control0 */
	volatile u32_t ctl1; /* SPDIFRX Control1 */
	volatile u32_t ctl2; /* SPDIFRX Control2 */
	volatile u32_t pending; /* SPDIFRX IRQ pending */
	volatile u32_t dbg; /* SPDIFRX debug */
	volatile u32_t cnt; /* SPDIFRX  counter */
	volatile u32_t csl; /* SPDIFRX Channel State Low */
	volatile u32_t csh; /* SPDIFRX Channel State High */
	volatile u32_t samp; /* SPDIFRX sample rate detect */
	volatile u32_t thres; /* SPDIFRX  sample rate detect timeout threshold */
	volatile u32_t fifoctl; /* SPDIFRX FIFO control */
	volatile u32_t fifostat; /* SPDIFRX FIFO state */
	volatile u32_t fifodat; /* SPDIFRX FIFO data */
};

static void phy_spdifrx_irq_enable(struct phy_audio_device *dev);
static void phy_spdifrx_irq_disable(struct phy_audio_device *dev);

/*
 * @brief  Get the SPDIFTX controller base address
 * @param  void
 * @return  The base address of the SPDIFTX controller
 */
static inline struct acts_audio_spdifrx *get_spdifrx_base(void)
{
	return (struct acts_audio_spdifrx *)AUDIO_SPDIFRX_REG_BASE;
}

/*
 * @brief Dump the SPDIFRX relative registers
 * @param void
 * @return void
 */
static void spdifrx_dump_regs(void)
{
#if CONFIG_SYS_LOG_DEFAULT_LEVEL >= 3
	struct acts_audio_spdifrx *spdifrx_base = (struct acts_audio_spdifrx *)get_spdifrx_base();

	SYS_LOG_INF("** spdifrx contoller regster **");
	SYS_LOG_INF("               BASE: %08x", (u32_t)spdifrx_base);
	SYS_LOG_INF("       SPDIFRX_CTL0: %08x", spdifrx_base->ctl0);
	SYS_LOG_INF("       SPDIFRX_CTL1: %08x", spdifrx_base->ctl1);
	SYS_LOG_INF("       SPDIFRX_CTL2: %08x", spdifrx_base->ctl2);
	SYS_LOG_INF("         SPDIFRX_PD: %08x", spdifrx_base->pending);
	SYS_LOG_INF("        SPDIFRX_DBG: %08x", spdifrx_base->dbg);
	SYS_LOG_INF("        SPDIFRX_CNT: %08x", spdifrx_base->cnt);
	SYS_LOG_INF("        SPDIFRX_CSL: %08x", spdifrx_base->csl);
	SYS_LOG_INF("        SPDIFRX_CSH: %08x", spdifrx_base->csh);
	SYS_LOG_INF("       SPDIFRX_SAMP: %08x", spdifrx_base->samp);
	SYS_LOG_INF(" SPDIFRX_STRO_THRES: %08x", spdifrx_base->thres);
	SYS_LOG_INF("    SPDIFRX_FIFOCTL: %08x", spdifrx_base->fifoctl);
	SYS_LOG_INF("    SPDIFRX_FIFOSTA: %08x", spdifrx_base->fifostat);
	SYS_LOG_INF("   SPDIFRX_DAT_FIFO: %08x", spdifrx_base->fifodat);
	SYS_LOG_INF("     CMU_SPDIFRXCLK: %08x", sys_read32(CMU_SPDIFRXCLK));
	SYS_LOG_INF("       CORE_PLL_CTL: %08x", sys_read32(CORE_PLL_CTL));
#endif
}

/*
 * @brief Prepare the clock and pinmux resources for the SPDIFRX enable
 * @param  void
 * @return 0 on success, negative errno code on failure
 */
static int phy_spdifrx_prepare_enable(u8_t sr)
{
    acts_pinmux_set(board_pin_spdifrx_config[0].pin_num, 0);

	/* spdif rx normal */
	acts_reset_peripheral(RESET_ID_SPDIF_RX);

	sys_write32((sys_read32(CMU_SPDIFRXCLK) & ~(CMU_SPDIFRXCLK_SPDIFRXCLKSRC_MASK
				| CMU_SPDIFRXCLK_SPDIFRXCLKDIV_MASK)), CMU_SPDIFRXCLK);

	/* Select SPDIFRX clock source to be corepll */
	sys_write32((sys_read32(CMU_SPDIFRXCLK) | (0x02 << CMU_SPDIFRXCLK_SPDIFRXCLKSRC_SHIFT)), CMU_SPDIFRXCLK);

#if defined(SPDIF_SAMPLE_RATE_FIXED) && defined(CONFIG_DVFS)
	printk("==> sample rate %d\n", sr);
	if (sr <= SAMPLE_RATE_16KHZ) {
		u8_t pll_index, seires;
		if (SAMPLE_RATE_11KHZ == sr) {
			seires = AUDIOPLL_44KSR;
		} else {
			seires = AUDIOPLL_48KSR;
		}
		if (audio_pll_check_config(seires, &pll_index)) {
			SYS_LOG_ERR("pll check and config error");
			return -EFAULT;
		}
		u32_t reg_val = (sys_read32(CMU_SPDIFRXCLK) & (~0x03FF)) | ((pll_index << 8) | (3 << 0));
		sys_write32((sys_read32(CMU_SPDIFRXCLK) & (~0x03FF)) | ((pll_index << 8) | (3 << 0)), CMU_SPDIFRXCLK);
		printk("reg_val 0x%x, RXCLK 0x%x\n", reg_val, sys_read32(CMU_SPDIFRXCLK));
	} else {
		soc_dvfs_set_spdif_rate(SAMPLE_RATE_192KHZ);
	}
#endif

	/* this will also enable the HOSC clock to detect Audio Sample Rate */
	acts_clock_peripheral_enable(CLOCK_ID_SPDIF_RX);

	/* spdif rx normal */
	acts_reset_peripheral(RESET_ID_SPDIF_RX);

	return 0;
}

/*
 * @brief Config the SPDIFRX FIFO source
 * @param  sel: select the fifo input source, e.g. CPU or DMA
 * @param  width: specify the transfer width when in dma mode
 * @return 0 on success, negative errno code on failure
 */
static int phy_spdifrx_fifo_config(audio_fifouse_sel_e sel, audio_dma_width_e width)
{
	struct acts_audio_spdifrx *spdifrx_base = (struct acts_audio_spdifrx *)get_spdifrx_base();

	if (FIFO_SEL_CPU == sel) {
		spdifrx_base->fifoctl = (SPDIFR0_FIFOCTL_FIFO_IEN | SPDIFR0_FIFOCTL_FIFO_RST);
	} else if (FIFO_SEL_DMA == sel) {
		spdifrx_base->fifoctl = (SPDIFR0_FIFOCTL_FIFO_OS_DMA | SPDIFR0_FIFOCTL_FIFO_DEN
									| SPDIFR0_FIFOCTL_FIFO_RST);
		if (DMA_WIDTH_16BITS == width)
			spdifrx_base->fifoctl |= SPDIFR0_FIFOCTL_FIFO_DMAWIDTH;
	} else {
		SYS_LOG_ERR("invalid fifo sel %d", sel);
		return -EINVAL;
	}

	return 0;
}

/*
 * @brief SPDIF control and config
 * @param  void
 * @return void
 */
static void phy_spdifrx_ctl_config(void)
{
	struct acts_audio_spdifrx *spdifrx_base = (struct acts_audio_spdifrx *)get_spdifrx_base();

	spdifrx_base->ctl0 = 0;

	/* Enable hardware detect T Width */
	spdifrx_base->ctl0 |= SPDIFR0_CTL0_CAL_MODE;

#ifndef SPDIFRX_SOFTWARE_DELTA_MODE
	/* Enable hardware automatically fill DELTAADD and DELTAMIN T Width */
	spdifrx_base->ctl0 |= SPDIFR0_CTL0_DELTA_MODE;

	/* Delta to minus from configured or detected T width */
	spdifrx_base->ctl0 |= SPDIFR0_CTL0_DELTAMIN(SPDIF_RX_INIT_DELTA);

	/* Delta to add on configured or detected T width */
	spdifrx_base->ctl0 |= SPDIFR0_CTL0_DELTAADD(SPDIF_RX_INIT_DELTA);
#endif

	/* DAMEN */
	spdifrx_base->ctl0 |= SPDIFR0_CTL0_DAMEN;

	/* spdif rx enable */
	spdifrx_base->ctl0 |= SPDIFR0_CTL0_SPDIF_RXEN;

	/* sample rate detect enable */
	spdifrx_base->samp &= (~SPDIFR0_SAMP_SAMP_DELTA_MASK);
	spdifrx_base->samp |= SPDIFR0_SAMP_SAMP_DELTA(SPDIF_RX_DEFAULT_SAMPLE_DELTA);
	spdifrx_base->samp |= SPDIFR0_SAMP_SAMP_EN;
}

/*
 * @brief Get the SPDIFRX channel status
 * @param sts: the setting of channel status
 * @return void
 */
static void phy_spdifrx_channel_status_get(audio_spdif_ch_status_t *sts)
{
	struct acts_audio_spdifrx *spdifrx_base = (struct acts_audio_spdifrx *)get_spdifrx_base();

	if (sts) {
		sts->csl = spdifrx_base->csl;
		sts->csh = (u16_t)spdifrx_base->csh;
	}
}

/*
 * @brief Check if the stream is the PCM format stream
 * @param void
 * @return true indicates is the pcm stream otherwise not.
 */
static bool phy_spdifrx_check_is_pcm(void)
{
	struct acts_audio_spdifrx *spdifrx_base = (struct acts_audio_spdifrx *)get_spdifrx_base();

	/* The channel status bit1 indicats the stream is a linear PCM samples */
	if (spdifrx_base->csl & (1 << 1))
		return false;

	return true;
}

/*
 * @brief Check if decode error happened
 * @param void
 * @return true indicates decode error happened otherwise not.
 */
static bool phy_spdifrx_check_decode_err(void)
{
	struct acts_audio_spdifrx *spdifrx_base = (struct acts_audio_spdifrx *)get_spdifrx_base();
	bool ret = false;

	if (spdifrx_base->pending & SPDIFR0_PD_BLKRCVPD) {
		spdifrx_base->pending |= SPDIFR0_PD_BLKRCVPD;
		ret = true;
	}

	if (spdifrx_base->pending & SPDIFR0_PD_SUBRCVPD) {
		spdifrx_base->pending |= SPDIFR0_PD_SUBRCVPD;
		ret = true;
	}

	if (spdifrx_base->pending & SPDIFR0_PD_BMCERPD) {
		spdifrx_base->pending |= SPDIFR0_PD_BMCERPD;
		ret = true;
	}

	return ret;
}

static int phy_spdifrx_enable(struct phy_audio_device *dev, void *param)
{
	ain_param_t *in_param = (ain_param_t *)param;
	spdifrx_setting_t *spdifrx_setting = in_param->spdifrx_setting;
	struct spdifrx_private_data *priv = (struct spdifrx_private_data *)dev->private;
	int ret;

	if (!in_param) {
		SYS_LOG_ERR("Invalid parameters");
		return -EINVAL;
	}

	if (in_param->sample_rate)
		priv->sample_rate = in_param->sample_rate;

	if (spdifrx_setting && spdifrx_setting->srd_callback) {
		priv->srd_callback = spdifrx_setting->srd_callback;
		priv->cb_data = spdifrx_setting->cb_data;
	} else {
		priv->srd_callback = NULL;
		priv->cb_data = NULL;
	}

	/* Prepare the spdifrx clock and pinmux etc. */
	ret = phy_spdifrx_prepare_enable(priv->sample_rate);
	if (ret)
		return ret;

	/* Config the SPDIFRX FIFO */
	ret = phy_spdifrx_fifo_config(FIFO_SEL_DMA, (in_param->channel_width == CHANNEL_WIDTH_16BITS)
									?  DMA_WIDTH_16BITS : DMA_WIDTH_32BITS);
	if (ret) {
		SYS_LOG_ERR("Config SPDIFRX FIFO error %d", ret);
		return ret;
	}

	/* Control spdifrx and enable sample rate detect function */
	phy_spdifrx_ctl_config();

	/* Enable SPDIFRX IRQ */
	phy_spdifrx_irq_enable(dev);

	return 0;
}

static int phy_spdifrx_disable(struct phy_audio_device *dev)
{
	int i;
	struct acts_audio_spdifrx *spdifrx_base = (struct acts_audio_spdifrx *)get_spdifrx_base();
	struct spdifrx_private_data *priv = (struct spdifrx_private_data *)dev->private;

	/* Set the spdiftx pins to be GPIO state for the power consume */
	for (i = 0; i < ARRAY_SIZE(board_pin_spdifrx_config); i++)
		acts_pinmux_set(board_pin_spdifrx_config[i].pin_num, 0);

	/* SPDIFRX FIFO reset */
	spdifrx_base->fifoctl &= ~SPDIFR0_FIFOCTL_FIFO_RST;

	/* SPDIF RX disable */
	spdifrx_base->ctl0 &= ~SPDIFR0_CTL0_SPDIF_RXEN;

	/* SPDIF RX clock gating disable */
	acts_clock_peripheral_disable(CLOCK_ID_SPDIF_RX);

	/* Disable SPDIFRX IRQ */
	phy_spdifrx_irq_disable(dev);

	priv->srd_callback = NULL;
	priv->cb_data = NULL;
	priv->sample_rate = 0;

	return 0;
}

#ifndef SPDIF_SAMPLE_RATE_FIXED
static void deal_for_new_samplerate(struct phy_audio_device *dev)
{
	struct spdifrx_private_data *priv = (struct spdifrx_private_data *)dev->private;
	u32_t rxclk;

#ifdef SPDIFRX_SOFTWARE_DELTA_MODE
	struct acts_audio_spdifrx *spdifrx_base = (struct acts_audio_spdifrx *)get_spdifrx_base();
	u32_t txclk, new_T, new_delta;
#endif

	if (!priv->sample_rate) {
		SYS_LOG_ERR("sample rate invalid");
        return;
	}

    rxclk = soc_dvfs_set_spdif_rate(priv->sample_rate);

#ifdef SPDIFRX_SOFTWARE_DELTA_MODE
	rxclk *= 1000;
	txclk = 128 * priv->sample_rate;
	new_T = 2 * (rxclk / txclk);
	new_delta = new_T / 4;

	SYS_LOG_INF("%d, rxclk: %d, txclk: %d\n", __LINE__, rxclk, txclk);
	SYS_LOG_INF("%d, soft_T: %d, hw_T: %d\n", __LINE__, new_T, spdifrx_base->ctl1 & SPDIFR0_CTL1_WID1TCFG_MASK);
	SYS_LOG_INF("%d, delta: %d\n", __LINE__, new_delta);

	if(new_delta > 15) {
		SYS_LOG_ERR("%d, delta is too large!\n", __LINE__);
	} else if(new_delta < 3) {
		SYS_LOG_ERR("%d, delta is too small!\n", __LINE__);
	} else {
		/* config new delta */
        SYS_LOG_INF("delta is %d ",new_delta);
		spdifrx_base->ctl0 &= (~SPDIFR0_CTL0_DELTAMIN_MASK);
		spdifrx_base->ctl0 &= (~SPDIFR0_CTL0_DELTAADD_MASK);
		spdifrx_base->ctl0 |= SPDIFR0_CTL0_DELTAMIN(new_delta);
		spdifrx_base->ctl0 |= SPDIFR0_CTL0_DELTAADD(new_delta);

        if (priv->sample_rate > SAMPLE_96KHZ) {
            spdifrx_base->samp &= (~SPDIFR0_SAMP_SAMP_DELTA_MASK);
            spdifrx_base->samp |= SPDIFR0_SAMP_SAMP_DELTA(SPDIF_RX_HIGH_SR_SAMPLE_DELTA);
        } else{
            spdifrx_base->samp &= (~SPDIFR0_SAMP_SAMP_DELTA_MASK);
            spdifrx_base->samp |= SPDIFR0_SAMP_SAMP_DELTA(SPDIF_RX_DEFAULT_SAMPLE_DELTA);
        }
	}
#endif
}
#endif

static int phy_spdifrx_samplerate_detect(struct phy_audio_device *dev, int wait_time_ms)
{
	struct acts_audio_spdifrx *spdifrx_base = (struct acts_audio_spdifrx *)get_spdifrx_base();
	struct spdifrx_private_data *priv = (struct spdifrx_private_data *)dev->private;
	u32_t time_stamp_begin, time_stamp_now;
	int result = -1;
	u8_t index;
	u32_t sample_cnt;

	static const u16_t sample_cnt_tbl[14][2] = {
		{120, 130},			// 192k,  24M/192K =  125
		{131, 141},			// 176.4k,  24M/176K = 136
		{245, 255},			// 96k,  24M/96K = 250
		{267, 277},			// 88.2k, 24M/88K = 272
		{369, 381},			// 64k, 24M/64K = 375
		{489, 510},			// 48k,  24M/48K = 500
		{532, 557},			// 44.1k, 24M/44K = 544
		{727, 774},			// 32k, 24M/32K = 750
		{960, 1043},		// 24k, 24M/24K = 1000
		{1041,1140},		// 22.05k, 24M/22.05K = 1088
		{1411, 1600},		// 16k, 24M/16K = 1500
		{1846, 2170},		// 12k, 24M/12K = 2000
		{2171, 2394},		// 11.025k, 24M/11.025K = 2176
		{2666, 3428},		// 8k, 24M/8K = 3000
	};

	const uint8_t sample_rate_tbl[14] = {
		SAMPLE_192KHZ, SAMPLE_176KHZ, SAMPLE_96KHZ, SAMPLE_88KHZ, SAMPLE_64KHZ, SAMPLE_48KHZ, SAMPLE_44KHZ,
		SAMPLE_32KHZ, SAMPLE_24KHZ, SAMPLE_22KHZ, SAMPLE_16KHZ, SAMPLE_12KHZ, SAMPLE_11KHZ, SAMPLE_8KHZ
	};

	if(priv->srd_status >= RX_SR_STATUS_GET) {
		SYS_LOG_INF("%d, have got spdifrx sample, no need to detect: %d!\n", __LINE__, priv->srd_status);
		return result;
	}

	if (wait_time_ms != K_FOREVER) {
		time_stamp_begin = k_cycle_get_32();
	}

    do {
		/* get the sample rate detect counter */
		sample_cnt = ((spdifrx_base->samp & SPDIFR0_SAMP_SAMP_CNT_MASK) >> SPDIFR0_SAMP_SAMP_CNT_SHIFT);

		if (wait_time_ms != K_FOREVER) {
			time_stamp_now = k_cycle_get_32();
			if((time_stamp_now - time_stamp_begin) / 24000 > wait_time_ms) {
				SYS_LOG_INF("%d, time over sample_cnt %d!!\n", __LINE__, sample_cnt);
				result = -1;
				break;
			}
		}

		/* check sample valid */
		if((spdifrx_base->samp & SPDIFR0_SAMP_SAMP_VALID) == 0)
        {
        	/* BMC_ERR or SAMP_CNT overflow */
            SYS_LOG_DBG("valid bit is invaild");
        } else {
			/* sample rate valid */
			for(index = 0; index < sizeof(sample_rate_tbl); index++) {
				if((sample_cnt >= sample_cnt_tbl[index][0]) && (sample_cnt <= sample_cnt_tbl[index][1])) {
					priv->sample_rate = sample_rate_tbl[index];
					result = 0;
					break;
				}
			}

			if (result == 0) {
				SYS_LOG_INF("%d, get spdifrx sr: %d!\n", __LINE__, priv->sample_rate);
				break;
			}
		}
	}while(1);

	return result;
}

static void spdifrx_acts_irq(void* arg)
{
	struct acts_audio_spdifrx *spdifrx_base = (struct acts_audio_spdifrx *)get_spdifrx_base();
	struct phy_audio_device *dev = (struct phy_audio_device *)arg;
	struct spdifrx_private_data *priv = (struct spdifrx_private_data *)dev->private;
	int result;
    u32_t pending;

	SYS_LOG_DBG("%d, spdifrx irq pending 0x%x", __LINE__, spdifrx_base->pending);

	pending = spdifrx_base->pending & (~0x7F);

	/* deal for spdif rx irq */
	if (spdifrx_base->pending & SPDIFR0_PD_SRCPD) {
		/* sample rate change happened */
		priv->srd_status = RX_SR_STATUS_CHANGE;

		result = phy_spdifrx_samplerate_detect(dev, SPDIF_RX_SR_DETECT_MS);
        if(result != 0) {
			SYS_LOG_ERR("%d, get new samplerate fail!\n", __LINE__);
			if (priv->srd_callback != NULL)
				priv->srd_callback(priv->cb_data, SPDIFRX_SRD_TIMEOUT, NULL);
		} else {
			/* get new sample rate,  need cal new delta
                      * get spdif rx sample rate
            		  */
			priv->srd_status = RX_SR_STATUS_GET;
#ifndef SPDIF_SAMPLE_RATE_FIXED
			deal_for_new_samplerate(dev);
#endif
		}

		spdifrx_base->ctl0 |= SPDIFR0_CTL0_DAMS;

		if (result == 0) {
			/* get new sample rate,  callback */
			if (priv->srd_callback != NULL) {
				priv->srd_callback(priv->cb_data, SPDIFRX_SRD_FS_CHANGE, (void *)&priv->sample_rate);
			}
		}
	}

	/* sample rate detect timeout pending */
	if ((spdifrx_base->pending & SPDIFR0_PD_SRTOPD)
		&& (priv->srd_status == RX_SR_STATUS_GET)) {
		if (priv->srd_callback != NULL) {
			priv->srd_callback(priv->cb_data, SPDIFRX_SRD_TIMEOUT, NULL);
		}
		priv->srd_status = RX_SR_STATUS_NOTGET;
	}

	/* clear pending */
	spdifrx_base->pending |= pending;

}

static int phy_spdifrx_ioctl(struct phy_audio_device *dev, u32_t cmd, void *param)
{
	ARG_UNUSED(dev);

	switch (cmd) {
	case PHY_CMD_DUMP_REGS:
	{
		spdifrx_dump_regs();
		break;
	}
	case AIN_CMD_SPDIF_GET_CHANNEL_STATUS:
	{
		audio_spdif_ch_status_t *sts = (audio_spdif_ch_status_t *)param;
		if (!sts) {
			SYS_LOG_ERR("Invalid parameters");
			return -EINVAL;
		}
		phy_spdifrx_channel_status_get(sts);
		break;
	}
	case AIN_CMD_SPDIF_IS_PCM_STREAM:
	{
		if (phy_spdifrx_check_is_pcm())
			*(bool *)param = true;
		else
			*(bool *)param = false;
		break;
	}
	case AIN_CMD_SPDIF_CHECK_DECODE_ERR:
	{
		if (phy_spdifrx_check_decode_err())
			*(bool *)param = true;
		else
			*(bool *)param = false;
		break;
	}
	default:
		SYS_LOG_ERR("Unsupport command %d", cmd);
		return -ENOTSUP;

	}

	return 0;
}

static int phy_spdifrx_init(struct phy_audio_device *dev, struct device *parent_dev)
{
	struct spdifrx_private_data *priv = (struct spdifrx_private_data *)dev->private;
	/* clear private data */
	memset(priv, 0, sizeof(struct spdifrx_private_data));
	priv->parent_dev = parent_dev;

	SYS_LOG_INF("PHY SPDIFRX probe");

	return 0;
}

static struct spdifrx_private_data spdifrx_priv;

const struct phy_audio_operations spdifrx_aops = {
	.audio_init = phy_spdifrx_init,
	.audio_enable = phy_spdifrx_enable,
	.audio_disable = phy_spdifrx_disable,
	.audio_ioctl = phy_spdifrx_ioctl,
};

PHY_AUDIO_DEVICE(spdifrx, PHY_AUDIO_SPDIFRX_NAME, &spdifrx_aops, &spdifrx_priv);
PHY_AUDIO_DEVICE_FN(spdifrx);

static void phy_spdifrx_irq_enable(struct phy_audio_device *dev)
{
	struct acts_audio_spdifrx *spdifrx_base = (struct acts_audio_spdifrx *)get_spdifrx_base();

	spdifrx_base->thres = SPDIF_RX_SRD_TIMEOUT_THRES;

	/* SPDIFRX sample rate detect IRQ enable */
	spdifrx_base->pending |= (SPDIFR0_PD_SRCIRQEN | SPDIFR0_PD_SRTOEN);

	/* clear all IRQ pendings */
	spdifrx_base->pending |= (SPDIFR0_PD_BLKRCVPD | SPDIFR0_PD_SUBRCVPD
							| SPDIFR0_PD_BMCERPD | SPDIFR0_PD_SRCPD
							| SPDIFR0_PD_CSUPPD | SPDIFR0_PD_CSSRUPPD
							| SPDIFR0_PD_SRTOPD | SPDIFR0_PD_BL_HEADPD);

	if (irq_is_enabled(IRQ_ID_SPDIF_RX) == 0) {
		IRQ_CONNECT(IRQ_ID_SPDIF_RX, CONFIG_IRQ_PRIO_HIGH,
			spdifrx_acts_irq, PHY_AUDIO_DEVICE_GET(spdifrx), 0);
		irq_enable(IRQ_ID_SPDIF_RX);

        /* set spdif rx pin state */
        acts_pinmux_setup_pins(board_pin_spdifrx_config, ARRAY_SIZE(board_pin_spdifrx_config));
	}
}

static void phy_spdifrx_irq_disable(struct phy_audio_device *dev)
{
	struct acts_audio_spdifrx *spdifrx_base = (struct acts_audio_spdifrx *)get_spdifrx_base();

	/* Disable sample rate change IRQ */
	spdifrx_base->pending &= ~SPDIFR0_PD_SRCIRQEN;

	if(irq_is_enabled(IRQ_ID_SPDIF_RX) != 0) {
        acts_pinmux_set(board_pin_spdifrx_config[0].pin_num, 0);
		irq_disable(IRQ_ID_SPDIF_RX);
	}
}
