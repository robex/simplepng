#include <stdio.h>
#include <stdlib.h>
#include "png.h"

int main()
{
	struct PNG png = png_init(3, 2, 8, 0, 0);
	uint8_t data[8] = {
		0x00, 0x00, 0x22, 0x54, 0x00, 0x8a, 0xbf, 0xff
	};
	uint8_t raw_data[6] = {
		0x00, 0x22, 0x54, 0x8a, 0xbf, 0xff
	};

	int size;
	uint8_t *filtered_data = greyscale_filter(&png, raw_data, &size);

	/*for (int i = 0; i < size; i++) {*/
		/*printf("%02x ", filtered_data[i] & 0xff);*/
	/*}*/

	png_write(&png, filtered_data, size);
	png_dump(&png, "testfile");
	free(filtered_data);
	png_close(&png);
	/*print_png_raw(&png);*/
}
