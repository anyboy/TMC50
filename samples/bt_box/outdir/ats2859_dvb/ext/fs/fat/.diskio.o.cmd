cmd_ext/fs/fat/diskio.o := /opt/mips-mti-elf/2019.09-01/bin/mips-mti-elf-gcc -Wp,-MMD,ext/fs/fat/.diskio.o.d  -nostdinc -isystem /opt/mips-mti-elf/2019.09-01/bin/../lib/gcc/mips-mti-elf/7.4.0/include -isystem /opt/mips-mti-elf/2019.09-01/bin/../lib/gcc/mips-mti-elf/7.4.0/include-fixed -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/kernel/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/boards/mips/ats2859_dvb  -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/samples/bt_box/outdir/ats2859_dvb/include/generated -include /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/samples/bt_box/outdir/ats2859_dvb/include/generated/autoconf.h  -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/lib/crypto/tinycrypt/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/fs/fat/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/audio -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/display -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/media -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/misc -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/network -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/ota -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/stream -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/wechat -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/porting/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/porting/include/al -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/library/"mips" -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/library/"mips"/al -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/bluetooth -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/utils/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/utils/include/stream -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/porting/hal/bluetooth/bt_drv/bt_drv_woodpecker_phoenix -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/memory/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/utils/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/utils/include/stream -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/memory/include  -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/fs/fat -Iext/fs/fat -DKERNEL -D__ZEPHYR__=1 -c -g -std=c99 -Wall -Wformat -Wformat-security -D_FORTIFY_SOURCE=2 -Wno-format-zero-length -Wno-main -ffreestanding -Werror -Os -fdiagnostics-color -fno-asynchronous-unwind-tables -fno-pic -fno-stack-protector -ffunction-sections -fdata-sections -Os -G0 -EL -gdwarf-2 -gstrict-dwarf -msoft-float -fno-common -minterlink-mips16 -mframe-header-opt -mips32r2 -mips16 -Wno-unused-but-set-variable -fno-reorder-functions -fno-defer-pop -Wno-pointer-sign -fno-strict-overflow -Werror=implicit-int    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(diskio)"  -D"KBUILD_MODNAME=KBUILD_STR(diskio)" -c -o ext/fs/fat/diskio.o /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/fs/fat/diskio.c

source_ext/fs/fat/diskio.o := /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/fs/fat/diskio.c

deps_ext/fs/fat/diskio.o := \
    $(wildcard include/config/diskio/cache.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/fs/fat/include/diskio.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/disk_access.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/zephyr/types.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include/stdint.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/fs/fat/include/integer.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/fs/fat/include/ffconf.h \
    $(wildcard include/config/support/gbk/display.h) \
    $(wildcard include/config/long/file/name.h) \
    $(wildcard include/config/fat/filesystem/elm/utf8.h) \
    $(wildcard include/config/xsfn/opt.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include/errno.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/logging/sys_log.h \
    $(wildcard include/config/sys/log/default/level.h) \
    $(wildcard include/config/sys/log/override/level.h) \
    $(wildcard include/config/sys/log.h) \
    $(wildcard include/config/sys/log/ext/hook.h) \
    $(wildcard include/config/sys/log/show/color.h) \
    $(wildcard include/config/sys/log/show/tags.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/misc/printk.h \
    $(wildcard include/config/assert.h) \
    $(wildcard include/config/assert/show/file/func.h) \
    $(wildcard include/config/assert/show/file.h) \
    $(wildcard include/config/assert/show/func.h) \
    $(wildcard include/config/printk.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/toolchain.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/toolchain/gcc.h \
    $(wildcard include/config/arm.h) \
    $(wildcard include/config/application/memory.h) \
    $(wildcard include/config/isa/thumb.h) \
    $(wildcard include/config/isa/thumb2.h) \
    $(wildcard include/config/isa/arm.h) \
    $(wildcard include/config/nios2.h) \
    $(wildcard include/config/riscv32.h) \
    $(wildcard include/config/xtensa.h) \
    $(wildcard include/config/mips.h) \
    $(wildcard include/config/arc.h) \
    $(wildcard include/config/x86.h) \
    $(wildcard include/config/csky.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/toolchain/common.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include/inttypes.h \

ext/fs/fat/diskio.o: $(deps_ext/fs/fat/diskio.o)

$(deps_ext/fs/fat/diskio.o):
