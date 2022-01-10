/*
 * Copyright (c) 2017 Actions Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief USB Actions OTG device controller driver private definitions
 *
 * This file contains the Actions OTG USB device controller driver private
 * definitions.
 */

#ifndef __USB_AOTG_REGISTERS_H__
#define __USB_AOTG_REGISTERS_H__

#include <board.h>

/* Max device address */
#define USB_AOTG_MAX_ADDR	0x7F

/* USB AOTG Max Packet Size */
#define USB_AOTG_CTRL_MAX_PKT	64
#define USB_AOTG_FS_ISOC_MAX_PKT	1023
#define USB_AOTG_FS_BULK_MAX_PKT	64
#define USB_AOTG_FS_INTR_MAX_PKT	1023
#define USB_AOTG_HS_ISOC_MAX_PKT	1024
#define USB_AOTG_HS_BULK_MAX_PKT	512
#define USB_AOTG_HS_INTR_MAX_PKT	1024

/* USB IN EP index */
enum usb_aotg_in_ep_idx {
	USB_AOTG_IN_EP_0 = 0,
	USB_AOTG_IN_EP_1,
	USB_AOTG_IN_EP_2,
	USB_AOTG_IN_EP_3,
	USB_AOTG_IN_EP_NUM
};

/* USB OUT EP index */
enum usb_aotg_out_ep_idx {
	USB_AOTG_OUT_EP_0 = 0,
	USB_AOTG_OUT_EP_1,
	USB_AOTG_OUT_EP_2,
	USB_AOTG_OUT_EP_3,
	USB_AOTG_OUT_EP_NUM
};

/* AOTG EP reset type */
enum usb_aotg_ep_reset_type {
	USB_AOTG_EP_FIFO_RESET = 0,
	USB_AOTG_EP_TOGGLE_RESET,
	/* Toggle & FIFO reset */
	USB_AOTG_EP_RESET
};

/* Keep pace with USB Endpoint type */
#if 0
enum usb_aotg_ep_type {
	USB_AOTG_EP_CTRL = 0,
	USB_AOTG_EP_ISOC,
	USB_AOTG_EP_BULK,
	USB_AOTG_EP_INTR
};
#endif

/* AOTG FIFO type */
enum usb_aotg_fifo_type {
	USB_AOTG_FIFO_SINGLE = 0,	/* default */
	USB_AOTG_FIFO_DOUBLE,
	USB_AOTG_FIFO_TRIPLE,
	USB_AOTG_FIFO_QUAD
};

/**
 * There are 2 or 3 stages: Setup, Data (optional), Status
 * for control transfer.
 * There are 3 cases that may exist during control transfer
 * which we have to take care to make AOTG work fine.
 * Case 1: Setup, In Data, Out Status;
 * Case 2: Setup, In Status;
 * Case 3: Setup, Out Data, In Status;
 */
enum usb_aotg_ep0_phase {
	USB_AOTG_SETUP = 1,
	USB_AOTG_IN_DATA,
	USB_AOTG_OUT_DATA,
	USB_AOTG_IN_STATUS,
	USB_AOTG_OUT_STATUS,
	/* Special phase which is necessary */
	USB_AOTG_SET_ADDRESS
};


/*********************************** Common **********************************/


#define USB_VDD	MULTI_USED
#define USB_VDD_EN	8
#define USB_VDD_VOLTAGE_MASK	(0x7 << 5)
/* defalut output voltage: 1.3V */
#define USB_VDD_VOLTAGE_DEFAULT	(0x6 << 5)


/* FIFO Clock */
#define USB_MEM_CLK	CMU_MEMCLKSEL
#define USB_MEM_CLK_URAM2_OFFSET	27
#define USB_MEM_CLK_URAM1_OFFSET	25
#define USB_MEM_CLK_URAM0_OFFSET	23


/* USB AOTG udc IRQ line */
#define USB_AOTG_UDC_IRQ	IRQ_ID_USB
/* Alias */
#define USB_AOTG_IRQ	IRQ_ID_USB


/*********************************** Global **********************************/


/* USB AOTG Base Address */
#define USB_AOTG_BASE	0xC0080000


/* Testing */
#define BKDOOR	(USB_AOTG_BASE + 0x40D)
#define HS_DISABLE	7


/** Line Status */
#define LINESTATUS	(USB_AOTG_BASE + 0x422)
#define LINESTATE	(0x3 << 3)
#define LINE_DM	4
#define LINE_DP	3
#define OTGRESET	0


