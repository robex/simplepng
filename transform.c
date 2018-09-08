#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "png.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

uint8_t *get_unfiltered(struct PNG *png, uint64_t *raw_len)
{
	uint64_t fil_len;
	uint8_t *raw_data;
	uint8_t *fil_data = png_get_raw_data(png, &fil_len);

	if (!remove_filter(png, fil_data, raw_len, &raw_data)) {
		return NULL;
	}
	free(fil_data);
	return raw_data;
}


int png_change_bit_depth(struct PNG *png, int bit_depth)
{
	// dummy png to get bpp of new png
	struct PNG dummy;
	uint64_t raw_len;
	uint64_t new_len;
	uint8_t *raw_data;
	uint8_t *new_data;
	int bpp, newbpp;
	if (!png_calc_bpp(png, &bpp))
		return 0;
	raw_data = get_unfiltered(png, &raw_len);
	dummy.IHDR_chunk.bit_depth = bit_depth;
	dummy.IHDR_chunk.color_type = png->IHDR_chunk.color_type;
	if (!png_calc_bpp(&dummy, &newbpp))
		return 0;
	new_len = png->IHDR_chunk.height * png->IHDR_chunk.width * newbpp;
	new_data = calloc(new_len, 1);
	// bytes per sample
	int bps = png->IHDR_chunk.bit_depth >> 3;
	int newbps = bit_depth >> 3;
	int cnt = 0;

	for (int i = 0; i < raw_len; i += bps) {
		memcpy(new_data + cnt, raw_data + i,
		       newbps > bps ? bps : newbps);
		cnt += newbps;
	}
	png->IHDR_chunk.bit_depth = bit_depth;
	png_write(png, new_data, new_len, 0);
	free(new_data);
	free(raw_data);
	return 1;
}

/* Invert color (doesn't invert alpha) */
int png_invert(struct PNG *png)
{
	uint64_t raw_len;
	uint8_t *raw_data;
	int bpp;
	int alpha = png_calc_alpha(png);
	if (!png_calc_bpp(png, &bpp))
		return 0;

	raw_data = get_unfiltered(png, &raw_len);
	for (int i = 0; i < raw_len; i+=bpp) {
		for (int j = 0; j < bpp-alpha; j++)
			raw_data[i+j] = 0xFF - raw_data[i+j];
	}
	png_write(png, raw_data, raw_len, 0);
	free(raw_data);
	return 1;
}

/* Append p2 to the right of p1 and return the result in a new PNG
 * ret is the return value (must be allocated)
 * Restrictions (hopefully to change soon):
 * 	- they must be the same color type
 * 	- they must have the same bit depth
 */
struct PNG png_append_horiz(struct PNG *p1, struct PNG *p2, int *ret)
{
	struct PNG res;
	uint64_t raw_len_1;
	uint8_t *raw_data_1;
	int bpp1;
	if (!png_calc_bpp(p1, &bpp1)) {
		*ret = 0;
		return res;
	}

	uint64_t raw_len_2;
	uint8_t *raw_data_2;

	raw_data_1 = get_unfiltered(p1, &raw_len_1);
	raw_data_2 = get_unfiltered(p2, &raw_len_2);

	int width1 = p1->IHDR_chunk.width;
	int width2 = p2->IHDR_chunk.width;
	int height1 = p1->IHDR_chunk.height;
	int height2 = p2->IHDR_chunk.height;

	int newwidth = width1 + width2;
	int newheight = MAX(height1, height2);
	uint64_t new_len = (newwidth * bpp1) * newheight;
	uint8_t *new_data = calloc(1, new_len);

	/*printf("newheight: %d\n", newheight);*/
	/*printf("newwidth: %d\n", newwidth);*/

	res = png_init(newwidth, newheight, p1->IHDR_chunk.bit_depth,
		       p1->IHDR_chunk.color_type, 0);

