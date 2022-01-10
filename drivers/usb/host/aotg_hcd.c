/**
 * usb host(aotg) driver
 *
 * For now, only support control transfers & bulk transfers.
 *
 * Something need to know:
 * 1. using polling for control transfers;
 * 2. using irq+cpu for bulk-in transfers(could be irq+thread), polling for bulk-out;
 * 3. using 1 fifo for bulk-in & bulk-out ep respectively;
 * 4. using ep1-in for bulk-in and ep2-out for bulk-out;
 * 5. too many magic numbers, need to fix!
 */

#include <kernel.h>
#include <sys_io.h>
#include <board.h>
#include <init.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/stack.h>
#include <drivers/ioapic.h>
#include <drivers/usb/usb_aotg_hcd.h>

#include "aotg_hcd_registers.h"

/* auto fifo mode: normally used for DMA */
//#define AOTG_AUTO_MODE

/* using a thread to receive data */
//#define AOTG_THREAD_READ

/* handling ep0 error */
//#define ENABLE_HANDLE_EP0_ERR

/* using irq for ep0 instead of polling */
//#define AOTG_IRQ_FOR_EP0

/* print debug info in aotg isr */
//#define AOTG_DEBUG_ISR

/**
 * polling timeout for aotg write(unit: ms).
 * FIMXE: default 100ms, need to optimize!
 */
#define AOTG_WRITE_TIMEOUT	100


static struct k_sem aotg_read_sem;
#ifdef AOTG_IRQ_FOR_EP0
static struct k_sem aotg_ep0_sem;
#endif

#ifdef AOTG_THREAD_READ
#define AOTG_STACK_SIZE (K_THREAD_SIZEOF + 500)
#define AOTG_PRIORITY 5

static char __noinit __stack aotg_stack_area[AOTG_STACK_SIZE];

struct k_work_q aotg_work_q;
#endif

static struct aotg_device_info {
#ifdef AOTG_THREAD_READ
	struct k_work work;
#endif
	u32_t *buf;
	u32_t len;
	u16_t num;	/* the number of IN Packets */
} aotg_device;

/* aotg hcd -> usb host -> usb storage */
#define AOTG_RW_OK		0
#define AOTG_RW_TIMEOUT -2
#define	AOTG_RW_ERROR	-3

static int read_result;
static int write_result;

/* rw mode flag   */
#define READ_MODE				0x00
#define WRITE_MODE				0x01

typedef volatile u32_t *REG32;
typedef volatile u16_t *REG16;
typedef volatile u8_t *REG8;

static inline u8_t aotg_read8(u32_t addr)
{
	return *(volatile u8_t *)addr;
}

static inline void aotg_write8(u32_t addr, u8_t val)
{
	*(volatile u8_t *)addr = val;
}

static inline u16_t aotg_read16(u32_t addr)
{
	return *(volatile u16_t *)addr;
}

static inline void aotg_write16(u32_t addr, u16_t val)
{
	*(volatile u16_t *)addr = val;
}

static inline u32_t aotg_read32(u32_t addr)
{
	return sys_read32(addr);
}

static inline void aotg_write32(u32_t addr, u32_t val)
{
	sys_write32(val, addr);
}

static inline void aotg_set_bits8(u32_t addr, u8_t val)
{
	u8_t temp = *(volatile u8_t *)addr;

	*(volatile u8_t *)addr = temp | val;
}

static inline void aotg_clear_bits8(u32_t addr, u8_t val)
{
	u8_t temp = *(volatile u8_t *)addr;

	*(volatile u8_t *)addr = temp & ~(val);
}

static inline void aotg_set_bit8(u32_t addr, u8_t val)
{
	u8_t temp = *(volatile u8_t *)addr;

	*(volatile u8_t *)addr = temp | (1 << val);
}

static inline void aotg_clear_bit8(u32_t addr, u8_t val)
{
	u8_t temp = *(volatile u8_t *)addr;

	*(volatile u8_t *)addr = temp & ~(1 << val);
}

static inline void aotg_set_bit16(u32_t addr, u16_t val)
{
	u16_t temp = *(volatile u16_t *)addr;

	*(volatile u16_t *)addr = temp | (1 << val);
}

static inline void aotg_clear_bit16(u32_t addr, u16_t val)
{
	u16_t temp = *(volatile u16_t *)addr;

	*(volatile u16_t *)addr = temp & ~(1 << val);
}

static inline void aotg_set_bit32(u32_t addr, u32_t val)
{
	sys_set_bit(addr, val);
}

static inline void aotg_clear_bit32(u32_t addr, u32_t val)
{
	sys_clear_bit(addr, val);
}

void aotg_set_ep0_max_packet_size(u8_t size)
{
	aotg_write8(HCIN0MAXPCK, size);
}

void aotg_controller_reset(void)
{
	u8_t times;

	/*
	 * Reset USB controller and enable USB clock.
	 * steps:
	 * 1. USB Reset2 & USB Reset1;
	 * 2. USB Reset2 back to normal;
	 * 3. USB clock enable;
	 * 4. USB Reset1 back to normal;
	 * 5. wait for reset done
	 */
	aotg_clear_bit32(MRCR, USB_RESET1);
	aotg_clear_bit32(MRCR, USB_RESET2);

	for (times = 8; times != 0; times--) {
		;	/* do nothing */
	}

	/* USB Reset2 -> Normal */
	aotg_set_bit32(MRCR, USB_RESET2);
	for (times = 8; times != 0; times--) {
		;	/* do nothing */
	}

	/**
	 * need to enable USBCLKEN & PLLEN simutaneously
	 * NOTE: USBRESET & GoSuspend are enabled default.
	 */
	/* USB controller clock enable */
	aotg_set_bit32(CMU_DEVCLKEN, CMU_USBCLKEN);

	aotg_set_bit32(MRCR, USB_RESET1);

	/* wait for USB controller until reset done */
	while ((aotg_read32(LINESTATUS) & (0x1 << LINESTATUS_OTGRESET)) != 0) {
		;	/* do nothing */
	}
}

