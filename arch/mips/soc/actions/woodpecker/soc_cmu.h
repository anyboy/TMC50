/********************************************************************************
 *                            USDK(ZS283A)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2018-10-12-AM11:58:29             1.0             build this file
 ********************************************************************************/
/*!
 * \file     soc_cmu.h
 * \brief
 * \author
 * \version  1.0
 * \date  2018-10-12-AM11:58:29
 *******************************************************************************/

#ifndef SOC_CMU_H_
#define SOC_CMU_H_

#define     CMU_HOSC_CTL                                                     (CMU_ANALOG_REG_BASE+0x00)
#define     CMU_LOSC_CTL                                                     (CMU_ANALOG_REG_BASE+0x04)
#define     CMU_SPLL_CTL                                                     (CMU_ANALOG_REG_BASE+0x10)
#define     CMU_COREPLL_CTL                                                  (CMU_ANALOG_REG_BASE+0x14)
#define     CMU_APLL0_CTL                                                    (CMU_ANALOG_REG_BASE+0x18)
#define     CMU_APLL1_CTL                                                    (CMU_ANALOG_REG_BASE+0x1C)
#define     CMU_SPLL_DEBUG                                                   (CMU_ANALOG_REG_BASE+0x20)
#define     CMU_COREPLL_DEBUG                                                (CMU_ANALOG_REG_BASE+0x24)
#define     CMU_COREPLL_DEBUG1                                               (CMU_ANALOG_REG_BASE+0x28)
#define     CMU_APLL0_DEBUG                                                  (CMU_ANALOG_REG_BASE+0x2C)
#define     CMU_APLL1_DEBUG                                                  (CMU_ANALOG_REG_BASE+0x30)
#define     CMU_ANADEBUG                                                     (CMU_ANALOG_REG_BASE+0x40)


/* CMU Digital registers */
#define     CMU_SYSCLK                                                        (CMU_DIGITAL_REG_BASE+0x0000)
#define     CMU_DEVCLKEN0                                                     (CMU_DIGITAL_REG_BASE+0x0008)
#define     CMU_DEVCLKEN1                                                     (CMU_DIGITAL_REG_BASE+0x000C)
#define     CMU_SD0CLK                                                        (CMU_DIGITAL_REG_BASE+0x0010)
#define     CMU_SD1CLK                                                        (CMU_DIGITAL_REG_BASE+0x0014)
#define     CMU_SPI0CLK                                                       (CMU_DIGITAL_REG_BASE+0x0020)
#define     CMU_SPI1CLK                                                       (CMU_DIGITAL_REG_BASE+0x0024)
#define     CMU_FMOUTCLK                                                      (CMU_DIGITAL_REG_BASE+0x002C)
#define     CMU_SECCLK                                                        (CMU_DIGITAL_REG_BASE+0x0030)
#define     CMU_LCDCLK                                                        (CMU_DIGITAL_REG_BASE+0x0034)
#define     CMU_SEGLEDCLK                                                     (CMU_DIGITAL_REG_BASE+0x0038)
#define     CMU_LRADCCLK                                                      (CMU_DIGITAL_REG_BASE+0x003C)
#define     CMU_TIMER0CLK                                                     (CMU_DIGITAL_REG_BASE+0x0040)
#define     CMU_TIMER1CLK                                                     (CMU_DIGITAL_REG_BASE+0x0044)
#define     CMU_TIMER2CLK                                                     (CMU_DIGITAL_REG_BASE+0x0048)
#define     CMU_TIMER3CLK                                                     (CMU_DIGITAL_REG_BASE+0x004C)
#define     CMU_PWM0CLK                                                       (CMU_DIGITAL_REG_BASE+0x0050)
#define     CMU_PWM1CLK                                                       (CMU_DIGITAL_REG_BASE+0x0054)
#define     CMU_PWM2CLK                                                       (CMU_DIGITAL_REG_BASE+0x0058)
#define     CMU_PWM3CLK                                                       (CMU_DIGITAL_REG_BASE+0x005C)
#define     CMU_PWM4CLK                                                       (CMU_DIGITAL_REG_BASE+0x0060)
#define     CMU_PWM5CLK                                                       (CMU_DIGITAL_REG_BASE+0x0064)
#define     CMU_PWM6CLK                                                       (CMU_DIGITAL_REG_BASE+0x0068)
#define     CMU_PWM7CLK                                                       (CMU_DIGITAL_REG_BASE+0x006C)
#define     CMU_PWM8CLK                                                       (CMU_DIGITAL_REG_BASE+0x0070)
#define     CMU_FFTCLK                                                        (CMU_DIGITAL_REG_BASE+0x0078)
#define     CMU_VADCLK                                                        (CMU_DIGITAL_REG_BASE+0x007C)
#define     CMU_ADCCLK                                                        (CMU_DIGITAL_REG_BASE+0x0080)
#define     CMU_DACCLK                                                        (CMU_DIGITAL_REG_BASE+0x0084)
#define     CMU_I2STXCLK                                                      (CMU_DIGITAL_REG_BASE+0x0088)
#define     CMU_I2SRXCLK                                                      (CMU_DIGITAL_REG_BASE+0x008C)
#define     CMU_SPDIFTXCLK                                                    (CMU_DIGITAL_REG_BASE+0x0090)
#define     CMU_SPDIFRXCLK                                                    (CMU_DIGITAL_REG_BASE+0x0094)
#define     CMU_HCL3K2CTL                                                     (CMU_DIGITAL_REG_BASE+0x00A0)
#define     CMU_MEMCLKEN                                                      (CMU_DIGITAL_REG_BASE+0x00B0)
#define     CMU_MEMCLKSRC                                                     (CMU_DIGITAL_REG_BASE+0x00B8)
#define     CMU_DIGITALDEBUG                                                  (CMU_DIGITAL_REG_BASE+0x00F0)
#define     CMU_TSTCTL                                                        (CMU_DIGITAL_REG_BASE+0x00F4)
#define     CMU_ASRCCLK                                                       (0xcccccccc) //no support
#define     CMU_DSP_WAIT                                                      (0xcccccccc) //no support
#define     CMU_DSP_AUDIO_VOLCLK_SEL                                          (0xcccccccc) //no support


