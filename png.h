#ifndef PNG_H
#define PNG_H

#include <stdint.h>

#define HEADER_SIZE 8
#define IHDR_SIZE_NO_CRC 17
#define IEND_SIZE 12

#define PNG_PLTE 3

uint8_t head[HEADER_SIZE];
uint8_t iend[IEND_SIZE];
uint8_t ihdr_type[4];
uint8_t idat_type[4];
uint8_t plte_type[4];

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
	uint8_t  type[4];
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
	struct chunk PLTE;
	struct chunk *IDAT;
	int nidat;
	// last chunk (always the same)
	uint8_t IEND[IEND_SIZE];
};

int png_open(struct PNG *png, char *filename); 
struct PNG png_init(int width, int height, uint8_t bit_depth,
		    uint8_t color_type, uint8_t interlace);
/* Write png data stream into struct png, setting up all necessary
 * fields.
 * isfiltered: if 0 -> *data is not filtered, a filter will be applied */
void png_write(struct PNG *png, uint8_t *data, int datalen, int isfiltered);
/* Return raw (uncompressed but filtered) data from png,
 * returns length of raw stream in rawlen (must be allocated) */
uint8_t *png_get_raw_data(struct PNG *png, uint64_t *rawlen);
/* Print metadata and raw (compressed and uncompressed) streams. */
void print_png_raw(struct PNG *png);
/* Write png to disk, to file filename. Returns 1 if succesful,
 * 0 otherwise. */
int png_dump(struct PNG *png, char *filename);
/* Clean png structures and free memory */
void png_close(struct PNG *png);

/* Get the bytes per pixel (including alpha) of *png */
int png_calc_bpp(struct PNG *png, int *bpp);
int png_calc_alpha(struct PNG *png);

// only free filtered_data if function returns 1
int apply_filter(struct PNG *png, uint8_t *data, uint64_t *filteredlen,
		 uint8_t **filtered_data);
// only free raw_data if function returns 1
int remove_filter(struct PNG *png, uint8_t *filtered_data, uint64_t *rawlen,
		  uint8_t **raw_data);


/* Invert color (doesn't invert alpha) */
int png_invert(struct PNG *png);
/* Swap each pixel with its right neighbour */
int png_swap(struct PNG *png);
/* Rotate png 90 degrees clockwise */
int png_rotate(struct PNG *png);
/* Replace src_color (must be the right size) with dst_color */
int png_replace(struct PNG *png, uint8_t *src_color, uint8_t *dst_color);
int png_flip_horizontal(struct PNG *png);
int png_flip_vertical(struct PNG *png);
/* Condense png by condratio: downscale image by taking the average of
 * all pixels in the square formed by condratio ^ 2 */
int png_condense(struct PNG *png, int condratio);
int png_pixelate(struct PNG *png, int condratio);
struct PNG png_append_horiz(struct PNG *p1, struct PNG *p2, int *ret);

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
