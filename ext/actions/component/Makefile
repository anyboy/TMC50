ifeq ($(CONFIG_NETWORKING),y)
ccflags-y += -I${ZEPHYR_BASE}/subsys/net/ip
obj-$(CONFIG_WIFI_MANAGER) += network/
endif

obj-$(CONFIG_DISPLAY) += display/
obj-$(CONFIG_HOTPLUG) += hotplug/
obj-$(CONFIG_INPUT) += input/
obj-$(CONFIG_AUDIO) += audio/
obj-y += media/
obj-$(CONFIG_POWER) += power/
obj-$(CONFIG_PROPERTY) += property/
obj-$(CONFIG_SERIALWITER) += serialwriter/
obj-$(CONFIG_MUTIPLE_VOLUME_MANAGER) += fs_manager/
obj-$(CONFIG_SYSTEM) += system/
obj-$(CONFIG_PLAYTTS) += tts/
obj-$(CONFIG_BT_MANAGER) += bt_manager/
obj-$(CONFIG_ALARM_MANAGER) += alarm_manager/
obj-$(CONFIG_FM) += fm/
obj-$(CONFIG_BT_DONGLE_SERVICE) += asdp/