static inline bool aotg_bus_reset_done(void)
{
	/* reset done after sending a SOF packet */
	if ((aotg_read8(USBIRQ_HCUSBIRQ) &
		((0x01 << USBIRQ_SOFIRQ) | (0x01 << USBIRQ_RSTIRQ)))
		== ((0x01 << USBIRQ_SOFIRQ) | (0x01 << USBIRQ_RSTIRQ)))
		return true;
	else
		return false;
}

/* clear all the usb related IRQs */
void aotg_clear_all_irqs(void)
{
    /* Note: must clear external IRQs first, then clear internal IRQs */
	aotg_write8(USBEIRQ, aotg_read8(USBEIRQ));
	aotg_write8(USBIRQ, aotg_read8(USBIRQ));
	aotg_write8(OTGSTAIR, aotg_read8(OTGSTAIR));
	aotg_write8(HCIN01ERRIRQ, aotg_read8(HCIN01ERRIRQ));
	aotg_write8(HCOUT02ERRIRQ, aotg_read8(HCOUT02ERRIRQ));
	aotg_write8(USBIRQ_HCUSBIRQ, aotg_read8(USBIRQ_HCUSBIRQ));
}

/* make sure that all state are right */
int aotg_state_check(void)
{
	/* dpdm 15k pull-down & in a_host state */
	if ((aotg_read8(DPDMCTRL) & 0xf) == 0 &&
		(aotg_read8(USBSTATE) & 0x3) == 0x3)
		return 0;
	else
		return -1;
}

/* clear again to make sure? */
void aotg_clear_ep_err_irqs(void)
{
	aotg_write8(HCIN01ERRIRQ, aotg_read8(HCIN01ERRIRQ));
	aotg_write8(HCOUT02ERRIRQ, aotg_read8(HCOUT02ERRIRQ));
}

void aotg_bus_reset(void)
{
	u32_t times;

	//printk("aotg bus reset\n");

	/* clear bus reset interrupt first */
	aotg_set_bit8(USBIRQ_HCUSBIRQ, USBIRQ_RSTIRQ);
	aotg_set_bit8(USBIRQ_HCUSBIRQ, USBIRQ_SOFIRQ);

	/* port reset */
	aotg_set_bit8(HCPORTCTRL, HCPORTCTRL_PORTRST);

	for (times = 0x400; times != 0; times--) {
		if (aotg_bus_reset_done())
			break;
		k_busy_wait(5000);
	}
	if (times == 0)
		printk("bus reset timeout!\n");

	/* first clear external irq, and then clear internal irq */
	aotg_write8(USBEIRQ, aotg_read8(USBEIRQ));
	aotg_write8(USBIRQ_HCUSBIRQ, aotg_read8(USBIRQ_HCUSBIRQ));
}

/* reset all eps */
void aotg_ep_reset(void)
{
	u8_t times;

	//printk("aotg ep reset\n");

	/* select all HCIN endpoints */
	aotg_clear_bits8(EPRST, EPRST_EPNUM_MASK);

	/* toggle reset */
	aotg_set_bit8(EPRST, EPRST_TOGRST);
	/* wait several cycles for reset done */
	for (times = 8; times != 0; times--) {
		;	/* do nothing */
	}

	/* fifo reset */
	aotg_set_bit8(EPRST, EPRST_FIFORST);
	/* wait several cycles for reset done */
	for (times = 8; times != 0; times--) {
		;	/* do nothing */
	}

	/* select all HCOUT endpoints */
	aotg_clear_bits8(EPRST, EPRST_EPNUM_MASK);
	aotg_set_bit8(EPRST, EPRST_IO_HCIO);

	/* toggle reset */
	aotg_set_bit8(EPRST, EPRST_TOGRST);
	/* wait several cycles for reset done */
	for (times = 8; times != 0; times--) {
		;	/* do nothing */
	}

	/* fifo reset */
	aotg_set_bit8(EPRST, EPRST_FIFORST);
	/* wait several cycles for reset done */
	for (times = 8; times != 0; times--) {
		;	/* do nothing */
	}

	/* set default address */
	aotg_write8(FNADDR, 0x0);

	/* peripheral device endpoint address is 0x0 */
	aotg_write8(HCEP0CTRL, 0x0);

	return;
}

/* reset the specific endpoint */
void aotg_reset_endpoint(u8_t ep_num)
{
	/* select ep */
	if (ep_num & 0x80)
		aotg_write8(EPRST, (aotg_read8(EPRST) & 0xe0) | 0x1);
	else
		aotg_write8(EPRST, (aotg_read8(EPRST) & 0xe0) | 0x12);

	aotg_set_bit8(EPRST, EPRST_TOGRST);		/* toggle reset */
	k_busy_wait(100);

	aotg_set_bit8(EPRST, EPRST_FIFORST);	/* fifo reset */
	k_busy_wait(100);

	return;
}

static inline int aotg_ep0_write(u8_t *data, u8_t size)
{
	u8_t count;

	/* fill setup data into ep0 fifo. for sending status, size is 0 */
	for (count = 0; count < size; count++) {
		/* HCOUT 0 ep register which length is 64-byte */
		aotg_write8(EP0INDATA + count, *(data + count));
	}
	/* start to send ep0out setup packet */
	aotg_write8(IN0BC_HCOUT0BC, size);

	return 0;
}

