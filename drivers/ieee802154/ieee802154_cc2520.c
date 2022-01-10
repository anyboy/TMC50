/* ieee802154_cc2520.c - TI CC2520 driver */

/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_IEEE802154_DRIVER_LEVEL
#define SYS_LOG_DOMAIN "dev/cc2520"
#include <logging/sys_log.h>

#include <errno.h>

#include <kernel.h>
#include <arch/cpu.h>

#include <board.h>
#include <device.h>
#include <init.h>
#include <net/net_if.h>
#include <net/net_pkt.h>

#include <misc/byteorder.h>
#include <string.h>
#include <rand32.h>

#include <gpio.h>

#ifdef CONFIG_IEEE802154_CC2520_CRYPTO

#include <crypto/cipher.h>
#include <crypto/cipher_structs.h>

#endif /* CONFIG_IEEE802154_CC2520_CRYPTO */

#include <net/ieee802154_radio.h>

#include "ieee802154_cc2520.h"

/**
 * Content is split as follows:
 * 1 - Debug related functions
 * 2 - Generic helper functions (for any parts)
 * 3 - GPIO related functions
 * 4 - TX related helper functions
 * 5 - RX related helper functions
 * 6 - Radio device API functions
 * 7 - Legacy radio device API functions
 * 8 - Initialization
 */


#define CC2520_AUTOMATISM		(FRMCTRL0_AUTOCRC | FRMCTRL0_AUTOACK)
#define CC2520_TX_THRESHOLD		(0x7F)
#define CC2520_FCS_LENGTH		(2)

/*********
 * DEBUG *
 ********/
#if CONFIG_SYS_LOG_IEEE802154_DRIVER_LEVEL == 4
static inline void _cc2520_print_gpio_config(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	SYS_LOG_DBG("GPIOCTRL0/1/2/3/4/5 = 0x%x/0x%x/0x%x/0x%x/0x%x/0x%x",
		    read_reg_gpioctrl0(&cc2520->spi),
		    read_reg_gpioctrl1(&cc2520->spi),
		    read_reg_gpioctrl2(&cc2520->spi),
		    read_reg_gpioctrl3(&cc2520->spi),
		    read_reg_gpioctrl4(&cc2520->spi),
		    read_reg_gpioctrl5(&cc2520->spi));
	SYS_LOG_DBG("GPIOPOLARITY: 0x%x",
		    read_reg_gpiopolarity(&cc2520->spi));
	SYS_LOG_DBG("GPIOCTRL: 0x%x",
		    read_reg_gpioctrl(&cc2520->spi));
}

static inline void _cc2520_print_exceptions(struct cc2520_context *cc2520)
{
	u8_t flag = read_reg_excflag0(&cc2520->spi);

	SYS_LOG_DBG("EXCFLAG0:");

	if (flag & EXCFLAG0_RF_IDLE) {
		SYS_LOG_BACKEND_FN("RF_IDLE ");
	}

	if (flag & EXCFLAG0_TX_FRM_DONE) {
		SYS_LOG_BACKEND_FN("TX_FRM_DONE ");
	}

	if (flag & EXCFLAG0_TX_ACK_DONE) {
		SYS_LOG_BACKEND_FN("TX_ACK_DONE ");
	}

	if (flag & EXCFLAG0_TX_UNDERFLOW) {
		SYS_LOG_BACKEND_FN("TX_UNDERFLOW ");
	}

	if (flag & EXCFLAG0_TX_OVERFLOW) {
		SYS_LOG_BACKEND_FN("TX_OVERFLOW ");
	}

	if (flag & EXCFLAG0_RX_UNDERFLOW) {
		SYS_LOG_BACKEND_FN("RX_UNDERFLOW ");
	}

	if (flag & EXCFLAG0_RX_OVERFLOW) {
		SYS_LOG_BACKEND_FN("RX_OVERFLOW ");
	}

	if (flag & EXCFLAG0_RXENABLE_ZERO) {
		SYS_LOG_BACKEND_FN("RXENABLE_ZERO");
	}

	SYS_LOG_BACKEND_FN("\n");

	flag = read_reg_excflag1(&cc2520->spi);

	SYS_LOG_DBG("EXCFLAG1:");

	if (flag & EXCFLAG1_RX_FRM_DONE) {
		SYS_LOG_BACKEND_FN("RX_FRM_DONE ");
	}

	if (flag & EXCFLAG1_RX_FRM_ACCEPTED) {
		SYS_LOG_BACKEND_FN("RX_FRM_ACCEPTED ");
	}

	if (flag & EXCFLAG1_SRC_MATCH_DONE) {
		SYS_LOG_BACKEND_FN("SRC_MATCH_DONE ");
	}

	if (flag & EXCFLAG1_SRC_MATCH_FOUND) {
		SYS_LOG_BACKEND_FN("SRC_MATCH_FOUND ");
	}

	if (flag & EXCFLAG1_FIFOP) {
		SYS_LOG_BACKEND_FN("FIFOP ");
	}

	if (flag & EXCFLAG1_SFD) {
		SYS_LOG_BACKEND_FN("SFD ");
	}

	if (flag & EXCFLAG1_DPU_DONE_L) {
		SYS_LOG_BACKEND_FN("DPU_DONE_L ");
	}

	if (flag & EXCFLAG1_DPU_DONE_H) {
		SYS_LOG_BACKEND_FN("DPU_DONE_H");
	}

	SYS_LOG_BACKEND_FN("\n");
}

static inline void _cc2520_print_errors(struct cc2520_context *cc2520)
{
	u8_t flag = read_reg_excflag2(&cc2520->spi);

	SYS_LOG_DBG("EXCFLAG2:");

	if (flag & EXCFLAG2_MEMADDR_ERROR) {
		SYS_LOG_BACKEND_FN("MEMADDR_ERROR ");
	}

	if (flag & EXCFLAG2_USAGE_ERROR) {
		SYS_LOG_BACKEND_FN("USAGE_ERROR ");
	}

	if (flag & EXCFLAG2_OPERAND_ERROR) {
		SYS_LOG_BACKEND_FN("OPERAND_ERROR ");
	}

	if (flag & EXCFLAG2_SPI_ERROR) {
		SYS_LOG_BACKEND_FN("SPI_ERROR ");
	}

	if (flag & EXCFLAG2_RF_NO_LOCK) {
		SYS_LOG_BACKEND_FN("RF_NO_LOCK ");
	}

	if (flag & EXCFLAG2_RX_FRM_ABORTED) {
		SYS_LOG_BACKEND_FN("RX_FRM_ABORTED ");
	}

	if (flag & EXCFLAG2_RFBUFMOV_TIMEOUT) {
		SYS_LOG_BACKEND_FN("RFBUFMOV_TIMEOUT");
	}

	SYS_LOG_BACKEND_FN("\n");
}
#else
#define _cc2520_print_gpio_config(...)
#define _cc2520_print_exceptions(...)
#define _cc2520_print_errors(...)
#endif /* CONFIG_SYS_LOG_IEEE802154_DRIVER_LEVEL == 4 */


