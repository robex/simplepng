#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "png.h"

/* 16-bit greyscale (no alpha) */
void test_8_bit_grey()
{
	// 3x2, 16 bit depth, no alpha
	struct PNG png = png_init(3, 2, 8, 0, 0);
	// reference filtered data
	uint8_t ref_data[8] = {
		0x00, 0x00, 0x22, 0x54, 0x00, 0x8a, 0xbf, 0xff
	};
	uint8_t raw_data[6] = {
		0x00, 0x22, 0x54, 0x8a, 0xbf, 0xff
	};

	int size;
	uint8_t *fil_data = greyscale_filter(&png, raw_data, &size);

	if (!memcmp(ref_data, fil_data, 8)) {
		printf("8bit grey data -> OK\n");
	} else {
		printf("8bit grey data -> ERROR\n");
	}

	png_write(&png, fil_data, size);
	if (!png_dump(&png, "samples/testfile_8bit_grey")) {
		printf("png_dump: error writing to directory\n");
	}
	free(fil_data);
	png_close(&png);
}

/* 16-bit greyscale (no alpha) */
void test_16_bit_grey()
{
	// 3x2, 8 bit depth, no alpha
	struct PNG png = png_init(3, 2, 16, 0, 0);
	// reference filtered data
	uint8_t ref_data[14] = {
		0x00, 0x10, 0x00, 0x20, 0x22, 0x30, 0x54, 0x00,
		0x50, 0x8a, 0x60, 0xbf, 0x70, 0xff
	};
	uint8_t raw_data[12] = {
		0x10, 0x00, 0x20, 0x22, 0x30, 0x54, 0x50, 0x8a,
		0x60, 0xbf, 0x70, 0xff
	};

	int size;
	uint8_t *fil_data = greyscale_filter(&png, raw_data, &size);

	if (!memcmp(ref_data, fil_data, 14)) {
		printf("16bit grey data -> OK\n");
	} else {
		printf("16bit grey data -> ERROR\n");
	}

	png_write(&png, fil_data, size);
	if (!png_dump(&png, "samples/testfile_16bit_grey")) {
		printf("png_dump: error writing to directory\n");
	}
	free(fil_data);
	png_close(&png);
}

/* 8-bit rgb (no alpha) */
void test_8_bit_rgb()
{
	// 3x2, 8 bit depth, no alpha
	struct PNG png = png_init(3, 2, 8, 2, 0);
	// reference filtered data
	uint8_t ref_data[20] = {
		0x00,			// filter byte
		0x00, 0x00, 0x00,	// black
		0xFF, 0x00, 0x00,	// red
		0x00, 0xFF, 0x00,	// green
		0x00,			// filter byte
		0x00, 0x00, 0xFF,	// blue
		0xFF, 0xFF, 0xFF,	// white
		0xFF, 0x00, 0xFF,	// purple
	};

	uint8_t raw_data[18] = {
		0x00, 0x00, 0x00,	// black
		0xFF, 0x00, 0x00,	// red
		0x00, 0xFF, 0x00,	// green
		0x00, 0x00, 0xFF,	// blue
		0xFF, 0xFF, 0xFF,	// white
		0xFF, 0x00, 0xFF,	// purple
	};

	int size;
	uint8_t *fil_data = rgb_filter(&png, raw_data, &size);

	if (!memcmp(ref_data, fil_data, 20)) {
		printf("8bit rgb data -> OK\n");
	} else {
		printf("8bit rgb data -> ERROR\n");
	}

	png_write(&png, fil_data, 20);
	if (!png_dump(&png, "samples/testfile_8bit_rgb")) {
		printf("png_dump: error writing to directory\n");
	}

	free(fil_data);
	png_close(&png);
}

/* 16-bit rgb (no alpha) */
void test_16_bit_rgb()
{
	// 3x2, 16 bit depth, no alpha
	struct PNG png = png_init(3, 2, 16, 2, 0);
	// reference filtered data
	uint8_t ref_data[38] = {
		0x00,					// filter byte
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// black
		0xFF, 0xFF, 0x00, 0x00 ,0x00, 0x00,	// red
		0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,	// green
		0x00,					// filter byte
		0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,	// blue
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// white
		0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF,	// purple
	};

	uint8_t raw_data[36] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// black
		0xFF, 0xFF, 0x00, 0x00 ,0x00, 0x00,	// red
		0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,	// green
		0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,	// blue
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// white
		0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF,	// purple
	};

	int size;
	uint8_t *fil_data = rgb_filter(&png, raw_data, &size);

	if (!memcmp(ref_data, fil_data, 38)) {
		printf("16bit rgb data -> OK\n");
	} else {
		printf("16bit rgb data -> ERROR\n");
	}

	png_write(&png, fil_data, 38);
	if (!png_dump(&png, "samples/testfile_16bit_rgb")) {
		printf("png_dump: error writing to directory\n");
	}
	free(fil_data);
	png_close(&png);
}

void test_open()
{
	struct PNG png;
	if (!(png_open("samples/testfile_8bit_grey", &png))) {
		printf("png_open: error opening png\n");
	} else {
		print_png_raw(&png);
	}
}

int main()
{
	test_8_bit_grey();
	test_16_bit_grey();
	test_8_bit_rgb();
	test_16_bit_rgb();

	test_open();
}
