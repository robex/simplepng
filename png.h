#ifndef PNG_H
#define PNG_H

#include <stdint.h>

#define HEADER_SIZE 8
#define IHDR_SIZE_NO_CRC 17
#define IEND_SIZE 12

uint8_t head[HEADER_SIZE];
uint8_t iend[IEND_SIZE];
uint8_t ihdr_type[4];
uint8_t idat[4];

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
	uint8_t type[4];
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
	struct chunk *IDAT;
	int nidat;
	// last chunk (always the same)
	uint8_t IEND[IEND_SIZE];
};

int png_open(struct PNG *png, char *filename); 
struct PNG png_init(int width, int height, uint8_t bit_depth,
		    uint8_t color_type, uint8_t interlace);
/* Write png data stream into struct png, setting up all necessary
 * fields */
void png_write(struct PNG *png, uint8_t *data, int datalen);
uint8_t *png_get_raw_data(struct PNG *png, uint64_t *rawlen);
void print_png_raw(struct PNG *png);
int png_dump(struct PNG *png, char *filename);
void png_close(struct PNG *png);

/* Get the bytes per pixel (including alpha) of *png */
int png_calc_bpp(struct PNG *png, int *bpp);
int png_calc_alpha(struct PNG *png);

// only free filtered_data if function returns 1
int apply_filter(struct PNG *png, uint8_t *data, int *filteredlen,
		 uint8_t **filtered_data);
// only free raw_data if function returns 1
int remove_filter(struct PNG *png, uint8_t *filtered_data, int *rawlen,
		  uint8_t **raw_data);


int png_invert(struct PNG *png);
int png_swap(struct PNG *png);
int png_rotate(struct PNG *png);

/* Table of CRCs of all 8-bit messages. */
extern uint32_t crc_table[256];

/* Update a running CRC with the bytes buf[0..len-1]--the CRC
should be initialized to all 1's, and the transmitted value
is the 1's complement of the final running CRC (see the
crc() routine below). */
uint32_t update_crc(uint32_t crc, unsigned char *buf,
			 int len);

/* Return the CRC of the bytes buf[0..len-1]. */
uint32_t crc(unsigned char *buf, int len);
#endif
