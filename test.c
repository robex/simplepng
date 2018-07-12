#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "png.h"

#define RED(x) "\033[1;41m" x "\033[0;m"
#define GREEN(x) "\033[1;42m" x "\033[0;m"
#define YELLOW(x) "\033[1;33m" x "\033[0;m"

void print_aligned(char *string, char *result, int column)
{
	int len = strlen(string) - 12;
	printf("%s ", string);
	for (int i = len; i < column; i++)
		printf(".");
	printf(" %s", result);
	printf("\n");
}

/* 8-bit greyscale (no alpha) */
void test_8_bit_grey()
{
	// 3x2, 8 bit depth, no alpha
	struct PNG png = png_init(3, 2, 8, 0, 0);
	// reference filtered data
	uint8_t ref_data[8] = {
		0x00, 0x00, 0x22, 0x54, 0x00, 0x8a, 0xbf, 0xff
	};
	uint8_t raw_data[6] = {
		0x00, 0x22, 0x54, 0x8a, 0xbf, 0xff
	};

	int size;
	uint8_t *fil_data;
	apply_filter(&png, raw_data, &size, &fil_data);

	if (!memcmp(ref_data, fil_data, 8)) {
		print_aligned(">>> "YELLOW("TEST")": 8bit grey data", GREEN("OK"), 70);
	} else {
		print_aligned(">>> "YELLOW("TEST")": 8bit grey data", RED("ERROR"), 70);
	}

	png_write(&png, fil_data, size, 1);
	if (!png_dump(&png, "samples/testfile_8bit_grey")) {
		printf("png_dump: error writing to directory\n");
	}
	free(fil_data);
	png_close(&png);
}

/* 8-bit greyscale alpha */
void test_8_bit_grey_alpha()
{
	// 3x2, 8 bit depth, alpha
	struct PNG png = png_init(3, 2, 8, 4, 0);
	// reference filtered data
	uint8_t ref_data[14] = {
		0x00,		// filter
		0x00, 0xFF,
		0x22, 0xEF,
		0x54, 0xCF,
		0x00,		// filter
		0x8a, 0xAF,
		0xbf, 0x7F,
		0xff, 0x1F
	};
	uint8_t raw_data[12] = {
		0x00, 0xFF,
		0x22, 0xEF,
		0x54, 0xCF,
		0x8a, 0xAF,
		0xbf, 0x7F,
		0xff, 0x1F
	};

	int size;
	uint8_t *fil_data;
	apply_filter(&png, raw_data, &size, &fil_data);


	if (!memcmp(ref_data, fil_data, 14)) {
		print_aligned(">>> "YELLOW("TEST")": 8bit grey alpha data", GREEN("OK"), 70);
	} else {
		print_aligned(">>> "YELLOW("TEST")": 8bit grey alpha data", RED("ERROR"), 70);
	}

	png_write(&png, fil_data, size, 1);
	if (!png_dump(&png, "samples/testfile_8bit_grey_alpha")) {
		printf("png_dump: error writing to directory\n");
	}
	free(fil_data);
	png_close(&png);
}

/* 16-bit greyscale (no alpha) */
void test_16_bit_grey()
{
	// 3x2, 16 bit depth, no alpha
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
	uint8_t *fil_data;
	apply_filter(&png, raw_data, &size, &fil_data);

	if (!memcmp(ref_data, fil_data, 14)) {
		print_aligned(">>> "YELLOW("TEST")": 16bit grey data", GREEN("OK"), 70);
	} else {
		print_aligned(">>> "YELLOW("TEST")": 16bit grey data", RED("ERROR"), 70);
	}

	png_write(&png, fil_data, size, 1);
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
	uint8_t *fil_data;
	apply_filter(&png, raw_data, &size, &fil_data);

	if (!memcmp(ref_data, fil_data, 20)) {
		print_aligned(">>> "YELLOW("TEST")": 8bit rgb data", GREEN("OK"), 70);
	} else {
		print_aligned(">>> "YELLOW("TEST")": 8bit rgb data", RED("ERROR"), 70);
	}

	png_write(&png, fil_data, 20, 1);
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
	uint8_t *fil_data;
	apply_filter(&png, raw_data, &size, &fil_data);

	if (!memcmp(ref_data, fil_data, 38)) {
		print_aligned(">>> "YELLOW("TEST")": 16bit rgb data", GREEN("OK"), 70);
	} else {
		print_aligned(">>> "YELLOW("TEST")": 16bit rgb data", RED("ERROR"), 70);
	}

	png_write(&png, fil_data, 38, 1);
	if (!png_dump(&png, "samples/testfile_16bit_rgb")) {
		printf("png_dump: error writing to directory\n");
	}
	free(fil_data);
	png_close(&png);
}