/*********************
 * Generic functions *
 ********************/
#define _usleep(usec) k_busy_wait(usec)

u8_t _cc2520_read_reg(struct cc2520_spi *spi,
			 bool freg, u8_t addr)
{
	u8_t len = freg ? 2 : 3;

	spi->cmd_buf[0] = freg ? CC2520_INS_REGRD | addr : CC2520_INS_MEMRD;
	spi->cmd_buf[1] = freg ? 0 : addr;
	spi->cmd_buf[2] = 0;

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, spi->cmd_buf, len,
			   spi->cmd_buf, len) == 0) {
		return spi->cmd_buf[len - 1];
	}

	return 0;
}

bool _cc2520_write_reg(struct cc2520_spi *spi, bool freg,
		       u8_t addr, u8_t value)
{
	u8_t len = freg ? 2 : 3;

	spi->cmd_buf[0] = freg ? CC2520_INS_REGWR | addr : CC2520_INS_MEMWR;
	spi->cmd_buf[1] = freg ? value : addr;
	spi->cmd_buf[2] = freg ? 0 : value;

	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, spi->cmd_buf, len) == 0);
}

bool _cc2520_write_ram(struct cc2520_spi *spi, u16_t addr,
		       u8_t *data_buf, u8_t len)
{
#ifndef CONFIG_IEEE802154_CC2520_CRYPTO
	u8_t *cmd_data = spi->cmd_buf;
#else
	u8_t cmd_data[128];
#endif

	cmd_data[0] = CC2520_INS_MEMWR | (addr >> 8);
	cmd_data[1] = addr;

	memcpy(&cmd_data[2], data_buf, len);

	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, cmd_data, len + 2) == 0);
}

static u8_t _cc2520_status(struct cc2520_spi *spi)
{
	spi->cmd_buf[0] = CC2520_INS_SNOP;

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, spi->cmd_buf, 1,
			   spi->cmd_buf, 1) == 0) {
		return spi->cmd_buf[0];
	}

	return 0;
}

static bool verify_osc_stabilization(struct cc2520_context *cc2520)
{
	u8_t timeout = 100;
	u8_t status;

	do {
		status = _cc2520_status(&cc2520->spi);
		_usleep(1);
		timeout--;
	} while (!(status & CC2520_STATUS_XOSC_STABLE_N_RUNNING) && timeout);

	return !!(status & CC2520_STATUS_XOSC_STABLE_N_RUNNING);
}


static inline u8_t *get_mac(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

#if defined(CONFIG_IEEE802154_CC2520_RANDOM_MAC)
	u32_t *ptr = (u32_t *)(cc2520->mac_addr + 4);

	UNALIGNED_PUT(sys_rand32_get(), ptr);

	cc2520->mac_addr[7] = (cc2520->mac_addr[7] & ~0x01) | 0x02;
#else
	cc2520->mac_addr[4] = CONFIG_IEEE802154_CC2520_MAC4;
	cc2520->mac_addr[5] = CONFIG_IEEE802154_CC2520_MAC5;
	cc2520->mac_addr[6] = CONFIG_IEEE802154_CC2520_MAC6;
	cc2520->mac_addr[7] = CONFIG_IEEE802154_CC2520_MAC7;
#endif

	cc2520->mac_addr[0] = 0x00;
	cc2520->mac_addr[1] = 0x12;
	cc2520->mac_addr[2] = 0x4b;
	cc2520->mac_addr[3] = 0x00;

	return cc2520->mac_addr;
}

/******************
 * GPIO functions *
 *****************/
static inline void set_reset(struct device *dev, u32_t value)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	gpio_pin_write(cc2520->gpios[CC2520_GPIO_IDX_RESET].dev,
		       cc2520->gpios[CC2520_GPIO_IDX_RESET].pin, value);
}

static inline void set_vreg_en(struct device *dev, u32_t value)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	gpio_pin_write(cc2520->gpios[CC2520_GPIO_IDX_VREG_EN].dev,
		       cc2520->gpios[CC2520_GPIO_IDX_VREG_EN].pin, value);
}

static inline u32_t get_fifo(struct cc2520_context *cc2520)
{
	u32_t pin_value;

	gpio_pin_read(cc2520->gpios[CC2520_GPIO_IDX_FIFO].dev,
		      cc2520->gpios[CC2520_GPIO_IDX_FIFO].pin, &pin_value);

	return pin_value;
}

static inline u32_t get_fifop(struct cc2520_context *cc2520)
{
	u32_t pin_value;

	gpio_pin_read(cc2520->gpios[CC2520_GPIO_IDX_FIFOP].dev,
		      cc2520->gpios[CC2520_GPIO_IDX_FIFOP].pin, &pin_value);

	return pin_value;
}

static inline u32_t get_cca(struct cc2520_context *cc2520)
{
	u32_t pin_value;

	gpio_pin_read(cc2520->gpios[CC2520_GPIO_IDX_CCA].dev,
		      cc2520->gpios[CC2520_GPIO_IDX_CCA].pin, &pin_value);

	return pin_value;
}

static inline void sfd_int_handler(struct device *port,
				   struct gpio_callback *cb, u32_t pins)
{
	struct cc2520_context *cc2520 =
		CONTAINER_OF(cb, struct cc2520_context, sfd_cb);

	if (atomic_get(&cc2520->tx) == 1) {
		atomic_set(&cc2520->tx, 0);
		k_sem_give(&cc2520->tx_sync);
	}
}

