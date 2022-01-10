/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2019-2-18-PM1:38:13             1.0             build this file
 ********************************************************************************/
/*!
 * \file     soc_cmu.c
 * \brief
 * \author
 * \version  1.0
 * \date  2019-2-18-PM1:38:13
 *******************************************************************************/
#include <kernel.h>
#include <device.h>
#include <soc.h>
#include <misc/printk.h>


#define     HOSC_CTL_HOSCI_BC_SEL_E                                           31
#define     HOSC_CTL_HOSCI_BC_SEL_SHIFT                                       29
#define     HOSC_CTL_HOSCI_BC_SEL_MASK                                        (0x7<<29)
#define     HOSC_CTL_HOSCI_TC_SEL_E                                           28
#define     HOSC_CTL_HOSCI_TC_SEL_SHIFT                                       24
#define     HOSC_CTL_HOSCI_TC_SEL_MASK                                        (0x1F<<24)
#define     HOSC_CTL_HOSCO_BC_SEL_E                                           23
#define     HOSC_CTL_HOSCO_BC_SEL_SHIFT                                       21
#define     HOSC_CTL_HOSCO_BC_SEL_MASK                                        (0x7<<21)
#define     HOSC_CTL_HOSCO_TC_SEL_E                                           20
#define     HOSC_CTL_HOSCO_TC_SEL_SHIFT                                       16
#define     HOSC_CTL_HOSCO_TC_SEL_MASK                                        (0x1F<<16)
#define     HOSC_CTL_HOSC_AMPLMT_E                                            14
#define     HOSC_CTL_HOSC_AMPLMT_SHIFT                                        13
#define     HOSC_CTL_HOSC_AMPLMT_MASK                                         (0x3<<13)
#define     HOSC_CTL_HOSC_AMPLMT_EN                                           12
#define     HOSC_CTL_HGMC_E                                                   9
#define     HOSC_CTL_HGMC_SHIFT                                               8
#define     HOSC_CTL_HGMC_MASK                                                (0x3<<8)
#define     HOSC_CTL_HOSC_VDD_SEL                                             6
#define     HOSC_CTL_HOSC_BUF_SEL_E                                           5
#define     HOSC_CTL_HOSC_BUF_SEL_SHIFT                                       4
#define     HOSC_CTL_HOSC_BUF_SEL_MASK                                        (0x3<<4)
#define     HOSC_CTL_HOSC_EN                                                  0

/*!
 * \brief get the HOSC capacitance
 * \n  capacitance is x10 fixed value
 */
extern int soc_get_hosc_cap(void)
{
    int  cap = 0;

    uint32_t  hosc_ctl = sys_read32(CMU_HOSC_CTL);

    uint32_t  bc_sel = (hosc_ctl & HOSC_CTL_HOSCO_BC_SEL_MASK) >> HOSC_CTL_HOSCO_BC_SEL_SHIFT;
    uint32_t  tc_sel = (hosc_ctl & HOSC_CTL_HOSCO_TC_SEL_MASK) >> HOSC_CTL_HOSCO_TC_SEL_SHIFT;

    cap = bc_sel * 30 + tc_sel;

    return cap;
}


/*!
 * \brief set the HOSC capacitance
 * \n  capacitance is x10 fixed value
 */
extern void soc_set_hosc_cap(int cap)
{
    printk("set hosc cap:%d.%d pf\n", cap / 10, cap % 10);

    uint32_t  hosc_ctl = sys_read32(CMU_HOSC_CTL);

    uint32_t  bc_sel = cap / 30;
    uint32_t  tc_sel = cap % 30;

    hosc_ctl &= ~(
        HOSC_CTL_HOSCI_BC_SEL_MASK |
        HOSC_CTL_HOSCI_TC_SEL_MASK |
        HOSC_CTL_HOSCO_BC_SEL_MASK |
        HOSC_CTL_HOSCO_TC_SEL_MASK);

    hosc_ctl |= (
        (bc_sel << HOSC_CTL_HOSCI_BC_SEL_SHIFT) |
        (tc_sel << HOSC_CTL_HOSCI_TC_SEL_SHIFT) |
        (bc_sel << HOSC_CTL_HOSCO_BC_SEL_SHIFT) |
        (tc_sel << HOSC_CTL_HOSCO_TC_SEL_SHIFT));

    sys_write32(hosc_ctl, CMU_HOSC_CTL);

    k_busy_wait(100);
}

void soc_cmu_init(void)
{
	u32_t hosc_ctl_value;

	/* set frequency offset inside compensation driver */
	hosc_ctl_value = sys_read32(CMU_HOSC_CTL) & 0xffff0000;
	hosc_ctl_value |= (0x01<<13) | (1<<12) | (0x01<<8) | (1<<6) | (0x01<<4);
	hosc_ctl_value |= (1<<0);
    sys_write32(hosc_ctl_value, CMU_HOSC_CTL);
}

