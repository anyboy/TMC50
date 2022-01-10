#ifndef _SDIO_PORT_H_
#define _SDIO_PORT_H_

//output timing
// 0: cmd  [0]:positive [1]:negative
// 1: data [0]:positive [1]:negative
#define SDIO_DEF_OUTPUT_TIMING 		3 // 0
extern bool sdio_writeb_cmd52(u32 addr, u8 data);
extern bool sdio_write_cmd53(u32 dataPort,u8 *dat, size_t size);
extern bool sdio_read_cmd53(u32 dataPort,u8 *dat, size_t size);
extern bool _sdio_read_reg(u32 addr, u32 *data);
extern bool sdio_host_init(void (*sdio_isr)(void));
extern bool sdio_host_enable_isr(bool enable);
extern bool is_truly_isr();
#endif /* _SDIO_PORT_H_ */

