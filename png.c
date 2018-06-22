#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <byteswap.h>
#include "png.h"

uint8_t head[HEADER_SIZE] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
};
uint8_t iend[IEND_SIZE] = {
	0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
	0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};
uint8_t ihdr_type[4] = {
	0x49, 0x48, 0x44, 0x52
};
uint8_t idat[4] = {0x49, 0x44, 0x41, 0x54};

int copy_ihdr(struct PNG *png, uint8_t *data, int *pos)
{
	uint32_t len = __bswap_32(13);
	if (memcmp(data+*pos, &len, 4)) {
		return 0;
	}
	png->IHDR_chunk.length = len;
	*pos += 4;
	// load all fields of ihdr, including crc
	memcpy(&png->IHDR_chunk.type, data+*pos, 4);
	*pos += 4;
	memcpy(&png->IHDR_chunk.width, data+*pos, 4);
	*pos += 4;
	memcpy(&png->IHDR_chunk.height, data+*pos, 4);
	*pos += 4;
	memcpy(&png->IHDR_chunk.bit_depth, data+*pos, 1);
	*pos += 1;
	memcpy(&png->IHDR_chunk.color_type, data+*pos, 1);
	*pos += 1;
	memcpy(&png->IHDR_chunk.compression, data+*pos, 1);
	*pos += 1;
	memcpy(&png->IHDR_chunk.filter, data+*pos, 1);
	*pos += 1;
	memcpy(&png->IHDR_chunk.interlace, data+*pos, 1);
	*pos += 1;
	memcpy(&png->IHDR_chunk.crc, data+*pos, 4);
	*pos += 4;
	// is crc correct?
	uint32_t crc_calc = __bswap_32(crc(png->IHDR_chunk.type,
					IHDR_SIZE_NO_CRC));
	if (memcmp(&png->IHDR_chunk.crc, &crc_calc, 4)) {
		return 0;
	}
	return 1;
};

int copy_header(struct PNG *png, uint8_t *data, int *pos)
{
	// invalid header
	if (memcmp(data, head, HEADER_SIZE)) {
		return 0;
	}
	memcpy(&png->header, data, 8);
	*pos += 8;
	return 1;
}

int copy_idat(struct PNG *png, uint8_t *data, int fsize)
{
	uint8_t *idat_type;
	if ((idat_type = memmem(data, fsize, idat, 4)) == NULL)
		return 0;
	memcpy(&png->IDAT.length, idat_type - 4, 4);
	int length = __bswap_32(png->IDAT.length);
	memcpy(&png->IDAT.type, idat_type, 4);
	png->IDAT.data = malloc(length);
	memcpy(png->IDAT.data, idat_type + 4, length);
	uint32_t crc_calc = __bswap_32(crc(idat_type, length + 4));

	memcpy(&png->IDAT.crc, idat_type + 4 + length, 4);
	if (memcmp(&png->IDAT.crc, &crc_calc, 4)) {
		return 0;
	}
	
	return 1;
}

int copy_iend(struct PNG *png, uint8_t *data, int fsize)
{
	uint8_t *idat_type;
	if ((idat_type = memmem(data, fsize, iend, 12)) == NULL)
		return 0;
	memcpy(&png->IEND, idat_type, 12);
	return 1;
}

int get_file_size(FILE *f)
{
	fseek(f, 0, SEEK_END);
	int fsize = ftell(f);
	rewind(f);
	return fsize;
}

/* Load png specified in filename into png (must be allocated) */
int png_open(char *filename, struct PNG *png)
{
	int pos = 0;
	FILE *f;
	if (!(f = fopen(filename, "rb"))) {
		png = NULL;
		return 0;
	}
	int fsize = get_file_size(f);

	uint8_t data[fsize];
	fread(data, fsize, 1, f);
	fclose(f);

	// header + ihdr + iend
	if (fsize < 30)
		return 0;

	if (!copy_header(png, data, &pos))
		return 0;
	if (!copy_ihdr(png, data, &pos))
		return 0;
	if (!copy_idat(png, data, fsize))
		return 0;
	if (!copy_iend(png, data, fsize))
		return 0;
	return 1;
}

struct PNG png_init(int width, int height, uint8_t bit_depth,
		    uint8_t color_type, uint8_t interlace)
{
	struct PNG png;
	memcpy(png.header, head, HEADER_SIZE);
	memcpy(png.IEND, iend, IEND_SIZE);
	memcpy(png.IHDR_chunk.type, ihdr_type, 4);
	png.IHDR_chunk.length      = __bswap_32(0x0d);
	png.IHDR_chunk.width       = __bswap_32(width);
	png.IHDR_chunk.height      = __bswap_32(height);
	png.IHDR_chunk.bit_depth   = bit_depth;
	png.IHDR_chunk.color_type  = color_type;
	png.IHDR_chunk.compression = 0;
	png.IHDR_chunk.filter      = 0;
	png.IHDR_chunk.interlace   = interlace;
	png.IHDR_chunk.crc = __bswap_32(crc(png.IHDR_chunk.type, 
					IHDR_SIZE_NO_CRC));

	return png;
}

