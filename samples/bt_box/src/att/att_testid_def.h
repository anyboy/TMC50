#ifndef _ATT_TESTID_DEF_H
#define _ATT_TESTID_DEF_H

//general test ID : 0x01-0xbf
#define TESTID_MODIFY_BTNAME        0x01
#define TESTID_MODIFY_BLENAME       0x02
#define TESTID_FM_TEST              0x05
#define TESTID_GPIO_TEST            0x06
#define TESTID_LINEIN_CH_TEST       0x07
#define TESTID_MIC_CH_TEST          0x08
#define TESTID_FM_CH_TEST           0x09
#define TESTID_SDCARD_TEST          0x0a
#define TESTID_UHOST_TEST           0x0b
#define TESTID_LINEIN_TEST          0x0c
#define TESTID_PREPARE_PRODUCT_TEST 0x0d
#define TESTID_PRODUCT_TEST         0x0e
#define TESTID_FLASHDUMP_TEST       0x0f
#define TESTID_BURN_IC              0x10
#define TESTID_CHECK_IC             0x11
#define TESTID_FLASHDOWNLOAD_TEST   0x12
#define TESTID_MEM_UPLOAD_TEST      0x13
#define TESTID_MEM_DOWNLOAD_TEST    0x14
#define TESTID_READ_BTNAME          0x16
#define TESTID_MONITOR              0x17
#define TESTID_FTMODE               0x18
#define TESTID_BQBMODE              0x19
#define TESTID_FCCMODE              0x1a
#define TESTID_LRADC_TEST           0x1b
#define TESTID_READ_FW_VERSION      0x1d

#define TESTID_LED_TEST             0x21
#define TESTID_KEY_TEST             0x22
#define TESTID_CAP_CTR_START        0x23
#define TESTID_CAP_TEST             0x24
#define TESTID_EXIT_MODE            0x25

#define TESTID_AGING_PB_START       0x28
#define TESTID_AGING_PB_CHECK       0x29

#define TESTID_CARD_PRODUCT_TEST    0x2e

#define TESTID_USER_RESERVED0       0x40
#define TESTID_USER_RESERVED1       0x41
#define TESTID_USER_RESERVED2       0x42
#define TESTID_USER_RESERVED3       0x43
#define TESTID_USER_RESERVED4       0x44
#define TESTID_USER_RESERVED5       0x45
#define TESTID_USER_RESERVED6       0x46
#define TESTID_USER_RESERVED7       0x47

//special test ID, need binding with ATT tool : 0xc0-0xfe
#define TESTID_MODIFY_BTADDR        0xc0
#define TESTID_BT_TEST              0xc1
#define TESTID_MP_TEST              0xc2
#define TESTID_MP_READ_TEST         0xc3
#define TESTID_READ_BTADDR          0xc4
#define TESTID_PWR_TEST             0xc5
#define TESTID_RSSI_TEST            0xc6
#define TESTID_MODIFY_BTADDR2       0xc7

//TEST ID WAIT, DUT need wait for ATT tool reply a valid test id
#define TESTID_ID_WAIT              0xfffe

//TEST ID QUIT, DUT will stop auto test, and quit
#define TESTID_ID_QUIT              0xffff

#endif  /* _ATT_TESTID_DEF_H */
