#ifndef CRC_CCITT_H__
#define CRC_CCITT_H__
#pragma once

extern uint16_t const crc_ccitt_table[];
extern uint16_t const crc_ccitt_false_table[];

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		crc_ccitt_byte
 \date		Created  on Wed May 24 22:29:32 2023
 \date		Modified on Wed May 24 22:29:32 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
static inline uint16_t crc_ccitt_byte(uint16_t crc, const uint8_t c)
{
	return (crc >> 8) ^ crc_ccitt_table[(crc ^ c) & 0xff];
}

/*+@@fnc@@----------------------------------------------------------------*//*!
 \brief		crc_ccitt_false_byte
 \date		Created  on Wed May 24 22:29:39 2023
 \date		Modified on Wed May 24 22:29:39 2023
\*//*-@@fnc@@----------------------------------------------------------------*/
static inline uint16_t crc_ccitt_false_byte(uint16_t crc, const uint8_t c)
{
    return (crc << 8) ^ crc_ccitt_false_table[(crc >> 8) ^ c];
}

uint16_t crc_ccitt(uint16_t crc, uint8_t const *buffer, size_t len);
uint16_t crc_ccitt_false(uint16_t crc, uint8_t const *buffer, size_t len);

#endif	// CRC_CCITT_H__