void test_open(char *filename)
{
	char string[80];
	struct PNG png;
	sprintf(string, ">>> "YELLOW("TEST")": parse \"%s\"\n", filename);
	if (!png_open(&png, filename)) {
		print_aligned(string, RED("ERROR"), 70);
		printf("png_open: error opening png %s\n", filename);
	} else {
		print_png_raw(&png);
		print_aligned(string, GREEN("OK"), 70);
		png_close(&png);
	}
}

void test_copy(char *in, char *out)
{
	char string[80];
	struct PNG png;
	sprintf(string, ">>> "YELLOW("TEST")": copy \"%s\" to \"%s\"", in, out);
	if (!png_open(&png, in)) {
		print_aligned(string, RED("ERROR"), 70);
		printf("png_open: error opening png %s\n", in);
		return;
	}
	if (!png_dump(&png, out))
		print_aligned(string, RED("ERROR"), 70);
	else
		print_aligned(string, GREEN("OK"), 70);

	png_close(&png);
}

void test_remove_filter()
{
	struct PNG png = png_init(3, 2, 8, 4, 0);
	// reference filtered data
	uint8_t raw_data[12] = {
		0x00, 0xFF,
		0x22, 0xEF,
		0x54, 0xCF,
		0x8a, 0xAF,
		0xbf, 0x7F,
		0xff, 0x1F
	};

	int size;
	uint8_t *fil_data;
	int sizerem;
	uint8_t *removed_filter;
	if (!apply_filter(&png, raw_data, &size, &fil_data)) {
		print_aligned(">>> "YELLOW("TEST")": removing filter", RED("ERROR"), 70);
		return;
	}
	if (!remove_filter(&png, fil_data, &sizerem, &removed_filter)) {
		print_aligned(">>> "YELLOW("TEST")": removing filter", RED("ERROR"), 70);
		return;
	}

	if (!memcmp(raw_data, removed_filter, 12)) {
		print_aligned(">>> "YELLOW("TEST")": removing filter", GREEN("OK"), 70);
	} else {
		print_aligned(">>> "YELLOW("TEST")": removing filter", RED("ERROR"), 70);
	}
	free(fil_data);
	free(removed_filter);
}

void test_invert(char *filename)
{
	char string[80];
	sprintf(string, ">>> "YELLOW("TEST")": invert png %s", filename);
	struct PNG png;
	if (!png_open(&png, filename)) {
		printf("png_open: error opening png %s\n", filename);
		return;
	}
	if (!png_invert(&png))
		print_aligned(string, RED("ERROR"), 70);
	else
		print_aligned(string, GREEN("OK"), 70);
	png_dump(&png, "samples/invert_test");
	png_close(&png);
}

void test_swap(char *filename)
{
	char string[80];
	sprintf(string, ">>> "YELLOW("TEST")": swap png %s", filename);
	struct PNG png;
	if (!png_open(&png, "samples/ruben.png")) {
		printf("png_open: error opening png %s\n", filename);
		return;
	}
	if (!png_swap(&png))
		print_aligned(string, RED("ERROR"), 70);
	else
		print_aligned(string, GREEN("OK"), 70);
	png_dump(&png, "samples/swap_test");
	png_close(&png);
}

void test_rotate(char *filename)
{
	char string[80];
	sprintf(string, ">>> "YELLOW("TEST")": rotate png %s", filename);
	struct PNG png;
	if (!png_open(&png, filename)) {
		printf("png_open: error opening png %s\n", filename);
		return;
	}
	if (!png_rotate(&png))
		print_aligned(string, RED("ERROR"), 70);
	else
		print_aligned(string, GREEN("OK"), 70);
	png_dump(&png, "samples/rotate_test");
	png_close(&png);
}

