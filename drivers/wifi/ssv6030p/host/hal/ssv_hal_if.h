#ifndef _SSV_HAL_IF_H_
#define _SSV_HAL_IF_H_

#include <host_config.h>

#define CMD_FLOW_TO_HW(data,a,b,c,d,e,f,g,h)  data[0]=(a&0xF)|((b&0xF)<<4)|((c&0xF)<<8)|((d&0xF)<<12)|((e&0xF)<<16)|((f&0xF)<<20)|((g&0xF)<<24)|((h&0xF)<<28)
#define CMD_FLOW_TO_MCU(data,a,b,c,d,e,f,g,h) data[1]=(a&0xF)|((b&0xF)<<4)|((c&0xF)<<8)|((d&0xF)<<12)|((e&0xF)<<16)|((f&0xF)<<20)|((g&0xF)<<24)|((h&0xF)<<28)

#define LBYTESWAP(a)  ((((a) & 0x00ff00ff) << 8) | \
    (((a) & 0xff00ff00) >> 8))

#define LONGSWAP(a)   ((LBYTESWAP(a) << 16) | (LBYTESWAP(a) >> 16))

typedef struct __chip_def {
    const char *chip_str;
    u32     chip_id;
} chip_def_S;
extern chip_def_S* chip_sel;

struct ssv_hal_ops
{
    char	name[32];
    int	(* chip_init)(void);
    int	(* init_mac)(u8 *self_mac);
    int	(* init_sta_mac)(u32 wifi_mode);
    int	(* init_ap_mac)(u8 *bssid, u8 channel);
    int (* ap_wep_setting)(ssv6xxx_sec_type sec_type, u8 *password);
    int (* tx_loopback_done)(u8 *dat);
    int	(* setup_ampdu_wmm)(bool IsAMPDU);
    int (* pbuf_alloc)(int size, int type);
    int (* pbuf_free)(int addr);
    int (* rf_enable)(void);
    int (* rf_disable)(void);
    int (* rf_load_default_setting)(void);
    int (* watch_dog_enable)(void);
    int (* watch_dog_disable)(void);
    int (* mcu_enable)(void);
    int (* mcu_disable)(void);
    int (* sw_reset)(u32 com);
    int (* gen_rand)(u8 *data, u32 len);
    int (* promiscuous_enable)(void);
    int (* promiscuous_disable)(void);
    int (* read_chip_id)(void);
    int (* read_efuse_mac)(u8 *mac);
    bool (* get_diagnosis)(void);
    int (* is_heartbeat)(void);
    int (* reset_heartbeat)(void);
    int (* get_fw_status)(u32 *val);
    int (* set_fw_status)(u32 val);
    int	(* reset_soc_irq)(void);
    int (* set_wakeup_bb_gpio)(bool en);
    int (* set_short_slot_time)(bool en);
    int (* soc_set_bcn)(enum ssv6xxx_tx_extra_type extra_type, void *frame, struct cfg_bcn_info *bcn_info, u8 dtim_cnt, u16 bcn_itv);
    int (* beacon_set)(void* beacon_skb, int dtim_offset);
    int (* beacon_enable)(bool bEnable);
    bool (*is_beacon_enable)(void);
    int (* get_tx_resources)(u16 *pFreePages, u16 *pFreeIDs, u16 *pFreeSpaces);
    int (* bytes_to_pages)(u32 size);
    int (* get_rssi_from_reg)(void);
    int (* get_rc_info)(u16 *tx_cnt, u16 *retry_cnt, u16 *phy_rate);
    int	(* get_agc_gain)(void);
    int	(* set_agc_gain)(u32 gain);
    int (* set_acs_agc_gain)(void);
    int	(* ap_listen_neighborhood)(bool en);
    int (* reduce_phy_cca_bits)(void);
    int (* recover_phy_cca_bits)(void);
    int (* update_cci_setting)(u16 input_level);
    int (* set_ext_rx_int)(u32 pin);
    int (* pause_resuem_recovery_int)(bool resume);
    int (* set_TXQ_SRC_limit)(u32 qidx,u32 val);
    int (* halt_txq)(u32 qidx,bool bHalt);
    int (* get_temperature)(u8 *sar_code, s8 *temperature);
    int	(* set_voltage_mode)(u32 mode);
    int (*display_hw_queue_status)(void);
    int (* set_gpio_output)(bool hi_lo, u32 pin);
    /** tx descriptor **/
    int	(* dump_txinfo)(void *p);
    int	(* get_txreq0_size)(void);
    int	(* get_txreq0_ctype)(void *p);
    int	(* set_txreq0_ctype)(void *p,u8 type);
    int	(* get_txreq0_len)(void *p);
    int	(* set_txreq0_len)(void *p,u32 len);
    int	(* get_txreq0_rsvd0)(void *p);
    int	(* set_txreq0_rsvd0)(void *p,u32 val);
    int	(* get_txreq0_padding)(void *p);
    int	(* set_txreq0_padding)(void *p, u32 val);
    int	(* get_txreq0_qos)(void *p);
    int	(* get_txreq0_ht)(void *p);
    int	(* get_txreq0_4addr)(void *p);
    int	(* set_txreq0_f80211)(void *p, u8 f80211);
    int	(* get_txreq0_f80211)(void *p);
    int	(* get_txreq0_more_data)(void *p);
    int	(* set_txreq0_more_data)(void *p, u8 more_data);
    u8 *(* get_txreq0_qos_ptr)(void *p);
    u8 *(* get_txreq0_data_ptr)(void *p);
    int (* tx_3to11)(void *frame, u32 len, u32 priority,
                            u16 *qos, u32 *ht, u8 *addr4,
                            bool f80211, u8 security);
    void *(* fill_txreq0)(void *frame, u32 len, u32 priority,
                            u16 *qos, u32 *ht, u8 *addr4,
                            bool f80211, u8 security, u8 tx_dscrp_flag);
    /** rx descriptor **/
    int (* rx_11to3)(void *p);
    int	(* dump_rxinfo)(void *p);
    int	(* get_rxpkt_size)(void);
    int	(* get_rxpkt_ctype)(void *p);
    int	(* get_rxpkt_len)(void *p);
    int	(* get_rxpkt_rcpi)(void *p);
    int	(* set_rxpkt_rcpi)(void *p, u32 rcpi);
    int	(* get_rxpkt_qos)(void *p);
    int	(* get_rxpkt_f80211)(void *p);
    int	(* get_rxpkt_psm)(void *p);
    int	(* get_rxpkt_chl_info)(void *p);
    int	(* get_rxpkt_wsid)(void *p);
    int	(* get_rxpkt_tid)(void *p);
    int	(* get_rxpkt_seqnum)(void *p);
    u8 *(* get_rxpkt_qos_ptr)(void *p);
    u8 *(* get_rxpkt_data_ptr)(void *p);
    int (* get_rxpkt_data_len)(void *p);
     /** load fw **/
    int (* download_fw)(u8 *fw_bin, u32 len);

    /**decision table**/
    int (* accept_none_wsid)(void);
    int (* drop_none_wsid)(void);
    int (* drop_porbe_request)(bool IsDrop);
    int (* sta_rcv_all_bcn)(void);
    int (* sta_rcv_specific_bcn)(void);
    int (* sta_reject_bcn)(void);

    /**data flow**/
    int (* acs_rx_mgmt_flow)(void);
    int (* ap_rx_mgmt_flow)(void);
    int (* sconfig_rx_data_flow)(void);
    int (* sta_rx_data_flow)(void);

};

#if((CONFIG_CHIP_ID==SSV6030P)||(CONFIG_CHIP_ID==SSV6051Z)||(CONFIG_CHIP_ID==SSV6052Q)||(CONFIG_CHIP_ID==SSV8031B))
	extern const struct ssv_hal_ops	g_hal_ssv6030;
#endif

#if(CONFIG_CHIP_ID==SSV6006)
	extern struct ssv_hal_ops	g_hal_ssv6006;
#endif
#endif /* _SSV_HAL_IF_H_ */

