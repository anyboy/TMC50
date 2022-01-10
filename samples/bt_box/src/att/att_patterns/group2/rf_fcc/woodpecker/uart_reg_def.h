
#ifndef _UART_REG_DEF_H_
#define _UART_REG_DEF_H_

//--------------GPIO_MFP-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     GPIO_MFP_BASE                                                     0xC0090000
#define     GPIO0_CTL                                                         (GPIO_MFP_BASE+0x0000)
//--------------Bits Location------------------------------------------//
#define     GPIO0_CTL_GPIO_INTC_MSK                                           25
#define     GPIO0_CTL_GPIO_TRIG_CTL_E                                         23
#define     GPIO0_CTL_GPIO_TRIG_CTL_SHIFT                                     21
#define     GPIO0_CTL_GPIO_TRIG_CTL_MASK                                      (0x7<<21)
#define     GPIO0_CTL_GPIO_INTCEN                                             20
#define     GPIO0_CTL_PADDRV_E                                                14
#define     GPIO0_CTL_PADDRV_SHIFT                                            12
#define     GPIO0_CTL_PADDRV_MASK                                             (0x7<<12)
#define     GPIO0_CTL_GPIO10KPUEN                                             11
#define     GPIO0_CTL_GPIO100KPDEN                                            9
#define     GPIO0_CTL_GPIO50KPUEN                                             8
#define     GPIO0_CTL_GPIOINEN                                                7
#define     GPIO0_CTL_GPIOOUTEN                                               6
#define     GPIO0_CTL_SMIT                                                    5
#define     GPIO0_CTL_AD_SELECT                                               4
#define     GPIO0_CTL_MFP_E                                                   3
#define     GPIO0_CTL_MFP_SHIFT                                               0
#define     GPIO0_CTL_MFP_MASK                                                (0xF<<0)

//--------------RMU-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     RMU_BASE                                                          0xC0000000
#define     MRCR0                                                             (RMU_BASE+0x0000)
//--------------Bits Location------------------------------------------//
#define     MRCR0_UART1_RESET                                                 19
#define     MRCR0_UART0_RESET                                                 18

//--------------CMU_DIGITAL-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     CMU_DIGITAL_BASE                                                  0xC0001000
#define     CMU_DEVCLKEN0                                                     (CMU_DIGITAL_BASE+0x0008)
//--------------Bits Location------------------------------------------//
#define     CMU_DEVCLKEN0_UART1CLKEN                                          19
#define     CMU_DEVCLKEN0_UART0CLKEN                                          18

//--------------UART0-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     UART0_BASE                                                        0xC01A0000
#define     UART0_CTL                                                         (UART0_BASE+0x0000)
#define     UART0_RXDAT                                                       (UART0_BASE+0x0004)
#define     UART0_TXDAT                                                       (UART0_BASE+0x0008)
#define     UART0_STA                                                         (UART0_BASE+0x000c)
#define     UART0_BR                                                          (UART0_BASE+0x0010)

//--------------Bits Location------------------------------------------//
#define     UART0_CTL_RXENABLE                                                31
#define     UART0_CTL_TXENABLE                                                30
#define     UART0_CTL_TX_FIFO_EN                                              29
#define     UART0_CTL_RX_FIFO_EN                                              28
#define     UART0_CTL_TX_FIFO_SEL                                             26
#define     UART0_CTL_RX_FIFO_SEL                                             24
#define     UART0_CTL_AFL_E                                                   23
#define     UART0_CTL_AFL_SHIFT                                               22
#define     UART0_CTL_AFL_MASK                                                (0x3<<22)
#define     UART0_CTL_DBGSEL                                                  21
#define     UART0_CTL_LBEN                                                    20
#define     UART0_CTL_TXIE                                                    19
#define     UART0_CTL_RXIE                                                    18
#define     UART0_CTL_TXDE                                                    17
#define     UART0_CTL_RXDE                                                    16
#define     UART0_CTL_EN                                                      15
#define     UART0_CTL_RTSE                                                    13
#define     UART0_CTL_AFE                                                     12
#define     UART0_CTL_RDIC_E                                                  11
#define     UART0_CTL_RDIC_SHIFT                                              10
#define     UART0_CTL_RDIC_MASK                                               (0x3<<10)
#define     UART0_CTL_TDIC_E                                                  9
#define     UART0_CTL_TDIC_SHIFT                                              8
#define     UART0_CTL_TDIC_MASK                                               (0x3<<8)
#define     UART0_CTL_PRS_E                                                   6
#define     UART0_CTL_PRS_SHIFT                                               4
#define     UART0_CTL_PRS_MASK                                                (0x7<<4)
#define     UART0_CTL_STPS                                                    2
#define     UART0_CTL_DWLS_E                                                  1
#define     UART0_CTL_DWLS_SHIFT                                              0
#define     UART0_CTL_DWLS_MASK                                               (0x3<<0)

#define     UART0_RXDAT_RXDAT_E                                               7
#define     UART0_RXDAT_RXDAT_SHIFT                                           0
#define     UART0_RXDAT_RXDAT_MASK                                            (0xFF<<0)

#define     UART0_TXDAT_TXDAT_E                                               7
#define     UART0_TXDAT_TXDAT_SHIFT                                           0
#define     UART0_TXDAT_TXDAT_MASK                                            (0xFF<<0)

#define     UART0_STA_PAER                                                    23
#define     UART0_STA_STER                                                    22
#define     UART0_STA_UTBB                                                    21
#define     UART0_STA_TXFL_E                                                  20
#define     UART0_STA_TXFL_SHIFT                                              16
#define     UART0_STA_TXFL_MASK                                               (0x1F<<16)
#define     UART0_STA_RXFL_E                                                  15
#define     UART0_STA_RXFL_SHIFT                                              11
#define     UART0_STA_RXFL_MASK                                               (0x1F<<11)
#define     UART0_STA_TFES                                                    10
#define     UART0_STA_RFFS                                                    9
#define     UART0_STA_RTSS                                                    8
#define     UART0_STA_CTSS                                                    7
#define     UART0_STA_TFFU                                                    6
#define     UART0_STA_RFEM                                                    5
#define     UART0_STA_RXST                                                    4
#define     UART0_STA_TFER                                                    3
#define     UART0_STA_RXER                                                    2
#define     UART0_STA_TIP                                                     1
#define     UART0_STA_RIP                                                     0

#define     UART0_BR_TXBRDIV_E                                                31
#define     UART0_BR_TXBRDIV_SHIFT                                            16
#define     UART0_BR_TXBRDIV_MASK                                             (0xFFFF<<16)
#define     UART0_BR_RXBRDIV_E                                                15
#define     UART0_BR_RXBRDIV_SHIFT                                            0
#define     UART0_BR_RXBRDIV_MASK                                             (0xFFFF<<0)

//--------------UART1-------------------------------------------//
//--------------Register Address---------------------------------------//
#define     UART1_BASE                                                        0xC01B0000
#define     UART1_CTL                                                         (UART1_BASE+0x0000)
#define     UART1_RXDAT                                                       (UART1_BASE+0x0004)
#define     UART1_TXDAT                                                       (UART1_BASE+0x0008)
#define     UART1_STA                                                         (UART1_BASE+0x000c)
#define     UART1_BR                                                          (UART1_BASE+0x0010)

#endif