/* polling to check if data transfer complete or error happends */
static int aotg_wait_ep0_write_complete(void)
{
#ifndef AOTG_IRQ_FOR_EP0
	u16_t count;

	/* Is 100ms at most enough? */
	for (count = 100; count != 0; count--) {
		/* check if OTG state is a_host, in case of plug-out */
		if ((aotg_read8(USBSTATE) & USBSTATE_ST3_ST0_MASK) != FSM_A_HOST) {
			printk("usb plug-out?!\n");
			return -1;
		}
		/* check whether error occurs for ep0out */
		if ((aotg_read8(HCOUT02ERRIRQ) & HCOUT02ERRIRQ_EP0) != 0) {
			printk("aotg ep0out error!\n");
			return -1;
		}
		/* check whether ep0out busy */
		else if (!(aotg_read8(EP0CS_HCEP0CS) & (0x1 << EP0CS_HCOUTBSY)))
			return 0;
		else
			k_busy_wait(1000);
	}
	if (count == 0) {
		printk("aotg ep0out busy!\n");
		return -1;
	}

	return 0;
#else
	/* FIMXE: default 100ms, need to optimize */
	if (k_sem_take(&aotg_ep0_sem, 100) != 0) {
		printk("aotg ep0 timeout\n");
		return -1;
	}

	return 0;
#endif
}

static int aotg_wait_ep0_read_complete(void)
{
#ifndef AOTG_IRQ_FOR_EP0
	u16_t count;

	/* Is 100ms at most enough? */
	for (count = 100; count != 0; count--) {
		/* check if OTG state is a_host, in case plug-out... */
		if ((aotg_read8(USBSTATE) & USBSTATE_ST3_ST0_MASK) != FSM_A_HOST) {
			printk("usb plug-out?!\n");
			return -1;
		}
		/* check whether error occurs for ep0in */
		if ((aotg_read8(HCIN01ERRIRQ) & 0x01) != 0) {
			printk("aotg ep0in error!\n");
			return -1;
		}
		/* check whether ep0out busy */
		else if (!(aotg_read8(EP0CS_HCEP0CS) & (0x1 << EP0CS_HCINBSY)))
			return 0;
		else
			k_busy_wait(1000);
	}
	if (count == 0) {
		printk("aotg ep0in busy! 0x%x\n", aotg_read8(EP0CS_HCEP0CS));
		return -1;
	}

	return 0;
#else
	if (k_sem_take(&aotg_ep0_sem, 100) != 0) {
		printk("aotg ep0 timeout\n");
		return -1;
	}

	return 0;
#endif
}

/**
 * the steps of setup transaction:
 * 1. set HCSET to clear ep0 busy and toggle;
 * 2. load ep0-out data;
 * 3. wait for completion.
 */
static int aotg_handle_setup(u8_t *data, u8_t size)
{
	aotg_set_bit8(EP0CS_HCEP0CS, EP0CS_HCSET);
	aotg_ep0_write(data, size);
	if (aotg_wait_ep0_write_complete())
		return -1;

	return 0;
}

static int aotg_handle_data(u8_t *desc, u8_t size)
{
	u8_t count;
	u8_t i;
	int ret = 0;

	while (size != 0) {
		/**
		 * send control IN token, then wait until get the handshake of IN token.
		 * if the handshake is ACK, that means data packet have already been
		 * received successfully.
		 */
		/* send IN token */
		aotg_write8(OUT0BC_HCIN0BC, 0x00);
		/* wait until get the handshake packet */
		if (aotg_wait_ep0_read_complete())
			ret = -1;	/* if busy, byte count should be 0, it could be a signal */
			//return -1;

		// need to wait in case of OUT0BC_HCIN0BC is 0!
		//k_busy_wait(1000);

		/* get the size of ep0in data */
		count = aotg_read8(OUT0BC_HCIN0BC);
		if (count > size) {
			printk("aotg ep0in babble, count: %d, size: %d\n", count, size);
			count = size;
		}

		/* unload data from ep0 fifo */
		/* Note:
		 * 1) Ep0outdata is IN 0 data port for Host.
		 * 2) Ep0outdata is a 64 bytes register.
		 */
		for (i = 0; i < count; i++) {
			*desc = aotg_read8(EP0OUTDATA + i);
			desc++;
		}
		size = size - count;

		/* if short packet, stop */
		if (count < 0x40) {
			//printk("short packet! count: %d\n", count);
			break;
		}
	}

	return ret;
}

static inline void aotg_set_ep0_toggle_bit(u8_t toggle)
{
	if (toggle == TOGGLE_BIT_SET)
		aotg_set_bit8(EP0CS_HCEP0CS, EP0CS_HXSETTOGGLE);
	else
		aotg_set_bit8(EP0CS_HCEP0CS, EP0CS_HCCLRTOGGLE);
}

static int aotg_handle_out_status(void)
{
	/**
	 * send status stage out token.
	 * clear hcset bit, then aotg will send OUT token instead of
	 * SETUP token during the nearest OUT transfer from ep0out.
	 */
	aotg_clear_bit8(EP0CS_HCEP0CS, EP0CS_HCSET);

	/* status always DATA1, so set toggle bit */
	aotg_set_ep0_toggle_bit(TOGGLE_BIT_SET);

	/* send status data */
	aotg_ep0_write(NULL, 0);
	if (aotg_wait_ep0_write_complete())
		return -1;

	return 0;
}

static int aotg_handle_in_status(void)
{
	/* status always DATA1, so set toggle bit */
	aotg_set_ep0_toggle_bit(TOGGLE_BIT_SET);

	/**
	 * send control IN token, then wait until get the handshake of IN token.
	 * if the handshake is ACK, that means data packet have already been
	 * received successfully.
	 */
	/* send IN token */
	aotg_write8(OUT0BC_HCIN0BC, 0x00);
	if (aotg_wait_ep0_read_complete()) {
		aotg_clear_ep_err_irqs();
		return -1;
	}

	return 0;
}

