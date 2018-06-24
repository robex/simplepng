#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "png.h"

/* Get the bytes per pixel (including alpha) of *png */
int png_calc_bpp(struct PNG *png, int *bpp)
{
	int alpha = 0;
	switch (png->IHDR_chunk.color_type) {
	case 0:
		if (png->IHDR_chunk.bit_depth < 8)
			return 0;
		*bpp = 1;
		break;
	case 2:
		*bpp = 3;
		break;
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

	*bpp *= (png->IHDR_chunk.bit_depth >> 3);
	*bpp += alpha;
	return 1;
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

	// 1 byte at the beginning, 1 byte per scanline
	*filteredlen = totalwidth * height;
	*filtered_data = malloc(*filteredlen);

	for (int i = 0; i < height; i++) {
		memset(*filtered_data + totalwidth * i, 0, 1);
		memcpy(*filtered_data + 1 + totalwidth * i,
		       data + (totalwidth - 1) * i, totalwidth - 1);
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

	for (int i = 0; i < height; i++) {
		memcpy(*raw_data + rawwidth * i, filtered_data +
		       totalwidth * i + 1, rawwidth);
	}
	return 1;
}
