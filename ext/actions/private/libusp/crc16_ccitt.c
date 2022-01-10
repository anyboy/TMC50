
const unsigned short crc_ta_4[16] = {
	/* CRC 半字节余式表 */
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
};

static unsigned short crc16_get(unsigned short current_crc, unsigned char byte)
{
	unsigned short high;

	//暂存CRC的高4位
	high = (unsigned char)(current_crc / 4096);
	current_crc <<= 4;
	current_crc ^= crc_ta_4[high ^ (byte / 16)];

	high = (unsigned char)(current_crc / 4096);
	current_crc <<= 4;
	current_crc ^= crc_ta_4[high ^ (byte & 0x0f)];

	return current_crc;
}

/******************************************************************************
 * Name:    CRC-16/CCITT        x16+x12+x5+1
 * Poly:    0x1021
 * Init:    0x0000
 * Refin:   True
 * Refout:  True
 * Xorout:  0x0000
 * Alias:   CRC-CCITT,CRC-16/CCITT-TRUE,CRC-16/KERMIT, MSB mode
 *****************************************************************************/
unsigned short crc16_ccitt_table(unsigned char *ptr, unsigned int len)
{
	unsigned short current_crc = 0;

	while (len != 0) {
		current_crc = crc16_get(current_crc, *ptr);
		ptr++;
		len--;
	}

	return current_crc;
}