/* Device Address */
#define FNADDR	(USB_AOTG_BASE + 0x1A6)


/** DPDM */
#define DPDMCTRL	(USB_AOTG_BASE + 0x421)
/* plug-in (connect/disconnect) */
#define PLUGIN	6
/* Line status detect enable (default: enable) */
#define LSDETEN	4
/* 500K pull-up */
#define DM_PULL_UP	3
#define DP_PULL_UP	2
/* 15K pull-down */
#define DM_PULL_DOWN	1
#define DP_PULL_DOWN	0


/** Idpin & Vbus */
#define IDVBUSCTRL	(USB_AOTG_BASE + 0x420)
#define SOFT_IDPIN	3
#define SOFT_IDEN	2


/** USB Status */
#define USBSTATUS	(USB_AOTG_BASE + 0x1BF)


/** Auto In mode IN Token timer */
#define AUTOINTIMER	(USB_AOTG_BASE + 0x410)


/* USB Clock Gate */
#define USBCLKGATE	(USB_AOTG_BASE + 0x1A7)
#define CLKGATE_SUSPEND	0


/** OUT1 Endpoint NAK Control */
#define OUT1NAKCTRL	(USB_AOTG_BASE + 0x40C)


/** OUT Endpoints Short Packet Control */
#define OUT_SHTPKT	(USB_AOTG_BASE + 0x401)
#define EP3OUT_SHTPKT_IRQ	7
#define EP2OUT_SHTPKT_IRQ	6
#define EP1OUT_SHTPKT_IRQ	5
#define EP3OUT_SHTPKT_IEN	3
#define EP2OUT_SHTPKT_IEN	2
#define EP1OUT_SHTPKT_IEN	1

/** USB OTG FSM State */
#define USBSTATE	(USB_AOTG_BASE + 0x1BD)
/* Alias */
#define OTGSTATE	USBSTATE
#define OTG_A_IDLE	0x0
#define OTG_A_WAIT_BCON	0x2
#define OTG_A_HOST	0x3
#define OTG_A_SUSPEND	0x4
#define OTG_B_IDLE	0x8
#define OTG_B_PERIPHERAL	0x9


/** USB OTG FSM Control */
#define USBCTRL	(USB_AOTG_BASE + 0x1BE)
/* Alias */
#define OTGCTRL	USBCTRL
#define OTGCTRL_FORCE	7
#define OTGCTRL_BUSREQ	0


/************************************ IRQ ************************************/


/** USB Core */
#define USBIEN	(USB_AOTG_BASE + 0x198)
#define USBIEN_NTR	6
#define USBIEN_HIGH_SPEED	5
#define USBIEN_RESET	4
#define USBIEN_SUSPEND	3
#define USBIEN_TOKEN	2
#define USBIEN_SOF	1
#define USBIEN_SETUP	0

#define USBIRQ	(USB_AOTG_BASE + 0x18C)
#define USBIRQ_NTR	6
#define USBIRQ_HIGH_SPEED	5
/* USB Bus Reset */
#define USBIRQ_RESET	4
/* Suspend Signal */
#define USBIRQ_SUSPEND	3
/* SETUP Token */
#define USBIRQ_TOKEN	2
/* SOF Packet */
#define USBIRQ_SOF	1
/* SETUP Data */
#define USBIRQ_SETUP	0


/** OTG FSM */
#define OTGIEN	(USB_AOTG_BASE + 0x1C0)
#define OTGIEN_PERIPHERAL	4
#define OTGIEN_VBUSERROR	3
#define OTGIEN_LOCAL	2
#define OTGIEN_SRPDET	1
#define OTGIEN_IDLE	0

#define OTGIRQ	(USB_AOTG_BASE + 0x1BC)
/* Enter a_peripheral or b_peripheral */
#define OTGIRQ_PERIPHERAL	4
/* Enter a_vbus_err state */
#define OTGIRQ_VBUSERROR	3
#define OTGIRQ_LOCAL	2
#define OTGIRQ_SRPDET	1
#define OTGIRQ_IDLE	0


