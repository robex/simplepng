#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "png.h"

int png_invert(struct PNG *png)
{
	int raw_len;
	uint8_t *raw_data;
	uint64_t fil_len;
	uint8_t *fil_data = png_get_raw_data(png, &fil_len);

	if (!remove_filter(png, fil_data, &raw_len, &raw_data)) {
		free(fil_data);
		return 0;
	}
	for (int i = 0; i < raw_len; i++)
	{
		raw_data[i] = 0xFF - raw_data[i];
	}
	free(fil_data);
	if (!apply_filter(png, raw_data, (int*)&fil_len, &fil_data)) {
		return 0;
	}
	png_write(png, fil_data, fil_len);
	free(fil_data);
	free(raw_data);
	return 1;
}
