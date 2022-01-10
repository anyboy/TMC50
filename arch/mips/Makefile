KBUILD_CFLAGS += $(call cc-option,-ffunction-sections,) $(call cc-option,-fdata-sections,)
 
KBUILD_CFLAGS += -Os -G0 -EL -gdwarf-2 -gstrict-dwarf -msoft-float -fno-common -minterlink-mips16  -mframe-header-opt
KBUILD_CXXFLAGS += -Os -G0 -EL -gdwarf-2 -gstrict-dwarf -msoft-float -fno-common -minterlink-mips16  -mframe-header-opt
KBUILD_AFLAGS += -Os -G0 -EL -gdwarf-2 -gstrict-dwarf -msoft-float -fno-common

cflags-m4k = $(call cc-option, -mips32r2,)

aflags-$(CONFIG_CPU_MIPS_M4K) += $(cflags-m4k)
cflags-$(CONFIG_CPU_MIPS_M4K) += $(cflags-m4k)
cxxflags-$(CONFIG_CPU_MIPS_M4K) += $(cflags-m4k)

cflags-$(CONFIG_USE_MIPS16E) += -mips16
cxxflags-$-$(CONFIG_USE_MIPS16E) += -mips16

KBUILD_AFLAGS += $(aflags-y)
KBUILD_CFLAGS += $(cflags-y)
KBUILD_CXXFLAGS += $(cflags-y)

include $(srctree)/arch/$(ARCH)/soc/$(SOC_PATH)/Makefile

soc-cxxflags ?= $(soc-cflags)
soc-aflags ?= $(soc-cflags)
KBUILD_CFLAGS += $(soc-cflags)
KBUILD_CXXFLAGS += $(soc-cxxflags)
KBUILD_AFLAGS += $(soc-aflags)

LDFLAGS += -EL
