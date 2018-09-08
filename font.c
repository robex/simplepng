#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "png.h"
#include "font.h"
#include "transform.h"

int png_draw_text(struct PNG *png, int x, int y, char *str)
{
	char    asc;
	int     index;
	uint8_t bit = 0;
	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;

	int width = tf.width * tf.bpp;

	for (int i = 0; i < strlen(str); i++) {
		asc = str[i];
		index = asc - 32;
		// bitmap is mirrored both vertically and horizontally
		for (int j = 12; j >= 0; j--) {
			// index in the bitmap font array
			bit = font[index][j];
			for (int k = 0; k < 8; k++) {
				if ((bit >> (8-k)) & 1) {
					for (int l = 0; l < tf.bpp; l++) {
						int pos = y * width + (x*tf.bpp) + i * 8 * tf.bpp + (12-j) * width + (k*tf.bpp) + l;
						if (pos >= tf.len)
							continue;
						if (l < tf.bpp - tf.alpha)
							tf.data[pos] = 0x00;
						else
							tf.data[pos] = 0xff;
					}
				}
			}
		}
	}
	png_write(png, tf.data, tf.len, 0);
	png_tform_free(&tf);
	return 1;
}
