/* linker.ld - Linker command/script file */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>
#include <linker/sections.h>
#include <generated_dts_board.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#ifdef CONFIG_SECTION_OVERLAY
#include <section_overlay.h>
#endif

IBANK_SIZE = 2K;

/* physical address of RAM */
#ifdef CONFIG_XIP
  #define ROMABLE_REGION FLASH
  #define RAMABLE_REGION SRAM
  #define BT_MEM_REGION  BT_MEM
#else
  #define ROMABLE_REGION SRAM
  #define RAMABLE_REGION SRAM
#endif


#if defined(CONFIG_XIP)
  #define _DATA_IN_ROM __data_rom_start
#else
  #define _DATA_IN_ROM
#endif

#if !defined(SKIP_TO_KINETIS_FLASH_CONFIG)
  #define SKIP_TO_KINETIS_FLASH_CONFIG
#endif

#define ROM_ADDR (CONFIG_FLASH_BASE_ADDRESS)
#if CONFIG_FLASH_LOAD_SIZE > 0
  #define ROM_SIZE CONFIG_FLASH_LOAD_SIZE
#else
  #define ROM_SIZE (CONFIG_FLASH_SIZE*1K)
#endif

#if defined(CONFIG_XIP)
	#if defined(CONFIG_IS_BOOTLOADER)
		#define RAM_SIZE (CONFIG_BOOTLOADER_SRAM_SIZE * 1K)
		#define RAM_ADDR (CONFIG_SRAM_BASE_ADDRESS + \
			(CONFIG_SRAM_SIZE * 1K - RAM_SIZE))
	#else
		#define RAM_SIZE CONFIG_SRAM_SIZE * 1K
		#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS
	#endif
#else
	#define RAM_SIZE (CONFIG_SRAM_SIZE * 1K)
	#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS
#endif

MEMORY
    {
    rom_dependent_ram     (wx) : ORIGIN = 0xbfc20000, LENGTH = 0x1000
    FLASH                 (rx) : ORIGIN = ROM_ADDR, LENGTH = ROM_SIZE
    SRAM                  (wx) : ORIGIN = RAM_ADDR, LENGTH = 0x20200
    BT_MEM             	  (wx) : ORIGIN = 0xBFC41200, LENGTH = 0x7600
    ram_mpool             (wx) : ORIGIN = 0xbfc48800, LENGTH = 0x3800

    /* PCMBUF1 2k*/
    ram_hw_pcmbuf         (wx) : ORIGIN = 0xbfc61000, LENGTH = 0x800

    /* FFT RAM 4k*/
    ram_hw_fft         (wx) : ORIGIN = 0xbfc60000, LENGTH = 0x1000

    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST  (wx)      : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
    }