static inline void fifop_int_handler(struct device *port,
				     struct gpio_callback *cb, u32_t pins)
{
	struct cc2520_context *cc2520 =
		CONTAINER_OF(cb, struct cc2520_context, fifop_cb);

	/* Note: Errata document - 1.2 */
	if (!get_fifop(cc2520) && !get_fifop(cc2520)) {
		return;
	}

	if (!get_fifo(cc2520)) {
		cc2520->overflow = true;
	}

	k_sem_give(&cc2520->rx_lock);
}

static void enable_fifop_interrupt(struct cc2520_context *cc2520,
				   bool enable)
{
	if (enable) {
		gpio_pin_enable_callback(
			cc2520->gpios[CC2520_GPIO_IDX_FIFOP].dev,
			cc2520->gpios[CC2520_GPIO_IDX_FIFOP].pin);
	} else {
		gpio_pin_disable_callback(
			cc2520->gpios[CC2520_GPIO_IDX_FIFOP].dev,
			cc2520->gpios[CC2520_GPIO_IDX_FIFOP].pin);
	}
}

static void enable_sfd_interrupt(struct cc2520_context *cc2520,
				 bool enable)
{
	if (enable) {
		gpio_pin_enable_callback(
			cc2520->gpios[CC2520_GPIO_IDX_SFD].dev,
			cc2520->gpios[CC2520_GPIO_IDX_SFD].pin);
	} else {
		gpio_pin_disable_callback(
			cc2520->gpios[CC2520_GPIO_IDX_SFD].dev,
			cc2520->gpios[CC2520_GPIO_IDX_SFD].pin);
	}
}

static inline void setup_gpio_callbacks(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	gpio_init_callback(&cc2520->sfd_cb, sfd_int_handler,
			   BIT(cc2520->gpios[CC2520_GPIO_IDX_SFD].pin));
	gpio_add_callback(cc2520->gpios[CC2520_GPIO_IDX_SFD].dev,
			  &cc2520->sfd_cb);

	gpio_init_callback(&cc2520->fifop_cb, fifop_int_handler,
			   BIT(cc2520->gpios[CC2520_GPIO_IDX_FIFOP].pin));
	gpio_add_callback(cc2520->gpios[CC2520_GPIO_IDX_FIFOP].dev,
			  &cc2520->fifop_cb);
}


/****************
 * TX functions *
 ***************/
static inline bool write_txfifo_length(struct cc2520_spi *spi,
				       u8_t len)
{
	spi->cmd_buf[0] = CC2520_INS_TXBUF;
	spi->cmd_buf[1] = len + CC2520_FCS_LENGTH;

	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, spi->cmd_buf, 2) == 0);
}

static inline bool write_txfifo_content(struct cc2520_spi *spi,
					u8_t *frame, u8_t len)
{
	u8_t cmd[128];

	cmd[0] = CC2520_INS_TXBUF;
	memcpy(&cmd[1], frame, len);

	spi_slave_select(spi->dev, spi->slave);

	return (spi_write(spi->dev, cmd, len + 1) == 0);
}

static inline bool verify_txfifo_status(struct cc2520_context *cc2520,
					u8_t len)
{
	if (read_reg_txfifocnt(&cc2520->spi) < len ||
	    (read_reg_excflag0(&cc2520->spi) & EXCFLAG0_TX_UNDERFLOW)) {
		return false;
	}

	return true;
}

static inline bool verify_tx_done(struct cc2520_context *cc2520)
{
	u8_t timeout = 10;
	u8_t status;

	do {
		_usleep(1);
		timeout--;
		status = read_reg_excflag0(&cc2520->spi);
	} while (!(status & EXCFLAG0_TX_FRM_DONE) && timeout);

	return !!(status & EXCFLAG0_TX_FRM_DONE);
}

/****************
 * RX functions *
 ***************/

static inline void flush_rxfifo(struct cc2520_context *cc2520)
{
	/* Note: Errata document - 1.1 */
	enable_fifop_interrupt(cc2520, false);

	instruct_sflushrx(&cc2520->spi);
	instruct_sflushrx(&cc2520->spi);

	enable_fifop_interrupt(cc2520, true);

	write_reg_excflag0(&cc2520->spi, EXCFLAG0_RESET_RX_FLAGS);
}

static inline u8_t read_rxfifo_length(struct cc2520_spi *spi)
{
	spi->cmd_buf[0] = CC2520_INS_RXBUF;
	spi->cmd_buf[1] = 0;

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, spi->cmd_buf, 2,
			   spi->cmd_buf, 2) == 0) {
		return spi->cmd_buf[1];
	}

	return 0;
}

static inline bool read_rxfifo_content(struct cc2520_spi *spi,
				       struct net_buf *frag, u8_t len)
{
	u8_t data[128+1];

	data[0] = CC2520_INS_RXBUF;
	memset(&data[1], 0, len);

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, data, len+1, data, len+1) != 0) {
		return false;
	}

	if (read_reg_excflag0(spi) & EXCFLAG0_RX_UNDERFLOW) {
		SYS_LOG_ERR("RX underflow!");
		return false;
	}

	memcpy(frag->data, &data[1], len);
	net_buf_add(frag, len);

	return true;
}

static inline bool verify_crc(struct cc2520_context *cc2520,
			      struct net_pkt *pkt)
{
	cc2520->spi.cmd_buf[0] = CC2520_INS_RXBUF;
	cc2520->spi.cmd_buf[1] = 0;
	cc2520->spi.cmd_buf[2] = 0;

	spi_slave_select(cc2520->spi.dev, cc2520->spi.slave);

	if (spi_transceive(cc2520->spi.dev, cc2520->spi.cmd_buf, 3,
			   cc2520->spi.cmd_buf, 3) != 0) {
		return false;
	}

	if (!(cc2520->spi.cmd_buf[2] & CC2520_FCS_CRC_OK)) {
		return false;
	}

	net_pkt_set_ieee802154_rssi(pkt, cc2520->spi.cmd_buf[1]);