int aotg_get_device_descriptor(u8_t *desc, u8_t *setup, u8_t size)
{
	if (aotg_handle_setup(setup, 8))	/* setup packet size defualt 8 */
		return -1;
	if (aotg_handle_data(desc, size))
		return -1;

	/* FIXME: need to update bMaxPacketSize0 */
	//aotg_set_ep0_max_packet_size(*(desc+7));
	if (aotg_handle_out_status())
		return -1;

	return 0;
}

int aotg_set_address(u8_t *setup, u8_t devnum)
{
	if (aotg_handle_setup(setup, 8))	/* setup packet size defualt 8 */
		return -1;
	if (aotg_handle_in_status())
		return -1;

	/* need to wait, is 200ms OK? */
	k_busy_wait(200000);

	/**
	 * set device address
	 * it will be copied to "addr" field fir all "token" packets sent by
	 * aotg host controller.
	 */
	aotg_write8(FNADDR, devnum);

	return 0;
}

int aotg_get_configuration(u8_t *config, u8_t *setup, u16_t length)
{
	if (aotg_handle_setup(setup, 8))	/* setup packet size defualt 8 */
		return -1;
	if (aotg_handle_data(config, (u8_t)length))
		return -1;
	if (aotg_handle_out_status())
		return -1;

	return 0;
}

int aotg_set_configuration(u8_t *setup)
{
	if (aotg_handle_setup(setup, 8))	/* setup packet size defualt 8 */
		return -1;
	if (aotg_handle_in_status())
		return -1;

	return 0;
}

int aotg_get_stor_max_lun(u8_t *setup, u8_t *max_lun)
{
	if (aotg_handle_setup(setup, 8))	/* setup packet size defualt 8 */
		return -1;
	if (aotg_handle_data(max_lun, 0x01))
		return -1;
	if (aotg_handle_out_status())
		return -1;

	return 0;
}

/* use ep1-in(HCIN) default */
void aotg_set_bulk_in(u8_t ep_num, u16_t ep_size, u8_t buf_num)
{
	/* aotg receive data from ep_num ep of peripheral to ep1-in fifo */
	aotg_write8(HCIN1CTRL, ep_num);		/* ep_num & 0xf */
	aotg_write8(HCIN1MAXPCKH, (u8_t)(ep_size >> 8));	/* ep_size / 256 */
	aotg_write8(HCIN1MAXPCKL, (u8_t)(ep_size));		/* ep_size % 256 */

	aotg_write32(OUT1STADDR, 0x00);
	aotg_write8(OUT1CTRL_HCIN1CTRL, 0x1<<7 | 0x2<<2 | buf_num);
}

/* use ep2-out(HCOUT) default */
void aotg_set_bulk_out(u8_t ep_num, u16_t ep_size, u8_t buf_num)
{
	/* aotg send data to ep_num ep of peripheral from ep2-out fifo */
	aotg_write8(HCOUT2CTRL, ep_num);
	aotg_write8(HCOUT2MAXPCKH, (u8_t)(ep_size >> 8));	/* ep_size / 256 */
	aotg_write8(HCOUT2MAXPCKL, (u8_t)(ep_size));			/* ep_size % 256 */

	aotg_write32(IN2STADDR, 0x100);
	aotg_write8(IN2CTRL_HCOUT2CTRL, 0x1<<7 | 0x2<<2 | buf_num);
}

/**
 * NOTE: be careful to modify
 * test: 40MHz -- need 652us
 */
int wait_reg8_status(u32_t reg, u8_t dat, u8_t check, u32_t ms)
{
	u32_t i;
	int res;

	res = 1;
	ms *= 100;
	for (i = 0; i < ms; i++) {
		if ((*((REG8) reg) & dat) == check) {
			res = 0;
			break;
		}
		k_busy_wait(10);
	}
	return res;
}

/**
 * handle data read alignment!
 * if address is not 4-byte aligned, we have to copy data
 * byte by byte.
 */
static void aotg_data_read(u32_t *address, u16_t data_lenth)
{
	u8_t i;

	if (data_lenth == 64) {
		if (aotg_read8(OUT1BCL_HCIN1BCL) != 64)
			printk("ep1in byte_count: %d\n", aotg_read8(OUT1BCL_HCIN1BCL));
		/* check if it is 4-byte aligned */
		if (((long)address & 0x03) != 0) {
			u8_t *tmp8 = (u8_t *)address;
			for (i = 0; i < 0x40; i++) {
				*tmp8 = *((REG8)(FIFO1DAT));
				tmp8++;
			}
		} else {
			u32_t *tmp32 = address;
			for (i = 0; i < 0x10; i++) {
				*tmp32 = *((REG32)(FIFO1DAT));
				tmp32++;
			}
		}
	} else {
		u8_t *tmp8 = (u8_t *)address;
		for (i = 0; i < data_lenth; i++) {
			*tmp8 = *((REG8)(FIFO1DAT));
			tmp8++;
		}
	}

#ifndef AOTG_AUTO_MODE
	aotg_set_bit8(OUT1CS_HCIN1CS, OUT1CS_HCBUSY);	/* set busy */
#endif

	return;
}

#if 0
static int clear_ep_busy(u32_t reg)
{
	u32_t i;
	int ret;

	ret = 1;

	/* NOTE: short packet, need to set busy bit */
	aotg_write8(reg, aotg_read8(reg) | 0x02);
	for (i = 0; i < 10000; i++) {
		if ((aotg_read8(reg) & 0x02) == 0) {
			//if ((aotg_read8(reg) & 0x0c) >= 0x0c)	// only for multi-fifo
				ret = 0;
				break;
		}
		k_busy_wait(10);
	}

	return ret;
}
#endif

