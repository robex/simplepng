#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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

	raw_data = get_unfiltered(png, &raw_len);
	for (int i = 0; i < raw_len; i++) {
		raw_data[i] = 0xFF - raw_data[i];
	}
	if (!write_unfiltered(png, raw_data))
		return 0;
	return 1;
}

/* Swap each pixel with its right neighbour */
int png_swap(struct PNG *png)
{
	int raw_len;
	uint8_t *raw_data;

	raw_data = get_unfiltered(png, &raw_len);
	uint8_t tmp;
	for (int i = 0; i < raw_len-1; i+=2) {
		tmp = raw_data[i];
		raw_data[i] = raw_data[i+1];
		raw_data[i+1] = tmp;
	}
	write_unfiltered(png, raw_data);
	return 1;
}
