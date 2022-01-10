/********************************************************************************
 *                            USDK(US283A_master)
 *                            Module: SYSTEM
 *                 Copyright(c) 2003-2017 Actions Semiconductor,
 *                            All Rights Reserved.
 *
 * History:
 *      <author>      <time>                      <version >          <desc>
 *      wuyufan   2018-02-13-PM4:55:31             1.0             build this file
 ********************************************************************************/
/*!
 * \file     image.h
 * \brief
 * \author
 * \version  1.0
 * \date  2018-02-13-PM4:55:31
 *******************************************************************************/

#ifndef PLATFORM_INCLUDE_BOOT_IMAGE_H_
#define PLATFORM_INCLUDE_BOOT_IMAGE_H_

//image header maigc (ACTH)
#define IMAGE_MAGIC0      0x48544341

#define IMAGE_MAGIC1      0x41435448

#define IMAGE_NAME_MBREC            "MBREC"
#define IMAGE_NAME_LOADER           "LOADER"
#define IMAGE_NAME_APP              "APP"

#ifdef BOOT_SECURITY
#define BOOT_HASH_TYPE_MD5          1
#define BOOT_HASH_TYPE_SHA256       2
#define BOOT_KEY_TYPE_RSA2048       1
#define BOOT_KEY_TYPE_ECDSA192      2
#define BOOT_KEY_TYPE_ECDSA256      3

#define PREFIX_CHAR                 0xBE

#define KEY_LEN                     (4 + 256 + 256)
#define SIG_LEN                     256

typedef struct security_data {
	struct boot_hdr {
		unsigned short security;		// enable asymmetric enctyption flag
		unsigned char hash_type;// hash type
		unsigned char key_type;//key type
		unsigned short key_len;//key lenght
		unsigned short sig_len;//signature type
		unsigned int reserved;
	}boot_hdr_t;
	unsigned char key[KEY_LEN];		//key context
	unsigned char sig[SIG_LEN];//signature context
}security_data_t;
#endif

#define MAX_VERSION_LEN            64

/*
 * rule: add up header and data and its checksum shall be 0xffff
 * add up data section and its checksum shall be 0xffff
 */
typedef struct image_head {
	uint32_t magic0;         //magic number
	uint32_t magic1;
	uint32_t load_addr;      //load address
	uint8_t  name[4];        //image name
	uint16_t version;
	uint16_t header_size;
	uint16_t header_chksum;  //checksum of header
	uint16_t data_chksum;    //checksum of data
	uint32_t body_size;      //body size
	uint32_t tail_size;      // tail size
	void (*entry)(void *api, void *arg);
	uint32_t attribute;
	uint32_t priv[2];        //private data
} image_head_t;

// The tail of image is not CRC, and need to change when excution process.
typedef struct{
	// record the relationship between the order and newer of current partintion/
    uint16_t image_count;
#ifdef BOOT_SECURITY
    /* image signature information */
    security_data_t security;
#endif
} image_tail_t;


#endif /* PLATFORM_INCLUDE_BOOT_IMAGE_H_ */