/**
 * handle data write alignment!
 * we can't presume "address" is always aligned to 4-byte.
 *
 * FIXME: If STALL happends, we don't know at all but wait
 * until timeout. It'll influnce the response time.
 */
static int aotg_data_write(u8_t *address, u32_t data_lenth)
{
	u8_t i, j;
	u32_t total_full_p, remain_byte;
	u32_t *tmp32;
	u8_t *tmp8;
	bool remainer = false;
	int ret;

	total_full_p = data_lenth / 0x40;
	remain_byte = data_lenth % 0x40;
	tmp32 = (u32_t *)address;
	tmp8 = (u8_t *)address;

	for (i = 0; i < total_full_p; i++) {
		/* check if it is 4-byte aligned */
		if (((long)tmp32 & 0x03) != 0) {
			ret = wait_reg8_status(IN2CS_HCOUT2CS, 0x02, 0, AOTG_WRITE_TIMEOUT);
			if (ret) {
				printk("Error: aotg write timeout!!!\n");
				return ret;
			}
			for (j = 0; j < 0x40; j++) {
				*((REG8)(FIFO2DAT)) = *tmp8;
				tmp8++;
			}
		} else {
			remainer = true;
			ret = wait_reg8_status(IN2CS_HCOUT2CS, 0x02, 0, AOTG_WRITE_TIMEOUT);
			if (ret) {
				printk("Error: aotg write timeout!!\n");
				return ret;
			}
			for (j = 0; j < 0x10; j++) {
				*((REG32)(FIFO2DAT)) = *tmp32;
				tmp32++;
			}
		}
#ifndef AOTG_AUTO_MODE
		/* set byte count & set busy */
		aotg_write8(IN2BCL_HCOUT2BCL, 64);
		aotg_set_bit8(IN2CS_HCOUT2CS, IN2CS_HCBUSY);
#endif
	}
	if (remain_byte != 0) {
		/*
		 * There are 4 cases:
		 * 1. no total_full_p, has remain_byte: tmp8 = address;
		 * 2. has total_full_p, no remain_byte: tmp8 is useless;
		 * 3. has total_full_p(4-byte aligned) & remain_byte: tmp8 = tmp32;
		 * 4. has total_full_p(not 4-byte aligned) & remain_byte: tmp8 done;
		 */
		if (remainer)
			tmp8 = (u8_t *)tmp32;
		ret = wait_reg8_status(IN2CS_HCOUT2CS, 0x02, 0, AOTG_WRITE_TIMEOUT);
		if (ret) {
			printk("Error: aotg write timeout(%d)!\n", remain_byte);
			return ret;
		}
		for (j = 0; j < remain_byte; j++) {
			*((REG8)(FIFO2DAT)) = *tmp8;
			tmp8++;
		}
#ifndef AOTG_AUTO_MODE
		aotg_write8(IN2BCL_HCOUT2BCL, remain_byte);
#endif
		/*
		 * NOTICE: If short packet, need to set busy!
		 *
		 * There is no need to make sure fifo is empty, because we'll
		 * check next time. Don't worry about the oder in complicate
		 * case, hardware will(or "must") take care of it!
		 */
		aotg_set_bit8(IN2CS_HCOUT2CS, IN2CS_HCBUSY);
#if 0
		ret = clear_ep_busy(IN2CS_HCOUT2CS);
		if (ret) {
			printk("Error: clear_ep_busy timeout(%d)\n", remain_byte);
			return ret;
		}
#endif
	}

	return 0;
}

int aotg_bulk_tranx_in(u32_t *buf, u32_t len, u16_t num, u16_t timeout)
{
	int ret;
	u32_t time_stamp, ms_spent;

	/* init before bulk-in transfer starts */
	read_result = 0;

	/* config Fifoctrl to send IN token automatically while bulk in data */
#ifdef AOTG_AUTO_MODE
	aotg_write8(FIFOCTRL, (0x1 << FIFOCTRL_FIFOAUTO) | 0x00 | BULK_IN_EP_NUM);
#else
	aotg_write8(FIFOCTRL, /*(0x1 << FIFOCTRL_FIFOAUTO) |*/ 0x00 | BULK_IN_EP_NUM);
#endif

	/* set Numbers of IN Packet */
	aotg_write16(HCIN1_COUNTL, num);

	/* write hcbusy for IN data. if hcbusy has been 1 already, do not set again, else error */
	if (!(aotg_read8(OUT1CS_HCIN1CS) & (0x1 << OUT1CS_HCBUSY)))
		aotg_set_bit8(OUT1CS_HCIN1CS, OUT1CS_HCBUSY);

	aotg_device.buf = buf;
	aotg_device.len = len;
	aotg_device.num = num;

	/**
	 * start to send IN Token according to HC IN Counter
	 * including HCINx_COUNTH and HCINx_COUNTL.
	 */
	aotg_set_bit8(HCINCTRL, HCINCTRL_HCIN1START);

	/*
	 * Swap may fail and return -EAGAIN even if not caused by timeout,
	 * it seems buggy during context switch. We try to prove "not my fault"
	 * by print out the ms_spent and return value.
	 */
	time_stamp = k_uptime_get_32();
	if ((ret = k_sem_take(&aotg_read_sem, K_MSEC(timeout))) != 0) {
		ms_spent = k_uptime_delta_32((int64_t *)&time_stamp);
		printk("aotg read timeout(%d ms), ret: %d, ms_spent: %d\n",
			timeout, ret, ms_spent);
		read_result = -ECONNRESET;
	}

	aotg_clear_bit8(HCINCTRL, HCINCTRL_HCIN1START);

#ifdef AOTG_AUTO_MODE
	aotg_clear_bit8(FIFOCTRL, FIFOCTRL_FIFOAUTO);
#endif

	return read_result;
}

