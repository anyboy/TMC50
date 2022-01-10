#include <kernel.h>
#include <string.h>
#include <soc.h>
#include <mem_manager.h>
#include <usp_protocol.h>
#include <lib_asdp.h>
#include "asdp_inner.h"
#include "asdp_transfer.h"

u16_t asdp_tlv_pack_tl(u8_t * buf, u8_t type, u16_t len)
{
	buf[0] = type;
	buf[1] = len & 0xff;
	buf[2] = (len >> 8) & 0xff;

	return 3;
}

u16_t asdp_tlv_pack_data(u8_t * buf, u8_t type, u16_t len, u32_t number)
{
	int i;

	buf[0] = type;
	buf[1] = len & 0xff;
	buf[2] = (len >> 8) & 0xff;

	for (i = 0; i < len; i ++) {
		//little endian
		buf[3+i] =  (number>>(i*8))&0xFF;
	}

	return 3+len;
}

u16_t asdp_tlv_pack_ack(u8_t * buf)
{
	return asdp_tlv_pack_data(buf, TLV_TYPE_ERROR_CODE, 4, 100000);
}

u16_t asdp_tlv_pack_nak(u8_t * buf)
{
	return asdp_tlv_pack_data(buf, TLV_TYPE_ERROR_CODE, 4, 100001);
}

u16_t asdp_tlv_unpack_data(u8_t* data, struct tlv* tlv)
{
	tlv->type = data[0];
	tlv->len = data[1] + (data[2] << 8);
	tlv->val = data+3;

	return 3+tlv->len;
}

int asdp_no_tlv_parse(u8_t server_id, u8_t cmd_id)
{
	int err;
	u8_t *tlv_buf;
	u16_t tlv_send;

	tlv_buf = adsp_get_tlv_buf();
	tlv_send = asdp_tlv_pack_ack(tlv_buf);
	if (tlv_send > 0) {
		err = asdp_send_cmd(server_id,
				cmd_id,
				tlv_send);
		if (err) {
			ASDP_ERR("failed to send cmd, error=%d", err);
			return -1;
		}

	} else {
		ASDP_ERR("failed to pack ack, tlv_send=%d", tlv_send);
	}

	return 1;
}

int asdp_fix_tlv_parse(u8_t server_id, u8_t cmd_id, u8_t* tlv_list, u16_t len)
{
	int err;
	u8_t *tlv_buf;
	u16_t tlv_send;
	u16_t offset;
	struct tlv tlv;
	bool cmd = false;

	offset = 0;
	while (offset < len) {
		offset += asdp_tlv_unpack_data(tlv_list+offset, &tlv);
		switch (tlv.type) {
		case 0x01:
			ASDP_DBG("Get fix tlv command.");
			cmd = true;
			break;
		default:
			ASDP_WRN("Not implemented tlv, type=%d.", tlv.type);
			break;
		}
	}

	tlv_buf = adsp_get_tlv_buf();
	if (cmd) {
		tlv_send = asdp_tlv_pack_ack(tlv_buf);
	} else {
		tlv_send = asdp_tlv_pack_nak(tlv_buf);
	}
	if (tlv_send > 0) {
		err = asdp_send_cmd(server_id,
				cmd_id,
				tlv_send);
		if (err) {
			ASDP_ERR("failed to send cmd, error=%d", err);
			return -1;
		}

	}else{
		ASDP_ERR("failed to pack ack/nak, tlv_send=%d", tlv_send);
	}

	if(cmd) {
		return 1;
	}

	return 0;
}
