#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "png.h"
#include "transform.h"

#define PI 3.14159265359

uint8_t *get_unfiltered(struct PNG *png, uint64_t *raw_len)
{
	uint64_t fil_len;
	uint8_t *raw_data;
	uint8_t *fil_data = png_get_raw_data(png, &fil_len);

	if (!remove_filter(png, fil_data, raw_len, &raw_data)) {
		return NULL;
	}
	free(fil_data);
	return raw_data;
}

int png_get_tform(struct PNG *png, struct _png_tform *tform)
{
	if (!png_calc_bpp(png, &tform->bpp))
		return 0;
	tform->alpha = png_calc_alpha(png);
	tform->width = png->IHDR_chunk.width;
	tform->height = png->IHDR_chunk.height;
	tform->bit_depth = png->IHDR_chunk.bit_depth;
	tform->color_type = png->IHDR_chunk.color_type;
	tform->data = get_unfiltered(png, &tform->len);
	if (tform->data == NULL)
		return 0;
	return 1;
}

void png_tform_free(struct _png_tform *tform)
{
	free(tform->data);
}

/* RGB(A) to GREY(A) */
void png_rgb_to_grey(struct _png_tform tf, uint64_t new_len,
		     uint8_t *new_data, int newbpp, int newalpha)
{
	int cnt = 0;
	uint32_t avg = 0;
	int bps = tf.bit_depth >> 3;
	for (int i = 0; i < tf.len; i += tf.bpp) {
		// get avg of all rgb pixels as grey value
		for (int j = 0; j < tf.bpp - tf.alpha; j+=bps)
			// TODO: GET PROPER AVERAGE (treat as 16-bit)
			for (int k = 0; k < bps; k++)
				avg += (uint32_t)tf.data[i + j + k] / bps;
		avg /= (tf.bpp - tf.alpha) / bps;
		// set all samples as avg (in case bit_depth > 8)
		for (int j = 0; j < newbpp - newalpha; j++)
			new_data[cnt++] = (uint8_t)avg;
		if (newalpha > 0) {
			if (tf.alpha > 0) {
				memcpy(new_data + cnt,
				       tf.data + i + tf.bpp - tf.alpha,
				       newalpha);
			} else {
				memset(new_data + cnt, 0xff, newalpha);
			}
			cnt += newalpha;
		}
		avg = 0;
	}
}

/* GREY(A) to RGB(A) */
void png_grey_to_rgb(struct _png_tform tf, uint64_t new_len,
		     uint8_t *new_data, int newbpp, int newalpha)
{
	int cnt = 0;
	int bps = tf.bit_depth >> 3;
	for (int i = 0; i < tf.len; i += tf.bpp) {
		for (int j = 0; j < newbpp - newalpha; j++) {
			memcpy(new_data + cnt, tf.data + i, bps);
			cnt += bps;
		}
	if (newalpha > 0) {
			if (tf.alpha > 0) {
				memcpy(new_data + cnt,
				       tf.data + i + tf.bpp - tf.alpha,
				       newalpha);
			} else {
				memset(new_data + cnt, 0xff, newalpha);
			}
			cnt += newalpha;
		}
	}
}

int png_plte_to_rgb(struct PNG *png)
{
	uint8_t *new_data;
	uint64_t new_len;
	struct _png_tform tf;

	if (!png_get_tform(png, &tf))
		return 0;

	new_len = tf.len * 3;
	new_data = malloc(new_len);
	for (int i = 0; i < tf.len; i++) {
		memcpy(new_data + i*3, png->PLTE.data + tf.data[i] * 3, 3);
	}
	png->IHDR_chunk.color_type = PNG_RGB;
	png->IHDR_chunk.bit_depth = 8;
	png_write(png, new_data, new_len, 0);
	free(png->PLTE.data);
	free(new_data);
	png_tform_free(&tf);
	return 1;
}

