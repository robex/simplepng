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
		printf("y");
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
	png_write(png, fil_data, len);
	free(raw_data);
	free(fil_data);
	return 1;
}


/* Invert color */
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

/* Replace color */
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

//TODO: cleanup and add png_mirror (transpose only)
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