int aotg_bulk_tranx_out(u32_t *buf, u32_t len)
{
	/* init before bulk-out transfer starts */
	write_result = 0;

	/**
	 * configuring Fifoctrl to send OUT token automatically while bulk out data
	 * NOTICE: need to set "auto mode" before DMA, otherwise it may be wrong!
	 */
#ifdef AOTG_AUTO_MODE
	aotg_write8(FIFOCTRL, (0x1 << FIFOCTRL_FIFOAUTO) |
		(0x1 << FIFOCTRL_IO_HCIO) | BULK_OUT_EP_NUM);
#else
	aotg_write8(FIFOCTRL, /*(0x1 << FIFOCTRL_FIFOAUTO) |*/
		(0x1 << FIFOCTRL_IO_HCIO) | BULK_OUT_EP_NUM);
#endif

	if (aotg_data_write((u8_t *)buf, len) && !write_result)
		write_result = -ECONNRESET;	/* timeout */

#ifdef AOTG_AUTO_MODE
	aotg_clear_bit8(FIFOCTRL, FIFOCTRL_FIFOAUTO);
#endif

	return write_result;
}

/* for clear feature and set feature both */
int aotg_clear_feature(u8_t *setup)
{
	/**
	 * ClearFeature(ENDPOINT_HALT) request always results in
	 * the data toggle being reinitialized to DATA0.
	 * ref: USB Spec20-9.4.5
	 */
	aotg_set_ep0_toggle_bit(TOGGLE_BIT_CLEAR);
	if (aotg_handle_setup(setup, 8))	/* setup packet size defualt 8 */
		return -1;
	if (aotg_handle_in_status())
		return -1;

	return 0;
}

#ifdef AOTG_THREAD_READ
/**
 * FIXME: better way: when aotg receives the first packet, use the
 * thread to wait for the remaining packets(if it has) not using irqs.
 */
static void aotg_work_q_data_read(struct k_work *work)
{
	struct aotg_device_info *info =
		CONTAINER_OF(work, struct aotg_device_info, work);

	//printk("work read %p, %d\n", aotg_device.buf, aotg_device.len);
	if (aotg_device.len >= 64) {
		aotg_data_read(aotg_device.buf, 64);
		aotg_device.len -= 64;
		aotg_device.num--;
		aotg_device.buf += 16;
	} else if (aotg_device.len > 0) {
		aotg_data_read(aotg_device.buf, (u16_t)aotg_device.len);
		aotg_device.len = 0;
		aotg_device.num--;
	} else
		printk("read 0 packet!\n");

	if (aotg_device.num == 0)
		k_sem_give(&aotg_read_sem);

	return;
}
#endif

/**
 * HCIN ep irq: only when all fifo are empty and one is full!
 *
 * FIXME: For now, we suppose only one fifo is used, if
 * multi-fifo used, the handler can't take care of that.
 */
static inline void aotg_hcd_handle_ep1in(void)
{
#ifdef AOTG_THREAD_READ
	k_work_init(&aotg_device.work, aotg_work_q_data_read);
	k_work_submit_to_queue(&aotg_work_q, &aotg_device.work);
#else
	/* cpu read */
	if (aotg_device.len >= 64) {
		aotg_data_read(aotg_device.buf, 64);
		aotg_device.len -= 64;
		aotg_device.num--;
		aotg_device.buf += 16;
	} else if (aotg_device.len > 0) {
		aotg_data_read(aotg_device.buf, (u16_t)aotg_device.len);
		aotg_device.len = 0;
		aotg_device.num--;
	} else
		printk("read 0 packet!\n");

	if (aotg_device.num == 0)
		k_sem_give(&aotg_read_sem);
#endif

	return;
}

static inline void aotg_hcd_handle_ep2out(void)
{
	return;
}

static inline void aotg_hcd_handle_ep0out(void)
{
#ifdef AOTG_IRQ_FOR_EP0
	k_sem_give(&aotg_ep0_sem);
#endif
}

static inline void aotg_hcd_handle_ep0in(void)
{
#ifdef AOTG_IRQ_FOR_EP0
	k_sem_give(&aotg_ep0_sem);
#endif
}

#define NO_ERR			0x0
#define ERR_CRC			0x1
#define ERR_TOGGLE		0x2
#define ERR_STALL		0x3
#define ERR_TIMEOUT		0x4
#define ERR_PID			0x5
#define ERR_OVERRUN		0x6
#define ERR_UNDERRUN	0x7

/* FIXME: consecutive 3 times timeout/stall, bus reset! */
static inline void aotg_hcd_handle_ep1in_err(void)
{
	u8_t type;	/* error type: bit[2:4] */

	type = (aotg_read8(HCIN1ERR) & 0x1c) >> 2;
	//printk("bulk-in err type: %d\n", type);
	switch (type) {
	/* timeout, crc error, pid error... */
	default:
		aotg_set_bit8(HCIN1ERR, 5); /* resend or try again */
		break;

	/* could it happend? */
	case NO_ERR:
		break;

	/* stall */
	case ERR_STALL:
		read_result = -EPIPE;
		k_sem_give(&aotg_read_sem);	/* quit the transaction */
		break;
	}

	return;
}

