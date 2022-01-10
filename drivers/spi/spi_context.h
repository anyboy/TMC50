/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Private API for SPI drivers
 */

#ifndef __SPI_DRIVER_COMMON_H__
#define __SPI_DRIVER_COMMON_H__

#include <gpio.h>
#include <spi.h>

#ifdef __cplusplus
extern "C" {
#endif
/*
struct spi_context {
	struct spi_config *config;

	struct k_sem lock;
	struct k_sem sync;
	int sync_status;

#ifdef CONFIG_POLL
	struct k_poll_signal *signal;
	bool asynchronous;
#endif
	const struct spi_buf *current_tx;
	size_t tx_count;
	struct spi_buf *current_rx;
	size_t rx_count;

	u8_t *tx_buf;
	size_t tx_len;
	u8_t *rx_buf;
	size_t rx_len;
};
*/
#define SPI_CONTEXT_INIT_LOCK(_data, _ctx_name)				\
	._ctx_name.lock = _K_SEM_INITIALIZER(_data._ctx_name.lock, 0, 1)

#define SPI_CONTEXT_INIT_SYNC(_data, _ctx_name)				\
	._ctx_name.sync = _K_SEM_INITIALIZER(_data._ctx_name.sync, 0, 1)

static inline bool spi_context_configured(struct spi_context *ctx,
					  struct spi_config *config)
{
	return !!(ctx->config == config);
}

static inline void spi_context_lock(struct spi_context *ctx,
				    bool asynchronous,
				    struct k_poll_signal *signal)
{
	k_sem_take(&ctx->lock, K_FOREVER);

#ifdef CONFIG_POLL
	ctx->asynchronous = asynchronous;
	ctx->signal = signal;
#endif
}

static inline void spi_context_release(struct spi_context *ctx, int status)
{
	if (!status && (ctx->config->operation & SPI_LOCK_ON)) {
		return;
	}

#ifdef CONFIG_POLL
	if (!ctx->asynchronous || status) {
		k_sem_give(&ctx->lock);
	}
#else
	k_sem_give(&ctx->lock);
#endif
}

static inline void spi_context_unlock_unconditionally(struct spi_context *ctx)
{
	if (!k_sem_count_get(&ctx->lock)) {
		k_sem_give(&ctx->lock);
	}
}

static inline int spi_context_wait_for_completion(struct spi_context *ctx)
{
	int status = 0;
#ifdef CONFIG_POLL
	if (!ctx->asynchronous) {
		k_sem_take(&ctx->sync, K_FOREVER);
		status = ctx->sync_status;
	}
#else
	k_sem_take(&ctx->sync, K_FOREVER);
	status = ctx->sync_status;
#endif
	return status;
}

static inline void spi_context_complete(struct spi_context *ctx, int status)
{
#ifdef CONFIG_POLL
	if (!ctx->asynchronous) {
		ctx->sync_status = status;
		k_sem_give(&ctx->sync);
	} else {
		if (ctx->signal) {
			k_poll_signal(ctx->signal, status);
		}

		if (!(ctx->config->operation & SPI_LOCK_ON)) {
			k_sem_give(&ctx->lock);
		}
	}
#else
	ctx->sync_status = status;
	k_sem_give(&ctx->sync);
#endif
}

static inline void spi_context_cs_configure(struct spi_context *ctx)
{
	if (ctx->config->cs) {
		gpio_pin_configure(ctx->config->cs->gpio_dev,
				   ctx->config->cs->gpio_pin, GPIO_DIR_OUT);
		gpio_pin_write(ctx->config->cs->gpio_dev,
			       ctx->config->cs->gpio_pin, 1);
	}
}

static inline void spi_context_cs_control(struct spi_config *config, bool on)
{

	if (config->cs) {
		if (on) {
			gpio_pin_write(config->cs->gpio_dev,
				       config->cs->gpio_pin, 0);
			k_busy_wait(config->cs->delay);
		} else {
			k_busy_wait(config->cs->delay);
			gpio_pin_write(config->cs->gpio_dev,
				       config->cs->gpio_pin, 1);
		}
	}
}


static inline void spi_context_buffers_setup(struct spi_context *ctx,
					     const struct spi_buf *tx_bufs,
					     size_t tx_count,
					     struct spi_buf *rx_bufs,
					     size_t rx_count,
					     u8_t dfs)
{
	SYS_LOG_DBG("tx_bufs %p (%zu) - rx_bufs %p (%zu) - %u",
		    tx_bufs, tx_count, rx_bufs, rx_count, dfs);

	ctx->current_tx = tx_bufs;
	ctx->tx_count = tx_count;
	ctx->current_rx = rx_bufs;
	ctx->rx_count = rx_count;

	if (tx_bufs) {
		ctx->tx_buf = tx_bufs->buf;
		ctx->tx_len = tx_bufs->len / dfs;
	} else {
		ctx->tx_buf = NULL;
		ctx->tx_len = 0;
	}

	if (rx_bufs) {
		ctx->rx_buf = rx_bufs->buf;
		ctx->rx_len = rx_bufs->len / dfs;
	} else {
		ctx->rx_buf = NULL;
		ctx->rx_len = 0;
	}

	ctx->sync_status = 0;

	SYS_LOG_DBG("current_tx %p (%zu), current_rx %p (%zu),"
		    " tx buf/len %p/%zu, rx buf/len %p/%zu",
		    ctx->current_tx, ctx->tx_count,
		    ctx->current_rx, ctx->rx_count,
		    ctx->tx_buf, ctx->tx_len, ctx->rx_buf, ctx->rx_len);
}

static ALWAYS_INLINE
void spi_context_update_tx(struct spi_context *ctx, u8_t dfs, u32_t len)
{
	if (!ctx->tx_len) {
		return;
	}

	if (len > ctx->tx_len) {
		SYS_LOG_ERR("Update exceeds current buffer");
		return;
	}

	ctx->tx_len -= len;
	if (!ctx->tx_len) {
		ctx->current_tx++;
		ctx->tx_count--;

		if (ctx->tx_count) {
			ctx->tx_buf = ctx->current_tx->buf;
			ctx->tx_len = ctx->current_tx->len / dfs;
		} else {
			ctx->tx_buf = NULL;
		}
	} else if (ctx->tx_buf) {
		ctx->tx_buf += dfs * len;
	}

	SYS_LOG_DBG("tx buf/len %p/%zu", ctx->tx_buf, ctx->tx_len);
}

static ALWAYS_INLINE
bool spi_context_tx_on(struct spi_context *ctx)
{
	return !!(ctx->tx_buf || ctx->tx_len);
}

static ALWAYS_INLINE
void spi_context_update_rx(struct spi_context *ctx, u8_t dfs, u32_t len)
{
	if (!ctx->rx_len) {
		return;
	}

	if (len > ctx->rx_len) {
		SYS_LOG_ERR("Update exceeds current buffer");
		return;
	}

	ctx->rx_len -= len;
	if (!ctx->rx_len) {
		ctx->current_rx++;
		ctx->rx_count--;

		if (ctx->rx_count) {
			ctx->rx_buf = ctx->current_rx->buf;
			ctx->rx_len = ctx->current_rx->len / dfs;
		} else {
			ctx->rx_buf = NULL;
		}
	} else if (ctx->rx_buf) {
		ctx->rx_buf += dfs * len;
	}

	SYS_LOG_DBG("rx buf/len %p/%zu", ctx->rx_buf, ctx->rx_len);
}

static ALWAYS_INLINE
bool spi_context_rx_on(struct spi_context *ctx)
{
	return !!(ctx->rx_buf || ctx->rx_len);
}

static inline size_t spi_context_longest_current_buf(struct spi_context *ctx)
{
	if (!ctx->tx_len) {
		return ctx->rx_len;
	} else if (!ctx->rx_len) {
		return ctx->tx_len;
	} else if (ctx->tx_len < ctx->rx_len) {
		return ctx->tx_len;
	}

	return ctx->rx_len;
}

#ifdef __cplusplus
}
#endif

#endif /* __SPI_DRIVER_COMMON_H__ */