/** USB External Interrupt request */
#define USBEIRQ	(USB_AOTG_BASE + 0x400)
/* Alias: same register */
#define USBEIEN	USBEIRQ
/* External */
#define USBEIRQ_EXTERN	7
/* Wakeup */
#define USBEIRQ_WAKEUP	6
/* Resume */
#define USBEIRQ_RESUME	5
/* Connnect/disconnect */
#define USBEIRQ_CONDISC	4
/* External */
#define USBEIEN_EXTERN	3
/* Wakeup */
#define USBEIEN_WAKEUP	2
/* Resume */
#define USBEIEN_RESUME	1
/* Connnect/disconnect */
#define USBEIEN_CONDISC	0


/** Interrupt Vector */
#define IVECT	(USB_AOTG_BASE + 0x1A0)
/* Device */
/* Setup Data */
#define UIV_SUDAV	0x00
#define UIV_SOF	0x04
/* Setup Token */
#define UIV_SUTOK	0x08
#define UIV_SUSPEND	0x0c
#define UIV_USBRST	0x10
#define UIV_HSPEED	0x14
/* Endpoint */
#define UIV_HCOUT0ERR	0x16
#define UIV_EP0IN	0x18
#define UIV_HCIN0ERR	0x1a
#define UIV_EP0OUT	0x1c
#define UIV_EP0PING	0x20
#define UIV_HCOUT1ERR	0x22
#define UIV_EP1IN	0x24
#define UIV_HCIN1ERR	0x26
#define UIV_EP1OUT	0x28
#define UIV_EP1PING	0x2c
#define UIV_HCOUT2ERR	0x2e
#define UIV_EP2IN	0x30
#define UIV_HCIN2ERR	0x32
#define UIV_EP2OUT	0x34
#define UIV_EP2PING	0x38
#define UIV_HCOUT3ERR	0x3a
#define UIV_EP3IN	0x3c
#define UIV_HCIN3ERR	0x3e
#define UIV_EP3OUT	0x40
#define UIV_EP3PING	0x44
/* IN Token */
#define UIV_INTOKEN	0xe0
/* OUT Token */
#define UIV_OUTTOKEN	0xe4
/* Controller */
#define UIV_RESUME	0xe6
#define UIV_CONDISC	0xe8
/* OTG */
#define UIV_OTGSTATE	0xea
/* EP OUT Short Packet */
#define UIV_EPOUTSHTPKT	0xec
#define UIV_OTGIRQ	0xd8


/** Endpoint Interrupt request */

/* OUT IRQ */
#define OUT03IEN	(USB_AOTG_BASE + 0x196)
#define OUTIEN	OUT03IEN
#define OUT03IRQ	(USB_AOTG_BASE + 0x18A)
#define OUTIRQ	OUT03IRQ
#define IRQ_EP3OUT	3
#define IRQ_EP2OUT	2
#define IRQ_EP1OUT	1
#define IRQ_EP0OUT	0

/* IN IRQ */
#define IN03IEN	(USB_AOTG_BASE + 0x194)
#define INIEN	IN03IEN
#define IN03IRQ	(USB_AOTG_BASE + 0x188)
#define INIRQ	IN03IRQ
#define IRQ_EP3IN	3
#define IRQ_EP2IN	2
#define IRQ_EP1IN	1
#define IRQ_EP0IN	0


/** IN Token IRQ */
#define IN03TOKIEN	(USB_AOTG_BASE + 0x19C)
#define IN_TOKIEN	IN03TOKIEN
#define IN03TOKIRQ	(USB_AOTG_BASE + 0x190)
#define IN_TOKIRQ	IN03TOKIRQ
#define EP0IN_TOKEN	0
#define EP1IN_TOKEN	1
#define EP2IN_TOKEN	2
#define EP3IN_TOKEN	3
#define EPIN_TOKEN_NUM	4
#define EPIN_TOKEN_MASK	0x0F

/** OUT Token IRQ */
#define OUT03TOKIEN	(USB_AOTG_BASE + 0x19D)
#define OUT_TOKIEN	OUT03TOKIEN
#define OUT03TOKIRQ	(USB_AOTG_BASE + 0x191)
#define OUT_TOKIRQ	OUT03TOKIRQ
#define EP0OUT_TOKEN	0
#define EP1OUT_TOKEN	1
#define EP2OUT_TOKEN	2
#define EP3OUT_TOKEN	3
#define EPOUT_TOKEN_NUM	4
#define EPOUT_TOKEN_MASK	0x0F


/************************************ FIFO ***********************************/