void test_replace(char *filename)
{
	char string[80];
	sprintf(string, ">>> "YELLOW("TEST")": replace png %s", filename);
	struct PNG png;
	uint8_t src[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	uint8_t dst[6] = {0xae, 0x22, 0xbb, 0xff, 0x77, 0x00};
	if (!png_open(&png, filename)) {
		printf("png_open: error opening png %s\n", filename);
		return;
	}
	if (!png_replace(&png, src, dst))
		print_aligned(string, RED("ERROR"), 70);
	else
		print_aligned(string, GREEN("OK"), 70);
	png_dump(&png, "samples/replace_test");
	png_close(&png);
}

void test_flip_horiz(char *filename)
{
	char string[80];
	sprintf(string, ">>> "YELLOW("TEST")": flip horizontal png %s", filename);
	struct PNG png;
	if (!png_open(&png, filename)) {
		printf("png_open: error opening png %s\n", filename);
		return;
	}
	if (!png_flip_horizontal(&png))
		print_aligned(string, RED("ERROR"), 70);
	else
		print_aligned(string, GREEN("OK"), 70);
	png_dump(&png, "samples/flip_hor_test");
	png_close(&png);
}

void test_flip_vert(char *filename)
{
	char string[80];
	sprintf(string, ">>> "YELLOW("TEST")": flip vertical png %s", filename);
	struct PNG png;
	if (!png_open(&png, filename)) {
		printf("png_open: error opening png %s\n", filename);
		return;
	}
	if (!png_flip_vertical(&png))
		print_aligned(string, RED("ERROR"), 70);
	else
		print_aligned(string, GREEN("OK"), 70);
	png_dump(&png, "samples/flip_ver_test");
	png_close(&png);
}

void test_condense(char *filename, int condratio)
{
	char string[80];
	sprintf(string, ">>> "YELLOW("TEST")": condense png %s with ratio %d",
		filename, condratio);
	struct PNG png;
	if (!png_open(&png, filename)) {
		printf("png_open: error opening png %s\n", filename);
		return;
	}
	if (!png_condense(&png, condratio))
		print_aligned(string, RED("ERROR"), 70);
	else
		print_aligned(string, GREEN("OK"), 70);

	png_dump(&png, "samples/condense_test");
	png_close(&png);
}

void test_pixelate(char *filename, int condratio)
{
	char string[80];
	sprintf(string, ">>> "YELLOW("TEST")": pixelate png %s with ratio %d",
		filename, condratio);
	struct PNG png;
	if (!png_open(&png, filename)) {
		printf("png_open: error opening png %s\n", filename);
		return;
	}
	if (!png_pixelate(&png, condratio))
		print_aligned(string, RED("ERROR"), 70);
	else
		print_aligned(string, GREEN("OK"), 70);

	png_dump(&png, "samples/pixelate_test");
	png_close(&png);
}

void test_write_unfiltered(char *filename)
{
	char string[80];
	sprintf(string, ">>> "YELLOW("TEST")": write unfiltered to png %s",
		filename);
	struct PNG png = png_init(3, 2, 16, 2, 0);

	uint8_t raw_data[36] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// black
		0xFF, 0xFF, 0x00, 0x00 ,0x00, 0x00,	// red
		0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00,	// green
		0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF,	// blue
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// white
		0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF,	// purple
	};

	png_write(&png, raw_data, 36, 0);
	if (!png_dump(&png, "samples/write_unfiltered"))
		print_aligned(string, RED("ERROR"), 70);
	else
		print_aligned(string, GREEN("OK"), 70);

	png_close(&png);
}

int main()
{
	test_8_bit_grey();
	test_8_bit_grey_alpha();
	test_16_bit_grey();
	test_8_bit_rgb();
	test_16_bit_rgb();

	/*printf("\n");*/
	/*test_open("samples/testfile_8bit_grey");*/
	/*printf("\n");*/
	/*test_open("samples/grey.png");*/
	/*printf("\n");*/
	/*test_open("samples/grad.png");*/
	/*printf("\n");*/
	/*test_open("samples/rms.png");*/
	/*printf("\n");*/
	/*test_open("samples/pngtest.png");*/
	/*printf("\n");*/
	/*test_open("samples/ruben_grey.png");*/
	/*printf("\n");*/
	/*test_open("samples/condense_test");*/
	/*printf("\n");*/
	/*test_open("samples/pixel.png");*/

	test_copy("samples/testfile_8bit_grey", "samples/copy_test");
	test_copy("samples/rms.png", "samples/copy_test");
	test_copy("samples/pixel.png", "samples/copy_test");
	test_remove_filter();
	test_invert("samples/ruben_grey.png");
	test_rotate("samples/rms16alpha.png");
	test_replace("samples/java.png");
	test_flip_horiz("samples/rms16alpha.png");
	test_flip_vert("samples/rms16alpha.png");
	test_write_unfiltered("samples/write_unfiltered");
	/*test_condense("samples/condense.png", 2);*/
	/*test_condense("samples/condense2.png", 3);*/
	/*test_condense("samples/condense3.png", 2);*/
	/*test_condense("samples/condense4.png", 9);*/
	/*test_condense("samples/gtasabin.png", 8);*/
	/*test_condense("samples/ruben.png", 8);*/
	test_condense("samples/rectangle.png", 2);
	test_pixelate("samples/ruben.png", 8);
	test_swap("samples/ruben.png");
}