/* Change color type of the png to color_type (see png.h #defines) */
int png_change_color_type(struct PNG *png, int color_type)
{
	int newbpp, newalpha;
	uint8_t *new_data;
	uint64_t new_len;
	struct _png_tform tf;

	// special case: convert from palette first
	if (png->IHDR_chunk.color_type == PNG_PLTE) {
		if (!png_plte_to_rgb(png))
			return 0;
	}
	// set alpha according to final color
	if (png->IHDR_chunk.color_type == color_type + 4) {
		png_remove_alpha(png);
	} else if (png->IHDR_chunk.color_type == color_type - 4) {
		png_add_alpha(png, 0xff);
	}

	// nothing to do
	if (color_type == png->IHDR_chunk.color_type)
		return 1;
	if (!png_get_tform(png, &tf))
		return 0;

	png->IHDR_chunk.color_type = color_type;
	if (!png_calc_bpp(png, &newbpp))
		return 0;
	newalpha = png_calc_alpha(png);
	new_len = tf.height * tf.width * newbpp;
	new_data = malloc(new_len);

	switch (color_type) {
	case PNG_GREY:
	case PNG_GREYA:
		png_rgb_to_grey(tf, new_len, new_data, newbpp, newalpha);
		break;
	case PNG_RGB:
	case PNG_RGBA:
		png_grey_to_rgb(tf, new_len, new_data, newbpp, newalpha);
		break;
	}

	png_write(png, new_data, new_len, 0);
	free(new_data);
	png_tform_free(&tf);
	return 1;
}

/* Change bit depth to bit_depth, valid values are 8 and 16. */
int png_change_bit_depth(struct PNG *png, int bit_depth)
{
	uint64_t new_len;
	uint8_t *new_data;
	int newbpp;

	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;

	png->IHDR_chunk.bit_depth = bit_depth;
	if (!png_calc_bpp(png, &newbpp))
		return 0;
	new_len = tf.height * tf.width * newbpp;
	new_data = calloc(new_len, 1);
	// bytes per sample
	int bps = tf.bit_depth >> 3;
	int newbps = bit_depth >> 3;
	int cnt = 0;

	for (int i = 0; i < tf.len; i += bps) {
		memcpy(new_data + cnt, tf.data + i,
		       newbps > bps ? bps : newbps);
		cnt += newbps;
	}
	png->IHDR_chunk.bit_depth = bit_depth;
	png_write(png, new_data, new_len, 0);
	free(new_data);
	png_tform_free(&tf);
	return 1;
}

/* Delete alpha channel from png. */
int png_remove_alpha(struct PNG *png)
{
	uint64_t new_len;
	uint8_t *new_data;
	int newbpp;

	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;
	if (tf.alpha == 0)
		return 0;

	newbpp = tf.bpp - tf.alpha;
	new_len = tf.height * tf.width * newbpp;
	new_data = malloc(new_len);
	int cnt = 0;
	for (int i = 0; i < tf.len; i += tf.bpp) {
		memcpy(new_data + cnt, tf.data + i, newbpp);
		cnt += newbpp;
	}

	png->IHDR_chunk.color_type -= 4;
	png_write(png, new_data, new_len, 0);
	free(new_data);
	png_tform_free(&tf);
	return 1;
}

/* Add an alpha channel with value alpha (0: transparent, ff: opaque) */
int png_add_alpha(struct PNG *png, int alpha)
{
	uint64_t new_len;
	uint8_t *new_data;
	int newbpp;

	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;
	if (tf.alpha != 0)
		return 0;

	int bps = tf.bit_depth >> 3;
	newbpp = tf.bpp + bps;
	new_len = tf.height * tf.width * newbpp;
	new_data = malloc(new_len);
	int cnt = 0;
	for (int i = 0; i < tf.len; i += tf.bpp) {
		memcpy(new_data + cnt, tf.data + i, tf.bpp);
		cnt += tf.bpp;
		memset(new_data + cnt, alpha, bps);
		cnt += bps;
	}

	png->IHDR_chunk.color_type += 4;
	png_write(png, new_data, new_len, 0);
	free(new_data);
	png_tform_free(&tf);
	return 1;
}

/* Invert color (doesn't invert alpha) */
int png_invert(struct PNG *png)
{
	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;
	for (int i = 0; i < tf.len; i+=tf.bpp) {
		for (int j = 0; j < tf.bpp - tf.alpha; j++)
			tf.data[i+j] = 0xFF - tf.data[i+j];
	}
	png_write(png, tf.data, tf.len, 0);
	png_tform_free(&tf);
	return 1;
}

/* Append p2 to the right of p1 and return the result in a new PNG
 * ret is the return value (must be allocated)
 * Restrictions (hopefully to change soon):
 * 	- they must be the same color type
 * 	- they must have the same bit depth
 */