	/**
	 * CC2520 does not provide an LQI but a correlation factor.
	 * See Section 20.6
	 * Such calculation can be loosely used to transform it to lqi:
	 * corr <= 50 ? lqi = 0
	 * or:
	 * corr >= 110 ? lqi = 255
	 * else:
	 * lqi = (lqi - 50) * 4
	 */
	cc2520->lqi = cc2520->spi.cmd_buf[2] & CC2520_FCS_CORRELATION;
	if (cc2520->lqi <= 50) {
		cc2520->lqi = 0;
	} else if (cc2520->lqi >= 110) {
		cc2520->lqi = 255;
	} else {
		cc2520->lqi = (cc2520->lqi - 50) << 2;
	}

	return true;
}

static inline bool verify_rxfifo_validity(struct cc2520_spi *spi,
					  u8_t pkt_len)
{
	if (pkt_len < 2 || read_reg_rxfifocnt(spi) != pkt_len) {
		return false;
	}

	return true;
}

static void cc2520_rx(int arg)
{
	struct device *dev = INT_TO_POINTER(arg);
	struct cc2520_context *cc2520 = dev->driver_data;
	struct net_buf *pkt_frag = NULL;
	struct net_pkt *pkt;
	u8_t pkt_len;

	while (1) {
		pkt = NULL;

		k_sem_take(&cc2520->rx_lock, K_FOREVER);

		if (cc2520->overflow) {
			SYS_LOG_ERR("RX overflow!");
			cc2520->overflow = false;

			goto flush;
		}

		pkt_len = read_rxfifo_length(&cc2520->spi) & 0x7f;
		if (!verify_rxfifo_validity(&cc2520->spi, pkt_len)) {
			SYS_LOG_ERR("Invalid content");
			goto flush;
		}

		pkt = net_pkt_get_reserve_rx(0, K_NO_WAIT);
		if (!pkt) {
			SYS_LOG_ERR("No pkt available");
			goto flush;
		}

#if defined(CONFIG_IEEE802154_CC2520_RAW)
		/**
		 * Reserve 1 byte for length
		 */
		net_pkt_set_ll_reserve(pkt, 1);
#endif
		pkt_frag = net_pkt_get_frag(pkt, K_NO_WAIT);
		if (!pkt_frag) {
			SYS_LOG_ERR("No pkt_frag available");
			goto flush;
		}

		net_pkt_frag_insert(pkt, pkt_frag);

#if defined(CONFIG_IEEE802154_CC2520_RAW)
		if (!read_rxfifo_content(&cc2520->spi, pkt_frag, pkt_len)) {
			SYS_LOG_ERR("No content read");
			goto flush;
		}

		if (!(pkt_frag->data[pkt_len - 1] & CC2520_FCS_CRC_OK)) {
			goto out;
		}

		cc2520->lqi = pkt_frag->data[pkt_len - 1] &
			CC2520_FCS_CORRELATION;
		if (cc2520->lqi <= 50) {
			cc2520->lqi = 0;
		} else if (cc2520->lqi >= 110) {
			cc2520->lqi = 255;
		} else {
			cc2520->lqi = (cc2520->lqi - 50) << 2;
		}
#else
		if (!read_rxfifo_content(&cc2520->spi, pkt_frag, pkt_len - 2)) {
			SYS_LOG_ERR("No content read");
			goto flush;
		}

		if (!verify_crc(cc2520, pkt)) {
			SYS_LOG_ERR("Bad packet CRC");
			goto out;
		}
#endif

		if (ieee802154_radio_handle_ack(cc2520->iface, pkt) == NET_OK) {
			SYS_LOG_DBG("ACK packet handled");
			goto out;
		}

#if defined(CONFIG_IEEE802154_CC2520_RAW)
		net_buf_add_u8(pkt_frag, cc2520->lqi);
#endif

		SYS_LOG_DBG("Caught a packet (%u) (LQI: %u)",
			    pkt_len, cc2520->lqi);

		if (net_recv_data(cc2520->iface, pkt) < 0) {
			SYS_LOG_DBG("Packet dropped by NET stack");
			goto out;
		}

		net_analyze_stack("CC2520 Rx Fiber stack",
				K_THREAD_STACK_BUFFER(cc2520->cc2520_rx_stack),
				K_THREAD_STACK_SIZEOF(cc2520->cc2520_rx_stack));
		continue;
flush:
		_cc2520_print_exceptions(cc2520);
		_cc2520_print_errors(cc2520);
		flush_rxfifo(cc2520);
out:
		if (pkt) {
			net_pkt_unref(pkt);
		}
	}
}

/********************
 * Radio device API *
 *******************/
static int cc2520_cca(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	if (!get_cca(cc2520)) {
		SYS_LOG_DBG("Busy");
		return -EBUSY;
	}

	return 0;
}

static int cc2520_set_channel(struct device *dev, u16_t channel)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	SYS_LOG_DBG("%u", channel);

	if (channel < 11 || channel > 26) {
		return -EINVAL;
	}

	/* See chapter 16 */
	channel = 11 + 5 * (channel - 11);

	if (!write_reg_freqctrl(&cc2520->spi, FREQCTRL_FREQ(channel))) {
		SYS_LOG_ERR("Failed");
		return -EIO;
	}

	return 0;
}

static int cc2520_set_pan_id(struct device *dev, u16_t pan_id)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	SYS_LOG_DBG("0x%x", pan_id);

	pan_id = sys_le16_to_cpu(pan_id);

	if (!write_mem_pan_id(&cc2520->spi, (u8_t *) &pan_id)) {
		SYS_LOG_ERR("Failed");
		return -EIO;
	}

	return 0;
}

static int cc2520_set_short_addr(struct device *dev, u16_t short_addr)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	SYS_LOG_DBG("0x%x", short_addr);

	short_addr = sys_le16_to_cpu(short_addr);

	if (!write_mem_short_addr(&cc2520->spi, (u8_t *) &short_addr)) {
		SYS_LOG_ERR("Failed");
		return -EIO;
	}

	return 0;
}

static int cc2520_set_ieee_addr(struct device *dev, const u8_t *ieee_addr)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	if (!write_mem_ext_addr(&cc2520->spi, (void *)ieee_addr)) {
		SYS_LOG_ERR("Failed");
		return -EIO;
	}

	SYS_LOG_DBG("IEEE address %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
		    ieee_addr[7], ieee_addr[6], ieee_addr[5], ieee_addr[4],
		    ieee_addr[3], ieee_addr[2], ieee_addr[1], ieee_addr[0]);

	return 0;
}

