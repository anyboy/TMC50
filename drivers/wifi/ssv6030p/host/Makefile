obj-$(CONFIG_WIFI_SSV6030P) += init.o ssv_frame.o macaddr.o custom_cfg.o
obj-$(CONFIG_WIFI_SSV6030P) += app/
obj-$(CONFIG_WIFI_SSV6030P) += netstack_wrapper/
obj-$(CONFIG_WIFI_SSV6030P) += os_wrapper/
obj-$(CONFIG_WIFI_SSV6030P) += platform/

subdir-ccflags-y += -DUSE_ICOMM_LWIP=0 -DKERNEL -DCONFIG_MIPS -Wno-implicit-function-declaration

subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/hal
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/hal/SSV6030
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/hal/SSV6030/firmware
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/hal/SSV6030/regs
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/include
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/include/priv
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/include/priv/hw
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/app
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/app/cli
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/app/cli/cmds
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/app/netapp
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/app/netapp/udhcp
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/app/netmgr
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/app/netmgr/SmartConfig
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/drv
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/drv/sdio
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/os_wrapper/Zephyr
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/platform/actions_ats3503
subdir-ccflags-y += -I$(srctree)/drivers/wifi/ssv6030p/host/netstack_wrapper/zephyr_net
