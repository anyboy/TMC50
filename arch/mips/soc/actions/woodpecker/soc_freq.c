/*
 * Copyright (c) 2019 Actions Semiconductor Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file clock frequence interface
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <soc.h>
#include <soc_freq.h>

void soc_freq_set_spi_clk_with_corepll(int spi_id, unsigned int spi_freq,
	unsigned int corepll_freq);

void soc_freq_set_spi_clk_with_spll(int spi_id, unsigned int spi_freq,
	unsigned int spll_freq);


unsigned int soc_freq_get_corepll_freq(void)
{
	unsigned int val;

	val = sys_read32(CORE_PLL_CTL);

	if (!(val & (1 << 7)))
		return 0;

	return ((val & 0x7f) * 6);
}

unsigned int soc_freq_get_hosc_freq(void)
{
    return CONFIG_HOSC_CLK_MHZ;
}

unsigned int soc_freq_get_spll_freq(void)
{
    return 64;
}

unsigned int soc_freq_get_core_freq(void)
{
	unsigned int freq;

	freq = soc_freq_get_corepll_freq();

	if(!freq){
        return 0;
	}

	return freq;
}

unsigned int soc_freq_get_cpu_freq(void)
{
	unsigned int freq, core_freq;
	unsigned int coefficient;
	unsigned int cpu_clk_src;

	cpu_clk_src = sys_read32(CMU_SYSCLK) & 0x03;

	if (cpu_clk_src == CMU_SYSCLK_CLKSEL_HOSC) /* HOSC */
		core_freq = soc_freq_get_hosc_freq();
	else if (cpu_clk_src == CMU_SYSCLK_CLKSEL_COREPLL) /* COREPLL */
		core_freq = soc_freq_get_core_freq();
	else if (cpu_clk_src == CMU_SYSCLK_CLKSEL_64M) /* CK_64M */
		core_freq = soc_freq_get_spll_freq();
	else
		core_freq = 0; /* 32KHz, forbid */

	coefficient = ((sys_read32(CMU_SYSCLK) >> 4) & 0xf) + 1;
	freq = core_freq * coefficient / 16;

	return freq;
}

unsigned int soc_freq_get_dsp_freq(void)
{
	return 0;
}


/* delay 100us when cpu clock is 24M HOSC */
static void delay_100us(void)
{
	volatile int i;

	for (i = 0; i < 150; i++)
		;
}

#ifdef CONFIG_SOC_SPREAD_SPECTRUM
void enable_spread_spectrum(void)
{
	sys_write32((1<<17) | (1<<16) | (1<<15) | (1<<12) | (7<<9) | (3<<6) | (9<<0), CORE_PLL_DEBUG);
}

void disable_spread_spectrum(void)
{
	sys_write32(sys_read32(CORE_PLL_DEBUG) & ~(1<<15), CORE_PLL_DEBUG);
}
#endif

static unsigned int calc_cpu_clk_div_hosc(unsigned int cpu_mhz)
{
	int sel;

	sel = (cpu_mhz*16 + CONFIG_HOSC_CLK_MHZ - 1)/CONFIG_HOSC_CLK_MHZ - 1;

	return sel;
}