static int cc2520_set_txpower(struct device *dev, s16_t dbm)
{
	struct cc2520_context *cc2520 = dev->driver_data;
	u8_t pwr;

	SYS_LOG_DBG("%d", dbm);

	/* See chapter 19 part 8 */
	switch (dbm) {
	case 5:
		pwr = 0xF7;
		break;
	case 3:
		pwr = 0xF2;
		break;
	case 2:
		pwr = 0xAB;
		break;
	case 1:
		pwr = 0x13;
		break;
	case 0:
		pwr = 0x32;
		break;
	case -2:
		pwr = 0x81;
		break;
	case -4:
		pwr = 0x88;
		break;
	case -7:
		pwr = 0x2C;
		break;
	case -18:
		pwr = 0x03;
		break;
	default:
		goto error;
	}

	if (!write_reg_txpower(&cc2520->spi, pwr)) {
		goto error;
	}

	return 0;
error:
	SYS_LOG_DBG("Failed");
	return -EIO;
}

static int cc2520_tx(struct device *dev,
		     struct net_pkt *pkt,
		     struct net_buf *frag)
{
	u8_t *frame = frag->data - net_pkt_ll_reserve(pkt);
	u8_t len = net_pkt_ll_reserve(pkt) + frag->len;
	struct cc2520_context *cc2520 = dev->driver_data;
	u8_t retry = 2;
	bool status;

	SYS_LOG_DBG("%p (%u)", frag, len);

	if (!write_reg_excflag0(&cc2520->spi, EXCFLAG0_RESET_TX_FLAGS) ||
	    !write_txfifo_length(&cc2520->spi, len) ||
	    !write_txfifo_content(&cc2520->spi, frame, len)) {
		SYS_LOG_ERR("Cannot feed in TX fifo");
		goto error;
	}

	if (!verify_txfifo_status(cc2520, len)) {
		SYS_LOG_ERR("Did not write properly into TX FIFO");
		goto error;
	}

#ifdef CONFIG_IEEE802154_CC2520_CRYPTO
	k_sem_take(&cc2520->access_lock, K_FOREVER);
#endif

	/* 1 retry is allowed here */
	do {
		atomic_set(&cc2520->tx, 1);
		k_sem_init(&cc2520->tx_sync, 0, UINT_MAX);

		if (!instruct_stxoncca(&cc2520->spi)) {
			SYS_LOG_ERR("Cannot start transmission");
			goto error;
		}

		k_sem_take(&cc2520->tx_sync, 10);

		retry--;
		status = verify_tx_done(cc2520);
	} while (!status && retry);

#ifdef CONFIG_IEEE802154_CC2520_CRYPTO
	k_sem_give(&cc2520->access_lock);
#endif

	if (status) {
		return 0;
	}

error:
#ifdef CONFIG_IEEE802154_CC2520_CRYPTO
	k_sem_give(&cc2520->access_lock);
#endif
	SYS_LOG_ERR("No TX_FRM_DONE");
	_cc2520_print_exceptions(cc2520);
	_cc2520_print_errors(cc2520);

	atomic_set(&cc2520->tx, 0);
	instruct_sflushtx(&cc2520->spi);

	return -EIO;
}

static int cc2520_start(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	SYS_LOG_DBG("");

	if (!instruct_sxoscon(&cc2520->spi) ||
	    !instruct_srxon(&cc2520->spi) ||
	    !verify_osc_stabilization(cc2520)) {
		SYS_LOG_ERR("Error starting CC2520");
		return -EIO;
	}

	flush_rxfifo(cc2520);

	enable_fifop_interrupt(cc2520, true);
	enable_sfd_interrupt(cc2520, true);

	return 0;
}

static int cc2520_stop(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	SYS_LOG_DBG("");

	flush_rxfifo(cc2520);

	enable_fifop_interrupt(cc2520, false);
	enable_sfd_interrupt(cc2520, false);

	if (!instruct_srfoff(&cc2520->spi) ||
	    !instruct_sxoscoff(&cc2520->spi)) {
		SYS_LOG_ERR("Error stopping CC2520");
		return -EIO;
	}

	return 0;
}

static u8_t cc2520_get_lqi(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	return cc2520->lqi;
}

/******************
 * Initialization *
 *****************/
static int power_on_and_setup(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	/* Switching to LPM2 mode */
	set_reset(dev, 0);
	_usleep(150);

	set_vreg_en(dev, 0);
	_usleep(250);

	/* Then to ACTIVE mode */
	set_vreg_en(dev, 1);
	_usleep(250);

	set_reset(dev, 1);
	_usleep(150);

	if (!verify_osc_stabilization(cc2520)) {
		return -EIO;
	}

	/* Default settings to always write (see chapter 28 part 1) */
	if (!write_reg_txpower(&cc2520->spi, CC2520_TXPOWER_DEFAULT) ||
	    !write_reg_ccactrl0(&cc2520->spi, CC2520_CCACTRL0_DEFAULT) ||
	    !write_reg_mdmctrl0(&cc2520->spi, CC2520_MDMCTRL0_DEFAULT) ||
	    !write_reg_mdmctrl1(&cc2520->spi, CC2520_MDMCTRL1_DEFAULT) ||
	    !write_reg_rxctrl(&cc2520->spi, CC2520_RXCTRL_DEFAULT) ||
	    !write_reg_fsctrl(&cc2520->spi, CC2520_FSCTRL_DEFAULT) ||
	    !write_reg_fscal1(&cc2520->spi, CC2520_FSCAL1_DEFAULT) ||
	    !write_reg_agcctrl1(&cc2520->spi, CC2520_AGCCTRL1_DEFAULT) ||
	    !write_reg_adctest0(&cc2520->spi, CC2520_ADCTEST0_DEFAULT) ||
	    !write_reg_adctest1(&cc2520->spi, CC2520_ADCTEST1_DEFAULT) ||
	    !write_reg_adctest2(&cc2520->spi, CC2520_ADCTEST2_DEFAULT)) {
		return -EIO;
	}

	/* EXTCLOCK0: Disabling external clock
	 * FRMCTRL0: AUTOACK and AUTOCRC enabled
	 * FRMCTRL1: SET_RXENMASK_ON_TX and IGNORE_TX_UNDERF
	 * FRMFILT0: Frame filtering (setting CC2520_FRAME_FILTERING)
	 * FIFOPCTRL: Set TX threshold (setting CC2520_TX_THRESHOLD)
	 */
	if (!write_reg_extclock(&cc2520->spi, 0) ||
	    !write_reg_frmctrl0(&cc2520->spi, CC2520_AUTOMATISM) ||
	    !write_reg_frmctrl1(&cc2520->spi, FRMCTRL1_IGNORE_TX_UNDERF |
				FRMCTRL1_SET_RXENMASK_ON_TX) ||
	    !write_reg_frmfilt0(&cc2520->spi, FRMFILT0_FRAME_FILTER_EN |
				FRMFILT0_MAX_FRAME_VERSION(3)) ||
	    !write_reg_frmfilt1(&cc2520->spi, FRMFILT1_ACCEPT_ALL) ||
	    !write_reg_srcmatch(&cc2520->spi, SRCMATCH_DEFAULTS) ||
	    !write_reg_fifopctrl(&cc2520->spi,
				 FIFOPCTRL_FIFOP_THR(CC2520_TX_THRESHOLD))) {
		return -EIO;
	}

	/* Cleaning up TX fifo */
	instruct_sflushtx(&cc2520->spi);

	setup_gpio_callbacks(dev);

	_cc2520_print_gpio_config(dev);

	return 0;
}

