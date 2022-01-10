/*
 * Copyright (c) 2016 Intel Corporation
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

#include <zephyr.h>
#include <device.h>
#include <errno.h>
#include <misc/util.h>
#include <misc/byteorder.h>
#include "sdio_io.h"

#define SDIO_WIFI_BLK_SIZE	512
struct sdio_func *wifi_sdio_func = NULL;
static sdio_irq_handler_t *wifi_sdio_handler = NULL;

//#define TEST_WIFI_SDIO
#ifdef TEST_WIFI_SDIO
#define ADDR_MASK 0x10000
#define LOCAL_ADDR_MASK 0x00000
static int rtw_fake_driver_probe(struct sdio_func *func);
#endif /* TEST_WIFI_SDIO */

static int sdio_bus_probe(void)
{
	int ret = 0;

	ret = sdio_bus_scan(0);
	if (ret) {
		printk("Cannot found device on sdio bus\n\r");
		return ret;
	}

	printk("Found wifi device\n");

	wifi_sdio_func = sdio_get_func(0);
	if (wifi_sdio_func == NULL) {
		printk("sdio_get_func wifi_sdio_func = NULL\r\n");
		return -1;
	}

	sdio_claim_host(wifi_sdio_func);
	ret = sdio_set_block_size(wifi_sdio_func, SDIO_WIFI_BLK_SIZE);
	sdio_release_host(wifi_sdio_func);
	if (ret) {
		printk("sdio_set_block_size, ret = %d\r\n", ret);
		return ret;
	}

	sdio_claim_host(wifi_sdio_func);
	ret = sdio_enable_func(wifi_sdio_func);
	sdio_release_host(wifi_sdio_func);
	if (ret) {
		printk("sdio_enable_func, ret = %d\r\n", ret);
		return ret;
	}

#ifdef TEST_WIFI_SDIO
	rtw_fake_driver_probe(wifi_sdio_func);
#endif

	return 0;
}

static int sdio_bus_remove(void)
{
	printk("%s,%d, do nothing\n", __func__, __LINE__);
	return 0;
}

static unsigned short sdio_readw(struct sdio_func *func, unsigned int addr, int *err_ret)
{
	unsigned short val = 0xFFFF;
	int ret;

	ret = sdio_memcpy_fromio(func, &val, addr, 2);
	if (err_ret)
		*err_ret = ret;

	return ret ? 0xFFFF : sys_le16_to_cpu(val);
}

static unsigned int sdio_readl(struct sdio_func *func, unsigned int addr, int *err_ret)
{
	unsigned int val = 0xFFFFFFFF;
	int ret;

	ret = sdio_memcpy_fromio(func, &val, addr, 4);
	if (err_ret)
		*err_ret = ret;

	return ret ? 0xFFFFFFFF : sys_le32_to_cpu(val);
}

static void sdio_writew(struct sdio_func *func, unsigned short val, unsigned int addr, int *err_ret)
{
	int ret;

	val = sys_cpu_to_le16(val);

	ret = sdio_memcpy_toio(func, addr, &val, 2);
	if (err_ret)
		*err_ret = ret;
}

static void sdio_writel(struct sdio_func *func, unsigned int val, unsigned int addr, int *err_ret)
{
	int ret;

	val = sys_cpu_to_le32(val);

	ret = sdio_memcpy_toio(func, addr, &val, 4);
	if (err_ret)
		*err_ret = ret;
}

static void sdio_hander_wrapper(struct sdio_func *func)
{
	if (wifi_sdio_handler) {
		sdio_claim_host(func);
		wifi_sdio_handler(func);
		sdio_release_host(func);
	}
}

static int sdio_claim_irq_wrapper(struct sdio_func *func, sdio_irq_handler_t *handler)
{
	wifi_sdio_handler = handler;
	return sdio_claim_irq(func, sdio_hander_wrapper);
}

static int sdio_release_irq_wrapper(struct sdio_func *func)
{
	sdio_release_irq(func);
	wifi_sdio_handler = NULL;
	return 0;
}