SECTIONS
    {
	/DISCARD/ :
	{
		*(.comment)
		*(.eh_frame)
		*(.eh_frame_hdr)
		*(.eh_frame_entry)
		*(.MIPS.abiflags)
		*(.MIPS.options)
		*(.options)
		*(.pdr)
		*(.reginfo)
	}

	GROUP_START(ROMABLE_REGION)

	_image_rom_start = ROM_ADDR;

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
	/* flash image header */
	. = 0x0;
	KEEP(*(.img_header))

	*(.reset)

	_vector_start = .;

	_image_text_start = .;

	/* KEEP for att */
	KEEP(*(.text.soc_get_hosc_cap .text.soc_set_hosc_cap))
	KEEP(*(.text.bt_manager_get_status .text.btif_br_get_dev_rdm_state))
	KEEP(*(.text.sys_pm_poweroff_dc5v))
	KEEP(*(.text.btif_br_set_auto_reconnect_info))
	KEEP(*(.text.bt_manager_is_inited))
	KEEP(*(.text.bt_manager_startup_reconnect))
	/* END of KEEP for att */

	*(.text)
	*(".text.*")
	*(.gnu.linkonce.t.*)
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_text_end = .;

#include <linker/common-rom.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		*(.rodata)
		*(".rodata.*")
		KEEP(*lib_m1_sbc_c.a:*(.rom_data*))
		*(.gnu.linkonce.r.*)

	    . = ALIGN(4);
	    crash_dump_start = .;
	    KEEP(*(SORT(.crash_dump_[_A-Z0-9]*)))
	    crash_dump_end = .;

		. = ALIGN(4);
	    __app_entry_table = .;
	    KEEP(*(.app_entry))
	    __app_entry_end = .;

		. = ALIGN(4);
	    __service_entry_table = .;
	    KEEP(*(.service_entry))
	    __service_entry_end = .;


#ifdef CONFIG_SECTION_OVERLAY
		. = ALIGN(4);
		__overlay_table = .;
		LONG(OVERLAY_TABLE_MAGIC)
		/* overlay items count */
		LONG(13)

		/* for lib_m1_plc_p.a */
		LONG(OVERLAY_ID_LIBHSPLC);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.plc)));
		LONG(SIZEOF(.overlay.data.plc));
		LONG(LOADADDR(.overlay.data.plc));
		LONG(ABSOLUTE(ADDR(.overlay.bss.plc)));
		//LONG(SIZEOF(.overlay.bss.plc));
		LONG(__overlay_bss_plc_end - ABSOLUTE(ADDR(.overlay.bss.plc)));

		/* for lib_m1_dae_s_p.a */
		LONG(OVERLAY_ID_LIBAPDAE);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(__overlay_data_apdae_beg);
		LONG(__overlay_data_apdae_end - __overlay_data_apdae_beg);
		LONG(LOADADDR(.overlay.data.adaac) + (__overlay_data_apdae_beg - ABSOLUTE(ADDR(.overlay.data.adaac))));
		LONG(__overlay_bss_apdae_beg);
		LONG(__overlay_bss_apdae_end - __overlay_bss_apdae_beg);

		/* for lib_m1_fad_p.a */
		LONG(OVERLAY_ID_LIBAPFAD);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(__overlay_data_apfad_beg);
		LONG(__overlay_data_apfad_end - __overlay_data_apfad_beg);
		LONG(LOADADDR(.overlay.data.adaac) + (__overlay_data_apfad_beg - ABSOLUTE(ADDR(.overlay.data.adaac))));
		LONG(__overlay_bss_apfad_beg);
		LONG(__overlay_bss_apfad_end - __overlay_bss_apfad_beg);

		/* for lib_m1_fmt_p.a */
		LONG(OVERLAY_ID_LIBFMTCHK);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.fmtchk)));
		LONG(SIZEOF(.overlay.data.fmtchk));
		LONG(LOADADDR(.overlay.data.fmtchk));
		LONG(ABSOLUTE(ADDR(.overlay.bss.fmtchk)));
		LONG(__overlay_bss_fmtchk_end - ABSOLUTE(ADDR(.overlay.bss.fmtchk)));

		/* for lib_m1_a13_d.a */
		LONG(OVERLAY_ID_LIBADAAC);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.adaac)));
		//LONG(SIZEOF(.overlay.data.adaac));
		LONG(__overlay_data_adaac_end - ABSOLUTE(ADDR(.overlay.data.adaac)));
		LONG(LOADADDR(.overlay.data.adaac));
		LONG(ABSOLUTE(ADDR(.overlay.bss.adaac)));
		//LONG(SIZEOF(.overlay.bss.adaac));
		LONG(__overlay_bss_adaac_end - ABSOLUTE(ADDR(.overlay.bss.adaac)));

		/* for lib_m1_act_d.a */
		LONG(OVERLAY_ID_LIBADACT);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.adact)));
		LONG(SIZEOF(.overlay.data.adact));
		LONG(LOADADDR(.overlay.data.adact));
		LONG(ABSOLUTE(ADDR(.overlay.bss.adact)));
		LONG(__overlay_bss_adact_end - ABSOLUTE(ADDR(.overlay.bss.adact)));

		/* for lib_m1_ape_d.a */
		LONG(OVERLAY_ID_LIBADAPE);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.adape)));
		LONG(SIZEOF(.overlay.data.adape));
		LONG(LOADADDR(.overlay.data.adape));
		LONG(ABSOLUTE(ADDR(.overlay.bss.adape)));
		LONG(__overlay_bss_adape_end - ABSOLUTE(ADDR(.overlay.bss.adape)));

		/* for lib_m1_fla_d.a */
		LONG(OVERLAY_ID_LIBADFLA);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.adfla)));
		LONG(SIZEOF(.overlay.data.adfla));
		LONG(LOADADDR(.overlay.data.adfla));
		LONG(ABSOLUTE(ADDR(.overlay.bss.adfla)));
		LONG(__overlay_bss_adfla_end - ABSOLUTE(ADDR(.overlay.bss.adfla)));

		/* for lib_m1_mp3_d.a */
		LONG(OVERLAY_ID_LIBADMP3);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.admp3)));
		LONG(SIZEOF(.overlay.data.admp3));
		LONG(LOADADDR(.overlay.data.admp3));
		LONG(ABSOLUTE(ADDR(.overlay.bss.admp3)));
		LONG(__overlay_bss_admp3_end - ABSOLUTE(ADDR(.overlay.bss.admp3)));

		/* for lib_m1_wav_d.a */
		LONG(OVERLAY_ID_LIBADWAV);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.adwav)));
		LONG(SIZEOF(.overlay.data.adwav));
		LONG(LOADADDR(.overlay.data.adwav));
		LONG(ABSOLUTE(ADDR(.overlay.bss.adwav)));
		LONG(__overlay_bss_adwav_end - ABSOLUTE(ADDR(.overlay.bss.adwav)));

		/* for lib_m1_w13_d.a */
		LONG(OVERLAY_ID_LIBADWMA);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.adwma)));
		LONG(SIZEOF(.overlay.data.adwma));
		LONG(LOADADDR(.overlay.data.adwma));
		LONG(ABSOLUTE(ADDR(.overlay.bss.adwma)));
		LONG(__overlay_bss_adwma_end - ABSOLUTE(ADDR(.overlay.bss.adwma)));

		/* for lib_m1_mp2_e.a */
		LONG(OVERLAY_ID_LIBAEMP3);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.aemp3)));
		LONG(SIZEOF(.overlay.data.aemp3));
		LONG(LOADADDR(.overlay.data.aemp3));
		LONG(ABSOLUTE(ADDR(.overlay.bss.aemp3)));
		//LONG(SIZEOF(.overlay.bss.aemp3));
		LONG(__overlay_bss_aemp3_end - ABSOLUTE(ADDR(.overlay.bss.aemp3)));

		/* for lib_m1_opu_e.a */
		LONG(OVERLAY_ID_LIBAEOPU);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.data.aeopu)));
		LONG(SIZEOF(.overlay.data.aeopu));
		LONG(LOADADDR(.overlay.data.aeopu));
		LONG(ABSOLUTE(ADDR(.overlay.bss.aeopu)));
		LONG(__overlay_bss_aeopu_end - ABSOLUTE(ADDR(.overlay.bss.aeopu)));

		/* for lib_m1_opu_e.a */
		LONG(OVERLAY_ID_LIBAEOPU);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(0);
		LONG(ABSOLUTE(ADDR(.overlay.bss.usb_mass)));
		LONG(SIZEOF(.overlay.bss.usb_mass));

		/* for lib_m1_wav_e.a */
		//LONG(OVERLAY_ID_LIBAEWAV);
		//LONG(0);
		//LONG(0);
		//LONG(0);
		//LONG(ABSOLUTE(ADDR(.overlay.data.aewav)));
		//LONG(SIZEOF(.overlay.data.aewav));
		//LONG(LOADADDR(.overlay.data.aewav));
		//LONG(ABSOLUTE(ADDR(.overlay.bss.aewav)));
		//LONG(SIZEOF(.overlay.bss.aewav));

		. = ALIGN(4);
