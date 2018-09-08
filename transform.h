#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "png.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct _png_tform {
	int bpp;
	int alpha;
	int width;
	int height;
	int bit_depth;
	int color_type;
	uint64_t len;
	uint8_t *data;
};

int png_get_tform(struct PNG *png, struct _png_tform *tform);
void png_tform_free(struct _png_tform *tform);

#endif
