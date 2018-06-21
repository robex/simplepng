#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <byteswap.h>
#include "png.h"

struct PNG png_init(int width, int height, uint8_t bit_depth,
		    uint8_t color_type, uint8_t interlace)
{
	struct PNG png;
	uint8_t head[HEADER_SIZE] = {
		0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a,
	};
	memcpy(png.header, head, HEADER_SIZE);
	uint8_t iend[IEND_SIZE] = {
		0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
		0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
	};
	memcpy(png.IEND, iend, IEND_SIZE);
	uint8_t type[4] = {
		0x49, 0x48, 0x44, 0x52
	};
	memcpy(png.IHDR_chunk.type, type, 4);
	png.IHDR_chunk.length      = __bswap_32(0x0d);
	png.IHDR_chunk.width       = __bswap_32(width);
	png.IHDR_chunk.height      = __bswap_32(height);
	png.IHDR_chunk.bit_depth   = bit_depth;
	png.IHDR_chunk.color_type  = color_type;
	png.IHDR_chunk.compression = 0;
	png.IHDR_chunk.filter      = 0;
	png.IHDR_chunk.interlace   = interlace;
	png.IHDR_chunk.crc = __bswap_32(crc(png.IHDR_chunk.type, 
					IHDR_SIZE_NO_CRC));

	return png;
}

/* Write png data stream into struct png, setting up all necessary
 * fields */
void png_write(struct PNG *png, uint8_t *data, int datalen)
{
	uint64_t compdatalen = datalen*2;
	uint8_t  *compdata   = malloc(compdatalen);
	char     idat[4]     = {0x49, 0x44, 0x41, 0x54};

	memcpy(&png->IDAT.type, idat, 4);
	compress(compdata, &compdatalen, data, datalen);
	// must be freed with png_close
	png->IDAT.data = malloc(compdatalen);
	// copy compressed data
	memcpy(png->IDAT.data, compdata, compdatalen);

	// crc starts at type (4 bytes), not data
	uint8_t *crcbytes = malloc(compdatalen + 4);
	memcpy(crcbytes, &png->IDAT.type, 4);
	memcpy(crcbytes + 4, png->IDAT.data, compdatalen);
	png->IDAT.crc = crc(crcbytes, compdatalen + 4);
	png->IDAT.crc = __bswap_32((uint32_t)png->IDAT.crc);
	png->IDAT.length = compdatalen;

	free(crcbytes);
	free(compdata);
}

/* Returns 1 if successful, 0 otherwise */
int png_dump(struct PNG *png, char *filename)
{
	FILE *f;
	if (!(f = fopen(filename, "w")))
		return 0;

	fwrite(png->header, HEADER_SIZE, 1, f);
	fwrite(&png->IHDR_chunk.length, 4, 1, f);
	fwrite(png->IHDR_chunk.type, 4, 1, f);
	fwrite(&png->IHDR_chunk.width, 4, 1, f);
	fwrite(&png->IHDR_chunk.height, 4, 1, f);
	fwrite(&png->IHDR_chunk.bit_depth, 1, 1, f);
	fwrite(&png->IHDR_chunk.color_type, 1, 1, f);
	fwrite(&png->IHDR_chunk.compression, 1, 1, f);
	fwrite(&png->IHDR_chunk.filter, 1, 1, f);
	fwrite(&png->IHDR_chunk.interlace, 1, 1, f);
	fwrite(&png->IHDR_chunk.crc, 4, 1, f);
	uint32_t bigendianlen = __bswap_32(png->IDAT.length);
	fwrite(&bigendianlen, 4, 1, f);
	fwrite(&png->IDAT.type, 4, 1, f);
	/*printf("raw:\n");*/
	/*for (int i = 0; i < png->IDAT.length; i++) {*/
		/*printf("%02x ", png->IDAT.data[i] );*/
	/*}*/
	fwrite(png->IDAT.data, 1, png->IDAT.length, f);
	fwrite(&png->IDAT.crc, 4, 1, f);
	fwrite(png->IEND, IEND_SIZE, 1, f);
	fclose(f);
	return 1;
}

uint8_t *greyscale_filter(struct PNG *png, uint8_t *data,
			  int *filteredlen)
{
	uint8_t *filtered_data;
	int width = __bswap_32(png->IHDR_chunk.width);
	int height = __bswap_32(png->IHDR_chunk.height);

	int totalwidth = width * (png->IHDR_chunk.bit_depth >> 3) + 1;

	// 1 byte at the beginning, 1 byte per scanline
	*filteredlen = totalwidth * height;
	filtered_data = malloc(*filteredlen);

	for (int i = 0; i < height; i++) {
		memset(filtered_data + totalwidth * i, 0, 1);
		memcpy(filtered_data + 1 + totalwidth * i,
		       data + (totalwidth - 1) * i, totalwidth - 1);
	}

	return filtered_data;
}

void png_close(struct PNG *png)
{
	free(png->IDAT.data);
}

void print_png_raw(struct PNG *png)
{
	for (int i = 0; i < HEADER_SIZE; i++) {
		printf("%02x ", png->header[i] & 0xFF);
	}

	uint8_t *p = (uint8_t*)&(png->IHDR_chunk);
	for (int i = 0; i < 25; i++) {
		printf("%02x ", *p++ & 0xFF);
	}

	for (int i = 0; i < png->IDAT.length; i++) {
		printf("%02x ", png->IDAT.data[i] & 0xFF);
	}

	for (int i = 0; i < IEND_SIZE; i++) {
		printf("%02x ", png->IEND[i] & 0xFF);
	}
}
