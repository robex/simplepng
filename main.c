#include <stdio.h>
#include "png.h"

int main()
{
	struct PNG png = png_init(2, 2, 8, 4, 0);
	uint8_t data[10] = {
		0x01, 0x00, 0xff, 0x54, 0x00,
		0x00, 0x8a, 0xff, 0xff, 0xff
	};

	png_write(&png, data, 10);
	png_dump(&png, "testfile");
	/*print_png_raw(&png);*/
}
