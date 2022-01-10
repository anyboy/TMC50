/*
 * Copyright (c) 2016 Actions Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief USB aotg host controller driver private definitions
 *
 * This file contains the USB aotg host controller driver private
 * definitions.
 */

#ifndef __AOTG_HCD_REGISTERS_H__
#define __AOTG_HCD_REGISTERS_H__

#include <board.h>

/* usb aotg hcd irq line */
#define AOTG_HCD_IRQ IRQ_USB

// common resource
//#define	CMU_ANA_REG_BASE	0xC0000100
//#define	SPLL_CTL			(CMU_ANA_REG_BASE+0x08)

//#define	CMU_CTL_REG_BASE	0xC0001000
//#define	CMU_DEVCLKEN		(CMU_CTL_REG_BASE+0x04)
#define	CMU_USBCLKEN		2

//#define	CMU_MEMCLKSEL		(CMU_CTL_REG_BASE+0x28)
#define	CMU_URAMCLKSEL		1

//#define	RMU_DIG_BASE		0xc0000000
//#define	MRCR				(RMU_DIG_BASE)
#define	MRCR				(RMU_MRCR)
#define USB_RESET1			9
#define USB_RESET2			10

#define	USB_REG_BASE		0xC0080000
#define	USBIRQ 				(USB_REG_BASE+0x1BC)
#define	USBSTATE 			(USB_REG_BASE+0x1BD)
#define	USBCTRL 			(USB_REG_BASE+0x1BE)
#define	USBSTATUS 			(USB_REG_BASE+0x1BF)
#define	USBIEN 				(USB_REG_BASE+0x1C0)
#define	HCEP0CTRL 				(USB_REG_BASE+0x0C0)
#define	HCOUT1CTRL 				(USB_REG_BASE+0x0C4)
#define	HCOUT2CTRL 				(USB_REG_BASE+0x0C8)
#define	HCOUT0ERR 				(USB_REG_BASE+0x0C1)
#define	HCOUT1ERR 				(USB_REG_BASE+0x0C5)
#define	HCOUT2ERR 				(USB_REG_BASE+0x0C9)
#define	HCIN1CTRL 				(USB_REG_BASE+0x0C6)
#define	HCIN0ERR 				(USB_REG_BASE+0x0C3)
#define	HCIN1ERR 				(USB_REG_BASE+0x0C7)
#define	HCPORTCTRL 				(USB_REG_BASE+0x1AB)
#define	HCFRMNL 				(USB_REG_BASE+0x1AC)
#define	HCFRMNH 				(USB_REG_BASE+0x1AD)
#define	HCFRMREMAINL 				(USB_REG_BASE+0x1AE)
#define	HCFRMREMAINH 				(USB_REG_BASE+0x1AF)
#define	HCIN01ERRIRQ 				(USB_REG_BASE+0x1B4)
#define	HCOUT02ERRIRQ 				(USB_REG_BASE+0x1B6)
#define	HCIN01ERRIEN 				(USB_REG_BASE+0x1B8)
#define	HCOUT02ERRIEN 				(USB_REG_BASE+0x1BA)
#define	HCIN0MAXPCK 				(USB_REG_BASE+0x1E0)
#define	HCIN1MAXPCKL 				(USB_REG_BASE+0x1E2)
#define	HCIN1MAXPCKH 				(USB_REG_BASE+0x1E3)
#define	HCOUT1MAXPCKL 				(USB_REG_BASE+0x3E2)
#define	HCOUT2MAXPCKL 				(USB_REG_BASE+0x3E4)
#define	HCOUT1MAXPCKH 				(USB_REG_BASE+0x3E3)
#define	HCOUT2MAXPCKH 				(USB_REG_BASE+0x3E5)
#define	OUT0BC_HCIN0BC 				(USB_REG_BASE+0x000)
#define	IN0BC_HCOUT0BC 				(USB_REG_BASE+0x001)
#define	EP0CS_HCEP0CS 				(USB_REG_BASE+0x002)
#define	OUT1BCL_HCIN1BCL 				(USB_REG_BASE+0x008)
#define	OUT1BCH_HCIN1BCH 				(USB_REG_BASE+0x009)
#define	OUT1CS_HCIN1CS 				(USB_REG_BASE+0x00B)
#define	OUT1CTRL_HCIN1CTRL 				(USB_REG_BASE+0x00A)
#define	IN1BCL_HCOUT1BCL 				(USB_REG_BASE+0x00C)
#define	IN2BCL_HCOUT2BCL 				(USB_REG_BASE+0x014)
#define	IN1BCH_HCOUT1BCH 				(USB_REG_BASE+0x00D)
#define	IN2BCH_HCOUT2BCH 				(USB_REG_BASE+0x015)
#define	IN1CS_HCOUT1CS 				(USB_REG_BASE+0x00F)
#define	IN2CS_HCOUT2CS 				(USB_REG_BASE+0x017)
#define	IN1CTRL_HCOUT1CTRL 				(USB_REG_BASE+0x00E)
#define	IN2CTRL_HCOUT2CTRL 				(USB_REG_BASE+0x016)
#define	FIFO1DAT 				(USB_REG_BASE+0x084)
#define	FIFO2DAT 				(USB_REG_BASE+0x088)
#define	EP0INDATA 				(USB_REG_BASE+0x100)
#define	EP0OUTDATA 				(USB_REG_BASE+0x140)
#define	SETUPDAT0 				(USB_REG_BASE+0x180)
#define	SETUPDAT1 				(USB_REG_BASE+0x181)
#define	SETUPDAT2 				(USB_REG_BASE+0x182)
#define	SETUPDAT3 				(USB_REG_BASE+0x183)
#define	SETUPDAT4 				(USB_REG_BASE+0x184)
#define	SETUPDAT5 				(USB_REG_BASE+0x185)
#define	SETUPDAT6 				(USB_REG_BASE+0x186)
#define	SETUPDAT7 				(USB_REG_BASE+0x187)
#define	OUT01IRQ 				(USB_REG_BASE+0x18A)
#define	IN02IRQ 				(USB_REG_BASE+0x188)
#define	USBIRQ_HCUSBIRQ 				(USB_REG_BASE+0x18C)
#define	OUT01IEN 				(USB_REG_BASE+0x196)
#define	IN02IEN 				(USB_REG_BASE+0x194)
#define USBIEN_HCUSBIEN 				(USB_REG_BASE+0x198)
#define IN02TOKIRQ 				(USB_REG_BASE+0x190)
#define OUT01TOKIRQ 				(USB_REG_BASE+0x191)
#define IN02TOKIEN 				(USB_REG_BASE+0x19C)
#define OUT01TOKIEN 				(USB_REG_BASE+0x19D)
#define IVECT 				(USB_REG_BASE+0x1A0)
#define EPRST 				(USB_REG_BASE+0x1A2)
#define USBCTRL_STUS 				(USB_REG_BASE+0x1A3)
#define FRMCNTL 				(USB_REG_BASE+0x1A4)
#define FRMCNTH 				(USB_REG_BASE+0x1A5)
#define FNADDR 				(USB_REG_BASE+0x1A6)
#define CLKGATE 				(USB_REG_BASE+0x1A7)
#define FIFOCTRL 				(USB_REG_BASE+0x1A8)
#define OUT1STADDR 				(USB_REG_BASE+0x304)
#define IN1STADDR 				(USB_REG_BASE+0x344)
#define IN2STADDR 				(USB_REG_BASE+0x348)
#define USBEIRQ 				(USB_REG_BASE+0x400)
#define NAKOUTCTRL 				(USB_REG_BASE+0x401)
#define HCINCTRL 				(USB_REG_BASE+0x402)
#define HCINENDINT 				(USB_REG_BASE+0x41F)
#define OSHRTPCKIR 				(USB_REG_BASE+0x403)
#define USBDEBUG 				(USB_REG_BASE+0x404)
#define HCOUTEMPINTCTRL 				(USB_REG_BASE+0x406)
#define OTGSTAIEN 				(USB_REG_BASE+0x410)
#define OTGSTAIR 				(USB_REG_BASE+0x411)
#define HCIN1_COUNTL 				(USB_REG_BASE+0x414)
#define HCIN1_COUNTH 				(USB_REG_BASE+0x415)
#define IDVBUSCTRL 				(USB_REG_BASE+0x418)
#define LINESTATUS 				(USB_REG_BASE+0x419)
#define DPDMCTRL 				(USB_REG_BASE+0x41A)
#define AUTOINTIMER 				(USB_REG_BASE+0x41E)
#define USBBIST 				(USB_REG_BASE+0x425)