#ifdef TEST_WIFI_SDIO
static int wifi_read(struct sdio_func *func, u32_t addr, u32_t cnt, void *pdata)
{
	int err;

	sdio_claim_host(func);
	err = sdio_memcpy_fromio(func, pdata, addr, cnt);
	sdio_release_host(func);

	if (err)
		printk("%s: FAIL(%d)! ADDR=%#x Size=%d\n", __func__, err, addr, cnt);

	return err;
}

static int wifi_write(struct sdio_func *func, u32_t addr, u32_t cnt, void *pdata)
{
	int err;

	sdio_claim_host(func);
	err = sdio_memcpy_toio(func, addr, pdata, cnt);
	sdio_release_host(func);

	if (err)
		printk("%s: FAIL(%d)! ADDR=%#x Size=%d\n", __func__, err, addr, cnt);

	return err;
}

static u8_t wifi_readb(struct sdio_func *func, u32_t addr)
{
	int err = 0;
	u8_t ret = 0;

	sdio_claim_host(func);
	ret = sdio_readb(func, ADDR_MASK | addr, &err);
	sdio_release_host(func);

	if (err)
		printk("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);

	return ret;
}

static u16_t wifi_readw(struct sdio_func *func, u32_t addr)
{
	int err = 0;
	u16_t v;

	sdio_claim_host(func);
	v = sdio_readw(func, ADDR_MASK | addr, &err);
	sdio_release_host(func);

	if (err)
		printk("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);

	return  v;
}

static u32_t wifi_readl(struct sdio_func *func, u32_t addr)
{
	int err = 0;
	u32_t v;

	sdio_claim_host(func);
	v = sdio_readl(func, ADDR_MASK | addr, &err);
	sdio_release_host(func);

	if (err)
		printk("%s: FAIL!(%d) addr=0x%05x\n", __func__, err, addr);

	return  v;
}

static void wifi_writeb(struct sdio_func *func, u32_t addr, u8_t val)
{
	int err = 0;

	sdio_claim_host(func);
	sdio_writeb(func, val, ADDR_MASK | addr, &err);
	sdio_release_host(func);

	if (err)
		printk("%s: FAIL!(%d) addr=0x%05x val=0x%02x\n", __func__, err, addr, val);
}

static void wifi_writew(struct sdio_func *func, u32_t addr, u16_t v)
{
	int err = 0;

	sdio_claim_host(func);
	sdio_writew(func, v, ADDR_MASK | addr, &err);
	sdio_release_host(func);

	if (err)
		printk("%s: FAIL!(%d) addr=0x%05x val=0x%04x\n", __func__, err, addr, v);
}

static void wifi_writel(struct sdio_func *func, u32_t addr, u32_t v)
{
	int err = 0;

	sdio_claim_host(func);
	sdio_writel(func, v, ADDR_MASK | addr, &err);
	sdio_release_host(func);

	if (err)
		printk("%s: FAIL!(%d) addr=0x%05x val=0x%04x\n", __func__, err, addr, v);
}

static u8_t wifi_readb_local(struct sdio_func *func, u32_t addr)
{
	int err;
	u8_t ret = 0;

	ret = sdio_readb(func, LOCAL_ADDR_MASK | addr, &err);

	return ret;
}

static void wifi_writeb_local(struct sdio_func *func, u32_t addr, u8_t val)
{
	int err;

	sdio_writeb(func, val, LOCAL_ADDR_MASK | addr, &err);
}

static void rtw_wifi_card_enable_flow(struct sdio_func *func)
{
	//RTL8188F_TRANS_CARDDIS_TO_CARDEMU
	wifi_writeb_local(func, 0x0086, wifi_readb_local(func, 0x0086) & (~(BIT(0))));
	k_sleep(20);
	if(BIT(1) != (wifi_readb_local(func, 0x0086) & BIT(1))) printk("wifi: power state to suspend fail \n");
	wifi_writeb(func, 0x0005, wifi_readb(func, 0x0005) & ~(BIT(3)|BIT(4)));

	//RTL8188F_TRANS_CARDEMU_TO_ACT
	wifi_writeb(func, 0x0005, wifi_readb(func, 0x0005) & ~(BIT(2)));
	k_sleep(20);
	if(BIT(1) != (wifi_readb(func, 0x06) & BIT(1))) printk("wifi: power state to ready fail \n");
	wifi_writeb(func, 0x0005, wifi_readb(func, 0x0005) & ~(BIT(7)));
	wifi_writeb(func, 0x0005, wifi_readb(func, 0x0005) & ~(BIT(3)));
	wifi_writeb(func, 0x0005, wifi_readb(func, 0x0005) | (BIT(0)));
	k_sleep(20);
 	if(0 != (wifi_readb(func, 0x05) & BIT(0))) printk("wifi: power on fail \n");
	wifi_writeb(func, 0x0027, 0x35);
}

static void rtw_wifi_card_disable_flow(struct sdio_func *func)
{
	//RTL8188F_TRANS_ACT_TO_CARDEMU
	wifi_writeb(func, 0x001F, 0x00);
	wifi_writeb(func, 0x004E, wifi_readb(func, 0x004E) & ~(BIT(7)));
	wifi_writeb(func, 0x0027, 0x34);
	wifi_writeb(func, 0x0005, wifi_readb(func, 0x0005) | (BIT(1)));
	k_sleep(20);
	if(0 != (wifi_readb(func, 0x05) & BIT(1))) printk("wifi: turn off MAC fail \n");

	//RTL8188F_TRANS_CARDEMU_TO_CARDDIS
	if(BIT(5) != (wifi_readb(func, 0x0007) & BIT(5)))
	{
		printk("wifi: SYMON register clock is off, enable SYMON register clock \n");
		wifi_writeb(func, 0x0007, (wifi_readb(func, 0x0007) | BIT(5)));
	}
	wifi_writeb(func, 0x0005, wifi_readb(func, 0x0005) | (BIT(3)));
	wifi_writeb(func, 0x0005, wifi_readb(func, 0x0005) & ~(BIT(4)));
	wifi_writeb_local(func, 0x0086, wifi_readb_local(func, 0x0086) | (BIT(0)));
	k_sleep(20);
	if(0 != (wifi_readb_local(func, 0x0086) & BIT(1))) printk("wifi: power state to suspend fail \n");
}

static int rtw_fake_driver_probe(struct sdio_func *func)
{
	volatile u32_t val32 = 0;
	volatile u16_t val16 = 0;
	volatile u8_t  val8 = 0;

	printk("wifi: %s enter\n",__func__);

	if (!func)
		return -1;

	rtw_wifi_card_enable_flow(func);

	printk("wifi: CMD52 read(08) test:0x93 is 0x%x, 0x23 is 0x%x, 0x07 is 0x%x \n",
		wifi_readb(func, 0x93), wifi_readb(func, 0x23),
		wifi_readb(func, 0x07));

	printk("wifi: CMD52 read(08) test:0x4c is 0x%x \n",wifi_readb(func, 0x4c));
	printk("wifi: CMD52 read(08) test:0x4d is 0x%x \n",wifi_readb(func, 0x4d));
	printk("wifi: CMD52 read(08) test:0x4e is 0x%x \n",wifi_readb(func, 0x4e));
	printk("wifi: CMD52 read(08) test:0x4f is 0x%x \n",wifi_readb(func, 0x4f));

	printk("wifi: CMD53 read(32) test:0x4c is 0x%x\n", wifi_readl(func, 0x4c));
	printk("wifi: CMD53 read(16) test:0x4c is 0x%x\n", wifi_readw(func, 0x4c));
	printk("wifi: CMD53 read(16) test:0x4e is 0x%x\n", wifi_readw(func, 0x4e));

	val8 = wifi_readb(func, 0x004e);
	printk("wifi: CMD52 read(08) test: 0x4e:0x%x\n", val8);
	wifi_writeb(func, 0x004e, val8 | BIT(7));
	val8 = wifi_readb(func, 0x004e);
	printk("wifi: CMD52 write(08) test(set 0x004e[7]): 0x004e:0x%x\n",val8);

	wifi_writeb(func, 0x004e, val8 & ~(BIT(7)));
	val8 = wifi_readb(func, 0x004e);
	printk("wifi: CMD52 write(08) test(clear 0x004e[7]): 0x004e:0x%x\n", val8);

	val16 = wifi_readw(func, 0x004e);
	printk("wifi: CMD53 read(16) test: 0x4e:0x%x\n", val16);
	wifi_writew(func, 0x004e, val16 | (BIT(7) | BIT(8)));
	val16 = wifi_readw(func, 0x004e);
	printk("wifi: CMD53 write(16) test(set 0x004e[7:8]): 0x004e:0x%x\n",val16);

	wifi_writew(func, 0x004e, val16 & ~(BIT(7) | BIT(8)));
	val16 = wifi_readw(func, 0x004e);
	printk("wifi: CMD53 write(16) test(clear 0x004e[7:8]): 0x004e:0x%x\n", val16);

	val32 = wifi_readl(func, 0x004c);
	printk("wifi: CMD53 read(32) test: 0x4c:0x%x\n", val32);
	wifi_writel(func, 0x004c, val32 | (BIT(23) | BIT(24)));
	val32 = wifi_readl(func, 0x004c);
	printk("wifi: CMD53 write(32) test(set 0x004c[23:24]): 0x004c:0x%x\n",val32);

	wifi_writel(func, 0x004c, val32 & ~(BIT(23) | BIT(24)));
	val32 = wifi_readl(func, 0x004c);
	printk("wifi: CMD53 write(32) test(clear 0x004c[23:24]): 0x004c:0x%x\n", val32);

	printk("wifi: CMD53 read(32) 0x100 = 0x%x \n", wifi_readl(func,0x100));

	wifi_writel(func,0x610,0x12345678);
	printk("wifi: CMD53 read(32) 610=0x%x \n",wifi_readl(func,0x610));

	rtw_wifi_card_disable_flow(func);

	printk("RTL871X(adapter): %s exit\n", __func__);

	return 0;
}
#endif /* #ifdef TEST_WIFI_SDIO */

SDIO_BUS_OPS rtw_sdio_bus_ops = {
	sdio_bus_probe,		//int (*bus_probe)(void);
	sdio_bus_remove,	//int (*bus_remove)(void);
	sdio_enable_func,	//int (*enable)(struct sdio_func*func);
	sdio_disable_func,	//int (*disable)(struct sdio_func *func);
	NULL,	//int (*reg_driver)(struct sdio_driver*);		//  Not used
	NULL,	//void (*unreg_driver)(struct sdio_driver *);	//  Not used
	sdio_claim_irq_wrapper,	//int (*claim_irq)(struct sdio_func *func, void(*handler)(struct sdio_func *));
	sdio_release_irq_wrapper,	//int (*release_irq)(struct sdio_func*func);
	sdio_claim_host,	//void (*claim_host)(struct sdio_func*func);
	sdio_release_host,	//void (*release_host)(struct sdio_func *func);
	sdio_readb,	//unsigned char (*readb)(struct sdio_func *func, unsigned int addr, int *err_ret);
	sdio_readw,	//unsigned short (*readw)(struct sdio_func *func, unsigned int addr, int *err_ret);
	sdio_readl,	//unsigned int (*readl)(struct sdio_func *func, unsigned int addr, int *err_ret);
	sdio_writeb,	//void (*writeb)(struct sdio_func *func, unsigned char b,unsigned int addr, int *err_ret);
	sdio_writew,	//void (*writew)(struct sdio_func *func, unsigned short b,unsigned int addr, int *err_ret);
	sdio_writel,	//void (*writel)(struct sdio_func *func, unsigned int b,unsigned int addr, int *err_ret);
	sdio_memcpy_fromio,	//int (*memcpy_fromio)(struct sdio_func *func, void *dst,unsigned int addr, int count);
	sdio_memcpy_toio	//int (*memcpy_toio)(struct sdio_func *func, unsigned int addr,void *src, int count);
};

