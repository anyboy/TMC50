obj- += dummy.o
obj-$(CONFIG_AUDIO_SYSTEM) += audio_system/

SDK_LIB_OUT_DIR=$(ZEPHYR_BASE)/ext/actions/library/$(ARCH)
SDK_LIB_OBJ_DIR=$(O)/ext/actions/private
SDK_LIB=

PRIVATE_LIB_OUT_DIR=$(ZEPHYR_BASE)/ext/actions/library/$(ARCH)
PRIVATE_LIB_OBJ_DIR=$(O)/ext/actions/private/libbt_service
PRIVATE_LIB=

ifdef CONFIG_OTA_UPGRADE
ifneq ($(wildcard $(srctree)/ext/actions/private/libota/Makefile),)
obj-y += libota/
SDK_LIB += libota

else
LINK_SYS_LIBS += $(SDK_LIB_OUT_DIR)/libota.a
endif
endif

ifdef CONFIG_BT_SERVICE
ifneq ($(wildcard $(srctree)/ext/actions/private/libbt_service/Makefile),)
obj-y += libbt_service/
SDK_LIB += libbt_service

ifdef CONFIG_TWS
ifneq ($(wildcard $(srctree)/ext/actions/private/libbt_service/libtws/Makefile),)
obj-y += libbt_service/libtws/
PRIVATE_LIB += libtws
else
LINK_SYS_LIBS += $(SDK_LIB_OUT_DIR)/libtws.a
endif
endif

else
LINK_SYS_LIBS += $(SDK_LIB_OUT_DIR)/libbt_service.a
endif
endif


ifdef CONFIG_BT_STACK
ifneq ($(wildcard $(srctree)/ext/actions/private/libbt_stack/Makefile),)
obj-y += libbt_stack/
SDK_LIB += libbt_stack
else
LINK_SYS_LIBS += $(SDK_LIB_OUT_DIR)/libbt_stack.a
endif
endif


ifdef CONFIG_MEDIA_SERVICE
ifneq ($(wildcard $(srctree)/ext/actions/private/libmedia/Makefile),)
obj-y += libmedia/
SDK_LIB += libmedia
else
LINK_SYS_LIBS += $(SDK_LIB_OUT_DIR)/libmedia.a
endif
endif

ifdef CONFIG_USP
obj-y += libusp/
endif