void soc_freq_set_cpu_clk(unsigned int dsp_mhz, unsigned int cpu_mhz)
{
	unsigned int val;

	unsigned int flags;
	
	unsigned int cpu_clk_src_old, cpu_clk_src_new;

	ARG_UNUSED(dsp_mhz);

	flags = irq_lock();

	cpu_clk_src_old = sys_read32(CMU_SYSCLK) & 0x3;
	cpu_clk_src_new = CMU_SYSCLK_CLKSEL_COREPLL;
	
	if (cpu_mhz > CONFIG_HOSC_CLK_MHZ && cpu_mhz < 36) {
		cpu_mhz = 36;
	} else if (cpu_mhz <= CONFIG_HOSC_CLK_MHZ) {
		cpu_clk_src_new = CMU_SYSCLK_CLKSEL_HOSC;
	}

#ifdef CONFIG_SOC_SPREAD_SPECTRUM
	//disable_spread_spectrum(); /*Woodpecker IC need't disable spread spectrum when setting corepll*/
#endif

    if (cpu_clk_src_new == CMU_SYSCLK_CLKSEL_HOSC) {
		/* switch to hosc */
		sys_write32((sys_read32(CMU_SYSCLK) & ~0x3) | CMU_SYSCLK_CLKSEL_HOSC, CMU_SYSCLK);
		/* wait switch over */
		while (!((sys_read32(CMU_SYSCLK) & 0x3) == CMU_SYSCLK_CLKSEL_HOSC))
			;

		sys_write32((1 << 8) | (calc_cpu_clk_div_hosc(cpu_mhz) << 4) | CMU_SYSCLK_CLKSEL_HOSC, CMU_SYSCLK);

#ifdef CONFIG_SOC_SPI_USE_COREPLL
		/* low performance : spi0 clock switch to 24Mhz */
	    soc_freq_set_spi_clk_with_spll(0, 24, 48);
#endif

		/* sys_write32(0x06, CMU_COREPLL_CTL); */ /* FIXME : if no other devices need corepll, corepll can be disable */
		goto set_cpu_clk_finish;
	}

#ifdef CONFIG_SOC_SPI_USE_COREPLL
	/* spi0 clock switch to 48Mhz temporarily */
    soc_freq_set_spi_clk_with_spll(0, 48, 48);
#endif

	/* set divider first */
	sys_write32((1 << 8) | (0xf << 4) | CMU_SYSCLK_CLKSEL_HOSC, CMU_SYSCLK);

	val = ((cpu_mhz + 5) / 6) | (1 << 7);
	sys_write32(val, CMU_COREPLL_CTL);

	if (cpu_clk_src_old != CMU_SYSCLK_CLKSEL_COREPLL) {
		delay_100us();
	}

	/* switch cpu clock to corepll */
	sys_write32((sys_read32(CMU_SYSCLK) & ~0x3) | CMU_SYSCLK_CLKSEL_COREPLL, CMU_SYSCLK);

	/* wait switch over */
	while (!((sys_read32(CMU_SYSCLK) & 0x3) == CMU_SYSCLK_CLKSEL_COREPLL))
		;

set_cpu_clk_finish:

	if (cpu_clk_src_new == CMU_SYSCLK_CLKSEL_COREPLL) {
#ifdef CONFIG_SOC_SPI_USE_COREPLL
		soc_freq_set_spi_clk_with_corepll(0, CONFIG_XSPI_ACTS_FREQ_MHZ, soc_freq_get_corepll_freq());
#endif

#ifdef CONFIG_SOC_SPREAD_SPECTRUM
    	enable_spread_spectrum();
#endif
	}

    irq_unlock(flags);
}

static unsigned int calc_spi_clk_div(unsigned int corepll_freq, unsigned int spi_freq)
{
	int sel;

	if (corepll_freq > spi_freq && corepll_freq <= (spi_freq * 3 / 2)) {
		/* /1.5 */
		sel = 14;
	} else if ((corepll_freq > 2 * spi_freq) && corepll_freq <= (spi_freq * 5 / 2)) {
		/* /2.5 */
		sel = 15;
	} else {
		/* /n */
		sel = (corepll_freq + spi_freq - 1) / spi_freq - 1;
		if (sel > 13) {
			sel = 13;
		}
	}

	return sel;
}

void soc_freq_set_spi_clk_with_corepll(int spi_id, unsigned int spi_freq,
	unsigned int corepll_freq)
{
#ifdef CONFIG_SOC_SPI_USE_COREPLL
	unsigned int sel, reg_val, key;

	sel = calc_spi_clk_div(corepll_freq, spi_freq) & 0xf;

	key = irq_lock();

	if (spi_id == 0) {
		reg_val = sys_read32(CMU_SPI0CLK);
		reg_val &= ~(0x30f);
		reg_val |= (sel << 0) | (0x02 << 8);
		sys_write32(reg_val, CMU_SPI0CLK);
	} else {
		reg_val = sys_read32(CMU_SPI1CLK);
		reg_val &= ~(0x30f);
		reg_val |= (sel << 0) | (0x02 << 8);
		sys_write32(reg_val, CMU_SPI1CLK);
	}

	irq_unlock(key);
#endif
}

void soc_freq_set_spi_clk_with_spll(int spi_id, unsigned int spi_freq,
	unsigned int spll_freq)
{
	unsigned int sel, reg_val, key;

	sel = calc_spi_clk_div(spll_freq, spi_freq) & 0xf;

	if ((sys_read32(VOUT_CTL1) & (1 << 15)) == 0) {
		sys_write32(sys_read32(VOUT_CTL1) | (1 << 15), VOUT_CTL1); /* enable SPLL AVDD */
		k_busy_wait(10);
	}

	if ((sys_read32(SPLL_CTL) & ((1<<6)|(1<<0))) != ((1<<6)|(1<<0))) {
		/* setup spll CK48 */
	    sys_write32(sys_read32(SPLL_CTL)|(1<<6)|(1<<0), SPLL_CTL);
	    k_busy_wait(10);
	}

	key = irq_lock();

	if (spi_id == 0) {
		reg_val = sys_read32(CMU_SPI0CLK);
		reg_val &= ~(0x30f);
		reg_val |= (sel << 0) | (0x03 << 8);
		sys_write32(reg_val, CMU_SPI0CLK);
	} else {
		reg_val = sys_read32(CMU_SPI1CLK);
		reg_val &= ~(0x30f);
		reg_val |= (sel << 0) | (0x03 << 8);
		sys_write32(reg_val, CMU_SPI1CLK);
	}

	irq_unlock(key);
}


