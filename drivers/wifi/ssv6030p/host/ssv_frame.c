/*
*  (C) Copyright 2014-2016 Shenzhen South Silicon Valley microelectronics co.,limited
*
*  All Rights Reserved
*/
#include <net/buf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/ethernet.h>
#include <pbuf.h>
#include <ssv_frame.h>
#include "ssv_lib.h"

#define RESERVE_FRAG_FOR_WIFI	1

#if defined(RESERVE_FRAG_FOR_WIFI)
static K_MUTEX_DEFINE(reserve_frag_lock);
static struct net_buf *reserveBuf = NULL;
#endif

void os_frame_free(void *frame)
{
	struct net_buf *frag;

	if (frame) {
		frag = (struct net_buf *)((char *)frame - sizeof(struct net_buf));
		net_pkt_frag_unref(frag);
	}
}

void* os_frame_alloc_fn(u32 size, bool SecPool,const char* file, const int line)
{
	struct net_buf *frag = NULL;
	PKT_Info * pPKtInfo;

#if defined(RESERVE_FRAG_FOR_WIFI)
	frag = net_pkt_get_reserve_rx_data(0, K_NO_WAIT);
	if (!frag && SecPool) {
		k_mutex_lock(&reserve_frag_lock, K_FOREVER);
		if (reserveBuf != NULL) {
			frag = reserveBuf;
			reserveBuf = NULL;
		}
		k_mutex_unlock(&reserve_frag_lock);
	}
#endif

	if (!frag) {
		frag = net_pkt_get_reserve_rx_data(0, K_FOREVER);
	}

	if (!frag) {
		return NULL;
	}

	pPKtInfo = (PKT_Info *)frag->data;
	pPKtInfo->len = size;
	pPKtInfo->hdr_offset = ETHERNET_WIFI_RESERVE;
	return pPKtInfo;
}

void* os_frame_alloc_fn_timeout(u32 size, bool SecPool, s32 timeout, const char* file, const int line)
{
	struct net_buf *frag;
	PKT_Info * pPKtInfo;

	frag = net_pkt_get_reserve_rx_data(0, timeout);
	if (!frag) {
		return NULL;
	}

	pPKtInfo = (PKT_Info *)frag->data;
	pPKtInfo->len = size;
	pPKtInfo->hdr_offset = ETHERNET_WIFI_RESERVE;
	return pPKtInfo;
}

void* os_frame_dup(void *frame)
{
	struct net_buf *frag;
	PKT_Info * pPKtInfo;

	frag = net_pkt_get_reserve_rx_data(0, K_FOREVER);
	if (!frag) {
		return NULL;
	}

	pPKtInfo = (PKT_Info *)frag->data;
	void * dframe = os_frame_alloc(OS_FRAME_GET_DATA_LEN(pPKtInfo),FALSE);
	MEMCPY(OS_FRAME_GET_DATA(dframe), OS_FRAME_GET_DATA(pPKtInfo), OS_FRAME_GET_DATA_LEN(pPKtInfo));
	return (void*)dframe;
}

//increase space to set header
void* os_frame_push(void *frame, u32 len)
{
	PKT_Info * pPKtInfo = (PKT_Info * )frame;

	pPKtInfo->hdr_offset -= len;
	pPKtInfo->len  += len;

	return (u8*)pPKtInfo+pPKtInfo->hdr_offset;
}


//decrease data space.
void* os_frame_pull(void *frame, u32 len)
{
	PKT_Info * pPKtInfo = (PKT_Info * )frame;

	pPKtInfo->hdr_offset += len;
	pPKtInfo->len  -= len;

	return (u8*)pPKtInfo+pPKtInfo->hdr_offset;
}

u8* os_frame_get_data_addr(void *_frame)
{
	return ((u8*)(_frame)+(((PKT_Info *)_frame)->hdr_offset));
}

u32 os_frame_get_data_len(void *_frame)
{
	return (((PKT_Info *)_frame)->len);
}

void os_frame_set_data_len(void *_frame, u32 _len)
{
	(((PKT_Info *)_frame)->len = _len);
}

void os_frame_set_hdr_offset(void *_frame, u32 _offset)
{
	(((PKT_Info *)_frame)->hdr_offset = _offset);
}

u32 os_frame_get_hdr_offset(void *_frame)
{
	return (((PKT_Info *)_frame)->hdr_offset);
}

void os_frame_set_debug_flag(void *frame, u32 flag)
{
}

bool enter_ps_reserv_buf(void)
{
#if defined(RESERVE_FRAG_FOR_WIFI)
	struct net_buf *frag;

	k_mutex_lock(&reserve_frag_lock, K_FOREVER);
	if (reserveBuf) {
		k_mutex_unlock(&reserve_frag_lock);
		return true;
	}

	frag = net_pkt_get_reserve_rx_data(0, K_NO_WAIT);
	if (!frag) {
		k_mutex_unlock(&reserve_frag_lock);
		return false;
	}

	reserveBuf = frag;
	k_mutex_unlock(&reserve_frag_lock);
#endif

	return true;
}