struct PNG png_append_horiz(struct PNG *p1, struct PNG *p2, int *ret)
{
	struct PNG res;
	struct _png_tform t1;
	struct _png_tform t2;
	if (!png_get_tform(p1, &t1) || !png_get_tform(p2, &t2)) {
		*ret = 0;
		return res;
	}

	int newwidth = t1.width + t2.width;
	int newheight = MAX(t1.height, t2.height);
	uint64_t new_len = (newwidth * t1.bpp) * newheight;
	uint8_t *new_data = calloc(1, new_len);

	/*printf("newheight: %d\n", newheight);*/
	/*printf("newwidth: %d\n", newwidth);*/

	res = png_init(newwidth, newheight, t1.bit_depth,
		       t1.color_type, 0);
	/*  new_data:
	 *        pos1               pos2
	 *         |                  |
	 *         v                  v
	 *	   +------------------------------------+
	 *	   |  png1 - row0     |  png2 - row0    |
	 *	   +------------------+-----------------+
	 *	   |  png1 - row1     |  png2 - row1    |
	 *	   +------------------------------------+
	 */
	for (int i = 0; i < newheight; i++) {
		int pos1 = i * (newwidth * t1.bpp);
		int pos2 = pos1 + t1.width * t1.bpp;
		int png1off = i * (t1.width * t1.bpp);
		int png2off = i * (t2.width * t1.bpp);
		// dont access out of bounds, image will be transparent
		// since the memory is allocated with calloc
		if (i < t1.height) {
			memcpy(new_data + pos1, t1.data + png1off,
			       t1.width * t1.bpp);
		}
		if (i < t2.height) {
			memcpy(new_data + pos2, t2.data + png2off,
			       t2.width * t1.bpp);
		}
	}
	png_write(&res, new_data, new_len, 0);

	png_tform_free(&t1);
	png_tform_free(&t2);
	free(new_data);
	*ret = 1;
	return res;
}

/* Append p2 below p1 and return the result in a new PNG
 * ret is the return value (must be allocated)
 * Restrictions (hopefully to change soon):
 * 	- they must be the same color type
 * 	- they must have the same bit depth
 */
struct PNG png_append_vert(struct PNG *p1, struct PNG *p2, int *ret)
{
	struct PNG res;
	struct _png_tform t1;
	struct _png_tform t2;
	if (!png_get_tform(p1, &t1) || !png_get_tform(p2, &t2)) {
		*ret = 0;
		return res;
	}

	int newwidth = MAX(t1.width, t2.width);
	int newheight = t1.height + t2.height;
	uint64_t new_len = (newwidth * t1.bpp) * newheight;
	uint8_t *new_data = calloc(1, new_len);

	/*printf("newheight: %d\n", newheight);*/
	/*printf("newwidth: %d\n", newwidth);*/

	res = png_init(newwidth, newheight, t1.bit_depth, t1.color_type, 0);

	for (int i = 0; i < t1.height; i++) {
		int bigrow = i * (newwidth * t1.bpp);
		int row = i * (t1.width * t1.bpp);
		memcpy(new_data + bigrow, t1.data + row, t1.width * t1.bpp);
	}
	for (int i = 0; i < t2.height; i++) {
		int bigrow = (i + t1.height) * (newwidth * t1.bpp);
		int row = i * (t2.width * t1.bpp);
		memcpy(new_data + bigrow, t2.data + row, t2.width * t1.bpp);
	}

	png_write(&res, new_data, new_len, 0);

	png_tform_free(&t1);
	png_tform_free(&t2);
	free(new_data);
	*ret = 1;
	return res;
}

/* Replace src_color (must be the right size) with dst_color */
int png_replace(struct PNG *png, uint8_t *src_color, uint8_t *dst_color)
{
	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;
	for (int i = 0; i < tf.len; i += tf.bpp) {
		if (!memcmp(tf.data + i, src_color, tf.bpp - tf.alpha)) {
			memcpy(tf.data + i, dst_color, tf.bpp - tf.alpha);
		}
	}
	png_write(png, tf.data, tf.len, 0);
	png_tform_free(&tf);
	return 1;
}