static inline int configure_spi(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;
	struct spi_config spi_conf = {
		.config = SPI_WORD(8),
		.max_sys_freq = CONFIG_IEEE802154_CC2520_SPI_FREQ,
	};

	cc2520->spi.dev = device_get_binding(
			CONFIG_IEEE802154_CC2520_SPI_DRV_NAME);
	if (!cc2520->spi.dev) {
		SYS_LOG_ERR("Unable to get SPI device");
		return -ENODEV;
	}

	cc2520->spi.slave = CONFIG_IEEE802154_CC2520_SPI_SLAVE;

	if (spi_configure(cc2520->spi.dev, &spi_conf) != 0 ||
	    spi_slave_select(cc2520->spi.dev,
			     cc2520->spi.slave) != 0) {
		cc2520->spi.dev = NULL;
		return -EIO;
	}

	return 0;
}

static int cc2520_init(struct device *dev)
{
	struct cc2520_context *cc2520 = dev->driver_data;

	atomic_set(&cc2520->tx, 0);
	k_sem_init(&cc2520->rx_lock, 0, UINT_MAX);

#ifdef CONFIG_IEEE802154_CC2520_CRYPTO
	k_sem_init(&cc2520->access_lock, 1, 1);
#endif

	cc2520->gpios = cc2520_configure_gpios();
	if (!cc2520->gpios) {
		SYS_LOG_ERR("Configuring GPIOS failed");
		return -EIO;
	}

	if (configure_spi(dev) != 0) {
		SYS_LOG_ERR("Configuring SPI failed");
		return -EIO;
	}

	SYS_LOG_DBG("GPIO and SPI configured");

	if (power_on_and_setup(dev) != 0) {
		SYS_LOG_ERR("Configuring CC2520 failed");
		return -EIO;
	}

	k_thread_create(&cc2520->cc2520_rx_thread, cc2520->cc2520_rx_stack,
			CONFIG_IEEE802154_CC2520_RX_STACK_SIZE,
			(k_thread_entry_t)cc2520_rx,
			dev, NULL, NULL, K_PRIO_COOP(2), 0, 0);

	SYS_LOG_INF("CC2520 initialized");

	return 0;
}

static void cc2520_iface_init(struct net_if *iface)
{
	struct device *dev = net_if_get_device(iface);
	struct cc2520_context *cc2520 = dev->driver_data;
	u8_t *mac = get_mac(dev);

	SYS_LOG_DBG("");

	net_if_set_link_addr(iface, mac, 8, NET_LINK_IEEE802154);

	cc2520->iface = iface;

	ieee802154_init(iface);
}

static struct cc2520_context cc2520_context_data;

static struct ieee802154_radio_api cc2520_radio_api = {
	.iface_api.init	= cc2520_iface_init,
	.iface_api.send	= ieee802154_radio_send,

	.cca		= cc2520_cca,
	.set_channel	= cc2520_set_channel,
	.set_pan_id	= cc2520_set_pan_id,
	.set_short_addr	= cc2520_set_short_addr,
	.set_ieee_addr	= cc2520_set_ieee_addr,
	.set_txpower	= cc2520_set_txpower,
	.start		= cc2520_start,
	.stop		= cc2520_stop,
	.tx		= cc2520_tx,
	.get_lqi	= cc2520_get_lqi,
};

#if defined(CONFIG_IEEE802154_CC2520_RAW)
DEVICE_AND_API_INIT(cc2520, CONFIG_IEEE802154_CC2520_DRV_NAME,
		    cc2520_init, &cc2520_context_data, NULL,
		    POST_KERNEL, CONFIG_IEEE802154_CC2520_INIT_PRIO,
		    &cc2520_radio_api);
#else
NET_DEVICE_INIT(cc2520, CONFIG_IEEE802154_CC2520_DRV_NAME,
		cc2520_init, &cc2520_context_data, NULL,
		CONFIG_IEEE802154_CC2520_INIT_PRIO,
		&cc2520_radio_api, IEEE802154_L2,
		NET_L2_GET_CTX_TYPE(IEEE802154_L2), 125);

NET_STACK_INFO_ADDR(RX, cc2520,
		    CONFIG_IEEE802154_CC2520_RX_STACK_SIZE,
		    CONFIG_IEEE802154_CC2520_RX_STACK_SIZE,
		    cc2520_context_data.cc2520_rx_stack,
		    0);
#endif


#ifdef CONFIG_IEEE802154_CC2520_CRYPTO

static bool _cc2520_read_ram(struct cc2520_spi *spi, u16_t addr,
			     u8_t *data_buf, u8_t len)
{
	u8_t cmd_buf[128];

	cmd_buf[0] = CC2520_INS_MEMRD | (addr >> 8);
	cmd_buf[1] = addr;

	spi_slave_select(spi->dev, spi->slave);

	if (spi_transceive(spi->dev, cmd_buf, len + 2,
			   cmd_buf, len + 2) != 0) {
		return false;
	}

	memcpy(data_buf, &cmd_buf[2], len);

	return true;
}