static inline void aotg_hcd_handle_ep2out_err(void)
{
	u8_t type;	/* error type: bit[2:4] */

	type = (aotg_read8(HCOUT2ERR) & 0x1c) >> 2;
	//printk("bulk-out err type: %d\n", type);
	switch (type) {
	/* timeout or pid error */
	default:
		aotg_set_bit8(HCOUT2ERR, 5); /* resend or try again */
		break;

	/* could it happend? */
	case NO_ERR:
		break;

	/* stall */
	case ERR_STALL:
		write_result = -EPIPE;
		break;
	}

	return;
}

/**
 * there are some ep0-in errors that can't explain
 * during bulk-in/bulk-out transfers.
 * so don't handle ep0 errors for now.
 */
static inline void aotg_hcd_handle_ep0out_err(void)
{
	u8_t type;	/* error type: bit[2:4] */

	type = (aotg_read8(HCOUT0ERR) & 0x1c) >> 2;

#ifdef AOTG_DEBUG_ISR
	printk("HCOUT0ERR type: 0x%x\n", type);
#endif

#ifdef ENABLE_HANDLE_EP0_ERR
	if (type == ERR_TIMEOUT) {
		aotg_set_bit8(HCOUT0ERR, 5);
	}
#endif

	return;
}

static inline void aotg_hcd_handle_ep0in_err(void)
{
	u8_t type;	/* error type: bit[2:4] */

	type = (aotg_read8(HCIN0ERR) & 0x1c) >> 2;

#ifdef AOTG_DEBUG_ISR
	printk("HCIN0ERR type: 0x%x\n", type);
#endif

#ifdef ENABLE_HANDLE_EP0_ERR
	if (type == ERR_TIMEOUT) {
		aotg_set_bit8(HCIN0ERR, 5);
	}
#endif

	return;
}

/* USB ISR handler */
static void aotg_hcd_isr_handler(void)
{
	u8_t irqvector;
	u32_t i;

	irqvector = aotg_read8(IVECT);
	i = 0;

	/*
	 * WARNNING: USBEIRQ can't be cleared right sometimes, it'll
	 * generate interrupt again and make the transfer process corrupted!
	 * Give it some more chances to bypass the bug.
	 */
	aotg_set_bit8(USBEIRQ, USBEIRQ_USBIRQ);
	while (aotg_read8(USBEIRQ) & (0x1 << USBEIRQ_USBIRQ)) {
		aotg_set_bit8(USBEIRQ, USBEIRQ_USBIRQ);
		i++;
#ifdef AOTG_DEBUG_ISR
		printk("vector: 0x%x\n", aotg_read8(IVECT));
#endif
	}

	/* NOTICE: be careful to print debug info in interrupt context */
#ifdef AOTG_DEBUG_ISR
	if (i > 0)
		printk("isr: i: %d\n", i);
#endif

	//printk("irqvector: 0x%x\n", irqvector);
	switch (irqvector) {
	case UIV_SOF:
		aotg_set_bit8(USBIRQ_HCUSBIRQ, 1);	/* 0x18c, bit 1 */
		break;

	/* HCIN 0 */
	case UIV_EP0OUT:
		//printk("OUT01IRQ: 0x%x\n", aotg_read8(OUT01IRQ));
		aotg_write8(OUT01IRQ, 0x1 << 0);
		aotg_hcd_handle_ep0in();
		break;

	/* HCOUT 0 */
	case UIV_EP0IN:
		//printk("IN02IRQ: 0x%x\n", aotg_read8(IN02IRQ));
		aotg_write8(IN02IRQ, 0x1 << 0);
		aotg_hcd_handle_ep0out();
		break;

	/* HCIN 1 */
	case UIV_EP1OUT:
		//printk("OUT01IRQ: 0x%x\n", aotg_read8(OUT01IRQ));
		aotg_write8(OUT01IRQ, 0x1 << 1);
		aotg_hcd_handle_ep1in();
		break;

	/* HCOUT 2 */
	case UIV_EP2IN:
		//printk("IN02IRQ: 0x%x\n", aotg_read8(IN02IRQ));
		aotg_write8(IN02IRQ, 0x1 << 2);
		aotg_hcd_handle_ep2out();
		break;

	/* HCIN 1 ERR */
	case UIV_HCIN1ERR:
		//printk("HCIN1ERR: 0x%x\n", aotg_read8(HCIN1ERR));
		aotg_write8(HCIN01ERRIRQ, 0x1 << 1);
		aotg_hcd_handle_ep1in_err();
		break;

	/* HCOUT 2 ERR */
	case UIV_HCOUT2ERR:
		aotg_write8(HCOUT02ERRIRQ, 0x1 << 2);
		aotg_hcd_handle_ep2out_err();
		break;

	/* HCOUT 0 ERR */
	case UIV_HCOUT0ERR:
		aotg_write8(HCOUT02ERRIRQ, 0x1 << 0);
		//aotg_hcd_handle_ep0out_err();
		break;

	/* HCIN 0 ERR */
	case UIV_HCIN0ERR:
		aotg_write8(HCIN01ERRIRQ, 0x1 << 0);
		//aotg_hcd_handle_ep0in_err();
		break;

	default:
		//printk("irqvector: 0x%x\n", irqvector);
		break;
	}

	return;
}

/* enable all related irqs */
void aotg_enable_irqs(void)
{
	aotg_set_bit8(USBEIRQ, 3);			/* USBIEN */
	aotg_set_bit8(OUT01IEN, 1);			/* ep1-in */
	aotg_set_bit8(IN02IEN, 2);			/* ep2-out */
	aotg_set_bit8(HCIN01ERRIEN, 1);		/* ep1-in err */
	aotg_set_bit8(HCOUT02ERRIEN, 2);	/* ep2-out err */

	//AOTG_IRQ_FOR_EP0
	aotg_set_bit8(OUT01IEN, 0);			/* ep0-in */
	aotg_set_bit8(IN02IEN, 0);			/* ep0-out */
	aotg_set_bit8(HCIN01ERRIEN, 0);		/* ep0-in err */
	aotg_set_bit8(HCOUT02ERRIEN, 0);	/* ep0-out err */
}