/*
 * AOTG USB Controller FIFO layout (add by order...)
 * Luckily, we don't need to take care of ep0 FIFO.
 *
 * URAM0 FIFO (only for ep1-in dma): 2KB [0x0, 0x800)
 * URAM1 FIFO (only for ep2-out dma): 2KB [0x800, 0x1000)
 * URAM2 FIFO (for ep1-out, ep2-in, ep3-in/out cpu): 2KB [0x1000, 0x1800)
 * URAM3 FIFO (only for ep0): 128-byte
 */
/* FIFO start address */
#define URAM0_FIFO_START	0x0
#define URAM1_FIFO_START	0x800
#define URAM2_FIFO_START	0x1000
#define NORMAL_FIFO_START	URAM2_FIFO_START
/* Arbitrary single independent FIFO block size */
#define NORMAL_FIFO_BLOCK	512
/*
 * Arbitrary allocaction for NORMAL_FIFO_START, need to optimize!
 */
#define EP1OUT_FIFO_START	NORMAL_FIFO_START
#define EP2IN_FIFO_START	(NORMAL_FIFO_START + NORMAL_FIFO_BLOCK)
#define EP3OUT_FIFO_START	(NORMAL_FIFO_START + NORMAL_FIFO_BLOCK * 2)
#define EP3IN_FIFO_START	(NORMAL_FIFO_START + NORMAL_FIFO_BLOCK * 3)


/** FIFO Control */
#define FIFOCTRL	(USB_AOTG_BASE + 0x1A8)
/* FIFO Auto */
#define FIFOCTRL_AUTO	5
/* FIFO Direction */
#define FIFOCTRL_IO_IN	(0x01 << 4)
/* Endpoint address */
#define FIFOCTRL_NUM_MASK	0x0F


/** FIFO Data */
#define FIFO1DAT	(USB_AOTG_BASE + 0x084)
#define FIFO2DAT	(USB_AOTG_BASE + 0x088)
#define FIFO3DAT	(USB_AOTG_BASE + 0x08C)
#define FIFOxDAT(ep)	(FIFO1DAT + (ep - 1) * 4)


/** FIFO Address */
#define EP1OUT_STADDR	(USB_AOTG_BASE + 0x304)
#define EP2OUT_STADDR	(USB_AOTG_BASE + 0x308)
#define EP3OUT_STADDR	(USB_AOTG_BASE + 0x30C)
#define EPxOUT_STADDR(ep)	(EP1OUT_STADDR + (ep - 1) * 4)

#define EP1IN_STADDR	(USB_AOTG_BASE + 0x344)
#define EP2IN_STADDR	(USB_AOTG_BASE + 0x348)
#define EP3IN_STADDR	(USB_AOTG_BASE + 0x34C)
#define EPxIN_STADDR(ep)	(EP1IN_STADDR + (ep - 1) * 4)


/** Ep0 In Data */
#define EP0INDATA	(USB_AOTG_BASE + 0x100)
#define EP0IN_FIFO	EP0INDATA
/** Ep0 Out Data */
#define EP0OUTDATA	(USB_AOTG_BASE + 0x140)
#define EP0OUT_FIFO	EP0OUTDATA
/** Setup Data */
#define SETUPDAT0	(USB_AOTG_BASE + 0x180)
#define SETUPDAT1	(USB_AOTG_BASE + 0x181)
#define SETUPDAT2	(USB_AOTG_BASE + 0x182)
#define SETUPDAT3	(USB_AOTG_BASE + 0x183)
#define SETUPDAT4	(USB_AOTG_BASE + 0x184)
#define SETUPDAT5	(USB_AOTG_BASE + 0x185)
#define SETUPDAT6	(USB_AOTG_BASE + 0x186)
#define SETUPDAT7	(USB_AOTG_BASE + 0x187)
#define SETUP_FIFO	SETUPDAT0


/** Byte Counter */
#define OUT0BC	(USB_AOTG_BASE + 0x000)
#define IN0BC	(USB_AOTG_BASE + 0x001)

/* Out Endpoint */
#define OUT1BCL	(USB_AOTG_BASE + 0x008)
#define OUT1BCH	(USB_AOTG_BASE + 0x009)
#define OUT1BC	OUT1BCL
#define OUT2BCL	(USB_AOTG_BASE + 0x010)
#define OUT2BCH	(USB_AOTG_BASE + 0x011)
#define OUT2BC	OUT2BCL
#define OUT3BCL	(USB_AOTG_BASE + 0x018)
#define OUT3BCH	(USB_AOTG_BASE + 0x019)
#define OUT3BC	OUT3BCL
#define OUTxBC(ep)	(OUT1BC + (ep - 1) * 8)