static inline bool instruct_ccm(struct cc2520_context *cc2520,
				u8_t key_addr,
				u8_t auth_crypt,
				u8_t nonce_addr,
				u16_t input_addr,
				u16_t output_addr,
				u8_t in_len,
				u8_t m)
{
	u8_t cmd[9];
	int ret;

	SYS_LOG_DBG("CCM(P={01} K={%02x} C={%02x} N={%02x}"
		    " A={%03x} E={%03x} F{%02x} M={%02x})",
		    key_addr, auth_crypt, nonce_addr,
		    input_addr, output_addr, in_len, m);

	cmd[0] = CC2520_INS_CCM | 1;
	cmd[1] = key_addr;
	cmd[2] = (auth_crypt & 0x7f);
	cmd[3] = nonce_addr;
	cmd[4] = (u8_t)(((input_addr & 0x0f00) >> 4) |
			   ((output_addr & 0x0f00) >> 8));
	cmd[5] = (u8_t)(input_addr & 0x00ff);
	cmd[6] = (u8_t)(output_addr & 0x00ff);
	cmd[7] = (in_len & 0x7f);
	cmd[8] = (m & 0x03);

	k_sem_take(&cc2520->access_lock, K_FOREVER);

	ret = spi_write(cc2520->spi.dev, cmd, 9);

	k_sem_give(&cc2520->access_lock);

	if (ret) {
		SYS_LOG_ERR("CCM Failed");
		return false;
	}

	return true;
}

static inline bool instruct_uccm(struct cc2520_context *cc2520,
				 u8_t key_addr,
				 u8_t auth_crypt,
				 u8_t nonce_addr,
				 u16_t input_addr,
				 u16_t output_addr,
				 u8_t in_len,
				 u8_t m)
{
	u8_t cmd[9];
	int ret;

	SYS_LOG_DBG("UCCM(P={01} K={%02x} C={%02x} N={%02x}"
		    " A={%03x} E={%03x} F{%02x} M={%02x})",
		    key_addr, auth_crypt, nonce_addr,
		    input_addr, output_addr, in_len, m);

	cmd[0] = CC2520_INS_UCCM | 1;
	cmd[1] = key_addr;
	cmd[2] = (auth_crypt & 0x7f);
	cmd[3] = nonce_addr;
	cmd[4] = (u8_t)(((input_addr & 0x0f00) >> 4) |
			   ((output_addr & 0x0f00) >> 8));
	cmd[5] = (u8_t)(input_addr & 0x00ff);
	cmd[6] = (u8_t)(output_addr & 0x00ff);
	cmd[7] = (in_len & 0x7f);
	cmd[8] = (m & 0x03);

	k_sem_take(&cc2520->access_lock, K_FOREVER);

	ret = spi_write(cc2520->spi.dev, cmd, 9);

	k_sem_give(&cc2520->access_lock);

	if (ret) {
		SYS_LOG_ERR("UCCM Failed");
		return false;
	}

	return true;
}

static inline void generate_nonce(u8_t *ccm_nonce, u8_t *nonce,
				  struct cipher_aead_pkt *apkt, u8_t m)
{
	nonce[0] = 0 | (apkt->ad_len ? 0x40 : 0) | (m << 3) | 1;

	memcpy(&nonce[1], ccm_nonce, 13);

	nonce[14] = (u8_t)(apkt->pkt->in_len >> 8);
	nonce[15] = (u8_t)(apkt->pkt->in_len);

	/* See section 26.8.1 */
	sys_mem_swap(nonce, 16);
}

static int insert_crypto_parameters(struct cipher_ctx *ctx,
				    struct cipher_aead_pkt *apkt,
				    u8_t *ccm_nonce, u8_t *auth_crypt)
{
	struct cc2520_context *cc2520 = ctx->device->driver_data;
	u8_t data[128];
	u8_t *in_buf;
	u8_t in_len;
	u8_t m = 0;

	if (!apkt->pkt->out_buf || !apkt->pkt->out_buf_max) {
		SYS_LOG_ERR("Out buffer needs to be set");
		return -EINVAL;
	}

	if (!ctx->key.bit_stream || !ctx->keylen) {
		SYS_LOG_ERR("No key installed");
		return -EINVAL;
	}

	if (!(ctx->flags & CAP_INPLACE_OPS)) {
		SYS_LOG_ERR("It supports only in-place operation");
		return -EINVAL;
	}

	if (!apkt->ad || !apkt->ad_len) {
		SYS_LOG_ERR("CCM needs associated data");
		return -EINVAL;
	}

	if (apkt->pkt->in_buf && apkt->pkt->in_buf - apkt->ad_len != apkt->ad) {
		SYS_LOG_ERR("In-place needs ad and input in same memory");
		return -EINVAL;
	}

	if (!apkt->pkt->in_buf) {
		if (!ctx->mode_params.ccm_info.tag_len) {
			SYS_LOG_ERR("Auth only needs a tag length");
			return -EINVAL;
		}

		in_buf = apkt->ad;
		in_len = apkt->ad_len;

		*auth_crypt = 0;
	} else {
		in_buf = data;

		memcpy(in_buf, apkt->ad, apkt->ad_len);
		memcpy(in_buf + apkt->ad_len,
		       apkt->pkt->in_buf, apkt->pkt->in_len);
		in_len = apkt->ad_len + apkt->pkt->in_len;

		*auth_crypt = !apkt->tag ? apkt->pkt->in_len :
			apkt->pkt->in_len - ctx->mode_params.ccm_info.tag_len;
	}

	if (ctx->mode_params.ccm_info.tag_len) {
		if ((ctx->mode_params.ccm_info.tag_len >> 2) > 3) {
			m = 3;
		} else {
			m = ctx->mode_params.ccm_info.tag_len >> 2;
		}
	}

	/* Writing the frame in RAM */
	if (!_cc2520_write_ram(&cc2520->spi, CC2520_MEM_DATA, in_buf, in_len)) {
		SYS_LOG_ERR("Cannot write the frame in RAM");
		return -EIO;
	}

	/* See section 26.8.1 */
	sys_memcpy_swap(data, ctx->key.bit_stream, ctx->keylen);

	/* Writing the key in RAM */
	if (!_cc2520_write_ram(&cc2520->spi, CC2520_MEM_KEY, data, 16)) {
		SYS_LOG_ERR("Cannot write the key in RAM");
		return -EIO;
	}

	generate_nonce(ccm_nonce, data, apkt, m);

	/* Writing the nonce in RAM */
	if (!_cc2520_write_ram(&cc2520->spi, CC2520_MEM_NONCE, data, 16)) {
		SYS_LOG_ERR("Cannot write the nonce in RAM");
		return -EIO;
	}

	return m;
}