#define     CMU_SYSCLK_CORECLKDIV_SHIFT                                       4
#define     CMU_SYSCLK_CORE_CLKSEL_MASK                                       (0x3<<0)

/* CMU_ADCCLK */
#define     CMU_ADCCLK_ADCFIR_EN                                              15
#define     CMU_ADCCLK_ADCCIC_EN                                              14
#define     CMU_ADCCLK_ADCANA_EN                                              13
#define     CMU_ADCCLK_ADCDMIC_EN                                             12
#define     CMU_ADCCLK_ADCFIRCLKRVS                                           11
#define     CMU_ADCCLK_ADCCICCLKRVS                                           10
#define     CMU_ADCCLK_ADCANACLKRVS                                           9
#define     CMU_ADCCLK_ADCDMICCLKRVS                                          8
#define     CMU_ADCCLK_ADCCLKSRC_E                                         		5
#define     CMU_ADCCLK_ADCCLKSRC_SHIFT                                     		4
#define     CMU_ADCCLK_ADCCLKSRC_MASK                                     		(0x3<<4)
#define     CMU_ADCCLK_ADCCLKPREDIV                                           3
#define     CMU_ADCCLK_ADCCLKDIV_E                                            2
#define     CMU_ADCCLK_ADCCLKDIV_SHIFT                                        0
#define     CMU_ADCCLK_ADCCLKDIV_MASK                                         (0x7<<0)

/* CMU_DACCLK */
#define     CMU_DACCLK_DACCLKSRC                                              4
#define     CMU_DACCLK_DACCLKPREDIV                                           3
#define     CMU_DACCLK_DACCLKDIV_E                                            2
#define     CMU_DACCLK_DACCLKDIV_SHIFT                                        0
#define     CMU_DACCLK_DACCLKDIV_MASK                                         (0x7<<0)

