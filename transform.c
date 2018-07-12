#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "png.h"

uint8_t *get_unfiltered(struct PNG *png, int *raw_len)
{
	uint64_t fil_len;
	uint8_t *raw_data;
	uint8_t *fil_data = png_get_raw_data(png, &fil_len);

	if (!remove_filter(png, fil_data, raw_len, &raw_data)) {
		free(fil_data);
	}
	free(fil_data);
	return raw_data;
}

int write_unfiltered(struct PNG *png, uint8_t *raw_data)
{
	uint8_t *fil_data;
	uint64_t len;
	if (!apply_filter(png, raw_data, (int*)&len, &fil_data)) {
		return 0;
	}
	png_write(png, fil_data, len, 1);
	free(raw_data);
	free(fil_data);
	return 1;
}

/* Invert color (doesn't invert alpha) */
int png_invert(struct PNG *png)
{
	int raw_len;
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
	if (!write_unfiltered(png, raw_data))
		return 0;
	return 1;
}

/* Replace src_color (must be the right size) with dst_color */
int png_replace(struct PNG *png, uint8_t *src_color, uint8_t *dst_color)
{
	int raw_len;
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
	if (!write_unfiltered(png, raw_data))
		return 0;
	return 1;
}

int png_flip_horizontal(struct PNG *png)
{
	int raw_len;
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

	if (!write_unfiltered(png, raw_data))
		return 0;
	return 1;
}

int png_flip_vertical(struct PNG *png)
{
	int raw_len;
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

	if (!write_unfiltered(png, raw_data))
		return 0;
	return 1;
}

/* Rotate png 90 degrees clockwise */
int png_rotate(struct PNG *png)
{
	int raw_len;
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
	if (!write_unfiltered(png, raw_data))
		return 0;
	return 1;
}

/* Swap each pixel with its right neighbour */
int png_swap(struct PNG *png)
{
	int raw_len;
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
	write_unfiltered(png, raw_data);
	return 1;
}


int int_ceil_div(int dividend, int divisor) {
	return (dividend + divisor - 1)/divisor;
}

/* Condense png by condratio: downscale image by taking the average of
 * all pixels in the square formed by condratio ^ 2 */
//TODO: make pixelation (set average to all pixels, dont change image size)
int png_condense(struct PNG *png, int condratio)
{
	int raw_len;
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
	write_unfiltered(png, cond_data);
	return 1;
}

int png_pixelate(struct PNG *png, int condratio)
{
	int raw_len;
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

	write_unfiltered(png, raw_data);
	return 1;
}