/* In Endpoint */
#define IN1BCL	(USB_AOTG_BASE + 0x00C)
#define IN1BCH	(USB_AOTG_BASE + 0x00D)
#define IN1BC	IN1BCL
#define IN2BCL	(USB_AOTG_BASE + 0x014)
#define IN2BCH	(USB_AOTG_BASE + 0x015)
#define IN2BC	IN2BCL
#define IN3BCL	(USB_AOTG_BASE + 0x01C)
#define IN3BCH	(USB_AOTG_BASE + 0x01D)
#define IN3BC	IN3BCL
#define INxBC(ep)	(IN1BC + (ep - 1) * 8)


/************************************ Endp ***********************************/


/** Endpoint 0 Control and Status */
#define EP0CS	(USB_AOTG_BASE + 0x002)
#define EP0CS_OUTBUSY	3
#define EP0CS_INBUSY	2
/* Endpoint 0 NAK */
#define EP0CS_NAK	1
/* Endpoint 0 STALL */
#define EP0CS_STALL	0


/** Endpoint Control and Status */
#define OUT1CS	(USB_AOTG_BASE + 0x00B)
#define OUT2CS	(USB_AOTG_BASE + 0x013)
#define OUT3CS	(USB_AOTG_BASE + 0x01B)
#define OUTxCS(ep)	(OUT1CS + (ep - 1) * 8)

#define IN1CS	(USB_AOTG_BASE + 0x00F)
#define IN2CS	(USB_AOTG_BASE + 0x017)
#define IN3CS	(USB_AOTG_BASE + 0x01F)
#define INxCS(ep)	(IN1CS + (ep - 1) * 8)

#define EPCS_AUTO_OUT	4
#define EPCS_NPAK	0x0C
#define EPCS_BUSY	1
#define EPCS_ERR	0


/** Endpoint Control */
#define OUT1CTRL	(USB_AOTG_BASE + 0x00A)
#define OUT2CTRL	(USB_AOTG_BASE + 0x012)
#define OUT3CTRL	(USB_AOTG_BASE + 0x01A)
#define OUTxCTRL(ep)	(OUT1CTRL + (ep - 1) * 8)

#define IN1CTRL	(USB_AOTG_BASE + 0x00E)
#define IN2CTRL	(USB_AOTG_BASE + 0x016)
#define IN3CTRL	(USB_AOTG_BASE + 0x01E)
#define INxCTRL(ep)	(IN1CTRL + (ep - 1) * 8)

#define EPCTRL_VALID	7
#define EPCTRL_STALL	6
#define EPCTRL_TYPE	0x0C
#define EPCTRL_BUF	0x03
/* 3 ISO packets per microframe */
#define EPCTRL_ISOC_3	5
/* 2 ISO packets per microframe */
#define EPCTRL_ISOC_2	4


/** USB Control and Status */
#define USBCS	(USB_AOTG_BASE + 0x1A3)
/* Soft disconnect */
#define USBCS_DISCONN	6
/* Remote wakeup */
#define USBCS_WAKEUP	5


/** Endpoint Reset */
#define EPRST	(USB_AOTG_BASE + 0x1A2)
/* Endpoint FIFO Reset */
#define EPRST_FIFORST	6
/* Endpoint Toggle Reset */
#define EPRST_TOGRST	5
/* Endpoint Direction */
#define EPRST_IO_IN	(0x01 << 4)
/* Endpoint address */
#define EPRST_NUM_MASK	0x0F


/** Ep max packet size */

/* Ep0 max packet size: not configure */
#define OUT0MAXPKT	(USB_AOTG_BASE + 0x1E0)

/* Ep-out max packet size */
#define OUT1MAXPKTL	(USB_AOTG_BASE + 0x1E2)
#define OUT1MAXPKTH	(USB_AOTG_BASE + 0x1E3)
#define OUT1MAXPKT	OUT1MAXPKTL
#define OUT2MAXPKTL	(USB_AOTG_BASE + 0x1E4)
#define OUT2MAXPKTH	(USB_AOTG_BASE + 0x1E5)
#define OUT2MAXPKT	OUT2MAXPKTL
#define OUT3MAXPKTL	(USB_AOTG_BASE + 0x1E6)
#define OUT3MAXPKTH	(USB_AOTG_BASE + 0x1E7)
#define OUT3MAXPKT	OUT3MAXPKTL
#define OUTxMAXPKT(ep)	(OUT1MAXPKT + (ep - 1) * 2)

