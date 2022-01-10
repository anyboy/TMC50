 OUTPUT_FORMAT("elf32-tradlittlemips")
IBANK_SIZE = 2K;
MEMORY
    {
    rom_dependent_ram (wx) : ORIGIN = 0xbfc20000, LENGTH = 0x1000
    FLASH (rx) : ORIGIN = (0xbf000000), LENGTH = (2048*1K)
    SRAM (wx) : ORIGIN = 0xbfc21000, LENGTH = 0x20200
    BT_MEM (wx) : ORIGIN = 0xBFC41200, LENGTH = 0x7600
    ram_mpool (wx) : ORIGIN = 0xbfc48800, LENGTH = 0x3800
    ram_hw_pcmbuf (wx) : ORIGIN = 0xbfc61000, LENGTH = 0x800
    ram_hw_fft (wx) : ORIGIN = 0xbfc60000, LENGTH = 0x1000
    IDT_LIST (wx) : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
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

 _image_rom_start = (0xbf000000);
    text :
 {
 . = 0x0;
 KEEP(*(.img_header))
 *(.reset)
 _vector_start = .;
 _image_text_start = .;
 KEEP(*(.text.soc_get_hosc_cap .text.soc_set_hosc_cap))
 KEEP(*(.text.bt_manager_get_status .text.btif_br_get_dev_rdm_state))
 KEEP(*(.text.sys_pm_poweroff_dc5v))
 KEEP(*(.text.btif_br_set_auto_reconnect_info))
 KEEP(*(.text.bt_manager_is_inited))
 KEEP(*(.text.bt_manager_startup_reconnect))
 *(.text)
 *(".text.*")
 *(.gnu.linkonce.t.*)
 } > FLASH
 _image_text_end = .;
 devconfig () :
 {
  __devconfig_start = .;
  *(".devconfig.*")
  KEEP(*(SORT(".devconfig*")))
  __devconfig_end = .;
 } > FLASH
 initshell () :
 {
  __shell_cmd_start = .; KEEP(*(".shell_*")); __shell_cmd_end = .;
 } > FLASH
 net_l2 () :
 {
  __net_l2_start = .;
  *(".net_l2.init")
  KEEP(*(SORT(".net_l2.init*")))
  __net_l2_end = .;
 } > FLASH
 tracing_backends_sections () : ALIGN_WITH_INPUT
 {
  _tracing_backend_list_start = .;
  KEEP(*("._tracing_backend.*"));
  _tracing_backend_list_end = .;
 } > FLASH
    rodata :
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
  . = ALIGN(4);
  __overlay_table = .;
  LONG(0x4c564f53)
  LONG(13)
  LONG(0x70636c70);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(ABSOLUTE(ADDR(.overlay.data.plc)));
  LONG(SIZEOF(.overlay.data.plc));
  LONG(LOADADDR(.overlay.data.plc));
  LONG(ABSOLUTE(ADDR(.overlay.bss.plc)));
  LONG(__overlay_bss_plc_end - ABSOLUTE(ADDR(.overlay.bss.plc)));
  LONG(0x70656164);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(__overlay_data_apdae_beg);
  LONG(__overlay_data_apdae_end - __overlay_data_apdae_beg);
  LONG(LOADADDR(.overlay.data.adaac) + (__overlay_data_apdae_beg - ABSOLUTE(ADDR(.overlay.data.adaac))));
  LONG(__overlay_bss_apdae_beg);
  LONG(__overlay_bss_apdae_end - __overlay_bss_apdae_beg);
  LONG(0x70646166);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(__overlay_data_apfad_beg);
  LONG(__overlay_data_apfad_end - __overlay_data_apfad_beg);
  LONG(LOADADDR(.overlay.data.adaac) + (__overlay_data_apfad_beg - ABSOLUTE(ADDR(.overlay.data.adaac))));
  LONG(__overlay_bss_apfad_beg);
  LONG(__overlay_bss_apfad_end - __overlay_bss_apfad_beg);
  LONG(0x63746d66);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(ABSOLUTE(ADDR(.overlay.data.fmtchk)));
  LONG(SIZEOF(.overlay.data.fmtchk));
  LONG(LOADADDR(.overlay.data.fmtchk));
  LONG(ABSOLUTE(ADDR(.overlay.bss.fmtchk)));
  LONG(__overlay_bss_fmtchk_end - ABSOLUTE(ADDR(.overlay.bss.fmtchk)));
  LONG(0x64636161);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(ABSOLUTE(ADDR(.overlay.data.adaac)));
  LONG(__overlay_data_adaac_end - ABSOLUTE(ADDR(.overlay.data.adaac)));
  LONG(LOADADDR(.overlay.data.adaac));
  LONG(ABSOLUTE(ADDR(.overlay.bss.adaac)));
  LONG(__overlay_bss_adaac_end - ABSOLUTE(ADDR(.overlay.bss.adaac)));
  LONG(0x64656774);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(ABSOLUTE(ADDR(.overlay.data.adact)));
  LONG(SIZEOF(.overlay.data.adact));
  LONG(LOADADDR(.overlay.data.adact));
  LONG(ABSOLUTE(ADDR(.overlay.bss.adact)));
  LONG(__overlay_bss_adact_end - ABSOLUTE(ADDR(.overlay.bss.adact)));
  LONG(0x64657061);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(ABSOLUTE(ADDR(.overlay.data.adape)));
  LONG(SIZEOF(.overlay.data.adape));
  LONG(LOADADDR(.overlay.data.adape));
  LONG(ABSOLUTE(ADDR(.overlay.bss.adape)));
  LONG(__overlay_bss_adape_end - ABSOLUTE(ADDR(.overlay.bss.adape)));
  LONG(0x64616c66);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(ABSOLUTE(ADDR(.overlay.data.adfla)));
  LONG(SIZEOF(.overlay.data.adfla));
  LONG(LOADADDR(.overlay.data.adfla));
  LONG(ABSOLUTE(ADDR(.overlay.bss.adfla)));
  LONG(__overlay_bss_adfla_end - ABSOLUTE(ADDR(.overlay.bss.adfla)));
  LONG(0x6433706d);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(ABSOLUTE(ADDR(.overlay.data.admp3)));
  LONG(SIZEOF(.overlay.data.admp3));
  LONG(LOADADDR(.overlay.data.admp3));
  LONG(ABSOLUTE(ADDR(.overlay.bss.admp3)));
  LONG(__overlay_bss_admp3_end - ABSOLUTE(ADDR(.overlay.bss.admp3)));
  LONG(0x64766177);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(ABSOLUTE(ADDR(.overlay.data.adwav)));
  LONG(SIZEOF(.overlay.data.adwav));
  LONG(LOADADDR(.overlay.data.adwav));
  LONG(ABSOLUTE(ADDR(.overlay.bss.adwav)));
  LONG(__overlay_bss_adwav_end - ABSOLUTE(ADDR(.overlay.bss.adwav)));
  LONG(0x64616d77);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(ABSOLUTE(ADDR(.overlay.data.adwma)));
  LONG(SIZEOF(.overlay.data.adwma));
  LONG(LOADADDR(.overlay.data.adwma));
  LONG(ABSOLUTE(ADDR(.overlay.bss.adwma)));
  LONG(__overlay_bss_adwma_end - ABSOLUTE(ADDR(.overlay.bss.adwma)));
  LONG(0x6533706d);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(ABSOLUTE(ADDR(.overlay.data.aemp3)));
  LONG(SIZEOF(.overlay.data.aemp3));
  LONG(LOADADDR(.overlay.data.aemp3));
  LONG(ABSOLUTE(ADDR(.overlay.bss.aemp3)));
  LONG(__overlay_bss_aemp3_end - ABSOLUTE(ADDR(.overlay.bss.aemp3)));
  LONG(0x65505553);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(ABSOLUTE(ADDR(.overlay.data.aeopu)));
  LONG(SIZEOF(.overlay.data.aeopu));
  LONG(LOADADDR(.overlay.data.aeopu));
  LONG(ABSOLUTE(ADDR(.overlay.bss.aeopu)));
  LONG(__overlay_bss_aeopu_end - ABSOLUTE(ADDR(.overlay.bss.aeopu)));
  LONG(0x65505553);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(0);
  LONG(ABSOLUTE(ADDR(.overlay.bss.usb_mass)));
  LONG(SIZEOF(.overlay.bss.usb_mass));
  . = ALIGN(4);
 . = ALIGN(4);
 } > FLASH
 _image_rom_end = .;
    __data_rom_start = .;
   
   
 .bss_rom_dependent_ram (NOLOAD) :
 {
  . = 0x80;
  *(.bss.rom_dependent_export_apis_fix)
  . = 0x100;
  *(.bss.rom_dependent_debug_buf)
  . = 0x280;
  KEEP(*(.BTCON_ROM_GDATA))
  *(.interrupt.noinit.stack*)
  . = 0xee0;
  _boot_info_start = .;
  . = 0xf00;
 } >rom_dependent_ram
    datas : ALIGN_WITH_INPUT
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
 . = ALIGN(4);
 KEEP(*(.gnu.linkonce.sw_isr_table))
 . = ALIGN(4);
 KEEP(*(.BTCON_ROM_TEXT))
 KEEP(*(.BTCON_RAM_TEXT))
    KEEP(*(.BTCON_FIX_CODE))
    KEEP(*(.BTCON_FIX_RODATA))
 KEEP(*(SORT(".coredata.*")))
    KEEP(*(SORT(".ramfunc.*")))
    . = ALIGN(4);
    KEEP(*(.gnu.linkonce.sw_isr_table))
 _image_text_ramfunc_end = .;
 KEEP(*(.BTCON_FIX_DATA))
    *(EXCLUDE_FILE (*lib_m1_spe_p.a:* *lib_m1_plc_p.a:* *lib_m1_dae_h_p.a:* *lib_m1_dae_s_p.a:* *lib_m1_fad_p.a:* *lib_m1_fmt_p.a:* *lib_m1_a13_d.a:* *lib_m1_act_d.a:* *lib_m1_ape_d.a:* *lib_m1_cvs_d.a:* *lib_m1_fla_d.a:* *lib_m1_mp3_d.a:* *lib_m1_wav_d.a:* *lib_m1_w13_d.a:* *lib_m1_cvs_e.a:* *lib_m1_mp2_e.a:* *lib_m1_opu_e.a:* *lib_m1_wav_e.a:* *libbt_con.a:*) .data)
    *(EXCLUDE_FILE (*lib_m1_spe_p.a:* *lib_m1_plc_p.a:* *lib_m1_dae_h_p.a:* *lib_m1_dae_s_p.a:* *lib_m1_fad_p.a:* *lib_m1_fmt_p.a:* *lib_m1_a13_d.a:* *lib_m1_act_d.a:* *lib_m1_ape_d.a:* *lib_m1_cvs_d.a:* *lib_m1_fla_d.a:* *lib_m1_mp3_d.a:* *lib_m1_wav_d.a:* *lib_m1_w13_d.a:* *lib_m1_cvs_e.a:* *lib_m1_mp2_e.a:* *lib_m1_opu_e.a:* *lib_m1_wav_e.a:* *libbt_con.a:*) .data.*)
    KEEP(*lib_m1_sbc_c.a:*(.data .data.*))
    KEEP(*lib_m1_wav_e.a:*(.data .data.*))
 } > SRAM AT> FLASH
 initlevel () : ALIGN_WITH_INPUT
 {
  __device_init_start = .; __device_PRE_KERNEL_1_start = .; KEEP(*(SORT(.init_PRE_KERNEL_1[0-9]))); KEEP(*(SORT(.init_PRE_KERNEL_1[1-9][0-9]))); __device_PRE_KERNEL_2_start = .; KEEP(*(SORT(.init_PRE_KERNEL_2[0-9]))); KEEP(*(SORT(.init_PRE_KERNEL_2[1-9][0-9]))); __device_POST_KERNEL_start = .; KEEP(*(SORT(.init_POST_KERNEL[0-9]))); KEEP(*(SORT(.init_POST_KERNEL[1-9][0-9]))); __device_APPLICATION_start = .; KEEP(*(SORT(.init_APPLICATION[0-9]))); KEEP(*(SORT(.init_APPLICATION[1-9][0-9]))); __device_init_end = .;
 } > SRAM AT> FLASH
 initlevel_error () : ALIGN_WITH_INPUT
 {
  KEEP(*(SORT(.init_[_A-Z0-9]*)))
 }
 ASSERT(SIZEOF(initlevel_error) == 0, "Undefined initialization levels used.")
 _static_thread_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _static_thread_data_list_start = .;
  KEEP(*(SORT("._static_thread_data.static.*")))
  _static_thread_data_list_end = .;
 } > SRAM AT> FLASH
 _k_timer_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_timer_list_start = .;
  KEEP(*(SORT("._k_timer.static.*")))
  _k_timer_list_end = .;
 } > SRAM AT> FLASH
 _k_mem_slab_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_mem_slab_list_start = .;
  KEEP(*(SORT("._k_mem_slab.static.*")))
  _k_mem_slab_list_end = .;
 } > SRAM AT> FLASH
 _k_mem_pool_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  KEEP(*(SORT("._k_memory_pool.struct*")))
  _k_mem_pool_list_start = .;
  KEEP(*(SORT("._k_mem_pool.static.*")))
  _k_mem_pool_list_end = .;
 } > SRAM AT> FLASH
 _k_sem_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_sem_list_start = .;
  KEEP(*(SORT("._k_sem.static.*")))
  _k_sem_list_end = .;
 } > SRAM AT> FLASH
 _k_mutex_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_mutex_list_start = .;
  KEEP(*(SORT("._k_mutex.static.*")))
  _k_mutex_list_end = .;
 } > SRAM AT> FLASH
 _k_alert_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_alert_list_start = .;
  KEEP(*(SORT("._k_alert.static.*")))
  _k_alert_list_end = .;
 } > SRAM AT> FLASH
 _k_queue_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_queue_list_start = .;
  KEEP(*(SORT("._k_queue.static.*")))
  _k_queue_list_end = .;
 } > SRAM AT> FLASH
 _k_stack_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_stack_list_start = .;
  KEEP(*(SORT("._k_stack.static.*")))
  _k_stack_list_end = .;
 } > SRAM AT> FLASH
 _k_msgq_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_msgq_list_start = .;
  KEEP(*(SORT("._k_msgq.static.*")))
  _k_msgq_list_end = .;
 } > SRAM AT> FLASH
 _k_mbox_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_mbox_list_start = .;
  KEEP(*(SORT("._k_mbox.static.*")))
  _k_mbox_list_end = .;
 } > SRAM AT> FLASH
 _k_pipe_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_pipe_list_start = .;
  KEEP(*(SORT("._k_pipe.static.*")))
  _k_pipe_list_end = .;
 } > SRAM AT> FLASH
 _k_work_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_work_list_start = .;
  KEEP(*(SORT("._k_work.static.*")))
  _k_work_list_end = .;
 } > SRAM AT> FLASH
 _k_task_list () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_task_list_start = .;
  *(._k_task_list.public.*)
  *(._k_task_list.private.*)
  _k_task_list_idle_start = .;
  *(._k_task_list.idle.*)
  KEEP(*(SORT("._k_task_list*")))
  _k_task_list_end = .;
 } > SRAM AT> FLASH
 _k_event_list () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _k_event_list_start = .;
  *(._k_event_list.event.*)
  KEEP(*(SORT("._k_event_list*")))
  _k_event_list_end = .;
 } > SRAM AT> FLASH
 _k_memory_pool () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  *(._k_memory_pool.struct*)
  KEEP(*(SORT("._k_memory_pool.struct*")))
  _k_mem_pool_start = .;
  *(._k_memory_pool.*)
  KEEP(*(SORT("._k_memory_pool*")))
  _k_mem_pool_end = .;
 } > SRAM AT> FLASH
 _net_buf_pool_area () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  _net_buf_pool_list = .;
  KEEP(*(SORT("._net_buf_pool.static.*")))
  _net_buf_pool_list_end = .;
 } > SRAM AT> FLASH
 net_if () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  __net_if_start = .;
  *(".net_if.*")
  KEEP(*(SORT(".net_if.*")))
  __net_if_end = .;
 } > SRAM AT> FLASH
 net_if_event () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  __net_if_event_start = .;
  *(".net_if_event.*")
  KEEP(*(SORT(".net_if_event.*")))
  __net_if_event_end = .;
 } > SRAM AT> FLASH
 net_l2_data () : ALIGN_WITH_INPUT SUBALIGN(4)
 {
  __net_l2_data_start = .;
  *(".net_l2.data")
  KEEP(*(SORT(".net_l2.data*")))
  __net_l2_data_end = .;
 } > SRAM AT> FLASH
 usb_descriptor () : ALIGN_WITH_INPUT SUBALIGN(1)
 {
  __usb_descriptor_rom_start = .;
  LONG(LOADADDR(usb_descriptor));
  __usb_descriptor_start = .;
  *(".usb.descriptor")
  KEEP(*(SORT(".usb.descriptor*")))
  __usb_descriptor_end = .;
 } > SRAM AT> FLASH
 usb_data () : ALIGN_WITH_INPUT SUBALIGN(1)
 {
  __usb_data_rom_start = .;
  LONG(LOADADDR(usb_data));
  __usb_data_start = .;
  *(".usb.data")
  KEEP(*(SORT(".usb.data*")))
  __usb_data_end = .;
 } > SRAM AT> FLASH
 . = ALIGN(4);
    OVERLAY : NOCROSSREFS
    {
        .overlay.data.fmtchk { *lib_m1_fmt_p.a:*(.data .data.*) }
        .overlay.data.adaac {
         *lib_m1_a13_d.a:*(.data .data.*);
         __overlay_data_adaac_end = .;
        }
        .overlay.data.adact { *lib_m1_act_d.a:*(.data .data.*) }
        .overlay.data.adape { *lib_m1_ape_d.a:*(.data .data.*) }
        .overlay.data.admp3 { *lib_m1_mp3_d.a:*(.data .data.*) }
        .overlay.data.adfla { *lib_m1_fla_d.a:*(.data .data.*) }
        .overlay.data.adwav { *lib_m1_wav_d.a:*(.data .data.*) }
        .overlay.data.adwma {
   *lib_m1_w13_d.a:*(.data .data.*);
         __overlay_data_apdae_beg = .;
         *lib_m1_dae_s_p.a:*(.data .data.*);
         __overlay_data_apdae_end = .;
         __overlay_data_apfad_beg = .;
         *lib_m1_fad_p.a:*(.data .data.*);
         __overlay_data_apfad_end = .;
  }
        .overlay.data.aemp3 { *lib_m1_mp2_e.a:*(.data .data.*) }
        .overlay.data.aeopu { *lib_m1_opu_e.a:*(.data .data.*) }
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
 } > SRAM AT> FLASH
    __data_ram_end = .;
    bss (NOLOAD) : ALIGN_WITH_INPUT
 {
        . = ALIGN(4);
  __bss_start = .;
     *(EXCLUDE_FILE (*libbtcon_phoenix.a:* *lib_m1_spe_p.a:* *lib_m1_plc_p.a:* *lib_m1_dae_h_p.a:* *lib_m1_dae_s_p.a:* *lib_m1_fad_p.a:* *lib_m1_fmt_p.a:* *lib_m1_a13_d.a:* *lib_m1_act_d.a:* *lib_m1_ape_d.a:* *lib_m1_cvs_d.a:* *lib_m1_fla_d.a:* *lib_m1_mp3_d.a:* *lib_m1_wav_d.a:* *lib_m1_w13_d.a:* *lib_m1_cvs_e.a:* *lib_m1_mp2_e.a:* *lib_m1_opu_e.a:* *lib_m1_wav_e.a:* *libbt_con.a:*) .bss)
     *(EXCLUDE_FILE (*libbtcon_phoenix.a:* *lib_m1_spe_p.a:* *lib_m1_plc_p.a:* *lib_m1_dae_h_p.a:* *lib_m1_dae_s_p.a:* *lib_m1_fad_p.a:* *lib_m1_fmt_p.a:* *lib_m1_a13_d.a:* *lib_m1_act_d.a:* *lib_m1_ape_d.a:* *lib_m1_cvs_d.a:* *lib_m1_fla_d.a:* *lib_m1_mp3_d.a:* *lib_m1_wav_d.a:* *lib_m1_w13_d.a:* *lib_m1_cvs_e.a:* *lib_m1_mp2_e.a:* *lib_m1_opu_e.a:* *lib_m1_wav_e.a:* *libbt_con.a:*) .bss.*)
     *(EXCLUDE_FILE (*libbtcon_phoenix.a:* *lib_m1_spe_p.a:* *lib_m1_plc_p.a:* *lib_m1_dae_h_p.a:* *lib_m1_dae_s_p.a:* *lib_m1_fad_p.a:* *lib_m1_fmt_p.a:* *lib_m1_a13_d.a:* *lib_m1_act_d.a:* *lib_m1_ape_d.a:* *lib_m1_cvs_d.a:* *lib_m1_fla_d.a:* *lib_m1_mp3_d.a:* *lib_m1_wav_d.a:* *lib_m1_w13_d.a:* *lib_m1_cvs_e.a:* *lib_m1_mp2_e.a:* *lib_m1_opu_e.a:* *lib_m1_wav_e.a:* *libbt_con.a:*) .scommon)
     *(EXCLUDE_FILE (*libbtcon_phoenix.a:* *lib_m1_spe_p.a:* *lib_m1_plc_p.a:* *lib_m1_dae_h_p.a:* *lib_m1_dae_s_p.a:* *lib_m1_fad_p.a:* *lib_m1_fmt_p.a:* *lib_m1_a13_d.a:* *lib_m1_act_d.a:* *lib_m1_ape_d.a:* *lib_m1_cvs_d.a:* *lib_m1_fla_d.a:* *lib_m1_mp3_d.a:* *lib_m1_wav_d.a:* *lib_m1_w13_d.a:* *lib_m1_cvs_e.a:* *lib_m1_mp2_e.a:* *lib_m1_opu_e.a:* *lib_m1_wav_e.a:* *libbt_con.a:*) COMMON)
 } > SRAM AT> SRAM
    .bt_bss (NOLOAD) : ALIGN_WITH_INPUT {
        *(.bthost_bss*)
        *(.btsrv_bss*)
        *libbtcon_phoenix.a:*(.bss .bss.* .scommon COMMON);
        __bss_end = ALIGN(4);
    } > SRAM
    noinit (NOLOAD) :
 {
        *(.noinit)
        *(".noinit.*")
        *(.bottom_of_image_ram)
        *(.bottom_of_image_ram.*)
    } > SRAM
    .sys_bss (NOLOAD) : ALIGN_WITH_INPUT {
        KEEP(*(SORT(".stacknoinit.*")))
        *(.ESD_DATA*)
  *(.gma.backend.bss*)
    } > SRAM
    .diskio_cache_bss (NOLOAD) : ALIGN_WITH_INPUT {
        *(.diskio.cache.pool*)
        *(.diskio.cache.stack*)
    } > SRAM
    .tracing_bss (NOLOAD) : ALIGN_WITH_INPUT {
        KEEP(*(.tracing.stack.noinit))
        KEEP(*(.tracing.buffer.noinit))
    } > SRAM
 _media_memory_start = .;
    .media_bss (NOLOAD) : ALIGN_WITH_INPUT {
        KEEP(*lib_m1_sbc_c.a:*(.bss .bss.* .scommon COMMON))
        KEEP(*lib_m1_wav_e.a:*(.bss .bss.* .scommon COMMON))
        *(.wav_enc.adpcm.inbuf*)
        *(.wav_enc.adpcm.outbuf*)
  *(.SBC_ENC_BUF*)
  *(.SBC_DEC_BUF*)
        KEEP(*(.SBC_ENC2_BUF*))
        *(.media.buff.noinit*)
    } > SRAM
 _media_al_memory_start = .;
 .codec_stack (NOLOAD) : ALIGN_WITH_INPUT {
  *(.codec.noinit.stack*)
    } > ram_hw_pcmbuf
    OVERLAY : NOCROSSREFS
    {
        .overlay.bss.usb_mass
        {
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
    } > SRAM
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
 } > SRAM
 __overlay_bss_max_end = MAX(__overlay_bss_music_end, __overlay_bss_voice_end);
 _media_memory_end = __overlay_bss_max_end;
 _image_ram_end = _media_memory_end;
    .btcon_bss (NOLOAD) : ALIGN_WITH_INPUT {
  _bt_memory_start = .;
        *(.BTCON_MEMPOOL*)
  *(.btcon.noinit.stack*)
    } > BT_MEM
    .bthost_bss (NOLOAD) : ALIGN_WITH_INPUT {
        KEEP(*(SORT("._bt_buf_pool.noinit.*")))
  KEEP(*(".bss._bt_data_host_tx_pool"))
  *(.bthost.noinit.stack*)
    } > BT_MEM
 _bt_memory_end = .;
 _bt_memory_size = SIZEOF(.btcon_bss) + SIZEOF(.bthost_bss);
 _media_memory_size = _media_memory_end - _media_memory_start;
 _free_memory_size = _bt_memory_start - _image_ram_end;
 __ram_mpool0_start = ORIGIN(ram_mpool);
 __ram_mpool0_size = LENGTH(ram_mpool);
 __ram_mpool0_num = (LENGTH(ram_mpool) + IBANK_SIZE - 1) / IBANK_SIZE;
    _end = .;
   
/DISCARD/ :
{
 KEEP(*(.irq_info))
 KEEP(*(.intList))
}
    }