#endif

#ifdef CONFIG_CUSTOM_RODATA_LD
/* Located in project source directory */
#include <custom-rodata.ld>
#endif
	/*
	 * For XIP images, in order to avoid the situation when __data_rom_start
	 * is 32-bit aligned, but the actual data is placed right after rodata
	 * section, which may not end exactly at 32-bit border, pad rodata
	 * section, so __data_rom_start points at data and it is 32-bit aligned.
	 *
	 * On non-XIP images this may enlarge image size up to 3 bytes. This
	 * generally is not an issue, since modern ROM and FLASH memory is
	 * usually 4k aligned.
	 */
	. = ALIGN(4);
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_rom_end = .;
    __data_rom_start = .;    /* XIP imaged DATA ROM start addr */

    GROUP_END(ROMABLE_REGION)

    GROUP_START(RAMABLE_REGION)

	.bss_rom_dependent_ram (NOLOAD)	:
	{
		/* BROM use data region */
		. = 0x80;
		*(.bss.rom_dependent_export_apis_fix) /* BROM export apis fix, fix to here */
		. = 0x100;
		*(.bss.rom_dependent_debug_buf) /* BROM trace inner buffer, can be move to other address */
		. = 0x280;
		KEEP(*(.BTCON_ROM_GDATA)) /* BTROM use data region */

		*(.interrupt.noinit.stack*)
		. = 0xee0;
		_boot_info_start = .;
		. = 0xf00; /* at least 0x20 Bytes keep for boot info */
	} >rom_dependent_ram

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
	_image_ram_start = .;
	__data_ram_start = .;
	_image_text_ramfunc_start = .;
	*(.top_of_image_ram)
	*(.top_of_image_ram.*)
	. = 0x0;
	KEEP(*(.BTCON_ROM_GDATA))

	. = 0x180;
	KEEP(*(.exc_vector))

	_vector_end = .;

