
SSV_SRC_DIRS := $(SSV_HOST_DIR)

SSV_SRC_DIRS += $(SSV_HOST_DIR)/app/cli
SSV_SRC_DIRS += $(SSV_HOST_DIR)/app/cli/cmds
SSV_SRC_DIRS += $(SSV_HOST_DIR)/app/netapp

SSV_SRC_DIRS += $(SSV_HOST_DIR)/os_wrapper/$(OS_WRAPPER)
SSV_SRC_DIRS += $(SSV_HOST_DIR)/platform/$(CUSTOMER_ID)

ifeq ($(CONFIG_USE_ICOMM_LWIP),y)
SSV_SRC_DIRS += $(SSV_LWIP_DIR)/src/api
SSV_SRC_DIRS += $(SSV_LWIP_DIR)/src/core
SSV_SRC_DIRS += $(SSV_LWIP_DIR)/src/core/ipv4
SSV_SRC_DIRS += $(SSV_LWIP_DIR)/src/core/snmp
SSV_SRC_DIRS += $(SSV_LWIP_DIR)/src/netif
SSV_SRC_DIRS += $(SSV_LWIP_DIR)/ports/icomm
SSV_SRC_DIRS += $(SSV_HOST_DIR)/app/netapp/ping
SSV_SRC_DIRS += $(SSV_HOST_DIR)/app/netapp/httpserver_raw
SSV_SRC_DIRS += $(SSV_HOST_DIR)/app/netapp/udhcp
SSV_SRC_DIRS += $(SSV_HOST_DIR)/netstack_wrapper/icomm_lwip
else
SSV_SRC_DIRS += $(SSV_HOST_DIR)/netstack_wrapper/zephyr_net
endif

SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/main.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/ethermac.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/version.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/app/netapp/udhcp/d6_dhcpc.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/app/netapp/udhcp/d6_packet.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/app/netapp/udhcp/d6_socket.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/app/netapp/udhcp/dhcprelay.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/app/netapp/udhcp/domain_codec.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/app/netapp/udhcp/signalpipe.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/app/netapp/udhcp/dhcpc.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/app/netapp/udhcp/dumpleases.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/app/netapp/httpserver_raw/fsdata.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/app/netapp/getopt.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/app/cli/cmds/cli_cmd_sim.c
SSV_EXCLUDE_FILES +=  $(SSV_HOST_DIR)/app/cli/cmds/cli_cmd_rf_test.c

SSV_CSRC=$(foreach dir, $(SSV_SRC_DIRS), $(wildcard $(dir)/*.c))
SSV_CFILES=$(filter-out $(SSV_EXCLUDE_FILES), $(SSV_CSRC))
SSV_COBJS= $(patsubst %.c, %.o, $(SSV_CFILES))





