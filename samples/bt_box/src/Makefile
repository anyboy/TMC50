subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/include
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/tool/
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/btmusic/
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/btcall/
subdir-ccflags-y += -I${ZEPHYR_BASE}/samples/bt_box/src/ota/

ifdef CONFIG_OTA_RECOVERY_APP
obj-y += ota_recovery/
else
obj-y += main/

obj-$(CONFIG_BT_MUSIC_APP) += btmusic/
obj-$(CONFIG_BT_CALL_APP) += btcall/
obj-$(CONFIG_TWS_APP)  += tws/
obj-$(CONFIG_FM_APP) += fm/
obj-$(CONFIG_DMA_APP) += dma/
obj-$(CONFIG_OTA_APP) += ota/
obj-$(CONFIG_TOOL) += tool/
obj-$(CONFIG_ACTIONS_ATT) +=att/

obj-$(CONFIG_LCMUSIC_APP) += lcmusic/
obj-$(CONFIG_LOCAL_PLAYER) += local_player/
obj-$(CONFIG_LOOP_PLAYER_APP) += loop_music/
obj-$(CONFIG_AUDIO_INPUT_APP) += audio_input/
obj-$(CONFIG_USOUND_APP) += usound/
obj-$(CONFIG_RECORD_APP) += record/
obj-$(CONFIG_RECORD_SERVICE) += record_service/
obj-$(CONFIG_ALARM_APP) += alarm/
obj-$(CONFIG_CARD_READER_APP) += card_reader/
obj-$(CONFIG_CHARGER_APP) += charger/
obj-$(CONFIG_GMA_APP) += ais_gma/

obj-$(CONFIG_AGING_PLAYBACK) += aging_playback/

obj-$(CONFIG_BT_BOX_TEST) += test/

endif
