#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "png.h"

/* Get the bytes per pixel (including alpha) of *png */
int png_calc_bpp(struct PNG *png, int *bpp)
{
	int alpha = 0;
	int bit_depth_bytes = png->IHDR_chunk.bit_depth >> 3;

	switch (png->IHDR_chunk.color_type) {
	case 0:
		if (png->IHDR_chunk.bit_depth < 8)
			return 0;
		*bpp = 1;
		break;
	case 2:
		*bpp = 3;
		break;
	case 3:
		*bpp = 1;
		return 1;
	case 4:
		*bpp = 1;
		alpha = 1;
		break;
	case 6:
		*bpp = 3;
		alpha = 1;
		break;
	default:
		return 0;
	}

	*bpp *= bit_depth_bytes;
	alpha *= bit_depth_bytes;
	*bpp += alpha;
	return 1;
}

int png_calc_alpha(struct PNG *png)
{
	return png->IHDR_chunk.color_type > 3 ?
	       png->IHDR_chunk.bit_depth >> 3 : 0;
}

int apply_filter(struct PNG *png, uint8_t *data, int *filteredlen,
		 uint8_t **filtered_data)
{
	int width = png->IHDR_chunk.width;
	int height = png->IHDR_chunk.height;
	// bytes per pixel
	int bpp;

	if (!png_calc_bpp(png, &bpp))
		return 0;

	int totalwidth = width * bpp + 1;

	// 1 extra byte per scanline
	*filteredlen = totalwidth * height;
	*filtered_data = malloc(*filteredlen);

	for (int i = 0; i < height; i++) {
		memset(*filtered_data + totalwidth * i, 0, 1);
		memcpy(*filtered_data + 1 + totalwidth * i,
		       data + (totalwidth - 1) * i, totalwidth - 1);
	}
	return 1;
}

int paeth_predictor(int a, int b, int c)
{
	int p = a + b - c;
	int pa = abs(p - a);
	int pb = abs(p - b);
	int pc = abs(p - c);
	if (pa <= pb && pa <= pc)
		return a;
	else if (pb <= pc)
		return b;
	else
		return c;
}

void sub_filter(uint8_t *line, int lineno, int width, int bpp)
{
	for (int i = bpp; i < width; i++)
		line[i] += line[i-bpp];
}

void up_filter(uint8_t *line, int lineno, int width, int bpp)
{
	if (lineno == 0)
		return;
	for (int i = 0; i < width; i++) {
		line[i] += line[i-width];
	}
}

void avg_filter(uint8_t *line, int lineno, int width, int bpp)
{
	if (lineno == 0)
		return;
	for (int i = 0; i < width; i++) {
		if (i < bpp) {
			line[i] += line[i-width] / 2;
		} else {
			line[i] += (((uint16_t)line[i-bpp] +
				    (uint16_t)line[i-width]) / 2) % 0x100;
		}
	}
}

void paeth_filter(uint8_t *line, int lineno, int width, int bpp)
{
	int paetha, paethb, paethc;
	if (lineno == 0)
		return;
	for (int i = 0; i < width; i++) {
		if (i < bpp) {
			paetha = 0;
			paethb = (int)line[i-width];
			paethc = 0;
		} else {
			paetha = (int)line[i-bpp];
			paethb = (int)line[i-width];
			paethc = (int)line[i-width-bpp];
		}

		line[i] += paeth_predictor(paetha, paethb,
			   paethc) % (uint32_t)0x100;
	}
}

int decode_filter(uint8_t *line, int lineno, int width, uint8_t filterbyte,
		  int bpp)
{
	/*printf("%02x ", filterbyte);*/
	switch (filterbyte) {
	case 0:
		break;
	case 1:
		sub_filter(line, lineno, width, bpp);
		break;
	case 2:
		up_filter(line, lineno, width, bpp);
		break;
	case 3:
		avg_filter(line, lineno, width, bpp);
		break;
	case 4:
		paeth_filter(line, lineno, width, bpp);
		break;
	default:
		break;
	}
	return 1;
}

int remove_filter(struct PNG *png, uint8_t *filtered_data, int *rawlen,
		  uint8_t **raw_data)
{
	int width = png->IHDR_chunk.width;
	int height = png->IHDR_chunk.height;
	// bytes per pixel
	int bpp;

	if (!png_calc_bpp(png, &bpp))
		return 0;

	int rawwidth = width * bpp;
	int totalwidth = rawwidth + 1;

	*rawlen = rawwidth * height;
	*raw_data = malloc(*rawlen);

	// remove all filter bytes
	for (int i = 0; i < height; i++) {
		memcpy(*raw_data + rawwidth * i, filtered_data +
		       totalwidth * i + 1, rawwidth);
		decode_filter(*raw_data + rawwidth * i, i, rawwidth,
			      filtered_data[totalwidth * i], bpp);
	}
	return 1;
}
