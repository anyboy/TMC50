#include <stdio.h>
#include <net/buf.h>
#include <net/net_if.h>
#include <net/net_core.h>
#include <net/ethernet.h>
#include <zephyr_service.h>

void os_frame_free(void *frame)
{
	struct net_buf *frag;

	if (frame) {
		frag = (struct net_buf *)((char *)frame - sizeof(struct net_buf));
		net_pkt_frag_unref(frag);
	}
}

/*
* timeout: 0: K_NO_WAIT,  -1: K_FOREVER;  other: wait timeout(ms)
*/
void* os_frame_alloc(int32_t timeout)
{
	u32_t start_time, stop_time;
	struct net_buf *frag;
	void *frame;

	start_time = k_uptime_get_32();
	frag = net_pkt_get_reserve_rx_data(0, timeout);
	if (!frag) {
		PR_ERR("NULL, timeout:%d", timeout);
		return NULL;
	}

	stop_time = k_uptime_get_32();
	if ((stop_time - start_time) >= 20) {
		PR_WRN("alloc time:%d", (stop_time - start_time));
	}

	frame = (void *)frag->data;
	return frame;
}
