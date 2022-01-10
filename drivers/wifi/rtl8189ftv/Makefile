ifeq ($(CONFIG_WIFI_RTL8189FTV),y)
obj-$(CONFIG_WIFI_RTL8189FTV) += app/
obj-$(CONFIG_WIFI_RTL8189FTV) += netif/
obj-$(CONFIG_WIFI_RTL8189FTV) += osdep/
obj-$(CONFIG_WIFI_RTL8189FTV) += sdio/
#obj-$(CONFIG_WIFI_RTL8189FTV) += wlan/

subdir-ccflags-y += -DSYS_LOG_DOMAIN=\"wifi\" -DSYS_LOG_LEVEL=4
subdir-ccflags-y += -I$(srctree)/drivers/wifi/rtl8189ftv/
subdir-ccflags-y += -I$(srctree)/drivers/wifi/rtl8189ftv/app
subdir-ccflags-y += -I$(srctree)/drivers/wifi/rtl8189ftv/netif
endif
