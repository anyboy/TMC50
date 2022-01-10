/* Macros for tagging symbols and putting them in the correct sections. */

/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _section_tags__h_
#define _section_tags__h_

#include <toolchain.h>

#if !defined(_ASMLANGUAGE)

#define __noinit		__in_section_unique(NOINIT)
#define __irq_vector_table	_GENERIC_SECTION(IRQ_VECTOR_TABLE)
#define __sw_isr_table		_GENERIC_SECTION(SW_ISR_TABLE)

#if defined(CONFIG_ARM)
#define __kinetis_flash_config_section __in_section_unique(KINETIS_FLASH_CONFIG)
#define __ti_ccfg_section _GENERIC_SECTION(TI_CCFG)
#endif /* CONFIG_ARM */

#define __wifi		__in_section_unique(wifi)

#ifdef CONFIG_XIP
#define __ramfunc	__in_section_unique(ramfunc) __attribute__((noinline))
#else
#define __ramfunc
#endif

#ifdef CONFIG_XIP
#define __coredata	__in_section_unique(coredata) 
#else
#define __coredata
#endif


/* the overlay sections for bluetooth subsystem */
#ifdef CONFIG_XIP
#define __btdrv_ovl_data	__in_section_unique(btdrv_ovl_data)
#define __btdrv_ovl_bss		_NODATA_SECTION(.btdrv_ovl_bss)
#define __btdrv_ovl_text	__in_section_unique(btdrv_ovl_text) __attribute__((noinline))
#else
#define __btdrv_ovl_data
#define __btdrv_ovl_bss
#define __btdrv_ovl_text
#endif

/* the overlay sections for bluetooth subsystem */
#ifdef CONFIG_XIP
#define __scache_ovl_bss	 _NODATA_SECTION(.scache_ovl_bss)
#define __amrpcm_ovl_bss     _NODATA_SECTION(.amrpcm_ovl_bss)
#define __opuspcm_ovl_bss    _NODATA_SECTION(.opuspcm_ovl_bss)
#define __pcm_ovl_bss        _NODATA_SECTION(.pcm_ovl_bss)
#define __sinwav_ovl_bss     _NODATA_SECTION(.sinwav_ovl_bss)
#define __pcm_converter_bss	 _NODATA_SECTION(.pcm_converter_bss)
#define __speexpcm_ovl_bss    _NODATA_SECTION(.speexpcm_ovl_bss)
#define __duer_mem_pool_ovl_bss    _NODATA_SECTION(.duer_mem_pool_ovl_bss)
#define __duerpcm_ovl_bss    _NODATA_SECTION(.duerpcm_ovl_bss)
#define __ifly_ovl_bss    _NODATA_SECTION(.ifly_ovl_bss)
#define __voice_wake_up_ovl_bss    _NODATA_SECTION(.voice_wake_up_ovl_bss)
#define __stack_noinit __in_section_unique(stacknoinit)
#define __hfp_plc_ovl_bss    _NODATA_SECTION(.hfp_plc_ovl_bss)
#define __hfp_aec_ovl_bss    _NODATA_SECTION(.hfp_aec_ovl_bss)
#else
#define __scache_ovl_bss
#define __amrpcm_ovl_bss
#define __opuspcm_ovl_bss
#define __pcm_ovl_bss
#define __sinwav_ovl_bss
#define __pcm_converter_bss
#define __speexpcm_ovl_bss
#define __duer_mem_pool_ovl_bss
#define __duerpcm_ovl_bss
#define __ifly_ovl_bss
#define __voice_wake_up_ovl_bss
#define __stack_noinit
#define __hfp_plc_ovl_bss
#define __hfp_aec_ovl_bss
#endif
#endif /* !_ASMLANGUAGE */

#endif /* _section_tags__h_ */