/* IVECT */
#define	UIV_SUDAV           	    (0x00)
#define UIV_SOF            	 	    (0x04)
#define UIV_SUTOK           	    (0x08)
#define UIV_SUSPEND        	        (0x0c)
#define	UIV_USBRESET        	    (0x10)
#define	UIV_HSPEED          	    (0x14)
#define	UIV_HCOUT0ERR          	    (0x16)
#define	UIV_EP0IN           		(0x18)
#define	UIV_HCIN0ERR           		(0x1a)
#define	UIV_EP0OUT          	    (0x1c)
#define	UIV_EP0PING         	    (0x20)
#define UIV_HCOUT1ERR			    (0x22)
#define	UIV_EP1IN          		    (0x24)
#define	UIV_HCIN1ERR          		(0x26)
#define	UIV_EP1OUT          	    (0x28)
#define	UIV_EP1PING         	    (0x2c)
#define	UIV_HCOUT2ERR         	    (0x2e)
#define	UIV_EP2IN           		(0x30)
#define UIV_HCIN2ERR			    (0x32)
#define	UIV_EP2OUT          	    (0x34)
#define	UIV_EP2PING         	    (0x38)
#define	UIV_OTGIRQ          	    (0xd8)



#define	LINESTATUS_OTGRESET		0