/* Write png data stream into struct png, setting up all necessary
 * fields */
void png_write(struct PNG *png, uint8_t *data, int datalen)
{
	uint64_t compdatalen = datalen*2;
	uint8_t  *compdata   = malloc(compdatalen);

	memcpy(&png->IDAT.type, idat, 4);
	compress(compdata, &compdatalen, data, datalen);
	// must be freed with png_close
	png->IDAT.data = malloc(compdatalen);
	// copy compressed data
	memcpy(png->IDAT.data, compdata, compdatalen);

	// crc starts at type (4 bytes), not data
	uint8_t *crcbytes = malloc(compdatalen + 4);
	memcpy(crcbytes, &png->IDAT.type, 4);
	memcpy(crcbytes + 4, png->IDAT.data, compdatalen);
	png->IDAT.crc = crc(crcbytes, compdatalen + 4);
	png->IDAT.crc = __bswap_32((uint32_t)png->IDAT.crc);
	png->IDAT.length = compdatalen;

	free(crcbytes);
	free(compdata);
}

/* Returns 1 if successful, 0 otherwise */
int png_dump(struct PNG *png, char *filename)
{
	FILE *f;
	if (!(f = fopen(filename, "wb")))
		return 0;

	fwrite(png->header, HEADER_SIZE, 1, f);
	fwrite(&png->IHDR_chunk.length, 4, 1, f);
	fwrite(png->IHDR_chunk.type, 4, 1, f);
	fwrite(&png->IHDR_chunk.width, 4, 1, f);
	fwrite(&png->IHDR_chunk.height, 4, 1, f);
	fwrite(&png->IHDR_chunk.bit_depth, 1, 1, f);
	fwrite(&png->IHDR_chunk.color_type, 1, 1, f);
	fwrite(&png->IHDR_chunk.compression, 1, 1, f);
	fwrite(&png->IHDR_chunk.filter, 1, 1, f);
	fwrite(&png->IHDR_chunk.interlace, 1, 1, f);
	fwrite(&png->IHDR_chunk.crc, 4, 1, f);
	uint32_t bigendianlen = __bswap_32(png->IDAT.length);
	fwrite(&bigendianlen, 4, 1, f);
	fwrite(&png->IDAT.type, 4, 1, f);
	/*printf("raw:\n");*/
	/*for (int i = 0; i < png->IDAT.length; i++) {*/
		/*printf("%02x ", png->IDAT.data[i] );*/
	/*}*/
	fwrite(png->IDAT.data, 1, png->IDAT.length, f);
	fwrite(&png->IDAT.crc, 4, 1, f);
	fwrite(png->IEND, IEND_SIZE, 1, f);
	fclose(f);
	return 1;
}

void png_close(struct PNG *png)
{
	free(png->IDAT.data);
}

void print_png_raw(struct PNG *png)
{
	int bigendianlen = __bswap_32(png->IDAT.length);
	uint8_t data_raw[bigendianlen*4];

	printf("#### HEADER ####\n");
	for (int i = 0; i < HEADER_SIZE; i++) {
		printf("%02x ", png->header[i] & 0xFF);
	}
	printf("\n\n");
	printf("#### IHDR ####\n");
	printf("length: %08x\n", png->IHDR_chunk.length);
	printf("type: ");
	for (int i = 0; i < 4; i++)
		printf("%c", png->IHDR_chunk.type[i]);
	printf("\nwidth: %08x\n", __bswap_32(png->IHDR_chunk.width));
	printf("height: %08x\n", __bswap_32(png->IHDR_chunk.height));
	printf("bit depth: %02x\n", png->IHDR_chunk.bit_depth);
	printf("color type: %02x\n", png->IHDR_chunk.color_type);
	printf("compression: %02x\n", png->IHDR_chunk.compression);
	printf("filter: %02x\n", png->IHDR_chunk.filter);
	printf("interlace: %02x\n", png->IHDR_chunk.interlace);
	printf("crc: %08x\n", __bswap_32(png->IHDR_chunk.crc));
	printf("\n");

	printf("#### IDAT ####\n");
	printf("length: %08x\n", bigendianlen);
	printf("type: ");
	uint8_t *p = (uint8_t*)&png->IDAT.type;
	for (int i = 0; i < 4; i++)
		printf("%c", *p++);
	printf("\ndata (compressed):");
	for (int i = 0; i < bigendianlen; i++) {
		if (i % 16 == 0) printf("\n");
		printf("%02x ", png->IDAT.data[i]);
	}
	
	printf("\ndata (raw):");
	uint64_t rawlen = bigendianlen;
	uncompress(data_raw, &rawlen, png->IDAT.data, bigendianlen);

	for (int i = 0; i < rawlen; i++) {
		if (i % 16 == 0) printf("\n");
		printf("%02x ", data_raw[i]);
	}
	printf("\ncrc: %08x\n", __bswap_32(png->IDAT.crc));

	printf("\n#### IEND ####\n");
	for (int i = 0; i < IEND_SIZE; i++) {
		printf("%02x " , png->IEND[i] & 0xFF);
	}
	printf("\n");
}