int png_flip_horizontal(struct PNG *png)
{
	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;
	uint8_t tmp[tf.bpp];

	// swap columns (by swapping each row individually)
	for (int j = 0; j < tf.height; j++) {
		for (int i = 0; i < tf.width/2; i++) {
			// left position, increases until the midpoint
			int beg = i * tf.bpp + tf.width * tf.bpp * j;
			// right position, decreases
			int end = (tf.width - 1 - i) * tf.bpp + tf.width *
				  tf.bpp * j;
			
			memcpy(tmp, tf.data + beg, tf.bpp);
			memcpy(tf.data + beg, tf.data + end, tf.bpp);
			memcpy(tf.data + end, tmp, tf.bpp);
		}
	}

	png_write(png, tf.data, tf.len, 0);
	png_tform_free(&tf);
	return 1;
}

int png_flip_vertical(struct PNG *png)
{
	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;
	uint8_t tmp[tf.width*tf.bpp];

	for (int i = 0; i < tf.height/2; i++) {
		int beg = i * tf.width * tf.bpp;
		int end = (tf.height - 1 - i) * tf.width * tf.bpp;
		memcpy(tmp, tf.data + beg, tf.width * tf.bpp);
		memcpy(tf.data + beg, tf.data + end, tf.width * tf.bpp);
		memcpy(tf.data + end, tmp, tf.width * tf.bpp);
	}

	png_write(png, tf.data, tf.len, 0);
	png_tform_free(&tf);
	return 1;
}

/* Rotate png 90 degrees clockwise */
int png_rotate(struct PNG *png)
{
	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;

	uint8_t *transp = malloc(tf.len);

	// reverse rows and transpose matrix
	for (int i = 0; i < tf.height; i++) {
		memcpy(transp + i * tf.width * tf.bpp,
		       tf.data + (tf.height - 1 -i) * tf.width * tf.bpp,
		       tf.width * tf.bpp);
	}

	for (int j = 0; j < tf.height; j++) {
		for (int i = 0; i < tf.width; i++) {
			int dst = (i * tf.height + j) * tf.bpp;
			int src = (j * tf.width + i) * tf.bpp;
			memcpy(tf.data + dst, transp + src, tf.bpp);
		}
	}

	png->IHDR_chunk.width = tf.height;
	png->IHDR_chunk.height = tf.width;
	png_write(png, tf.data, tf.len, 0);
	png_tform_free(&tf);
	free(transp);
	return 1;
}

/* Swap each pixel with its right neighbour */
int png_swap(struct PNG *png)
{
	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;
	uint8_t tmp[tf.bpp];

	for (int i = 0; i < tf.len - tf.bpp; i += tf.bpp) {
		memcpy(tmp, tf.data + i, tf.bpp);
		memcpy(tf.data + i, tf.data + i + tf.bpp, tf.bpp);
		memcpy(tf.data + i + tf.bpp, tmp, tf.bpp);
	}
	png_write(png, tf.data, tf.len, 0);
	png_tform_free(&tf);
	return 1;
}


int int_ceil_div(int dividend, int divisor) {
	return (dividend + divisor - 1)/divisor;
}

/* Condense png by condratio: downscale image by taking the average of
 * all pixels in the square formed by condratio ^ 2 */
int png_condense(struct PNG *png, int condratio)
{
	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;

	if (condratio <= 0 || condratio > tf.width || condratio > tf.height)
		return 0;
	int newwidth = int_ceil_div(tf.width, condratio);
	int newheight = int_ceil_div(tf.height, condratio);
	int newlength = newwidth * newheight * tf.bpp;

	uint8_t *cond_data = malloc(newlength);

	uint32_t tmpbyte = 0;
	int subindex = 0;
	int condindex = 0;
	// account for edge pixels that dont make up an entire square
	int npixels = 0;
	int rwidth = tf.width * tf.bpp;

	//TODO: not enough image data when result is 1px
	
	// total tf.height, increased in condratio steps
	for (int j = 0; j < tf.height; j+=condratio) {
		// total tf.width, increased in condratio steps
		for (int i = 0; i < rwidth; i+=condratio*tf.bpp) {
			// bytes per pixel inside each submatrix
			for (int m = 0; m < tf.bpp; m++) {
				// submatrix tf.height
				for (int k = 0; k < condratio; k++) {
					// submatrix tf.width
					for (int l = 0; l < condratio; l++) {
						subindex = i + j*rwidth + l*tf.bpp
							   + k*rwidth + m;
						// check we're within boundaries (l stays in same column)
						if (subindex < tf.len && i+l*tf.bpp < rwidth) {
							tmpbyte += tf.data[subindex];
							npixels++;
						}
					}
				}
				tmpbyte /= npixels;
				npixels = 0;
				/*printf("\ntmpbyte: %d\n", tmpbyte);*/
				condindex = (i/condratio) +
				            (j/condratio)*newwidth*tf.bpp + m;
				/*printf("condindex:%d\n", condindex);*/
				if (condindex < newlength)
					cond_data[condindex] = (uint8_t)tmpbyte;
				tmpbyte = 0;
			}
		}
	}

	png->IHDR_chunk.width = newwidth;
	png->IHDR_chunk.height = newheight;

	png_tform_free(&tf);
	png_write(png, cond_data, newlength, 0);
	free(cond_data);
	return 1;
}

