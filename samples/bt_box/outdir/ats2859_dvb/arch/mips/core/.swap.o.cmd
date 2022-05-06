cmd_arch/mips/core/swap.o := /opt/mips-mti-elf/2019.09-01/bin/mips-mti-elf-gcc -Wp,-MMD,arch/mips/core/.swap.o.d  -nostdinc -isystem /opt/mips-mti-elf/2019.09-01/bin/../lib/gcc/mips-mti-elf/7.4.0/include -isystem /opt/mips-mti-elf/2019.09-01/bin/../lib/gcc/mips-mti-elf/7.4.0/include-fixed -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/kernel/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/soc/actions/woodpecker -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/boards/mips/ats2859_dvb  -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/samples/bt_box/outdir/ats2859_dvb/include/generated -include /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/samples/bt_box/outdir/ats2859_dvb/include/generated/autoconf.h  -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/lib/crypto/tinycrypt/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/fs/fat/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/audio -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/display -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/media -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/misc -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/network -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/ota -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/stream -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/wechat -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/porting/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/porting/include/al -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/library/"mips" -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/library/"mips"/al -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/include/bluetooth -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/utils/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/utils/include/stream -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/ext/actions/porting/hal/bluetooth/bt_drv/bt_drv_woodpecker_phoenix -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/memory/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/libc/minimal/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/utils/include -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/utils/include/stream -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/lib/memory/include -DKERNEL -D__ZEPHYR__=1 -c -g -xassembler-with-cpp -D_ASMLANGUAGE -Os -G0 -EL -gdwarf-2 -gstrict-dwarf -msoft-float -fno-common -mips32r2   -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/drivers   -I/home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/drivers   -c -o arch/mips/core/swap.o /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/core/swap.S

source_arch/mips/core/swap.o := /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/core/swap.S

deps_arch/mips/core/swap.o := \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/kernel/include/kernel_structs.h \
    $(wildcard include/config/stack/sentinel.h) \
    $(wildcard include/config/sys/clock/exists.h) \
    $(wildcard include/config/sys/power/management.h) \
    $(wildcard include/config/fp/sharing.h) \
    $(wildcard include/config/thread/monitor.h) \
    $(wildcard include/config/init/stacks.h) \
    $(wildcard include/config/thread/stack/info.h) \
    $(wildcard include/config/thread/custom/data.h) \
    $(wildcard include/config/cpu/load/stat.h) \
    $(wildcard include/config/thread/timer.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/include/kernel.h \
    $(wildcard include/config/kernel/debug.h) \
    $(wildcard include/config/coop/enabled.h) \
    $(wildcard include/config/preempt/enabled.h) \
    $(wildcard include/config/num/coop/priorities.h) \
    $(wildcard include/config/num/preempt/priorities.h) \
    $(wildcard include/config/object/tracing.h) \
    $(wildcard include/config/poll.h) \
    $(wildcard include/config/errno.h) \
    $(wildcard include/config/cpu/task/block/stat.h) \
    $(wildcard include/config/cpu/load/debug.h) \
    $(wildcard include/config/main/stack/size.h) \
    $(wildcard include/config/idle/stack/size.h) \
    $(wildcard include/config/isr/stack/size.h) \
    $(wildcard include/config/system/workqueue/stack/size.h) \
    $(wildcard include/config/printk.h) \
    $(wildcard include/config/x86.h) \
    $(wildcard include/config/sse.h) \
    $(wildcard include/config/tickless/kernel.h) \
    $(wildcard include/config/num/mbox/async/msgs.h) \
    $(wildcard include/config/multithreading.h) \
    $(wildcard include/config/cplusplus.h) \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/include/kernel_arch_data.h \
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
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/include/kernel_arch_thread.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/kernel/include/offsets_short.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/samples/bt_box/outdir/ats2859_dvb/include/generated/offsets.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/include/offsets_short_arch.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/include/mips32_regs.h \
  /home/book/work/release_for_SDK_TAG_ZS2859_1900_20210601/TMC-50_1003/arch/mips/include/mips32_cp0.h \

arch/mips/core/swap.o: $(deps_arch/mips/core/swap.o)

$(deps_arch/mips/core/swap.o):
