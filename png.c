#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <byteswap.h>
#include "png.h"

/*#define TEST_PRINT_DATA*/

uint8_t head[HEADER_SIZE] = {
	0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a
};
uint8_t iend[IEND_SIZE] = {
	0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
	0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
};
uint8_t ihdr_type[4] = { 0x49, 0x48, 0x44, 0x52 };
uint8_t idat_type[4] = { 0x49, 0x44, 0x41, 0x54 };
uint8_t plte_type[4] = { 0x50, 0x4c, 0x54, 0x45 };

int copy_ihdr(struct PNG *png, uint8_t *data, int *pos)
{
	uint32_t len = __bswap_32(13);

	if (memcmp(data+*pos, &len, 4)) {
		return 0;
	}
	png->IHDR_chunk.length = __bswap_32(len);
	*pos += 4;
	// load all fields of ihdr, including crc
	memcpy(&png->IHDR_chunk.type, data+*pos, 4);
	*pos += 4;
	png->IHDR_chunk.width = *(uint32_t*)(data+*pos);
	uint32_t lewidth = __bswap_32(png->IHDR_chunk.width);
	*pos += 4;
	png->IHDR_chunk.height = *(uint32_t*)(data+*pos);
	uint32_t leheight = __bswap_32(png->IHDR_chunk.height);
	*pos += 4;
	png->IHDR_chunk.bit_depth = *(data+*pos);
	*pos += 1;
	png->IHDR_chunk.color_type = *(data+*pos);
	*pos += 1;
	png->IHDR_chunk.compression = *(data+*pos);
	*pos += 1;
	png->IHDR_chunk.filter = *(data+*pos);
	*pos += 1;
	png->IHDR_chunk.interlace = *(data+*pos);
	*pos += 1;
	png->IHDR_chunk.crc = *(uint32_t*)(data+*pos);
	*pos += 4;
	// is crc correct?
	uint32_t crc_calc = crc(png->IHDR_chunk.type, IHDR_SIZE_NO_CRC);
	png->IHDR_chunk.crc = __bswap_32(png->IHDR_chunk.crc);
	png->IHDR_chunk.width = lewidth;
	png->IHDR_chunk.height = leheight;

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

int parse_idat(struct PNG *png, uint8_t *data, int *pos, int length)
{
	int i = png->nidat;
	png->nidat++;

	png->IDAT = realloc(png->IDAT, sizeof(struct chunk) * (i + 1));
	png->IDAT[i].length = length;
	memcpy(&png->IDAT[i].type, data + 4, 4);
	png->IDAT[i].data = malloc(length);
	memcpy(png->IDAT[i].data, data + 8, length);
	uint32_t crc_calc = crc(data + 4, length + 4);

	memcpy(&png->IDAT[i].crc, data + 8 + length, 4);
	png->IDAT[i].crc = __bswap_32(png->IDAT[i].crc);
	if (memcmp(&png->IDAT[i].crc, &crc_calc, 4)) {
		// incorrect crc
		return 0;
	}
	return 1;
}

int parse_plte(struct PNG *png, uint8_t *data, int *pos, int length)
{
	png->PLTE.length = length;
	png->PLTE.data = malloc(length);
	memcpy(png->PLTE.type, data + 4, 4);
	memcpy(png->PLTE.data, data + 8, length);
	uint32_t crc_calc = crc(data + 4, length + 4);

	memcpy(&png->PLTE.crc, data + 8 + length, 4);
	png->PLTE.crc = __bswap_32(png->PLTE.crc);
	if (memcmp(&png->PLTE.crc, &crc_calc, 4)) {
		// incorrect crc
		return 0;
	}

	return 1;
}

int parse_chunks(struct PNG *png, uint8_t *data, int fsize, int *pos)
{
	uint8_t chunk_type[4];
	png->IDAT = NULL;
	png->nidat = 0;
	// data here is at length of first chunk after IHDR
	data += *pos;

	while (1) {
		// copy type of next chunk
		uint32_t length;
		memcpy(&length, data, 4);
		length = __bswap_32(length);
		memcpy(chunk_type, data + 4, 4);

		if (!memcmp(chunk_type, idat_type, 4)) {
			if (!parse_idat(png, data, pos, length))
				return 0;
		} else if (!memcmp(chunk_type, plte_type, 4)) {
			if (!parse_plte(png, data, pos, length))
				return 0;
		} else if (!memcmp(chunk_type, iend + 4, 4)) {
			memcpy(&png->IEND, iend, 12);
			break;
		} else {
		}
		// move data ptr to length of next chunk
		data += 4 + 4 + length + 4;
	}

	// didn't go inside the loop
	return png->nidat != 0;
}

int get_file_size(FILE *f)
{
	fseek(f, 0, SEEK_END);
	int fsize = ftell(f);
	rewind(f);
	return fsize;
}

/*Load png specified in filename into png (must be allocated)*/
int png_open(struct PNG *png, char *filename)
{
	int pos = 0;
	FILE *f;
	if ((f = fopen(filename, "rb")) == NULL) {
		png = NULL;
		return 0;
	}
	int fsize = get_file_size(f);

	// TODO: valgrind complains about uninitialized
	uint8_t data[fsize];
	if (!fread(data, fsize, 1, f))
		return 0;
	fclose(f);

	// header + ihdr + iend
	if (fsize < 30)
		return 0;

	if (!copy_header(png, data, &pos)) {
		return 0;
	}
	if (!copy_ihdr(png, data, &pos)) {
		return 0;
	}
	if (!parse_chunks(png, data, fsize, &pos)) {
		return 0;
	}
	return 1;
}

struct PNG png_init(int width, int height, uint8_t bit_depth,
		    uint8_t color_type, uint8_t interlace)
{
	struct PNG png;
	memcpy(png.header, head, HEADER_SIZE);
	png.nidat = 0;
	memcpy(png.IEND, iend, IEND_SIZE);
	memcpy(png.IHDR_chunk.type, ihdr_type, 4);
	png.IHDR_chunk.length      = 0x0d;
	png.IHDR_chunk.width       = width;
	png.IHDR_chunk.height      = height;
	png.IHDR_chunk.bit_depth   = bit_depth;
	png.IHDR_chunk.color_type  = color_type;
	png.IHDR_chunk.compression = 0;
	png.IHDR_chunk.filter      = 0;
	png.IHDR_chunk.interlace   = interlace;

	return png;
}

/* Write png data stream into struct png, setting up all necessary
 * fields.
 * isfiltered: if 0 -> *data is not filtered, a filter will be applied */
void png_write(struct PNG *png, uint8_t *data, int datalen, int isfiltered)
{
	uint64_t compdatalen = datalen*2;
	uint8_t  *compdata   = malloc(compdatalen);
	if (png->nidat != 0) {
		png_close(png);
		png->nidat = 0;
	}
	png->nidat++;
	png->IDAT = malloc(sizeof(struct chunk));
 
	unsigned char *fil_data;
	if (!isfiltered) {
		uint64_t size;

		apply_filter(png, data, &size, &fil_data);
		data = fil_data;
		datalen = size;
	}

	memcpy(&png->IDAT[0].type, idat_type, 4);
	compress(compdata, &compdatalen, data, datalen);
	// must be freed with png_close
	png->IDAT[0].data = malloc(compdatalen);
	// copy compressed data
	memcpy(png->IDAT[0].data, compdata, compdatalen);

	// crc starts at type (4 bytes), not data
	uint8_t *crcbytes = malloc(compdatalen + 4);
	memcpy(crcbytes, &png->IDAT[0].type, 4);
	memcpy(crcbytes + 4, png->IDAT[0].data, compdatalen);
	png->IDAT[0].crc = crc(crcbytes, compdatalen + 4);
	png->IDAT[0].length = compdatalen;

	if (!isfiltered)
		free(fil_data);
	free(crcbytes);
	free(compdata);
}

/* Returns 1 if successful, 0 otherwise */
int png_dump(struct PNG *png, char *filename)
{
	FILE *f;
	if (!(f = fopen(filename, "wb")))
		return 0;

	png->IHDR_chunk.length = __bswap_32(png->IHDR_chunk.length);
	png->IHDR_chunk.width = __bswap_32(png->IHDR_chunk.width);
	png->IHDR_chunk.height = __bswap_32(png->IHDR_chunk.height);
	png->IHDR_chunk.crc = crc(png->IHDR_chunk.type, IHDR_SIZE_NO_CRC);
	uint32_t beihdrlen = png->IHDR_chunk.length;
	uint32_t bewidth = png->IHDR_chunk.width;
	uint32_t beheight = png->IHDR_chunk.height;
	png->IHDR_chunk.length = __bswap_32(png->IHDR_chunk.length);
	png->IHDR_chunk.width = __bswap_32(png->IHDR_chunk.width);
	png->IHDR_chunk.height = __bswap_32(png->IHDR_chunk.height);

	fwrite(png->header, HEADER_SIZE, 1, f);
	fwrite(&beihdrlen, 4, 1, f);
	fwrite(png->IHDR_chunk.type, 4, 1, f);
	fwrite(&bewidth, 4, 1, f);
	fwrite(&beheight, 4, 1, f);
	fwrite(&png->IHDR_chunk.bit_depth, 1, 1, f);
	fwrite(&png->IHDR_chunk.color_type, 1, 1, f);
	fwrite(&png->IHDR_chunk.compression, 1, 1, f);
	fwrite(&png->IHDR_chunk.filter, 1, 1, f);
	fwrite(&png->IHDR_chunk.interlace, 1, 1, f);
	uint32_t beihdrcrc = __bswap_32(png->IHDR_chunk.crc);
	fwrite(&beihdrcrc, 4, 1, f);
	if (png->IHDR_chunk.color_type == PNG_PLTE) {
		uint32_t beidatlen = __bswap_32(png->PLTE.length);
		fwrite(&beidatlen, 4, 1, f);
		fwrite(&png->PLTE.type, 4, 1, f);
		fwrite(png->PLTE.data, 1, png->PLTE.length, f);
		uint32_t beidatcrc = __bswap_32(png->PLTE.crc);
		fwrite(&beidatcrc, 4, 1, f);
	}
	for (int i = 0; i < png->nidat; i++) {
		uint32_t beidatlen = __bswap_32(png->IDAT[i].length);
		fwrite(&beidatlen, 4, 1, f);
		fwrite(&png->IDAT[i].type, 4, 1, f);
		fwrite(png->IDAT[i].data, 1, png->IDAT[i].length, f);
		uint32_t beidatcrc = __bswap_32(png->IDAT[i].crc);
		fwrite(&beidatcrc, 4, 1, f);
	}
	fwrite(png->IEND, IEND_SIZE, 1, f);
	fclose(f);
	return 1;
}

/* Mirror structure src into dst (allocates all necessary memory!) */
void png_copy(struct PNG *src, struct PNG *dst)
{
	memcpy(dst->IHDR_chunk.type, src->IHDR_chunk.type, 4);
	dst->IHDR_chunk.length      = src->IHDR_chunk.length;
	dst->IHDR_chunk.width       = src->IHDR_chunk.width;
	dst->IHDR_chunk.height      = src->IHDR_chunk.height;
	dst->IHDR_chunk.bit_depth   = src->IHDR_chunk.bit_depth;
	dst->IHDR_chunk.color_type  = src->IHDR_chunk.color_type;
	dst->IHDR_chunk.compression = src->IHDR_chunk.compression;
	dst->IHDR_chunk.interlace   = src->IHDR_chunk.interlace;
	dst->IHDR_chunk.crc         = src->IHDR_chunk.crc;

	if (dst->IHDR_chunk.color_type == PNG_PLTE) {
		dst->PLTE.length = src->PLTE.length;
		dst->PLTE.data = malloc(src->PLTE.length);
		memcpy(dst->PLTE.type, src->PLTE.type, 4);
		memcpy(dst->PLTE.data, src->PLTE.data, src->PLTE.length);
		dst->PLTE.crc = src->PLTE.crc;
	}

	dst->nidat = src->nidat;
	dst->IDAT = malloc(sizeof(struct chunk) * src->nidat);
	for (int i = 0; i < src->nidat; i++) {
		dst->IDAT[i].length = src->IDAT[i].length;
		memcpy(dst->IDAT[i].type, src->IDAT[i].type, 4);
		dst->IDAT[i].data = malloc(src->IDAT[i].length);
		memcpy(dst->IDAT[i].data, src->IDAT[i].data,
		       src->IDAT[i].length);
		dst->IDAT[i].crc = src->IDAT[i].crc;
	}
	memcpy(dst->IEND, src->IEND, IEND_SIZE);
}

/* Returns raw (filtered) data and its length in rawlen */
uint8_t *png_get_raw_data(struct PNG *png, uint64_t *rawlen)
{
	int bpp;
	png_calc_bpp(png, &bpp);
	*rawlen = (png->IHDR_chunk.width*bpp+1) * png->IHDR_chunk.height;
	uint8_t *data_raw = malloc(*rawlen);
	uint8_t *tmp = malloc(*rawlen);
	int len = 0;

	for (int i = 0; i < png->nidat; i++) {
		memcpy(tmp + len, png->IDAT[i].data, png->IDAT[i].length);
		len += png->IDAT[i].length;
	}
	uncompress(data_raw, rawlen, tmp, len);
	free(tmp);
	return data_raw;
}

void png_close(struct PNG *png)
{
	for (int i = 0; i < png->nidat; i++) {
		free(png->IDAT[i].data);
	}
	if (png->IHDR_chunk.color_type == PNG_PLTE)
		free(png->PLTE.data);
	free(png->IDAT);
}

void print_png_raw(struct PNG *png)
{
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
	printf("\nwidth: %08x\n", png->IHDR_chunk.width);
	printf("height: %08x\n", png->IHDR_chunk.height);
	printf("bit depth: %02x\n", png->IHDR_chunk.bit_depth);
	printf("color type: %02x\n", png->IHDR_chunk.color_type);
	printf("compression: %02x\n", png->IHDR_chunk.compression);
	printf("filter: %02x\n", png->IHDR_chunk.filter);
	printf("interlace: %02x\n", png->IHDR_chunk.interlace);
	printf("crc: %08x\n", png->IHDR_chunk.crc);
	printf("\n");

	// palette png
	if (png->IHDR_chunk.color_type == PNG_PLTE) {
		printf("#### PLTE ####\n");
		printf("length: %02x\n", png->PLTE.length);
#ifdef TEST_PRINT_DATA
		printf("\npalette data:");
		for (int i = 0; i < png->PLTE.length; i++) {
			if (i % 16 == 0) printf("\n");
			printf("%02x ", png->PLTE.data[i]);
		}
#endif
		printf("\ncrc: %08x\n", png->PLTE.crc);
	}

	for (int idats = 0; idats < png->nidat; idats++) {
		int len = png->IDAT[idats].length;
		int bpp;
		png_calc_bpp(png, &bpp);
		printf("#### IDAT %d ####\n", idats);
		printf("length: %08x\n", len);
		printf("type: ");
		for (int i = 0; i < 4; i++)
			printf("%c", png->IDAT[idats].type[i]);
#ifdef TEST_PRINT_DATA
		uint64_t rawlen = (png->IHDR_chunk.width * bpp + 1)
				  * png->IHDR_chunk.height;
		uint8_t data_raw[rawlen];
		printf("\ndata (compressed):");
		for (int i = 0; i < len; i++) {
			if (i % 16 == 0) printf("\n");
			printf("%02x ", png->IDAT[idats].data[i]);
		}
		
		printf("\ndata (raw):");

		uncompress(data_raw, &rawlen, png->IDAT[idats].data, len);
		for (int i = 0; i < rawlen; i++) {
			if (i % 16 == 0) printf("\n");
			printf("%02x ", data_raw[i]);
		}
#endif
		printf("\ncrc: %08x\n", png->IDAT[idats].crc);
	}

	printf("\n#### IEND ####\n");
	for (int i = 0; i < IEND_SIZE; i++) {
		printf("%02x " , png->IEND[i] & 0xFF);
	}
	printf("\n");
}
