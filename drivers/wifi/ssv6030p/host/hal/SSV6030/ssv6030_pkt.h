#ifndef _SSV6030_PKT_H_
#define _SSV6030_PKT_H_

//tx
int ssv6030_hal_dump_txinfo(void *_p);
int ssv6030_hal_get_txreq0_size(void);
int ssv6030_hal_get_txreq0_ctype(void *p);
int ssv6030_hal_set_txreq0_ctype(void *p,u8 c_type);
int ssv6030_hal_get_txreq0_len(void *p);
int ssv6030_hal_set_txreq0_len(void *p,u32 len);
int ssv6030_hal_get_txreq0_rsvd0(void *p);
int ssv6030_hal_set_txreq0_rsvd0(void *p,u32 val);
int ssv6030_hal_get_txreq0_padding(void *p);
int ssv6030_hal_set_txreq0_padding(void *p, u32 val);
int ssv6030_hal_get_txreq0_qos(void *p);
int ssv6030_hal_get_txreq0_ht(void *p);
int ssv6030_hal_get_txreq0_4addr(void *p);
int ssv6030_hal_get_txreq0_f80211(void *p);
int ssv6030_hal_set_txreq0_f80211(void *p,u8 f80211);
int ssv6030_hal_get_txreq0_more_data(void *p);
int ssv6030_hal_set_txreq0_more_data(void *p,u8 more_data);
u8 *ssv6030_hal_get_txreq0_qos_ptr(void *_req0);
u8 *ssv6030_hal_get_txreq0_data_ptr(void *_req0);
void * ssv6030_hal_fill_txreq0(void *frame, u32 len, u32 priority,
                                            u16 *qos, u32 *ht, u8 *addr4,
                                            bool f80211, u8 security, u8 tx_dscrp_flag);


// rx
int ssv6030_hal_dump_rxinfo(void *_p);
int ssv6030_hal_get_rxpkt_size(void);
int ssv6030_hal_get_rxpkt_ctype(void *p);
int ssv6030_hal_get_rxpkt_len(void *p);
int ssv6030_hal_get_rxpkt_rsvd(void *p);
int ssv6030_hal_get_rxpkt_rcpi(void *p);
int ssv6030_hal_set_rxpkt_rcpi(void *p, u32 RCPI);
int ssv6030_hal_get_rxpkt_qos(void *p);
int ssv6030_hal_get_rxpkt_f80211(void *p);
int ssv6030_hal_get_rxpkt_wsid(void *p);
int ssv6030_hal_get_rxpkt_tid(void *p);
int ssv6030_hal_get_rxpkt_seqnum(void *p);

int ssv6030_hal_get_rxpkt_psm(void *p);
u8 *ssv6030_hal_get_rxpkt_qos_ptr(void *_rxpkt);
u8 *ssv6030_hal_get_rxpkt_data_ptr(void *_rxpkt);
int ssv6030_hal_get_rxpkt_data_len(void *rxpkt);

#endif /* _SSV6030_PKT_H_ */

