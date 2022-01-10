#ifndef _ASDP_PARSER_H_
#define _ASDP_PARSER_H_
u16_t asdp_tlv_pack_tl(u8_t * buf, u8_t type, u16_t len);
u16_t asdp_tlv_pack_data(u8_t * buf, u8_t type, u16_t len, u32_t number);
u16_t asdp_tlv_pack_ack(u8_t * buf);
u16_t asdp_tlv_pack_nak(u8_t * buf);
u16_t asdp_tlv_unpack_data(u8_t* data, struct tlv* tlv);
int asdp_no_tlv_parse(u8_t server_id, u8_t cmd_id);
int asdp_fix_tlv_parse(u8_t server_id, u8_t cmd_id, u8_t* tlv_list, u16_t len);
#endif

