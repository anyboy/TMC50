cmd_drivers/mmc/sd.o := /opt/mips-mti-elf/2019.09-01/bin/mips-mti-elf-gcc -Wp,-MMD,drivers/mmc/.sd.o.d  -nostdinc -isystem /opt/mips-mti-elf/2019.09-01/bin/../lib/gcc/mips-mti-elf/7.4.0/include -isystem /opt/mips-mti-elf/2019.09-01/bin/../lib/gcc/mips-mti-elf/7.4.0/include-fixed -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/kernel/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/boards/mips/ats2859_dvb  -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/samples/bt_box/outdir/ats2859_dvb/include/generated -include /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/samples/bt_box/outdir/ats2859_dvb/include/generated/autoconf.h  -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/lib/crypto/tinycrypt/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/fs/fat/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/audio -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/display -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/media -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/misc -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/network -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/ota -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/stream -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/wechat -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/porting/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/porting/include/al -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/library/"mips" -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/library/"mips"/al -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/bluetooth -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/utils/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/utils/include/stream -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/porting/hal/bluetooth/bt_drv/bt_drv_woodpecker_phoenix -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/memory/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/utils/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/utils/include/stream -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/memory/include  -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/drivers/mmc -Idrivers/mmc -DKERNEL -D__ZEPHYR__=1 -c -g -std=c99 -Wall -Wformat -Wformat-security -D_FORTIFY_SOURCE=2 -Wno-format-zero-length -Wno-main -ffreestanding -Werror -Os -fdiagnostics-color -fno-asynchronous-unwind-tables -fno-pic -fno-stack-protector -ffunction-sections -fdata-sections -Os -G0 -EL -gdwarf-2 -gstrict-dwarf -msoft-float -fno-common -minterlink-mips16 -mframe-header-opt -mips32r2 -mips16 -Wno-unused-but-set-variable -fno-reorder-functions -fno-defer-pop -Wno-pointer-sign -fno-strict-overflow -Werror=implicit-int   -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/drivers    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(sd)"  -D"KBUILD_MODNAME=KBUILD_STR(sd)" -c -o drivers/mmc/sd.o /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/drivers/mmc/sd.c

source_drivers/mmc/sd.o := /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/drivers/mmc/sd.c

deps_drivers/mmc/sd.o := \
    $(wildcard include/config/sys/log/mmc/level.h) \
    $(wildcard include/config/mmc/sdcard/show/perf.h) \
    $(wildcard include/config/mmc/sdcard/low/power.h) \
    $(wildcard include/config/mmc/sdcard/low/power/sleep.h) \
    $(wildcard include/config/mmc/sdcard/err/retry/num.h) \
    $(wildcard include/config/mmc/sdcard/retry/times.h) \
    $(wildcard include/config/mmc/sdcard/mmc/dev/name.h) \
    $(wildcard include/config/mmc/sdcard.h) \
    $(wildcard include/config/mmc/sdcard/dev/name.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/kernel.h \
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
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/zephyr/types.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include/stdint.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include/limits.h \
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
    $(wildcard include/config/csky.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/toolchain/common.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/linker/sections.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/linker/section_tags.h \
    $(wildcard include/config/xip.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/atomic.h \
    $(wildcard include/config/atomic/operations/builtin.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include/errno.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/misc/__assert.h \
    $(wildcard include/config/assert.h) \
    $(wildcard include/config/assert/level.h) \
    $(wildcard include/config/actions/trace.h) \
    $(wildcard include/config/assert/show/file/func.h) \
    $(wildcard include/config/assert/show/file.h) \
    $(wildcard include/config/assert/show/func.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/misc/printk.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include/inttypes.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/misc/dlist.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/misc/slist.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include/stdbool.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/misc/util.h \
    $(wildcard include/config/myfeature.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/kernel_version.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/drivers/rand32.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/include/kernel_arch_thread.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/sys_clock.h \
    $(wildcard include/config/sys/clock/hw/cycles/per/sec.h) \
    $(wildcard include/config/tickless/kernel/time/unit/in/micro/secs.h) \
    $(wildcard include/config/sys/clock/ticks/per/sec.h) \
    $(wildcard include/config/timer/reads/its/frequency/at/runtime.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/arch/cpu.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/arch/mips/arch.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/arch/mips/exc.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/arch/mips/error.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/arch/mips/irq.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/irq.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/sw_isr_table.h \
    $(wildcard include/config/irq/stat.h) \
    $(wildcard include/config/num/irqs.h) \
    $(wildcard include/config/gen/irq/start/vector.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/arch/mips/asm_inline.h \
    $(wildcard include/config/use/mips16e.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/arch/mips/sys_io.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/sys_io.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/init.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/device.h \
    $(wildcard include/config/kernel/init/priority/default.h) \
    $(wildcard include/config/device/power/management.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include/string.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include/bits/restrict.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/disk_access.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/flash.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include/sys/types.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/gpio.h \
    $(wildcard include/config/idx.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/misc/byteorder.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/memory/include/mem_manager.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/mmc/mmc.h \
    $(wildcard include/config/acc/mask.h) \
    $(wildcard include/config/acc/boot0.h) \
    $(wildcard include/config/acc/rpmb.h) \
    $(wildcard include/config/acc/gp0.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/mmc/sd.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/boards/mips/ats2859_dvb/board.h \
    $(wildcard include/config/audio/in/i2srx0/support.h) \
    $(wildcard include/config/i2s/5wire.h) \
    $(wildcard include/config/i2s/pseudo/5wire.h) \
    $(wildcard include/config/input/dev/acts/irkey.h) \
    $(wildcard include/config/panel/st7789v.h) \
    $(wildcard include/config/outside/flash.h) \
    $(wildcard include/config/panel/ssd1306.h) \
    $(wildcard include/config/encoder/input.h) \
    $(wildcard include/config/input/dev/acts/adc/sr.h) \
    $(wildcard include/config/usp.h) \
    $(wildcard include/config/acts/timer3/capture.h) \
    $(wildcard include/config/soc/series/woodpeckerfpga.h) \
    $(wildcard include/config/audio/out/spdiftx/support.h) \
    $(wildcard include/config/audio/in/spdifrx/support.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/ext_types.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_regs.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_clock.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_reset.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_irq.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_gpio.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_pinmux.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_pm.h \
    $(wildcard include/config/pm/sleep/time/trace.h) \
    $(wildcard include/config/pm/cpu/idle/low/power.h) \
    $(wildcard include/config/auto/standby/s2/lowfreq.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_adc.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_dma.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_cmu.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_memctrl.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_pmu.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_cache.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_asrc.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_ui.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_timer.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_dsp.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_uart.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_spi.h \
    $(wildcard include/config/hosc/clk/mhz.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_atp.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_debug.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_freq.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_boot.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_se.h \
    $(wildcard include/config/crc32/mode.h) \
    $(wildcard include/config/crc16/mode.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/dma.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/soc_watchdog.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker/brom_interface.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/drivers/mmc/mmc_ops.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/logging/sys_log.h \
    $(wildcard include/config/sys/log/default/level.h) \
    $(wildcard include/config/sys/log/override/level.h) \
    $(wildcard include/config/sys/log.h) \
    $(wildcard include/config/sys/log/ext/hook.h) \
    $(wildcard include/config/sys/log/show/color.h) \
    $(wildcard include/config/sys/log/show/tags.h) \

drivers/mmc/sd.o: $(deps_drivers/mmc/sd.o)

$(deps_drivers/mmc/sd.o):