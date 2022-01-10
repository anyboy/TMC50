cmd_kernel/section_overlay.o := /opt/mips-mti-elf/2019.09-01/bin/mips-mti-elf-gcc -Wp,-MMD,kernel/.section_overlay.o.d  -nostdinc -isystem /opt/mips-mti-elf/2019.09-01/bin/../lib/gcc/mips-mti-elf/7.4.0/include -isystem /opt/mips-mti-elf/2019.09-01/bin/../lib/gcc/mips-mti-elf/7.4.0/include-fixed -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/kernel/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/arch/mips/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/arch/mips/soc/actions/woodpecker -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/boards/mips/ats2859_dvb  -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/samples/bt_box/outdir/ats2859_dvb/include/generated -include /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/samples/bt_box/outdir/ats2859_dvb/include/generated/autoconf.h  -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/lib/crypto/tinycrypt/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/fs/fat/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/include/audio -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/include/display -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/include/media -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/include/misc -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/include/network -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/include/ota -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/include/stream -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/include/wechat -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/porting/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/porting/include/al -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/library/"mips" -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/library/"mips"/al -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/include/bluetooth -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/utils/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/utils/include/stream -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/ext/actions/porting/hal/bluetooth/bt_drv/bt_drv_woodpecker_phoenix -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/memory/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/libc/minimal/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/utils/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/utils/include/stream -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/memory/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/kernel/memory/include  -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/kernel -Ikernel -DKERNEL -D__ZEPHYR__=1 -c -g -std=c99 -Wall -Wformat -Wformat-security -D_FORTIFY_SOURCE=2 -Wno-format-zero-length -Wno-main -ffreestanding -Werror -Os -fdiagnostics-color -fno-asynchronous-unwind-tables -fno-pic -fno-stack-protector -ffunction-sections -fdata-sections -Os -G0 -EL -gdwarf-2 -gstrict-dwarf -msoft-float -fno-common -minterlink-mips16 -mframe-header-opt -mips32r2 -mips16 -Wno-unused-but-set-variable -fno-reorder-functions -fno-defer-pop -Wno-pointer-sign -fno-strict-overflow -Werror=implicit-int   -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/kernel/include    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(section_overlay)"  -D"KBUILD_MODNAME=KBUILD_STR(section_overlay)" -c -o kernel/section_overlay.o /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/kernel/section_overlay.c

source_kernel/section_overlay.o := /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/kernel/section_overlay.c

deps_kernel/section_overlay.o := \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/kernel.h \
    $(wildcard include/config/kernel/debug.h) \
    $(wildcard include/config/coop/enabled.h) \
    $(wildcard include/config/preempt/enabled.h) \
    $(wildcard include/config/num/coop/priorities.h) \
    $(wildcard include/config/num/preempt/priorities.h) \
    $(wildcard include/config/object/tracing.h) \
    $(wildcard include/config/poll.h) \
    $(wildcard include/config/thread/monitor.h) \
    $(wildcard include/config/sys/clock/exists.h) \
    $(wildcard include/config/thread/stack/info.h) \
    $(wildcard include/config/thread/custom/data.h) \
    $(wildcard include/config/errno.h) \
    $(wildcard include/config/cpu/load/stat.h) \
    $(wildcard include/config/cpu/task/block/stat.h) \
    $(wildcard include/config/thread/timer.h) \
    $(wildcard include/config/cpu/load/debug.h) \
    $(wildcard include/config/main/stack/size.h) \
    $(wildcard include/config/idle/stack/size.h) \
    $(wildcard include/config/isr/stack/size.h) \
    $(wildcard include/config/system/workqueue/stack/size.h) \
    $(wildcard include/config/init/stacks.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/fp/sharing.h) \
    $(wildcard include/config/x86.h) \
    $(wildcard include/config/sse.h) \
    $(wildcard include/config/tickless/kernel.h) \
    $(wildcard include/config/num/mbox/async/msgs.h) \
    $(wildcard include/config/multithreading.h) \
    $(wildcard include/config/cplusplus.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/zephyr/types.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/libc/minimal/include/stdint.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/libc/minimal/include/limits.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/toolchain.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/toolchain/gcc.h \
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
    $(wildcard include/config/csky.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/toolchain/common.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/linker/sections.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/linker/section_tags.h \
    $(wildcard include/config/xip.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/atomic.h \
    $(wildcard include/config/atomic/operations/builtin.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/libc/minimal/include/errno.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/misc/__assert.h \
    $(wildcard include/config/assert.h) \
    $(wildcard include/config/assert/level.h) \
    $(wildcard include/config/actions/trace.h) \
    $(wildcard include/config/assert/show/file/func.h) \
    $(wildcard include/config/assert/show/file.h) \
    $(wildcard include/config/assert/show/func.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/misc/printk.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/libc/minimal/include/inttypes.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/misc/dlist.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/misc/slist.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/libc/minimal/include/stdbool.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/misc/util.h \
    $(wildcard include/config/myfeature.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/kernel_version.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/drivers/rand32.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/arch/mips/include/kernel_arch_thread.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/sys_clock.h \
    $(wildcard include/config/sys/clock/hw/cycles/per/sec.h) \
    $(wildcard include/config/tickless/kernel/time/unit/in/micro/secs.h) \
    $(wildcard include/config/sys/clock/ticks/per/sec.h) \
    $(wildcard include/config/timer/reads/its/frequency/at/runtime.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/arch/cpu.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/arch/mips/arch.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/arch/mips/exc.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/arch/mips/error.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/arch/mips/irq.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/irq.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/sw_isr_table.h \
    $(wildcard include/config/irq/stat.h) \
    $(wildcard include/config/num/irqs.h) \
    $(wildcard include/config/gen/irq/start/vector.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/arch/mips/asm_inline.h \
    $(wildcard include/config/use/mips16e.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/arch/mips/sys_io.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/sys_io.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/libc/minimal/include/string.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/lib/libc/minimal/include/bits/restrict.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/section_overlay.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1002/include/logging/sys_log.h \
    $(wildcard include/config/sys/log/default/level.h) \
    $(wildcard include/config/sys/log/override/level.h) \
    $(wildcard include/config/sys/log.h) \
    $(wildcard include/config/sys/log/ext/hook.h) \
    $(wildcard include/config/sys/log/show/color.h) \
    $(wildcard include/config/sys/log/show/tags.h) \

kernel/section_overlay.o: $(deps_kernel/section_overlay.o)

$(deps_kernel/section_overlay.o):