int png_pixelate(struct PNG *png, int condratio)
{
	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;

	if (condratio <= 0 || condratio > tf.width || condratio > tf.height)
		return 0;

	uint32_t tmpbyte = 0;
	int subindex = 0;
	// account for edge pixels that dont make up an entire square
	int npixels = 0;
	int rwidth = tf.width * tf.bpp;

	//TODO: not enough image data when result is 1px
	
	// total height, increased in condratio steps
	for (int j = 0; j < tf.height; j+=condratio) {
		// total width, increased in condratio steps
		for (int i = 0; i < rwidth; i+=condratio*tf.bpp) {
			// bytes per pixel inside each submatrix
			for (int m = 0; m < tf.bpp; m++) {
				// submatrix height
				for (int k = 0; k < condratio; k++) {
					// submatrix width
					for (int l = 0; l < condratio; l++) {
						subindex = i + j*rwidth + l*tf.bpp
							   + k*rwidth + m;
						// check we're within boundaries (l stays in same column)
						if (subindex < tf.len && i+l*tf.bpp < rwidth) {
							tmpbyte += tf.data[subindex];
							npixels++;
						}
					}
				}
				tmpbyte /= npixels;
				npixels = 0;
				/*printf("\ntmpbyte: %d\n", tmpbyte);*/
				for (int k = 0; k < condratio; k++) {
					// submatrix width
					for (int l = 0; l < condratio; l++) {
						subindex = i + j*rwidth + l*tf.bpp
							   + k*rwidth + m;
						// check we're within boundaries (l stays in same column)
						if (subindex < tf.len && i+l*tf.bpp < rwidth) {
							tf.data[subindex] = tmpbyte;
						}
					}
				}
				tmpbyte = 0;
			}
		}
	}

	png_write(png, tf.data, tf.len, 0);
	png_tform_free(&tf);
	return 1;
}

int png_rotate_arb(struct PNG *png, float angle)
{
	angle = angle*PI/180.0f;
	struct _png_tform tf;
	if (!png_get_tform(png, &tf))
		return 0;

	/*int newwidth = ceil(sqrt(tf.width*tf.width + tf.height*tf.height));*/
	/*int newheight = newwidth;*/
	/*int newlength = newwidth * newheight * tf.bpp;*/

	/*uint8_t *rot_data = calloc(1, newlength);*/
	uint8_t *rot_data = calloc(1, tf.len);
	float cosa = cos(angle);
	float sina = sin(angle);
	int xdif, ydif;

	/*printf("newwidth: %d, newheight: %d\n", newwidth, newheight);*/
	for (int j = 0; j < tf.height; j++) {
		ydif = tf.height/2 - j;
		for (int i = 0; i < tf.width; i++) {
			xdif = tf.width/2 - i;
			int xidx = tf.width/2 + (-xdif*cosa - ydif*sina);
			int yidx = tf.height/2 + (xdif*sina - ydif*cosa);
			if (xidx < tf.width && yidx < tf.height) {
				/*printf("x: %d, y: %d\n", xidx, yidx);*/
				int dstidx = j*tf.width*tf.bpp + i*tf.bpp;
				int srcidx = yidx*tf.width*tf.bpp + xidx*tf.bpp;

				memcpy(rot_data + dstidx, tf.data + srcidx, tf.bpp);
			}
		}
	}

	/*png->IHDR_chunk.width = newwidth;*/
	/*png->IHDR_chunk.height = newheight;*/

	png_tform_free(&tf);
	/*png_write(png, rot_data, newlength, 0);*/
	png_write(png, rot_data, tf.len, 0);
	free(rot_data);
	return 1;
}
