#ifndef CRC_H
#define CRC_H

#include <stdint.h>

/* Table of CRCs of all 8-bit messages. */
extern uint32_t crc_table[256];

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
   should be initialized to all 1's, and the transmitted value
   is the 1's complement of the final running CRC (see the
   crc() routine below)). */
uint32_t update_crc(uint32_t crc, unsigned char *buf,
			 int len);

/* Return the CRC of the bytes buf[0..len-1]. */
uint32_t crc(unsigned char *buf, int len);
#endif
