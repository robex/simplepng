#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "png.h"
#include "font.h"

void png_draw_text(struct PNG *png, int x, int y, char *str)
{
	char    asc;
	int     index;
	uint8_t bit = 0;
	uint64_t rawlen;
	uint8_t *data = get_unfiltered(png, &rawlen);

	int width = png->IHDR_chunk.width;
	int height = png->IHDR_chunk.height;

	for (int i = 0; i < strlen(str); i++) {
		asc = str[i];
		index = asc - 32;
		// bitmap is mirrored both vertically and horizontally
		for (int j = 12; j >= 0; j--) {
			// index in the bitmap font array
			bit = font[index][j];
			for (int k = 0; k < 8; k++) {
				int pos = y * width + x + i * 9 + (12-j) * width + k;
				if ((bit >> (8-k)) & 1) {
					data[pos] = 0xaa;
				}
			}
		}
	}
	png_write(png, data, rawlen, 0);
}