/* CMU_I2STXCLK */
#define     CMU_I2STXCLK_I2SSRDCLKSRC_E                                       29
#define     CMU_I2STXCLK_I2SSRDCLKSRC_SHIFT                                   28
#define     CMU_I2STXCLK_I2SSRDCLKSRC_MASK                                    (0x3<<28)
#define     CMU_I2STXCLK_I2STXMCLKEXTREV                                      12
#define     CMU_I2STXCLK_I2STXMCLKSRCH                                        10
#define     CMU_I2STXCLK_I2STXMCLKSRC_E                                       9
#define     CMU_I2STXCLK_I2STXMCLKSRC_SHIFT                                   8
#define     CMU_I2STXCLK_I2STXMCLKSRC_MASK                                    (0x3<<8)
#define     CMU_I2STXCLK_I2STXMCLKDIV                                         7
#define     CMU_I2STXCLK_I2STXCLKSRC                                          4
#define     CMU_I2STXCLK_I2STXCLKPREDIV                                       3
#define     CMU_I2STXCLK_I2STXCLKDIV_E                                        2
#define     CMU_I2STXCLK_I2STXCLKDIV_SHIFT                                    0
#define     CMU_I2STXCLK_I2STXCLKDIV_MASK                                     (0x7<<0)

/* CMU_I2SRXCLK */
#define     CMU_I2SRXCLK_I2SRXMCLKEXTREV                                      12
#define     CMU_I2SRXCLK_I2SRXMCLKSRCH                                        10
#define     CMU_I2SRXCLK_I2SRXMCLKSRC_E                                       9
#define     CMU_I2SRXCLK_I2SRXMCLKSRC_SHIFT                                   8
#define     CMU_I2SRXCLK_I2SRXMCLKSRC_MASK                                    (0x3<<8)
#define     CMU_I2SRXCLK_I2SRXCLPREKSRC                                       4
#define     CMU_I2SRXCLK_I2SRXCLKPREDIV                                       3
#define     CMU_I2SRXCLK_I2SRXCLKDIV_E                                        2
#define     CMU_I2SRXCLK_I2SRXCLKDIV_SHIFT                                    0
#define     CMU_I2SRXCLK_I2SRXCLKDIV_MASK                                     (0x7<<0)

/* CMU_SPDIFTXCLK */
#define     CMU_SPDIFTXCLK_SPDIFTXCLKSRC_E                                    1
#define     CMU_SPDIFTXCLK_SPDIFTXCLKSRC_SHIFT                                0
#define     CMU_SPDIFTXCLK_SPDIFTXCLKSRC_MASK                                 (0x3<<0)

/* CMU_SPDIFRXCLK */
#define     CMU_SPDIFRXCLK_SPDIFRXCLKSRC_E                                    9
#define     CMU_SPDIFRXCLK_SPDIFRXCLKSRC_SHIFT                                8
#define     CMU_SPDIFRXCLK_SPDIFRXCLKSRC_MASK                                 (0x3<<8)
#define     CMU_SPDIFRXCLK_SPDIFRXCLKDIV_E                                    1
#define     CMU_SPDIFRXCLK_SPDIFRXCLKDIV_SHIFT                                0
#define     CMU_SPDIFRXCLK_SPDIFRXCLKDIV_MASK                                 (0x3<<0)

#define     CMU_MEMCLKEN_FFTRAMCLKEN                                          14
#define     CMU_MEMCLKEN_URAMCLKEN                                            13
#define     CMU_MEMCLKEN_PCMRAM1CLKEN                                         12
#define     CMU_MEMCLKEN_RAM6CLKEN                                            8
#define     CMU_MEMCLKEN_RAM5CLKEN                                            7
#define     CMU_MEMCLKEN_RAM4CLKEN                                            6
#define     CMU_MEMCLKEN_RAM3CLKEN                                            5
#define     CMU_MEMCLKEN_RAM2CLKEN                                            4
#define     CMU_MEMCLKEN_RAM1CLKEN                                            3
#define     CMU_MEMCLKEN_RAM0CLKEN                                            2
#define     CMU_MEMCLKEN_ROM1CLKEN                                            1
#define     CMU_MEMCLKEN_ROM0CLKEN                                            0

