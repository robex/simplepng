#ifndef PNG_H
#define PNG_H

#include <stdint.h>
#include "crc.h"

#define HEADER_SIZE 8
#define IEND_SIZE 12

struct IHDR {
	// width and height, in pixels
	uint32_t length;
	uint8_t  type[4];
	uint32_t width;
	uint32_t height;
	// bits per sample (not per pixel, maybe alpha)
	uint8_t  bit_depth;
	// 0: greyscale
	// 2: RGB
	// 3: palette
	// 4: greyscale + alpha
	// 6: RGB + alpha
	uint8_t  color_type;
	// must be 0
	uint8_t  compression;
	// must be 0
	uint8_t  filter;
	// 0: no interlace
	// 1: interlace
	uint8_t  interlace;
	uint32_t crc;
};

struct chunk {
	// length of data
	uint32_t length;
	// chunk type: 4 case-sensitive ascii chars
	uint32_t type;
	// chunk data, zlib compressed
	uint8_t  *data;
	// crc-32 of type + data
	uint32_t crc;
};

struct PNG {
	// magic number
	uint8_t header[HEADER_SIZE];
	// image metadata
	struct IHDR IHDR_chunk;
	struct chunk *chunks;
	struct chunk IDAT;
	// last chunk (always the same)
	uint8_t IEND[IEND_SIZE];
};

struct PNG png_init(int width, int height, uint8_t bit_depth,
		    uint8_t color_type, uint8_t interlace);
/* Write png data stream into struct png, setting up all necessary
 * fields */
void png_write(struct PNG *png, uint8_t *data, int datalen);

void print_png_raw(struct PNG *png);

void png_dump(struct PNG *png, char *filename);
#endif