	/*
	 *  new_data:
	 *
	 *        pos1               pos2
	 *         |                  |
	 *         v                  v
	 *	   +------------------------------------+
	 *	   |  png1 - row0     |  png2 - row0    |
	 *	   +------------------+-----------------+
	 *	   |  png1 - row1     |  png2 - row1    |
	 *	   +------------------------------------+
	 */

	for (int i = 0; i < newheight; i++) {
		int pos1 = i * (newwidth * bpp1);
		int pos2 = pos1 + width1 * bpp1;
		int png1off = i * (width1 * bpp1);
		int png2off = i * (width2 * bpp1);
		// dont access out of bounds, image will be transparent
		// since the memory is allocated with calloc
		if (i < height1) {
			memcpy(new_data + pos1, raw_data_1 + png1off,
			       width1 * bpp1);
		}
		if (i < height2) {
			memcpy(new_data + pos2, raw_data_2 + png2off,
			       width2 * bpp1);
		}
	}
	png_write(&res, new_data, new_len, 0);

	free(raw_data_1);
	free(raw_data_2);
	free(new_data);
	*ret = 1;
	return res;
}

/* Append p2 below p1 and return the result in a new PNG
 * ret is the return value (must be allocated)
 * Restrictions (hopefully to change soon):
 * 	- they must be the same color type
 * 	- they must have the same bit depth
 */
struct PNG png_append_vert(struct PNG *p1, struct PNG *p2, int *ret)
{
	struct PNG res;
	int bpp1;
	if (!png_calc_bpp(p1, &bpp1)) {
		*ret = 0;
		return res;
	}

	uint64_t raw_len_1;
	uint8_t *raw_data_1;
	uint64_t raw_len_2;
	uint8_t *raw_data_2;

	raw_data_1 = get_unfiltered(p1, &raw_len_1);
	raw_data_2 = get_unfiltered(p2, &raw_len_2);
	if (raw_data_1 == NULL || raw_data_2 == NULL) {
		*ret = 0;
		return res;
	}

	int width1 = p1->IHDR_chunk.width;
	int width2 = p2->IHDR_chunk.width;
	int height1 = p1->IHDR_chunk.height;
	int height2 = p2->IHDR_chunk.height;

	int newwidth = MAX(width1, width2);
	int newheight = height1 + height2;
	uint64_t new_len = (newwidth * bpp1) * newheight;
	uint8_t *new_data = calloc(1, new_len);

	/*printf("newheight: %d\n", newheight);*/
	/*printf("newwidth: %d\n", newwidth);*/

	res = png_init(newwidth, newheight, p1->IHDR_chunk.bit_depth,
		       p1->IHDR_chunk.color_type, 0);

	for (int i = 0; i < height1; i++) {
		int bigrow = i * (newwidth * bpp1);
		int row = i * (width1 * bpp1);
		memcpy(new_data + bigrow, raw_data_1 + row, width1 * bpp1);
	}
	for (int i = 0; i < height2; i++) {
		int bigrow = (i + height1) * (newwidth * bpp1);
		int row = i * (width2 * bpp1);
		memcpy(new_data + bigrow, raw_data_2 + row, width2 * bpp1);
	}

	png_write(&res, new_data, new_len, 0);

	free(raw_data_1);
	free(raw_data_2);
	free(new_data);
	*ret = 1;
	return res;
}

/* Replace src_color (must be the right size) with dst_color */
int png_replace(struct PNG *png, uint8_t *src_color, uint8_t *dst_color)
{
	uint64_t raw_len;
	uint8_t *raw_data;
	int bpp;
	int alpha = png_calc_alpha(png);
	if (!png_calc_bpp(png, &bpp))
		return 0;

	raw_data = get_unfiltered(png, &raw_len);
	for (int i = 0; i < raw_len; i+=bpp) {
		if (!memcmp(raw_data+i, src_color, bpp-alpha)) {
			memcpy(raw_data+i, dst_color, bpp-alpha);
		}
	}
	png_write(png, raw_data, raw_len, 0);
	free(raw_data);
	return 1;
}