#define DPDMCTRL_PLUGIN			6
#define	DPDMCTRL_LINEDETEN		4
#define	DPDMCTRL_DMPUEN			3
#define	DPDMCTRL_DPPUEN			2
#define	DPDMCTRL_DMPDDIS		1
#define	DPDMCTRL_DPPDDIS		0

#define USBCTRL_BUSREQ			0

#define USBSTATE_ST3_ST0_e		3
#define	USBSTATE_ST3_ST0_MASK	(0xf<<0)

/* OTG state machine */
#define FSM_A_WAIT_BCON			0x02
#define FSM_A_HOST				0x03

/* USBEIRQ */
#define USBEIRQ_USBIRQ			7

/* USBIRQ_HCUSBIRQ */
#define	USBIRQ_NTRIRQ			6
#define	USBIRQ_RSTIRQ			4
#define	USBIRQ_SUSPIRQ			3
#define	USBIRQ_SUTOKIRQ			2
#define	USBIRQ_SOFIRQ			1
#define	USBIRQ_SUDAVIRQ			0

#define	HCPORTCTRL_PORTRST		5

#define EPRST_EPNUM_MASK		(0xf)
#define EPRST_IO_HCIO		4
#define EPRST_TOGRST			5
#define EPRST_FIFORST			6

#define EP0CS_HCSET				4

#define HCOUT02ERRIRQ_EP0		0x01
#define HCOUT02ERRIRQ_EP1		0x02
#define HCOUT02ERRIRQ_EP2		0x04

#define	EP0CS_HXSETTOGGLE		6
#define	EP0CS_HCCLRTOGGLE		5
#define	EP0CS_HCSET				4
#define	EP0CS_HCINBSY			3
#define	EP0CS_HCOUTBSY			2
#define	EP0CS_HSNAK				1
#define	EP0CS_STALL				0

#define TOGGLE_BIT_CLEAR		0x00
#define TOGGLE_BIT_SET			0x01

#define FIFOCTRL_FIFOAUTO		5
#define FIFOCTRL_IO_HCIO		4

/* default bulk-in ep num */
#define	BULK_IN_EP_NUM			1
/* default bulk-out ep num */
#define	BULK_OUT_EP_NUM			2

#define	OUT1CS_HCBUSY			1
#define	IN2CS_HCBUSY			1

#define HCINCTRL_HCIN1START		1

#endif	/* __AOTG_HCD_REGISTERS_H__ */
