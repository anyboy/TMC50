ccflags-y += -I${srctree}/include/usb -I${srctree}/subsys/usb

obj-$(CONFIG_USB_CDC_ACM) += cdc_acm.o
obj-$(CONFIG_USB_MASS_STORAGE) += mass_storage.o
obj-$(CONFIG_USB_DEVICE_BLUETOOTH) += bluetooth.c
obj-$(CONFIG_USB_DFU_CLASS) += usb_dfu.c

obj-$(CONFIG_USB_DEVICE_NETWORK) += netusb/
obj-$(CONFIG_USB_DEVICE_HID) += hid/
obj-$(CONFIG_USB_DEVICE_SS) += sourcesink/
obj-$(CONFIG_USB_DEVICE_STUB) += stub/


obj-$(CONFIG_USB_AUDIO_SOURCESINK) += audio_sourcesink/
