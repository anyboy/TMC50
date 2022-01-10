/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file boot related interface
 */

#ifndef	_ACTIONS_SOC_BOOT_H_
#define	_ACTIONS_SOC_BOOT_H_

/*
 * current firmware's boot file always mapping to 0x100000,
 * the mirrored boot mapping to 0x200000
 */
#define	SOC_BOOT_A_ADDRESS				(0xbe000000)
#define	SOC_BOOT_B_ADDRESS				(0xbe200000)

#define	SOC_BOOT_PARTITION_TABLE_OFFSET			(0x0)
#define	SOC_BOOT_FIRMWARE_VERSION_OFFSET		(0x180)

#define SOC_BOOT_SYSTEM_PARAM0_OFFSET		(0x1000)
#define SOC_BOOT_SYSTEM_PARAM1_OFFSET		(0x3000)
#define SOC_BOOT_SYSTEM_PARAM_SIZE			(0x1000)

#define SOC_BOOT_MBREC0_OFFSET				(0x0)
#define SOC_BOOT_MBREC1_OFFSET				(0x2000)
#define SOC_BOOT_MBREC_SIZE					(0x1000)

#define BOOT_INFOR_RAM_ADDR					((void *)0xBFC20EE0)
#define BOOT_INFOR_MAX_SIZE					(0x20)

typedef struct {
	u32_t boot_phy_addr;
	u32_t param_phy_addr;
	u32_t reboot_reason;
	u32_t watchdog_reboot : 8;
	u32_t prod_adfu_reboot : 1;
	u32_t prod_card_reboot : 1;
} boot_info_t;

typedef struct {
	u32_t card_product_att_reboot : 8;
} boot_user_info_t;

extern boot_user_info_t g_boot_user_info;

#ifndef _ASMLANGUAGE

static inline u32_t soc_boot_get_part_tbl_addr(void)
{
	return (SOC_BOOT_A_ADDRESS + SOC_BOOT_PARTITION_TABLE_OFFSET);
}

static inline u32_t soc_boot_get_fw_ver_addr(void)
{
	return (SOC_BOOT_A_ADDRESS + SOC_BOOT_FIRMWARE_VERSION_OFFSET);
}

static inline const boot_info_t *soc_boot_get_info(void)
{
	/*boot inforamtion transmitted by mbrec */
	extern char _boot_info_start[];
	return (const boot_info_t *)_boot_info_start;
}

static inline u32_t soc_boot_get_reboot_reason(void)
{
	boot_info_t *p_boot_info = (boot_info_t *)(BOOT_INFOR_RAM_ADDR);
	return p_boot_info->reboot_reason;
}

static inline u32_t soc_boot_get_watchdog_is_reboot(void)
{
	boot_info_t *p_boot_info = (boot_info_t *)(BOOT_INFOR_RAM_ADDR);
	return p_boot_info->watchdog_reboot;
}

static inline void soc_boot_user_set_card_product_att_reboot(bool card_product_att_reboot)
{
	g_boot_user_info.card_product_att_reboot = card_product_att_reboot;
}

static inline bool soc_boot_user_get_card_product_att_reboot(void)
{
	return g_boot_user_info.card_product_att_reboot;
}

#endif /* _ASMLANGUAGE */

#endif /* _ACTIONS_SOC_BOOT_H_	*/