void soc_freq_set_spi_clk(int spi_id, unsigned int spi_freq)
{
	if (spi_id == 0) {
#ifdef CONFIG_SOC_SPI_USE_COREPLL
		if ((sys_read32(CMU_SYSCLK) & 0x3) == CMU_SYSCLK_CLKSEL_COREPLL) {
			soc_freq_set_spi_clk_with_corepll(spi_id, spi_freq, soc_freq_get_corepll_freq());
		} else {
			/* spi0 clock switch to 48Mhz temporarily */
    		soc_freq_set_spi_clk_with_spll(spi_id, 48, 48);
    	}
#endif

#ifdef CONFIG_SOC_SPI_USE_SPLL
    	soc_freq_set_spi_clk_with_spll(spi_id, spi_freq, 48);
#endif
	} else {
		soc_freq_set_spi_clk_with_spll(spi_id, spi_freq, 48);
	}
}

unsigned int soc_freq_set_asrc_freq(u32_t asrc_freq)
{
    return 0;
}

unsigned int soc_freq_get_spdif_corepll_freq(u32_t sample_rate)
{
	u32_t core_pll_value = 0;
	
	switch(sample_rate) {
		case SAMPLE_192KHZ:
		case SAMPLE_176KHZ:
		case SAMPLE_96KHZ:
		case SAMPLE_88KHZ:
			// corepll 198MHz,  rxclk  198MHz
			core_pll_value = SPDIF_RX_COREPLL_MHZ;
			break;

		case SAMPLE_64KHZ:
		case SAMPLE_48KHZ:
		case SAMPLE_44KHZ:
			// corepll 198MHz,  , rxclk  198*2/3= 132MHz
			core_pll_value = SPDIF_RX_COREPLL_MHZ;
			break;	

		case SAMPLE_32KHZ:
		case SAMPLE_24KHZ:
		case SAMPLE_22KHZ:
			// corepll 198MHz,  , rxclk  198/3= 66MHz
			core_pll_value = SPDIF_RX_COREPLL_MHZ;
			break;

		case SAMPLE_16KHZ:
			// corepll 48MHz,  , rxclk  48MHz
			core_pll_value = 48;
			break;			

		case SAMPLE_12KHZ:
		case SAMPLE_11KHZ:
			// corepll 36MHz,  , rxclk  36MHz
			core_pll_value = 36;
			break;

		default:
			break;			
	} 

    return core_pll_value;
}

unsigned int soc_freq_set_spdif_freq(u32_t sample_rate)
{
	u8_t  spdif_rx_clk_div = 0;
	u32_t core_pll_value;
	u32_t rxclk = 0;

	core_pll_value = soc_freq_get_spdif_corepll_freq(sample_rate);
	
	switch(sample_rate) {
		case SAMPLE_192KHZ:
		case SAMPLE_176KHZ:
		case SAMPLE_96KHZ:
		case SAMPLE_88KHZ:
			// corepll 198MHz,  rxclk  198MHz
			rxclk = core_pll_value;
			spdif_rx_clk_div = 0;
			break;

		case SAMPLE_64KHZ:
		case SAMPLE_48KHZ:
		case SAMPLE_44KHZ:
			// corepll 198MHz,  , rxclk  198*2/3= 132MHz
			rxclk = core_pll_value*2/3;
			spdif_rx_clk_div = 1;
			break;	

		case SAMPLE_32KHZ:
		case SAMPLE_24KHZ:
		case SAMPLE_22KHZ:
			// corepll 198MHz,  , rxclk  198/3= 66MHz
			rxclk = core_pll_value/3;
			spdif_rx_clk_div = 3;
			break;

		case SAMPLE_16KHZ:
			// corepll 48MHz,  , rxclk  48MHz
			rxclk = core_pll_value;
			spdif_rx_clk_div = 0;
			break;			

		case SAMPLE_12KHZ:
		case SAMPLE_11KHZ:
			// corepll 36MHz,  , rxclk  36MHz
			rxclk = core_pll_value;
			spdif_rx_clk_div = 0;
			break;

		default:
			printk("%d, not support !\n", sample_rate);
			break;			
	} 

    if(core_pll_value){
        sys_write32(((sys_read32(CMU_SPDIFRXCLK) & (~0x03)) \
            | (spdif_rx_clk_div << 0)), CMU_SPDIFRXCLK);
    }

	return rxclk;
}

