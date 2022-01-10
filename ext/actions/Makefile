LIB_INCLUDE_DIR += -L$(ZEPHYR_BASE)/ext/actions/library/$(CONFIG_ARCH)
LIB_INCLUDE_DIR += -L$(ZEPHYR_BASE)/ext/actions/library/$(CONFIG_ARCH)/al
LIB_INCLUDE_DIR += -L$(ZEPHYR_BASE)/ext/actions/library/$(CONFIG_ARCH)/$(CONFIG_SOC_SERIES)

ZEPHYRINCLUDE += -I$(srctree)/ext/actions/include
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/include/audio
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/include/display
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/include/media
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/include/misc
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/include/network
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/include/ota
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/include/stream
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/include/wechat
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/porting/include
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/porting/include/al
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/library/$(CONFIG_ARCH)
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/library/$(CONFIG_ARCH)/al
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/include/bluetooth
ZEPHYRINCLUDE += -I$(srctree)/lib/utils/include
ZEPHYRINCLUDE += -I$(srctree)/lib/utils/include/stream
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/porting/hal/bluetooth/bt_drv/bt_drv_woodpecker_phoenix
-include $(srctree)/ext/actions/private/Makefile

ifdef CONFIG_ACTIONS_ALGORITHMS_LIBRARY

  ifdef CONFIG_ACTIONS_DECODER
    ALL_LIBS += _m1_fmt_p
  endif

  ifdef CONFIG_DECODER_AAC
    ifndef CONFIG_DECODER_FLAC_HW_ACCELERATION
      ALL_LIBS += _m1_a13_d
    endif
  endif

  ifdef CONFIG_DECODER_ACT
    ALL_LIBS += _m1_act_d
  endif

  ifdef CONFIG_DECODER_APE
    ifndef CONFIG_DECODER_APE_HW_ACCELERATION
      ALL_LIBS += _m1_ape_d
    endif
  endif

  ifdef CONFIG_DECODER_CVSD
    ifndef CONFIG_DECODER_CVSD_HW_ACCELERATION
      ALL_LIBS += _m1_cvs_d
    endif
  endif

  ifdef CONFIG_DECODER_FLAC
    ifndef CONFIG_DECODER_FLAC_HW_ACCELERATION
      ALL_LIBS += _m1_fla_d
    endif
  endif

  ifdef CONFIG_DECODER_MP3
    ifndef CONFIG_DECODER_MP3_HW_ACCELERATION
      ALL_LIBS += _m1_mp3_d
    endif
  endif

  ifdef CONFIG_DECODER_SBC
    ifndef CONFIG_DECODER_SBC_HW_ACCELERATION
      ALL_LIBS += _m1_sbc_c
    endif
  endif

  ifdef CONFIG_DECODER_WAV
    ifndef CONFIG_DECODER_WAV_HW_ACCELERATION
      ALL_LIBS += _m1_wav_d
    endif
  endif

  ifdef CONFIG_DECODER_WMA
    ifndef CONFIG_DECODER_WMA_HW_ACCELERATION
      ALL_LIBS += _m1_w13_d
    endif
  endif

  ifdef CONFIG_ENCODER_CVSD
    ifndef CONFIG_ENCODER_CVSD_HW_ACCELERATION
      ALL_LIBS += _m1_cvs_e
    endif
  endif

  ifdef CONFIG_ENCODER_MP3
    ifndef CONFIG_ENCODER_OPUS_HW_ACCELERATION
      ALL_LIBS += _m1_mp2_e
    endif
  endif

  ifdef CONFIG_ENCODER_OPUS
    ifndef CONFIG_ENCODER_OPUS_HW_ACCELERATION
      ALL_LIBS += _m1_opu_e
    endif
  endif

  ifdef CONFIG_ENCODER_WAV
    ifndef CONFIG_ENCODER_WAV_HW_ACCELERATION
      ALL_LIBS += _m1_wav_e
    endif
  endif

  ifdef CONFIG_HFP_PLC
    ALL_LIBS += _m1_plc_p
  endif

  ifdef CONFIG_HFP_SPEECH
    ALL_LIBS += _m1_spe_p
  endif

  ifdef CONFIG_HFP_DAE
    ALL_LIBS += _m1_dae_h_p
  endif

  ifdef CONFIG_MUSIC_DAE
    ALL_LIBS += _m1_dae_s_p
  endif

  ifdef CONFIG_MUSIC_DAE_FADE
    ALL_LIBS += _m1_fad_p
  endif

  ifdef CONFIG_AUDIO_MIX
    ALL_LIBS += _m1_mix_p
  endif

  ifdef CONFIG_RESAMPLE
	  ifdef CONFIG_SF_APS
	    ALL_LIBS += _m1_res_a_p
	    ALL_LIBS += _m1_res_h_p
	  else
	     ifdef CONFIG_OUTPUT_RESAMPLE_SW
	         ALL_LIBS += _m1_res_s_p
	     else
	         ALL_LIBS += _m1_res_h_p
	     endif
	  endif
  endif

endif # CONFIG_ACTIONS_ALGORITHMS_LIBRARY

ifndef CONFIG_SOC_SERIES_ANDES
ALL_LIBS += socoem
endif

ifdef CONFIG_BT_WECHAT
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/include/wechat
endif

ifdef CONFIG_AI_SPEECH
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/external/speech/aispeech/speech_http_api
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/external/speech/aispeech/inc
ZEPHYRINCLUDE += -I$(srctree)/ext/actions/external/speech/aispeech/src
LIB_INCLUDE_DIR += -L$(ZEPHYR_BASE)/ext/actions/external/speech/aispeech
ALL_LIBS += aispeech
endif

ifdef CONFIG_EXTERNAL_WRAPPER
ifdef CONFIG_SAIR_APP_DUER_PROTOCOL
LIB_INCLUDE_DIR += -L$(ZEPHYR_BASE)/ext/actions/external/duer
ALL_LIBS += duer-device
endif

ifdef CONFIG_SAIR_APP_IFLY
LIB_INCLUDE_DIR += -L$(ZEPHYR_BASE)/ext/actions/external/ifly
ALL_LIBS += ifly
endif
endif

ifdef CONFIG_BT_DRV
ALL_LIBS += btcon_phoenix
endif

ZEPHYRINCLUDE += -I$(srctree)/lib/memory/include