/* Ep-in max packet size */
#define IN1MAXPKTL	(USB_AOTG_BASE + 0x3E2)
#define IN1MAXPKTH	(USB_AOTG_BASE + 0x3E3)
#define IN1MAXPKT	IN1MAXPKTL
#define IN2MAXPKTL	(USB_AOTG_BASE + 0x3E4)
#define IN2MAXPKTH	(USB_AOTG_BASE + 0x3E5)
#define IN2MAXPKT	IN2MAXPKTL
#define IN3MAXPKTL	(USB_AOTG_BASE + 0x3E6)
#define IN3MAXPKTH	(USB_AOTG_BASE + 0x3E7)
#define IN3MAXPKT	IN3MAXPKTL
#define INxMAXPKT(ep)	(IN1MAXPKT + (ep - 1) * 2)


/************************************ PHY ************************************/


#define USBPHYCTRL	(USB_AOTG_BASE + 0x423)
#define PHY_PLLEN	7
#define PHY_DALLUALLEN	6

#define VDCTRL	(USB_AOTG_BASE + 0x424)
#define VDSTATE	(USB_AOTG_BASE + 0x425)
#define USBEFUSEREF	(USB_AOTG_BASE + 0x426)


/************************************ DMA ************************************/

/* IN Endpoint DMA */
#define IN1_DMACTL	(USB_AOTG_BASE + 0x430)

#define IN1_DMALEN1L	(USB_AOTG_BASE + 0x434)
#define IN1_DMALEN1M	(USB_AOTG_BASE + 0x435)
#define IN1_DMALEN1H	(USB_AOTG_BASE + 0x436)
#define IN1_DMALEN2L	(USB_AOTG_BASE + 0x438)
#define IN1_DMALEN2M	(USB_AOTG_BASE + 0x439)
#define IN1_DMALEN2H	(USB_AOTG_BASE + 0x43A)

#define IN1_DMAREMAIN1L	(USB_AOTG_BASE + 0x43C)
#define IN1_DMAREMAIN1M	(USB_AOTG_BASE + 0x43D)
#define IN1_DMAREMAIN1H	(USB_AOTG_BASE + 0x43E)
#define IN1_DMAREMAIN2L	(USB_AOTG_BASE + 0x440)
#define IN1_DMAREMAIN2M	(USB_AOTG_BASE + 0x441)
#define IN1_DMAREMAIN2H	(USB_AOTG_BASE + 0x442)

/* OUT Endpoint DMA */
#define OUT2_DMACTL	(USB_AOTG_BASE + 0x444)

#define OUT2_DMALENL	(USB_AOTG_BASE + 0x448)
#define OUT2_DMALENM	(USB_AOTG_BASE + 0x449)
#define OUT2_DMALENH	(USB_AOTG_BASE + 0x44A)

#define OUT2_DMAREMAINL	(USB_AOTG_BASE + 0x44C)
#define OUT2_DMAREMAINM	(USB_AOTG_BASE + 0x44D)
#define OUT2_DMAREMAINH	(USB_AOTG_BASE + 0x44E)





#define IN1_HCOUT1DMACTL	(USB_AOTG_BASE + 0x430)
#define IN1_HCOUT1DMALEN1L	(USB_AOTG_BASE + 0x434)
#define IN1_HCOUT1DMALEN1M	(USB_AOTG_BASE + 0x435)
#define IN1_HCOUT1DMALEN1H	(USB_AOTG_BASE + 0x436)
#define IN1_HCOUT1DMALEN2H	(USB_AOTG_BASE + 0x43A)

#define OUT2_HCIN2DMACTL	(USB_AOTG_BASE + 0x444)
#define OUT2_HCIN2DMALENL	(USB_AOTG_BASE + 0x448)
#define OUT2_HCIN2DMALENM	(USB_AOTG_BASE + 0x449)
#define OUT2_HCIN2DMALENH	(USB_AOTG_BASE + 0x44A)


/************************************ URAM ***********************************/


#define URAM0_ADDR	((void *)0x35000)
#define URAM1_ADDR	((void *)0x35800)
#define URAM2_ADDR	((void *)0x38000)

#define URAM0_SIZE	2048
#define URAM1_SIZE	2048
#define URAM2_SIZE	2048

#endif /* __USB_AOTG_REGISTERS_H__ */