int png_flip_horizontal(struct PNG *png)
{
	uint64_t raw_len;
	uint8_t *raw_data = get_unfiltered(png, &raw_len);
	int width = png->IHDR_chunk.width;
	int height = png->IHDR_chunk.height;
	int bpp;
	if (!png_calc_bpp(png, &bpp))
		return 0;
	uint8_t tmp[bpp];

	// swap columns (by swapping each row individually)
	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width/2; i++) {
			// left position, increases until the midpoint
			int beg = i * bpp + width * bpp * j;
			// right position, decreases
			int end = (width - 1 - i) * bpp + width * bpp * j;
			
			memcpy(tmp, raw_data + beg, bpp);
			memcpy(raw_data + beg, raw_data + end, bpp);
			memcpy(raw_data + end, tmp, bpp);
		}
	}

	png_write(png, raw_data, raw_len, 0);
	free(raw_data);
	return 1;
}

int png_flip_vertical(struct PNG *png)
{
	uint64_t raw_len;
	uint8_t *raw_data = get_unfiltered(png, &raw_len);
	int width = png->IHDR_chunk.width;
	int height = png->IHDR_chunk.height;
	int bpp;
	if (!png_calc_bpp(png, &bpp))
		return 0;
	uint8_t tmp[width*bpp];

	for (int i = 0; i < height/2; i++) {
		int beg = i * width * bpp;
		int end = (height - 1 - i) * width * bpp;
		memcpy(tmp, raw_data + beg, width * bpp);
		memcpy(raw_data + beg, raw_data + end, width * bpp);
		memcpy(raw_data + end, tmp, width * bpp);
	}

	png_write(png, raw_data, raw_len, 0);
	free(raw_data);
	return 1;
}

/* Rotate png 90 degrees clockwise */
int png_rotate(struct PNG *png)
{
	uint64_t raw_len;
	uint8_t *raw_data = get_unfiltered(png, &raw_len);
	int width = png->IHDR_chunk.width;
	int height = png->IHDR_chunk.height;
	uint8_t *transp = malloc(raw_len);
	int bpp;
	if (!png_calc_bpp(png, &bpp))
		return 0;

	// reverse rows and transpose matrix
	for (int i = 0; i < height; i++) {
		memcpy(transp+i*width*bpp, raw_data+(height-1-i)*width*bpp,
		       width*bpp);
	}

	for (int j = 0; j < height; j++) {
		for (int i = 0; i < width; i++) {
			int dst = (i*height+j)*bpp;
			int src = (j*width+i)*bpp;
			memcpy(raw_data+dst, transp+src, bpp);
		}
	}

	png->IHDR_chunk.width = height;
	png->IHDR_chunk.height = width;
	free(transp);
	png_write(png, raw_data, raw_len, 0);
	free(raw_data);
	return 1;
}

/* Swap each pixel with its right neighbour */
int png_swap(struct PNG *png)
{
	uint64_t raw_len;
	uint8_t *raw_data;
	int bpp;
	if (!png_calc_bpp(png, &bpp))
		return 0;

	raw_data = get_unfiltered(png, &raw_len);
	uint8_t tmp[bpp];
	for (int i = 0; i < raw_len-bpp; i+=bpp) {
		memcpy(tmp, raw_data+i, bpp);
		memcpy(raw_data+i, raw_data+i+bpp, bpp);
		memcpy(raw_data+i+bpp, tmp, bpp);
	}
	png_write(png, raw_data, raw_len, 0);
	free(raw_data);
	return 1;
}


int int_ceil_div(int dividend, int divisor) {
	return (dividend + divisor - 1)/divisor;
}

/* Condense png by condratio: downscale image by taking the average of
 * all pixels in the square formed by condratio ^ 2 */
