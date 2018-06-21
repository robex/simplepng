#include <byteswap.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "png.h"

uint8_t *greyscale_filter(struct PNG *png, uint8_t *data, int *filteredlen)
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

uint8_t *rgb_filter(struct PNG *png, uint8_t *data, int *filteredlen)
{
	uint8_t *filtered_data;
	int width = __bswap_32(png->IHDR_chunk.width);
	int height = __bswap_32(png->IHDR_chunk.height);

	int totalwidth = width * 3 * (png->IHDR_chunk.bit_depth >> 3) + 1;

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
