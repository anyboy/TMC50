ATT_ROOT := $(CURDIR)/../../../
SDK_ROOT := $(CURDIR)/../../../../../../../

IC_TYPE ?= woodpecker
ARCH ?= mips

LIB = libatt.a
SOURCES	:= $(wildcard *.c) $(wildcard ../*.c)
# SOURCES	+= $(wildcard $(ATT_ROOT)/att_common/*.c)

INCLUDES += -I. -I./..
INCLUDES += -I$(ATT_ROOT)
INCLUDES += -I$(ATT_ROOT)/att_patterns/include -I$(ATT_ROOT)/att_patterns/include/mips-include -I$(ATT_ROOT)/att_patterns/include/gcc-include
INCLUDES += -I$(SDK_ROOT)/arch/mips/include
INCLUDES += -I$(SDK_ROOT)/include
INCLUDES += -I$(SDK_ROOT)/include/arch/mips
INCLUDES += -I$(SDK_ROOT)/include/bluetooth
INCLUDES += -I$(SDK_ROOT)/include/display
INCLUDES += -I$(SDK_ROOT)/include/drivers
INCLUDES += -I$(SDK_ROOT)/include/fs
INCLUDES += -I$(SDK_ROOT)/include/logging
INCLUDES += -I$(SDK_ROOT)/include/misc
INCLUDES += -I$(SDK_ROOT)/include/mmc
INCLUDES += -I$(SDK_ROOT)/include/usb -I$(SDK_ROOT)/include/class
INCLUDES += -I$(SDK_ROOT)/include/usb_actions -I$(SDK_ROOT)/include/usb_actions/class
INCLUDES += -I$(SDK_ROOT)/include/zephyr
INCLUDES += -I$(SDK_ROOT)/ext/actions/include
INCLUDES += -I$(SDK_ROOT)/ext/actions/porting/include -I$(SDK_ROOT)/ext/actions/porting/include/al
INCLUDES += -I$(SDK_ROOT)/ext/actions/include/bluetooth
INCLUDES += -I$(SDK_ROOT)/lib/json/include
INCLUDES += -I$(SDK_ROOT)/lib/memory/include
INCLUDES += -I$(SDK_ROOT)/lib/libc/minimal/include -I$(SDK_ROOT)/lib/libc/minimal/include/bits -I$(SDK_ROOT)/lib/libc/minimal/include/sys
INCLUDES += -I$(SDK_ROOT)/lib/utils/include -I$(SDK_ROOT)/lib/utils/include/stream -I$(SDK_ROOT)/lib/utils/include/iterator
INCLUDES += -I$(SDK_ROOT)/samples/bt_box/src/include

include $(ATT_ROOT)/att_patterns/scripts/$(IC_TYPE).mk
$(eval $(call MAKE_LIB,$(LIB),$(SOURCES)))