#if defined(CONFIG_GEN_SW_ISR_TABLE) && defined(CONFIG_IRQ_STAT)
	. = ALIGN(4);
	KEEP(*(SW_ISR_TABLE))
#endif
	. = ALIGN(4);
	/* BT controller code */
	KEEP(*(.BTCON_ROM_TEXT))
	KEEP(*(.BTCON_RAM_TEXT))

    /* BT controller fix code */
    KEEP(*(.BTCON_FIX_CODE))
    KEEP(*(.BTCON_FIX_RODATA))

	KEEP(*(SORT_BY_NAME(".coredata.*")))

    /* sections for function run in ram when XIP is enabled */
    KEEP(*(SORT_BY_NAME(".ramfunc.*")))

#if defined(CONFIG_GEN_SW_ISR_TABLE) && defined(CONFIG_IRQ_STAT)
    . = ALIGN(4);
    KEEP(*(SW_ISR_TABLE))
#endif

	_image_text_ramfunc_end = .;

	KEEP(*(.BTCON_FIX_DATA))

#ifdef CONFIG_SECTION_OVERLAY
    *(EXCLUDE_FILE (*lib_m1_spe_p.a:* *lib_m1_plc_p.a:* *lib_m1_dae_h_p.a:* *lib_m1_dae_s_p.a:* *lib_m1_fad_p.a:* *lib_m1_fmt_p.a:* *lib_m1_a13_d.a:* *lib_m1_act_d.a:* *lib_m1_ape_d.a:* *lib_m1_cvs_d.a:* *lib_m1_fla_d.a:* *lib_m1_mp3_d.a:* *lib_m1_wav_d.a:* *lib_m1_w13_d.a:* *lib_m1_cvs_e.a:* *lib_m1_mp2_e.a:* *lib_m1_opu_e.a:* *lib_m1_wav_e.a:* *libbt_con.a:*) .data)
    *(EXCLUDE_FILE (*lib_m1_spe_p.a:* *lib_m1_plc_p.a:* *lib_m1_dae_h_p.a:* *lib_m1_dae_s_p.a:* *lib_m1_fad_p.a:* *lib_m1_fmt_p.a:* *lib_m1_a13_d.a:* *lib_m1_act_d.a:* *lib_m1_ape_d.a:* *lib_m1_cvs_d.a:* *lib_m1_fla_d.a:* *lib_m1_mp3_d.a:* *lib_m1_wav_d.a:* *lib_m1_w13_d.a:* *lib_m1_cvs_e.a:* *lib_m1_mp2_e.a:* *lib_m1_opu_e.a:* *lib_m1_wav_e.a:* *libbt_con.a:*) .data.*)

    KEEP(*lib_m1_sbc_c.a:*(.data .data.*))
    KEEP(*lib_m1_wav_e.a:*(.data .data.*))
#else
	*(.data)
	*(".data.*")
#endif

#ifdef CONFIG_CUSTOM_RWDATA_LD
/* Located in project source directory */
#include <custom-rwdata.ld>
#endif

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <linker/common-ram.ld>
	. = ALIGN(4);