int png_condense(struct PNG *png, int condratio)
{
	uint64_t raw_len;
	uint8_t *raw_data = get_unfiltered(png, &raw_len);
	int width = png->IHDR_chunk.width;
	int height = png->IHDR_chunk.height;
	int bpp;

	if (!png_calc_bpp(png, &bpp))
		return 0;

	if (condratio <= 0 || condratio > width || condratio > height)
		return 0;
	int newwidth = int_ceil_div(width, condratio);
	int newheight = int_ceil_div(height, condratio);
	int newlength = newwidth * newheight * bpp;

	uint8_t *cond_data = malloc(newlength);

	uint32_t tmpbyte = 0;
	int subindex = 0;
	int condindex = 0;
	// account for edge pixels that dont make up an entire square
	int npixels = 0;

	//TODO: not enough image data when result is 1px
	
	// total height, increased in condratio steps
	for (int j = 0; j < height; j+=condratio) {
		// total width, increased in condratio steps
		for (int i = 0; i < width*bpp; i+=condratio*bpp) {
			// bytes per pixel inside each submatrix
			for (int m = 0; m < bpp; m++) {
				// submatrix height
				for (int k = 0; k < condratio; k++) {
					// submatrix width
					for (int l = 0; l < condratio; l++) {
						subindex = i + j*width*bpp + l*bpp
							   + k*width*bpp + m;
						// check we're within boundaries (l stays in same column)
						if (subindex < raw_len && i+l*bpp < width*bpp) {
							tmpbyte += raw_data[subindex];
							npixels++;
						}
					}
				}
				tmpbyte /= npixels;
				npixels = 0;
				/*printf("\ntmpbyte: %d\n", tmpbyte);*/
				condindex = (i/condratio) +
				            (j/condratio)*newwidth*bpp + m;
				/*printf("condindex:%d\n", condindex);*/
				if (condindex < newlength)
					cond_data[condindex] = (uint8_t)tmpbyte;
				tmpbyte = 0;
			}
		}
	}

	png->IHDR_chunk.width = newwidth;
	png->IHDR_chunk.height = newheight;

	free(raw_data);
	png_write(png, cond_data, newlength, 0);
	free(cond_data);
	return 1;
}

int png_pixelate(struct PNG *png, int condratio)
{
	uint64_t raw_len;
	uint8_t *raw_data = get_unfiltered(png, &raw_len);
	int width = png->IHDR_chunk.width;
	int height = png->IHDR_chunk.height;
	int bpp;

	if (!png_calc_bpp(png, &bpp))
		return 0;

	if (condratio <= 0 || condratio > width || condratio > height)
		return 0;

	uint32_t tmpbyte = 0;
	int subindex = 0;
	// account for edge pixels that dont make up an entire square
	int npixels = 0;

	//TODO: not enough image data when result is 1px
	
	// total height, increased in condratio steps
	for (int j = 0; j < height; j+=condratio) {
		// total width, increased in condratio steps
		for (int i = 0; i < width*bpp; i+=condratio*bpp) {
			// bytes per pixel inside each submatrix
			for (int m = 0; m < bpp; m++) {
				// submatrix height
				for (int k = 0; k < condratio; k++) {
					// submatrix width
					for (int l = 0; l < condratio; l++) {
						subindex = i + j*width*bpp + l*bpp
							   + k*width*bpp + m;
						// check we're within boundaries (l stays in same column)
						if (subindex < raw_len && i+l*bpp < width*bpp) {
							tmpbyte += raw_data[subindex];
							npixels++;
						}
					}
				}
				tmpbyte /= npixels;
				npixels = 0;
				/*printf("\ntmpbyte: %d\n", tmpbyte);*/
				for (int k = 0; k < condratio; k++) {
					// submatrix width
					for (int l = 0; l < condratio; l++) {
						subindex = i + j*width*bpp + l*bpp
							   + k*width*bpp + m;
						// check we're within boundaries (l stays in same column)
						if (subindex < raw_len && i+l*bpp < width*bpp) {
							raw_data[subindex] = tmpbyte;
						}
					}
				}
				tmpbyte = 0;
			}
		}
	}

	png_write(png, raw_data, raw_len, 0);
	free(raw_data);
	return 1;
}