#define     CMU_MEMCLKSRC_FFTRAMCLKSRC                                        14
#define     CMU_MEMCLKSRC_URAMCLKSRC                                          13
#define     CMU_MEMCLKSRC_PCMRAM1CLKSRC                                       12
#define     CMU_MEMCLKSRC_RAM4CLKSRC_E                                        7
#define     CMU_MEMCLKSRC_RAM4CLKSRC_SHIFT                                    6
#define     CMU_MEMCLKSRC_RAM4CLKSRC_MASK                                     (0x3<<6)
#define     CMU_MEMCLKSRC_RAM3CLKSRC                                          5

/* CMU AUDIOPLL0 CTL bits */
#define CMU_AUDIOPLL0_CTL_PMD         8
#define CMU_AUDIOPLL0_CTL_MODE        5
#define CMU_AUDIOPLL0_CTL_EN          4
#define CMU_AUDIOPLL0_CTL_APS0_SHIFT  0
#define CMU_AUDIOPLL0_CTL_APS0(x)    ((x) << CMU_AUDIOPLL0_CTL_APS0_SHIFT)
#define CMU_AUDIOPLL0_CTL_APS0_MASK	  CMU_AUDIOPLL0_CTL_APS0(0xF)

/* CMU AUDIOPLL1 CTL bits */
#define CMU_AUDIOPLL1_CTL_PMD         8
#define CMU_AUDIOPLL1_CTL_MODE        5
#define CMU_AUDIOPLL1_CTL_EN          4
#define CMU_AUDIOPLL1_CTL_APS1_SHIFT  0
#define CMU_AUDIOPLL1_CTL_APS1(x)    ((x) << CMU_AUDIOPLL1_CTL_APS1_SHIFT)
#define CMU_AUDIOPLL1_CTL_APS1_MASK	  CMU_AUDIOPLL1_CTL_APS1(0xF)

//S2 mode cpu clock
#define CMU_SYSCLK_CLKSEL_32K       0x0
//CK24M cpu clock when system start up
#define CMU_SYSCLK_CLKSEL_HOSC      0x1
//CORE PLL cpu clock when system normal running
#define CMU_SYSCLK_CLKSEL_COREPLL   0x2
//CK64M bluetooth clock
#define CMU_SYSCLK_CLKSEL_64M       0x3

//corepll lowest frequency
#define MIN_CMU_COREPLL_MHZ         (36)

#define     MMC_LOW_FREQ				(0)

#define     MMC_LOW_FREQ_VAL            (0x11)

#define     CMU_SD0CLK_SD0CLKSRC            8
#define     CMU_SD1CLK_SD1CLKDIV_SHIFT      0

#define     CMU_SYSCLK_CPUCLKDIV_SHIFT      4

#define     CMU_SPICLK_SPI0CLKSRC_SHIFT     8
#define     CMU_SPICLK_SPI0CLKDIV_SHIFT     0
#define     CMU_SPICLK_SPI1CLKDIV_SHIFT     8


typedef enum
{
    MEM_CLK_ROM0 = 0,
    MEM_CLK_ROM1 = 1,
    MEM_CLK_RAM0 = 2,
    MEM_CLK_RAM1 = 3,
    MEM_CLK_RAM2 = 4,
    MEM_CLK_RAM3 = 5,
    MEM_CLK_RAM4 = 6,
    MEM_CLK_RAM5 = 7,
    MEM_CLK_RAM6 = 8,
    MEM_CLK_PCMRAM1 = 12,
    MEM_CLK_URAM = 13,
    MEM_CLK_FFTRAM = 14,
} mem_clk_type_e;

void cmu_corepll_clk_set(unsigned int corepll_mhz);
unsigned int cmu_core_periph_clk_get_mhz(uint32_t core_type);
void cmu_asrc_clk_set(unsigned int corepll_clk, unsigned int asrc_clk);

extern int soc_get_hosc_cap(void);
extern void soc_set_hosc_cap(int cap);
void soc_cmu_init(void);


#endif /* SOC_CMU_H_ */