#ifdef CONFIG_SECTION_OVERLAY
    OVERLAY : NOCROSSREFS
    {
        .overlay.data.fmtchk { *lib_m1_fmt_p.a:*(.data .data.*) }
        .overlay.data.adaac {
        	*lib_m1_a13_d.a:*(.data .data.*);
        	__overlay_data_adaac_end = .;
		#ifdef CONFIG_DECODER_AAC
        	__overlay_data_apdae_beg = .;
        	*lib_m1_dae_s_p.a:*(.data .data.*);
        	__overlay_data_apdae_end = .;
        	__overlay_data_apfad_beg = .;
        	*lib_m1_fad_p.a:*(.data .data.*);
        	__overlay_data_apfad_end = .;
		#endif
        }
        .overlay.data.adact { *lib_m1_act_d.a:*(.data .data.*) }
        .overlay.data.adape { *lib_m1_ape_d.a:*(.data .data.*) }
        .overlay.data.admp3 { *lib_m1_mp3_d.a:*(.data .data.*) }
        .overlay.data.adfla { *lib_m1_fla_d.a:*(.data .data.*) }
        .overlay.data.adwav { *lib_m1_wav_d.a:*(.data .data.*) }
        .overlay.data.adwma {
			*lib_m1_w13_d.a:*(.data .data.*);
		#ifndef CONFIG_DECODER_AAC
        	__overlay_data_apdae_beg = .;
        	*lib_m1_dae_s_p.a:*(.data .data.*);
        	__overlay_data_apdae_end = .;
        	__overlay_data_apfad_beg = .;
        	*lib_m1_fad_p.a:*(.data .data.*);
        	__overlay_data_apfad_end = .;
		#endif
		}
        .overlay.data.aemp3 { *lib_m1_mp2_e.a:*(.data .data.*) }
        .overlay.data.aeopu { *lib_m1_opu_e.a:*(.data .data.*) }
        //.overlay.data.aewav { *lib_m1_wav_e.a:*(.data .data.*) }

        /* FIXME: must place the largest overlay section at the bottom,
         *        to avoid the linker issue.
         */
        .overlay.data.plc {
        	*lib_m1_plc_p.a:*(.data .data.*);
        	*lib_m1_spe_p.a:*(.data .data.*);
        	*lib_m1_dae_h_p.a:*(.data .data.*);

        	__overlay_data_adcvsd_beg = .;
        	*lib_m1_cvs_d.a:*(.data .data.*);
        	__overlay_data_adcvsd_end = .;

        	__overlay_data_aecvsd_beg = .;
        	*lib_m1_cvs_e.a:*(.data .data.*);
        	__overlay_data_aecvsd_end = .;
        }
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

    __data_ram_end = .;
    SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
        /*
         * For performance, BSS section is assumed to be 4 byte aligned and
         * a multiple of 4 bytes
         */
        . = ALIGN(4);
		__bss_start = .;

	#ifdef CONFIG_SECTION_OVERLAY
    	*(EXCLUDE_FILE (*libbtcon_phoenix.a:* *lib_m1_spe_p.a:* *lib_m1_plc_p.a:* *lib_m1_dae_h_p.a:* *lib_m1_dae_s_p.a:* *lib_m1_fad_p.a:* *lib_m1_fmt_p.a:* *lib_m1_a13_d.a:* *lib_m1_act_d.a:* *lib_m1_ape_d.a:* *lib_m1_cvs_d.a:* *lib_m1_fla_d.a:* *lib_m1_mp3_d.a:* *lib_m1_wav_d.a:* *lib_m1_w13_d.a:* *lib_m1_cvs_e.a:* *lib_m1_mp2_e.a:* *lib_m1_opu_e.a:* *lib_m1_wav_e.a:* *libbt_con.a:*) .bss)
    	*(EXCLUDE_FILE (*libbtcon_phoenix.a:* *lib_m1_spe_p.a:* *lib_m1_plc_p.a:* *lib_m1_dae_h_p.a:* *lib_m1_dae_s_p.a:* *lib_m1_fad_p.a:* *lib_m1_fmt_p.a:* *lib_m1_a13_d.a:* *lib_m1_act_d.a:* *lib_m1_ape_d.a:* *lib_m1_cvs_d.a:* *lib_m1_fla_d.a:* *lib_m1_mp3_d.a:* *lib_m1_wav_d.a:* *lib_m1_w13_d.a:* *lib_m1_cvs_e.a:* *lib_m1_mp2_e.a:* *lib_m1_opu_e.a:* *lib_m1_wav_e.a:* *libbt_con.a:*) .bss.*)
    	*(EXCLUDE_FILE (*libbtcon_phoenix.a:* *lib_m1_spe_p.a:* *lib_m1_plc_p.a:* *lib_m1_dae_h_p.a:* *lib_m1_dae_s_p.a:* *lib_m1_fad_p.a:* *lib_m1_fmt_p.a:* *lib_m1_a13_d.a:* *lib_m1_act_d.a:* *lib_m1_ape_d.a:* *lib_m1_cvs_d.a:* *lib_m1_fla_d.a:* *lib_m1_mp3_d.a:* *lib_m1_wav_d.a:* *lib_m1_w13_d.a:* *lib_m1_cvs_e.a:* *lib_m1_mp2_e.a:* *lib_m1_opu_e.a:* *lib_m1_wav_e.a:* *libbt_con.a:*) .scommon)
    	*(EXCLUDE_FILE (*libbtcon_phoenix.a:* *lib_m1_spe_p.a:* *lib_m1_plc_p.a:* *lib_m1_dae_h_p.a:* *lib_m1_dae_s_p.a:* *lib_m1_fad_p.a:* *lib_m1_fmt_p.a:* *lib_m1_a13_d.a:* *lib_m1_act_d.a:* *lib_m1_ape_d.a:* *lib_m1_cvs_d.a:* *lib_m1_fla_d.a:* *lib_m1_mp3_d.a:* *lib_m1_wav_d.a:* *lib_m1_w13_d.a:* *lib_m1_cvs_e.a:* *lib_m1_mp2_e.a:* *lib_m1_opu_e.a:* *lib_m1_wav_e.a:* *libbt_con.a:*) COMMON)
	#else
   		*(.scommon)
		*(.bss)
		*(".bss.*")
		COMMON_SYMBOLS
	#endif
        /*
         * As memory is cleared in words only, it is simpler to ensure the BSS
         * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
		 */
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

    SECTION_DATA_PROLOGUE(.bt_bss,(NOLOAD),) {
        *(.bthost_bss*)
        *(.btsrv_bss*)
        *libbtcon_phoenix.a:*(.bss .bss.* .scommon COMMON);
        __bss_end = ALIGN(4);
    } GROUP_LINK_IN(RAMABLE_REGION)

    SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
	{
        /*
         * This section is used for non-initialized objects that
         * will not be cleared during the boot process.
         */
        *(.noinit)
        *(".noinit.*")
        *(.bottom_of_image_ram)
        *(.bottom_of_image_ram.*)
    } GROUP_LINK_IN(RAMABLE_REGION)

#ifdef CONFIG_OUTPUT_RESAMPLE_SW
    SECTION_DATA_PROLOGUE(.sys_bss,(NOLOAD),) {
        KEEP(*(SORT_BY_NAME(".stacknoinit.*")))
        *(.ESD_DATA*)
		*(.gma.backend.bss*)
		KEEP(*(.bss.trace_print_buffer))
    } GROUP_LINK_IN(ram_hw_fft)
#else
    SECTION_DATA_PROLOGUE(.sys_bss,(NOLOAD),) {
        KEEP(*(SORT_BY_NAME(".stacknoinit.*")))
        *(.ESD_DATA*)
		*(.gma.backend.bss*)
    } GROUP_LINK_IN(RAMABLE_REGION)
#endif

    SECTION_DATA_PROLOGUE(.diskio_cache_bss,(NOLOAD),) {
        *(.diskio.cache.pool*)
        *(.diskio.cache.stack*)
    } GROUP_LINK_IN(RAMABLE_REGION)

    SECTION_DATA_PROLOGUE(.tracing_bss,(NOLOAD),) {
        KEEP(*(.tracing.stack.noinit))
        KEEP(*(.tracing.buffer.noinit))
    } GROUP_LINK_IN(RAMABLE_REGION)

	_media_memory_start = .;
    SECTION_DATA_PROLOGUE(.media_bss,(NOLOAD),) {
        KEEP(*lib_m1_sbc_c.a:*(.bss .bss.* .scommon COMMON))
        KEEP(*lib_m1_wav_e.a:*(.bss .bss.* .scommon COMMON))
        *(.wav_enc.adpcm.inbuf*)
        *(.wav_enc.adpcm.outbuf*)
		*(.SBC_ENC_BUF*)
		*(.SBC_DEC_BUF*)
        KEEP(*(.SBC_ENC2_BUF*))
        *(.media.buff.noinit*)
    } GROUP_LINK_IN(RAMABLE_REGION)

	_media_al_memory_start = .;

	SECTION_DATA_PROLOGUE(.codec_stack,(NOLOAD),) {
		*(.codec.noinit.stack*)
    } GROUP_LINK_IN(ram_hw_pcmbuf)

#ifdef CONFIG_SECTION_OVERLAY
    OVERLAY : NOCROSSREFS
    {
        .overlay.bss.usb_mass
        {
		    /**only used udisk mode ,after tts */
		    *(.usb.mass_storage);
        }

        .overlay.bss.aemp3
        {
        	*lib_m1_mp2_e.a:*(.bss .bss.* .scommon COMMON);
        	__overlay_bss_aemp3_end = .;
        	*(.encoder_ovl_bss);
        }

        .overlay.bss.aeopu
        {
		   *lib_m1_opu_e.a:*(.bss .bss.* .scommon COMMON);
		   __overlay_bss_aeopu_end = .;
        }

        //.overlay.bss.aewav { *lib_m1_wav_e.a:*(.bss .bss.* .scommon COMMON) }

        .overlay.bss.fmtchk
        {
		    *lib_m1_fmt_p.a:*(.bss .bss.* .scommon COMMON);
		    __overlay_bss_fmtchk_end = .;
        }

        .overlay.bss.adpcm
        {
        	*(.decoder_pcm_bss*);
        	*(.btmusic_pcm_bss);
            __overlay_bss_adpcm_end = .;
        }

        .overlay.bss.adact
        {
            *lib_m1_act_d.a:*(.bss .bss.* .scommon COMMON);
            __overlay_bss_adact_end = .;
        }

        .overlay.bss.adwav
        {
		    *lib_m1_wav_d.a:*(.bss .bss.* .scommon COMMON);
		    __overlay_bss_adwav_end = .;
        }

       .overlay.bss.adape
       {
		   *lib_m1_ape_d.a:*(.bss .bss.* .scommon COMMON);
		   __overlay_bss_adape_end = .;
       }

       .overlay.bss.admp3
       {
		    *lib_m1_mp3_d.a:*(.bss .bss.* .scommon COMMON);
		  __overlay_bss_admp3_end = .;
       }

       .overlay.bss.adwma
       {
           *lib_m1_w13_d.a:*(.bss .bss.* .scommon COMMON);
           __overlay_bss_adwma_end = .;
       }

       .overlay.bss.adfla
       {
           *lib_m1_fla_d.a:*(.bss .bss.* .scommon COMMON);
           __overlay_bss_adfla_end = .;
        }

        .overlay.bss.adaac
        {
        	*lib_m1_a13_d.a:*(.bss .bss.* .scommon COMMON);
        	__overlay_bss_adaac_end = .;
        }
        /* FIXME: must place the largest overlay section at the bottom,
         *        to avoid the linker issue.
         */
        .overlay.bss.plc
        {
            *lib_m1_plc_p.a:*(.bss .bss.* .scommon COMMON);
            *lib_m1_spe_p.a:*(.bss .bss.* .scommon COMMON);
            *lib_m1_dae_h_p.a:*(.bss .bss.* .scommon COMMON);

            __overlay_bss_adcvsd_beg = .;
            *lib_m1_cvs_d.a:*(.bss .bss.* .scommon COMMON);
            __overlay_bss_adcvsd_end = .;

            __overlay_bss_aecvsd_beg = .;
            *lib_m1_cvs_e.a:*(.bss .bss.* .scommon COMMON);
            __overlay_bss_aecvsd_end = .;

            __overlay_bss_plc_end = .;

            *(.hfp_plc_ovl_bss)
            *(.hfp_aec_ovl_bss)
            _speech_codec_end = .;
            *(.resample.global_buf_plc)
            *(.resample.share_buf_plc)
            *(.resample.frame_buf_plc)
            *(.tool_asqt_ovl_bss)
            *(tool.asqt.buf.noinit)
            __overlay_bss_voice_end = .;
        }
    }  GROUP_LINK_IN(RAMABLE_REGION)

	_media_al_memory_end = .;

	_media_al_memory_size = _media_al_memory_end - _media_al_memory_start;

	__overlay_bss_music_max_end = MAX(__overlay_bss_adaac_end, __overlay_bss_adfla_end);
	__overlay_bss_music_max_end = MAX(__overlay_bss_music_max_end, __overlay_bss_adwma_end);
	__overlay_bss_music_max_end = MAX(__overlay_bss_music_max_end, __overlay_bss_admp3_end);
	__overlay_bss_music_max_end = MAX(__overlay_bss_music_max_end, __overlay_bss_adape_end);
	__overlay_bss_music_max_end = MAX(__overlay_bss_music_max_end, __overlay_bss_adwav_end);
	__overlay_bss_music_max_end = MAX(__overlay_bss_music_max_end, __overlay_bss_adact_end);
	__overlay_bss_music_max_end = MAX(__overlay_bss_music_max_end, __overlay_bss_adpcm_end);

	.overlay.bss.audio_pp (__overlay_bss_music_max_end) : {
    	_audiopp_memory_start = .;
    	__overlay_bss_apdae_beg = .;
    	*lib_m1_dae_s_p.a:*(.bss .bss.* .scommon COMMON);
    	__overlay_bss_apdae_end = .;
        __overlay_bss_apfad_beg = .;
       	*lib_m1_fad_p.a:*(.bss .bss.* .scommon COMMON);
        __overlay_bss_apfad_end = .;
    	*(.music_dae_ovl_bss)
    	*(.decoder_ovl_bss)
		_audiopp_memory_end = .;
		*(.msbc.dec2.global_bss)
		*(.cascade.extranel_buf)
        *(.resample.voice_global_buf)
		*(.resample.global_buf)
		*(.resample.share_buf)
		*(.resample.frame_buf)
        *(tool.ectt.buf.noinit)
		__overlay_bss_music_end = .;
	} GROUP_LINK_IN(RAMABLE_REGION)
	__overlay_bss_max_end = MAX(__overlay_bss_music_end, __overlay_bss_voice_end);
#else
	__overlay_bss_max_end = .;
#endif
	_media_memory_end = __overlay_bss_max_end;
	_image_ram_end = _media_memory_end;

    SECTION_DATA_PROLOGUE(.btcon_bss,(NOLOAD),) {
		_bt_memory_start = .;
        *(.BTCON_MEMPOOL*)
		*(.btcon.noinit.stack*)
    } GROUP_LINK_IN(BT_MEM_REGION)

    SECTION_DATA_PROLOGUE(.bthost_bss,(NOLOAD),) {
        KEEP(*(SORT_BY_NAME("._bt_buf_pool.noinit.*")))
		KEEP(*(".bss._bt_data_host_tx_pool"))
		*(.bthost.noinit.stack*)
    } GROUP_LINK_IN(BT_MEM_REGION)
	_bt_memory_end = .;
	_bt_memory_size = SIZEOF(.btcon_bss) + SIZEOF(.bthost_bss);
	_media_memory_size = _media_memory_end - _media_memory_start;
	_free_memory_size = _bt_memory_start - _image_ram_end;
    /* Define linker symbols */
#ifdef CONFIG_APP_USED_MEM_PAGE
	__ram_mpool0_start = ORIGIN(ram_mpool);
	__ram_mpool0_size = LENGTH(ram_mpool);
	__ram_mpool0_num = (LENGTH(ram_mpool) + IBANK_SIZE - 1) / IBANK_SIZE;
#endif

    _end = .; /* end of image */

    GROUP_END(RAMABLE_REGION)

#ifdef CONFIG_CUSTOM_SECTIONS_LD
/* Located in project source directory */
#include <custom-sections.ld>
#endif

#ifdef CONFIG_GEN_ISR_TABLES
#include <linker/intlist.ld>
#endif
    }

