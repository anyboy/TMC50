/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2018-10-12-PM12:53:37             1.0             build this file
 ********************************************************************************/
/*!
 * \file     soc_dsp.h
 * \brief
 * \author
 * \version  1.0
 * \date  2018-10-12-PM12:53:37
 *******************************************************************************/

/*woodpecker not support dsp*/

#ifndef SOC_DSP_H_
#define SOC_DSP_H_

static unsigned int inline mcu_to_dsp_code_address(unsigned int mcu_addr)
{
    return 0;
}

/* 0x00000 <= dsp_addr < 0x20000
 */
static unsigned int inline dsp_code_to_mcu_address(unsigned int dsp_addr)
{
    return 0;
}

static unsigned int inline mcu_to_dsp_data_address(unsigned int mcu_addr)
{
    return 0;
}

/* 0x20000 <= dsp_addr < 0x40000
 */
static unsigned int inline dsp_data_to_mcu_address(unsigned int dsp_addr)
{
    return 0;
}

static unsigned int inline mcu_to_mpu_dsp_addr(unsigned int mcu_addr)
{
    return 0;
}

static unsigned int inline mpu_dsp_to_mcu_addr(unsigned int dsp_addr)
{
    return 0;
}

#endif /* SOC_DSP_H_ */