static int _cc2520_crypto_ccm(struct cipher_ctx *ctx,
			      struct cipher_aead_pkt *apkt,
			      u8_t *ccm_nonce)
{
	struct cc2520_context *cc2520 = ctx->device->driver_data;
	u8_t auth_crypt;
	int m;

	if (!apkt || !apkt->pkt) {
		SYS_LOG_ERR("Invalid crypto packet to operate with");
		return -EINVAL;
	}

	if (apkt->tag) {
		SYS_LOG_ERR("CCM encryption does not take a tag");
		return -EINVAL;
	}

	m = insert_crypto_parameters(ctx, apkt, ccm_nonce, &auth_crypt);
	if (m < 0) {
		SYS_LOG_ERR("Inserting crypto parameters failed");
		return m;
	}

	apkt->pkt->out_len = apkt->pkt->in_len + apkt->ad_len +
		(m ? ctx->mode_params.ccm_info.tag_len : 0);

	if (apkt->pkt->out_len > apkt->pkt->out_buf_max) {
		SYS_LOG_ERR("Result will not fit into out buffer %u vs %u",
			    apkt->pkt->out_len, apkt->pkt->out_buf_max);
		return -ENOBUFS;
	}

	if (!instruct_ccm(cc2520, CC2520_MEM_KEY >> 4, auth_crypt,
			  CC2520_MEM_NONCE >> 4, CC2520_MEM_DATA,
			  0x000, apkt->ad_len, m) ||
	    !_cc2520_read_ram(&cc2520->spi, CC2520_MEM_DATA,
			      apkt->pkt->out_buf, apkt->pkt->out_len)) {
		SYS_LOG_ERR("CCM or reading result from RAM failed");
		return -EIO;
	}

	apkt->tag = apkt->pkt->out_buf + apkt->pkt->in_len;

	return 0;
}

static int _cc2520_crypto_uccm(struct cipher_ctx *ctx,
			       struct cipher_aead_pkt *apkt,
			       u8_t *ccm_nonce)
{
	struct cc2520_context *cc2520 = ctx->device->driver_data;
	u8_t auth_crypt;
	int m;

	if (!apkt || !apkt->pkt) {
		SYS_LOG_ERR("Invalid crypto packet to operate with");
		return -EINVAL;
	}

	if (ctx->mode_params.ccm_info.tag_len && !apkt->tag) {
		SYS_LOG_ERR("In case of MIC you need to provide a tag");
		return -EINVAL;
	}

	m = insert_crypto_parameters(ctx, apkt, ccm_nonce, &auth_crypt);
	if (m < 0) {
		return m;
	}

	apkt->pkt->out_len = apkt->pkt->in_len + apkt->ad_len;

	if (!instruct_uccm(cc2520, CC2520_MEM_KEY >> 4, auth_crypt,
			   CC2520_MEM_NONCE >> 4, CC2520_MEM_DATA,
			   0x000, apkt->ad_len, m) ||
	    !_cc2520_read_ram(&cc2520->spi, CC2520_MEM_DATA,
			      apkt->pkt->out_buf, apkt->pkt->out_len)) {
		SYS_LOG_ERR("UCCM or reading result from RAM failed");
		return -EIO;
	}

	if (m && (!(read_reg_dpustat(&cc2520->spi) & DPUSTAT_AUTHSTAT_H))) {
		SYS_LOG_ERR("Authentication of the frame failed");
		return -EBADMSG;
	}

	return 0;
}

static int cc2520_crypto_hw_caps(struct device *dev)
{
	return CAP_RAW_KEY | CAP_INPLACE_OPS | CAP_SYNC_OPS;
}

static int cc2520_crypto_begin_session(struct device *dev,
				       struct cipher_ctx *ctx,
				       enum cipher_algo algo,
				       enum cipher_mode mode,
				       enum cipher_op op_type)
{
	if (algo != CRYPTO_CIPHER_ALGO_AES || mode != CRYPTO_CIPHER_MODE_CCM) {
		SYS_LOG_ERR("Wrong algo (%u) or mode (%u)", algo, mode);
		return -EINVAL;
	}

	if (ctx->mode_params.ccm_info.nonce_len != 13) {
		SYS_LOG_ERR("Nonce length erroneous (%u)",
			    ctx->mode_params.ccm_info.nonce_len);
		return -EINVAL;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		ctx->ops.ccm_crypt_hndlr = _cc2520_crypto_ccm;
	} else {
		ctx->ops.ccm_crypt_hndlr = _cc2520_crypto_uccm;
	}

	ctx->ops.cipher_mode = mode;
	ctx->device = dev;

	return 0;
}

static int cc2520_crypto_free_session(struct device *dev,
				      struct cipher_ctx *ctx)
{
	ARG_UNUSED(dev);

	ctx->ops.ccm_crypt_hndlr = NULL;
	ctx->device = NULL;

	return 0;
}

static int cc2520_crypto_init(struct device *dev)
{
	SYS_LOG_INF("CC2520 crypto part initialized");

	return 0;
}

struct crypto_driver_api cc2520_crypto_api = {
	.query_hw_caps			= cc2520_crypto_hw_caps,
	.begin_session			= cc2520_crypto_begin_session,
	.free_session			= cc2520_crypto_free_session,
	.crypto_async_callback_set	= NULL
};

DEVICE_AND_API_INIT(cc2520_crypto, CONFIG_IEEE802154_CC2520_CRYPTO_DRV_NAME,
		    cc2520_crypto_init, &cc2520_context_data, NULL,
		    POST_KERNEL, CONFIG_IEEE802154_CC2520_CRYPTO_INIT_PRIO,
		    &cc2520_crypto_api);

#endif /* CONFIG_IEEE802154_CC2520_CRYPTO */
