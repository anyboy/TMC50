/********************************************************************************
 *                            USDK(US283A_master)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 ********************************************************************************/
/*!
 * \file     stub.c
 * \brief    usb stub protocol
 * \author
 * \version  1.0
 * \date
 *******************************************************************************/

/*
 * stub_interface.c
 */

#include <errno.h>
#include <stdlib.h>
#include <misc/byteorder.h>
#include <stub/stub.h>
#include <usb/class/usb_stub.h>

#define SYS_LOG_DOMAIN "STUB"
#include <logging/sys_log.h>

struct stub_usb_data_t {
	u32_t stub_usb_timeout;
};

static int stub_usb_write(struct device *dev, u16_t opcode, u8_t *data_buffer, u32_t data_len)
{
	struct stub_usb_data_t *stub = dev->driver_data;
	stub_trans_cmd_t zero_cmd;
	stub_trans_cmd_t *cmd = data_buffer ? (stub_trans_cmd_t *)data_buffer : &zero_cmd;

	cmd->type = 0xae;
	cmd->opcode = (u8_t)(opcode >> 8);
	cmd->sub_opcode = (u8_t)opcode;
	cmd->reserv = 0;
	cmd->data_len = data_len;       // alway 0

	return usb_stub_ep_write((u8_t *)cmd, (data_len + STUB_COMMAND_HEAD_SIZE), stub->stub_usb_timeout);
}

static int stub_usb_read(struct device *dev, u16_t opcode, u8_t *data_buffer, u32_t data_len)
{
	struct stub_usb_data_t *stub = dev->driver_data;
	stub_trans_cmd_t cmd;
	int ret = 0;

	cmd.type = 0xae;
	cmd.opcode = (u8_t)(opcode >> 8);
	cmd.sub_opcode = (u8_t)opcode;
	cmd.reserv = 0;
	cmd.data_len = data_len;        // alway 0

	ret = usb_stub_ep_write((u8_t *)&cmd, (u32_t)sizeof(cmd), stub->stub_usb_timeout);
	if (ret) {
		SYS_LOG_DBG("write err (ret=%d)", ret);
		return ret;
	}

	if (cmd.opcode == 0xff)
		data_len = 1;

	return usb_stub_ep_read(data_buffer, data_len, stub->stub_usb_timeout);
}

static int stub_usb_ext_rw(struct device *dev, stub_ext_param_t *ext_param, u32_t rw_mode)
{
	struct stub_usb_data_t *stub = dev->driver_data;
	stub_ext_cmd_t *stub_ext_cmd = (stub_ext_cmd_t *)ext_param->rw_buffer;
	u16_t *pdata16 = (u16_t *)ext_param->rw_buffer;
	int cmd_len = sizeof(stub_ext_cmd_t) + ext_param->payload_len;
	u16_t check_sum;
	int ret, i;

	if (rw_mode == 1) {
		stub_ext_cmd->type = 0xAE;
		stub_ext_cmd->reserved = 0;
		stub_ext_cmd->opcode = (u8_t)(ext_param->opcode >> 8);
		stub_ext_cmd->sub_opcode = (u8_t)(ext_param->opcode);
		stub_ext_cmd->payload_len = ext_param->payload_len;

		check_sum = 0;
		for (i = 0; i < (cmd_len / 2); i++)
			check_sum += pdata16[i];

		if (cmd_len & 0x1)
			check_sum += *((u8_t*)pdata16 + cmd_len - 1);

		sys_put_le16(check_sum, (u8_t *)pdata16 + cmd_len);

		// send command
		ret = usb_stub_ep_write((u8_t *)stub_ext_cmd, (u32_t)cmd_len + 2, stub->stub_usb_timeout);
		if (ret)
			SYS_LOG_ERR("write err (ret=%d)", ret);
	} else {
		ret = usb_stub_ep_read((u8_t *)stub_ext_cmd, (u32_t)cmd_len + 2, stub->stub_usb_timeout);
		if (!ret) {
			check_sum = 0;
			for (i = 0; i < (cmd_len / 2); i++)
				check_sum += pdata16[i];

			if (cmd_len & 0x1)
				check_sum += *((u8_t *)pdata16 + cmd_len - 1);

			if (sys_get_le16((u8_t*)pdata16 + cmd_len) != check_sum) {
				SYS_LOG_ERR("check_sum err");
				ret = -EIO;
			}
		}
	}

	return ret;
}

int stub_usb_ioctl(struct device *dev, u16_t op_code, void *param1, void *param2)
{
	return -ENOTTY;
}

static int stub_usb_raw_rw(struct device *dev, u8_t *buf, u32_t data_len, u32_t rw_mode)
{
	return -1;
}

static int stub_usb_close(struct device *dev)
{
	return usb_stub_exit();
}

static int stub_usb_open(struct device *dev, void *param)
{
	return usb_stub_init(NULL, param);
}

static const struct stub_driver_api stub_driver_api =
{
	.open = stub_usb_open,
	.close = stub_usb_close,
	.write = stub_usb_write,
	.read = stub_usb_read,
	.raw_rw = stub_usb_raw_rw,
	.ext_rw = stub_usb_ext_rw,
	.ioctl = stub_usb_ioctl,
};

static int stub_usb_init(struct device *dev)
{
	struct stub_usb_data_t *stub = dev->driver_data;
	stub->stub_usb_timeout = 500;
	return 0;
}

static struct stub_usb_data_t stub_usb_data;

DEVICE_AND_API_INIT(usb_stub, CONFIG_STUB_DEV_USB_NAME,
		stub_usb_init, &stub_usb_data, NULL,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &stub_driver_api);
