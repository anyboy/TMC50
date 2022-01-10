obj-y += img_header.o soc.o soc_pinmux.o soc_reset.o soc_clock.o \
	 soc_pm.o soc_debug.o soc_cmu.o soc_pmu.o soc_freq.o soc_memctrl.o soc_sleep.o \
	 soc_se_crc.o soc_se_aes.o soc_se_trng.o
	 
obj-$(CONFIG_SOC_DVFS) += soc_dvfs.o

obj-$(CONFIG_SOC_SPICACHE_PROFILE) += spicache.o
obj-y += soc_psram_mapping.o

LIB_INCLUDE_DIR += -L$(ZEPHYR_BASE)/arch/mips/soc/actions/woodpecker
ALL_LIBS += socatp

#include $(srctree)/arch/mips/soc/actions/andes/private/Makefile
