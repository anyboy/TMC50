CC    = $(CROSS_COMPILE)gcc
AR    = $(CROSS_COMPILE)ar
AS    = $(CROSS_COMPILE)as
LD    = $(CROSS_COMPILE)ld
RM    = rm -rf
CP    = cp
MKDIR = mkdir -p

TOP_DIR = ${ZEPHYR_BASE}
WIFI_DIR = $(TOP_DIR)/drivers/wifi/ssv6030p

CUSTOMER_ID=actions_ats3503
DRV_INTERFACE=sdio
OS_WRAPPER=Zephyr

ifeq ($(CONFIG_USE_ICOMM_LWIP),y)
DEFINE = -DUSE_ICOMM_LWIP=1
else
DEFINE = -DUSE_ICOMM_LWIP=0
endif

ifeq ($(USE_EXT_RX_INT),y)
DEFINE += -DCONFIG_WIFI_EXT_RX_INT=1
else
DEFINE += -DCONFIG_WIFI_EXT_RX_INT=0
endif

ifeq ($(USE_DCDC_MODE),y)
DEFINE += -DCONFIG_WIFI_DCDC_MODE=1
else
DEFINE += -DCONFIG_WIFI_DCDC_MODE=0
endif

DEFINE += -DKERNEL -DCONFIG_MIPS

CROSS_COMPILE_PATH = $${CROSS_COMPILE%/*}

CFLAGS		= -nostdinc $(DEFINE) -D__ZEPHYR__=1 -c -g -std=c99 -fno-asynchronous-unwind-tables -Wall -Wformat -Wformat-security -D_FORTIFY_SOURCE=2 -Wno-format-zero-length -Wno-main -ffreestanding -O0 -fno-stack-protector -fno-omit-frame-pointer -ffunction-sections -fdata-sections -O0 -G0 -EL -gdwarf-2 -gstrict-dwarf -mips32r2 -Wno-unused-but-set-variable -fno-reorder-functions -fno-defer-pop -Wno-pointer-sign -fno-strict-overflow -Werror=implicit-int -msoft-float
CFLAGS		+= -isystem $(CROSS_COMPILE_PATH)/../lib/gcc/mips-sde-elf/4.9.1/include
CFLAGS		+= -isystem $(CROSS_COMPILE_PATH)/../lib/gcc/mips-sde-elf/4.9.1/include-fixed

C_INCLUDE_PATH = $C_INCLUDE_PATH:$(CROSS_COMPILE_PATH)/../lib/gcc/mips-sde-elf/4.9.1/include

ifndef OUTDIR
OUTDIR = $(O)
endif

WIFI_OUTDIR = $(OUTDIR)/drivers/wifi/ssv6030p/host
CUSTOMER_OUTPUT	= $(OUTDIR)
CUSTOMER_DIR    = -I$(TOP_DIR)/.
CUSTOMER_DIR	 += -I$(TOP_DIR)/kernel/include
CUSTOMER_DIR	 += -I$(TOP_DIR)/arch/mips/include
CUSTOMER_DIR	 += -I$(TOP_DIR)/arch/mips/soc/$(SOC_PATH)
CUSTOMER_DIR	 += -I$(TOP_DIR)/boards/mips/$(BOARD_NAME)
CUSTOMER_DIR	 += -I$(TOP_DIR)/include
CUSTOMER_DIR	 += -I$(TOP_DIR)/lib/libc/minimal/include
CUSTOMER_DIR	 += -I$(TOP_DIR)/lib/libc/minimal/source/stdout
CUSTOMER_DIR	 += -I$(CUSTOMER_OUTPUT)/include/generated
CUSTOMER_DIR	 += -I$(CUSTOMER_OUTPUT)/misc/generated/sysgen
CUSTOMER_DIR	 += -include $(CUSTOMER_OUTPUT)/include/generated/autoconf.h

SSV_INC_DIR_EXT = $(CUSTOMER_DIR)