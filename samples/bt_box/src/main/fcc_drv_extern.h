
#ifndef _FCC_DRV_EXTERN_H_
#define _FCC_DRV_EXTERN_H_

#define FCC_WORD_MODE_SHIFT   (0) /* 0 - fcc */
#define FCC_UART_PORT_SHIFT   (2) /* 0 - uart0, 1 - uart1 */
#define FCC_UART_BAUD_SHIFT   (16) /* baud / 100, 1152 = 115200 */

#define FCC_UART_BAUD         (115200)

#define FCC_DRV_ENTRY_BASE_ADDR                  (0xbfc42000)

#define FCC_DRV_INSTALL_ENTRY_ADDR               (*(unsigned int *)(FCC_DRV_ENTRY_BASE_ADDR + 0x0))
#define system_install_fcc_drv(a)                ((int(*)(unsigned int))FCC_DRV_INSTALL_ENTRY_ADDR)((a))

extern int system_enter_FCC_mode(void);

#endif
