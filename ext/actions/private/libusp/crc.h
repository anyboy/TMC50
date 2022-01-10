/*!
 * \file  crc.h
 * \brief crc calculation
 */

#ifndef __CRC_H_
#define __CRC_H_

/**
  \fn          unsigned short crc16_ccitt_table(unsigned char *ptr, unsigned int len)
  \brief       Caculate CRC16 result use CCITT mode.
  \param[in]   ptr          Input data buffer pointer.
  \param[in]   len          data length, in byte unit.
  \return      The CRC16 value.
*/
unsigned short crc16_ccitt_table(unsigned char *ptr, unsigned int len);

/**
  \fn          unsigned char crc8_maxim_table(unsigned char *ptr, unsigned int len)
  \brief       Caculate CRC8 result use maxim mode.
  \param[in]   ptr          Input data buffer pointer.
  \param[in]   len          data length, in byte unit.
  \return      The CRC8 value.
*/
unsigned char crc8_maxim_table(unsigned char *ptr, unsigned int len);

/**
  \fn          unsigned char crc8_itu(unsigned char *ptr, unsigned int len)
  \brief       Caculate CRC8 result use itu mode.
  \param[in]   ptr          Input data buffer pointer.
  \param[in]   len          data length, in byte unit.
  \return      The CRC8 value.
*/
unsigned char crc8_itu(unsigned char *ptr, unsigned int len);

#define crc8    crc8_maxim_table
#define crc16   crc16_ccitt_table
#endif				// __CRC_H_
