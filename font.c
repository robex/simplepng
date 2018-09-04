#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "png.h"
#include "font.h"

int png_draw_text(struct PNG *png, int x, int y, char *str)
{
	char    asc;
	int     index;
	uint8_t bit = 0;
	uint64_t rawlen;
	uint8_t *data = get_unfiltered(png, &rawlen);
	int bpp;
	int alpha = png_calc_alpha(png);
	if (!png_calc_bpp(png, &bpp))
		return 0;

	int width = png->IHDR_chunk.width * bpp;

	for (int i = 0; i < strlen(str); i++) {
		asc = str[i];
		index = asc - 32;
		// bitmap is mirrored both vertically and horizontally
		for (int j = 12; j >= 0; j--) {
			// index in the bitmap font array
			bit = font[index][j];
			for (int k = 0; k < 8; k++) {
				if ((bit >> (8-k)) & 1) {
					for (int l = 0; l < bpp; l++) {
						int pos = y * width + (x*bpp) + i * 8 * bpp + (12-j) * width + (k*bpp) + l;
						if (pos >= rawlen)
							continue;
						if (l < bpp - alpha)
							data[pos] = 0x00;
						else
							data[pos] = 0xff;
					}
				}
			}
		}
	}
	png_write(png, data, rawlen, 0);
	free(data);
	return 1;
}