int aotg_enable(bool in_reset)
{
	u32_t times;

	if (!in_reset)
#ifdef AOTG_AUTO_MODE
		printk("*** usb aotg hcd: v1.0(with auto mode) ***\n");
#else
		printk("*** usb aotg hcd: v1.0 ***\n");
#endif
	/* spll clk enable */
	aotg_write32(SPLL_CTL, aotg_read32(SPLL_CTL) | 0x3f);

	/* usb clk enable */
	aotg_write32(CMU_DEVCLKEN,
		aotg_read32(CMU_DEVCLKEN) | (0x1 << CMU_USBCLKEN));

	/* uram clk select usbctl_uram_clk */
	aotg_write32(CMU_MEMCLKSEL, aotg_read32(CMU_MEMCLKSEL) | 0x02);

	do {
		/* (1) set related clock.(DMA clock has been set by OS) */

		/* uram clock select usbctl_uram_clk */
		aotg_write32(CMU_MEMCLKSEL,
			aotg_read32(CMU_MEMCLKSEL) | (0x1 << CMU_URAMCLKSEL));
		/* set related clock over */

		/* (2) Reset USB controller and enable USB clock */
		aotg_controller_reset();

		/* delay about 100 cycles, at least 10 cycles */
		for (times = 100; times != 0x00; times--) {
			;	/* do nothing */
		}

		/* (3) enable device detect, DPDMCTRL set to 0x10 */
		/* re-configure DPDM detection after resetting */
		aotg_write8(DPDMCTRL,
			(aotg_read8(DPDMCTRL) & 0xf0) | (0x01 << DPDMCTRL_LINEDETEN));

		aotg_write8(DPDMCTRL, 0x10 | (aotg_read8(DPDMCTRL) & 0xe0));
		aotg_write8(IDVBUSCTRL, 0x07 | (aotg_read8(IDVBUSCTRL) & 0xf0));
		aotg_write8(USBCTRL, 0x01 | (aotg_read8(USBCTRL) & 0xfd));

		/* (5) clear all the usb related IRQs */
		aotg_clear_all_irqs();

		/* (6) configure the FSM as A_host state */
		/* enable softID and softVBUS. SoftID=0, SoftVBUS=1, FSM will go to A_idle */
		aotg_write8(IDVBUSCTRL, 0x04);

		/**
		 * a_bus_drop=0, a_bus_req=1
		 * FSM will go to A_wait_vrise, and then go to a_wait_bcon,
		 * that means it is waiting for device to connect.
		 */
		aotg_write8(USBCTRL, aotg_read8(USBCTRL) | (0x1 << USBCTRL_BUSREQ));

		/* After device connects, host will enter A_host state */
		/* timeout at least 160ms */
		for (times = 100; times != 0; times--) {
			if ((aotg_read8(USBSTATE) & USBSTATE_ST3_ST0_MASK) == FSM_A_HOST)
				break;

			k_busy_wait(5000);
		}
		//printk("USBSTATE: 0x%x\n", aotg_read8(USBSTATE));
		if (times == 0) {
			printk("No Device Plug-in!\n");
			return -1;
		}
		/* (6) configure the FSM as A_host state over */

		/* (7) wait for bus reset to finish */
		/*
		 * After host enter A_host state, a bus reset signal will be sent by host
		 * and we here wait for reset to finish.
		 * Meanwhile, we wait for SOF packet to be sent for security.
		 */
		// delay at least 146ms(test in FPGA)
		for (times = 500; times != 0; times--) {
			if(aotg_bus_reset_done())
				break;
			k_busy_wait(5000);
		}
		if (times == 0) {
			printk("bus reset timeout\n");
			return -1;
		}

		/* clear bus reset interrupt request */
		aotg_set_bit8(USBIRQ_HCUSBIRQ, USBIRQ_RSTIRQ);
		aotg_set_bit8(USBIRQ_HCUSBIRQ, USBIRQ_SOFIRQ);

		/* until now the controller has been enabled */
	} while (0);

	if (in_reset) {
		aotg_clear_all_irqs();
		aotg_enable_irqs();
		return 0;
	}

	/* aotg data read sem */
	k_sem_init(&aotg_read_sem, 0, 1);

#ifdef AOTG_IRQ_FOR_EP0
	k_sem_init(&aotg_ep0_sem, 0, 1);	/* aotg ep0 data read/write sem */
#endif

#ifdef AOTG_THREAD_READ
	/* aotg data read work queue */
	k_work_q_start(&aotg_work_q,
		aotg_stack_area, AOTG_STACK_SIZE, 0);
#endif
	/* clear all the usb related IRQs */
	aotg_clear_all_irqs();
	aotg_enable_irqs();

	/* Connect and enable USB interrupt */
	IRQ_CONNECT(AOTG_HCD_IRQ, 0,
		aotg_hcd_isr_handler, 0, IOAPIC_EDGE | IOAPIC_HIGH);
	irq_enable(AOTG_HCD_IRQ);

	return 0;
}

/* re-enable aotg */
int aotg_reset(void)
{
#if 1
	return aotg_enable(true);
#else
	aotg_clear_all_irqs();
	aotg_enable_irqs();

	/**
	 * need to disable irq first, then re-enable irq, otherwise
	 * there is no interrrupt during re-enumeration.
	 * TODO: find the reason!
	 */
	irq_disable(AOTG_HCD_IRQ);
	irq_enable(AOTG_HCD_IRQ);
#endif
}
