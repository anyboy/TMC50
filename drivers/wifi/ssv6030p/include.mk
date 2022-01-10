SSV_HOST_DIR := $(WIFI_DIR)/host
SSV_LIB_DIR  := $(WIFI_DIR)/host/lib
SSV_LWIP_DIR := $(SSV_HOST_DIR)/tcpip/lwip-1.4.0

SSV_INC_DIR := \
	$(SSV_HOST_DIR) \
	$(SSV_HOST_DIR)/hal \
	$(SSV_HOST_DIR)/hal/SSV6030 \
	$(SSV_HOST_DIR)/hal/SSV6030/firmware \
	$(SSV_HOST_DIR)/hal/SSV6030/regs \
	$(SSV_HOST_DIR)/include \
	$(SSV_HOST_DIR)/include/priv \
	$(SSV_HOST_DIR)/include/priv/hw \
	$(SSV_HOST_DIR)/app \
	$(SSV_HOST_DIR)/app/cli \
	$(SSV_HOST_DIR)/app/cli/cmds \
	$(SSV_HOST_DIR)/app/netapp \
	$(SSV_HOST_DIR)/app/netapp/udhcp \
	$(SSV_HOST_DIR)/app/netmgr \
	$(SSV_HOST_DIR)/app/netmgr/SmartConfig \
	$(SSV_HOST_DIR)/drv \
	$(SSV_HOST_DIR)/drv/sdio \
	$(SSV_HOST_DIR)/ap \
	$(SSV_HOST_DIR)/ap/common \
	$(SSV_HOST_DIR)/core \

ifeq ($(CONFIG_USE_ICOMM_LWIP),y)
SSV_INC_DIR += \
	$(SSV_HOST_DIR)/netstack_wrapper/icomm_lwIP \
	$(SSV_LWIP_DIR) \
	$(SSV_LWIP_DIR)/src/include \
	$(SSV_LWIP_DIR)/ports/icomm/include \
	$(SSV_LWIP_DIR)/src/include/ipv4 \
	$(SSV_LWIP_DIR)/src/include \
	$(SSV_LWIP_DIR)/src/include/lwip \
	$(SSV_LWIP_DIR)/src/include/netif \
	$(SSV_LWIP_DIR)/ports/icomm/include/arch
else
SSV_INC_DIR += \
	$(SSV_HOST_DIR)/netstack_wrapper/zephyr_net
endif


SSV_INC_DIR += $(SSV_HOST_DIR)/os_wrapper/$(OS_WRAPPER)
SSV_INC_DIR += $(SSV_HOST_DIR)/platform/$(CUSTOMER_ID)
SSV_INC_DIR += $(SSV_HOST_DIR)/drv/$(DRV_INTERFACE)/$(CUSTOMER_ID)

SSV_INC_DIR := $(foreach ssvincdir,$(SSV_INC_DIR),$(addprefix -I,$(ssvincdir)))

