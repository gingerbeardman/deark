// This file is part of Deark.
// Copyright (C) 2016 Jason Summers
// See the file COPYING for terms of use.

// deark-bitmap.c
//
// Functions related to bitmaps and struct deark_bitmap.

#define DE_NOT_IN_MODULE
#include "deark-config.h"
#include "deark-private.h"

int de_good_image_dimensions_noerr(deark *c, de_int64 w, de_int64 h)
{
	if(w<1 || h<1 || w>c->max_image_dimension || h>c->max_image_dimension) {
		return 0;
	}
	return 1;
}

int de_good_image_dimensions(deark *c, de_int64 w, de_int64 h)
{
	if(!de_good_image_dimensions_noerr(c, w, h)) {
		de_err(c, "Bad or unsupported image dimensions (%dx%d)\n",
			(int)w, (int)h);
		return 0;
	}
	return 1;
}

#define DE_MAX_IMAGES_PER_FILE 10000

// This is meant as a sanity check for fields that indicate how many images
// are in a file.
// TODO: It is not used very consistently, and should probably be re-thought
// or removed.
int de_good_image_count(deark *c, de_int64 n)
{
	de_int64 maximages;

	maximages = DE_MAX_IMAGES_PER_FILE;
	if(c->max_output_files>DE_MAX_IMAGES_PER_FILE) {
		maximages = c->max_output_files;
	}

	if(n<0 || n>maximages) {
		de_err(c, "Bad or unsupported number of images (%d)\n", (int)n);
		return 0;
	}
	return 1;
}

int de_is_grayscale_palette(const de_uint32 *pal, de_int64 num_entries)
{
	de_int64 k;
	de_byte cr;

	for(k=0; k<num_entries; k++) {
		cr = DE_COLOR_R(pal[k]);
		if(cr != DE_COLOR_G(pal[k])) return 0;
		if(cr != DE_COLOR_B(pal[k])) return 0;
	}
	return 1;
}

static void de_bitmap_alloc_pixels(struct deark_bitmap *img)
{
	if(img->bitmap) {
		de_free(img->c, img->bitmap);
	}

	if(!de_good_image_dimensions(img->c, img->width, img->height)) {
		// This function is not allowed to fail. If something goes wrong, create
		// a dummy image, and set invalid_image_flag.
		img->invalid_image_flag = 1;

		img->width = 1;
		img->height = 1;
	}

	img->bitmap_size = (img->width*img->bytes_per_pixel) * img->height;
	img->bitmap = de_malloc(img->c, img->bitmap_size);
}

void de_bitmap_write_to_file(struct deark_bitmap *img, const char *token,
	unsigned int createflags)
{
	dbuf *f;
	char buf[80];

	if(!img) return;
	if(img->invalid_image_flag) return;

	if(token==NULL || token[0]=='\0') {
		de_strlcpy(buf, "png", sizeof(buf));
	}
	else {
		de_snprintf(buf, sizeof(buf), "%s.png", token);
	}

	if(!img->bitmap) de_bitmap_alloc_pixels(img);

	f = dbuf_create_output_file(img->c, buf, NULL, createflags);
	de_write_png(img->c, img, f);
	dbuf_close(f);
}

void de_bitmap_write_to_file_finfo(struct deark_bitmap *img, de_finfo *fi,
	unsigned int createflags)
{
	const char *token = NULL;
	if(fi && fi->file_name) {
		token = fi->file_name;
	}
	de_bitmap_write_to_file(img, token, createflags);
}

// samplenum 0=Red, 1=Green, 2=Blue, 3=Alpha
void de_bitmap_setsample(struct deark_bitmap *img, de_int64 x, de_int64 y,
	de_int64 samplenum, de_byte v)
{
	de_int64 pos;

	if(!img->bitmap) de_bitmap_alloc_pixels(img);
	if(x<0 || y<0 || x>=img->width || y>=img->height) return;
	if(samplenum<0 || samplenum>3) return;
	pos = (img->width*img->bytes_per_pixel)*y + img->bytes_per_pixel*x;

	switch(img->bytes_per_pixel) {
	case 1: // gray
		if(samplenum<3) {
			img->bitmap[pos] = v;
		}
		break;
	case 2: // gray+alpha
		if(samplenum==3) {
			img->bitmap[pos+1] = v;
		}
		else {
			img->bitmap[pos] = v;
		}
		break;
	case 3: // RGB
		if(samplenum<3) {
			img->bitmap[pos+samplenum] = v;
		}
		break;
	case 4: // RGBA
		img->bitmap[pos+samplenum] = v;
		break;
	}
}

void de_bitmap_setpixel_gray(struct deark_bitmap *img, de_int64 x, de_int64 y, de_byte v)
{
	de_int64 pos;

	if(!img->bitmap) de_bitmap_alloc_pixels(img);
	if(x<0 || y<0 || x>=img->width || y>=img->height) return;
	pos = (img->width*img->bytes_per_pixel)*y + img->bytes_per_pixel*x;

	img->bitmap[pos] = v;
	switch(img->bytes_per_pixel) {
	case 2: // gray+alpha
		img->bitmap[pos+1] = 255;
		break;
	case 3: // RGB
		img->bitmap[pos+1] = v;
		img->bitmap[pos+2] = v;
		break;
	case 4: // RGBA
		img->bitmap[pos+1] = v;
		img->bitmap[pos+2] = v;
		img->bitmap[pos+3] = 255;
		break;
	}
}

// TODO: Decide if this should just be an alias of setpixel_rgba, or if it will
// force colors to be opaque.
void de_bitmap_setpixel_rgb(struct deark_bitmap *img, de_int64 x, de_int64 y,
	de_uint32 color)
{
	de_bitmap_setpixel_rgba(img, x, y, color);
}

void de_bitmap_setpixel_rgba(struct deark_bitmap *img, de_int64 x, de_int64 y,
	de_uint32 color)
{
	de_int64 pos;

	if(!img->bitmap) de_bitmap_alloc_pixels(img);
	if(x<0 || y<0 || x>=img->width || y>=img->height) return;
	pos = (img->width*img->bytes_per_pixel)*y + img->bytes_per_pixel*x;

	switch(img->bytes_per_pixel) {
	case 4:
		img->bitmap[pos]   = DE_COLOR_R(color);
		img->bitmap[pos+1] = DE_COLOR_G(color);
		img->bitmap[pos+2] = DE_COLOR_B(color);
		img->bitmap[pos+3] = DE_COLOR_A(color);
		break;
	case 3:
		img->bitmap[pos]   = DE_COLOR_R(color);
		img->bitmap[pos+1] = DE_COLOR_G(color);
		img->bitmap[pos+2] = DE_COLOR_B(color);
		break;
	case 2:
		img->bitmap[pos]   = DE_COLOR_G(color);
		img->bitmap[pos+1] = DE_COLOR_A(color);
		break;
	case 1:
		// TODO: We could do real grayscale conversion, but for now we
		// assume this won't happen, or that if it does the color given to
		// us is a gray shade.
		img->bitmap[pos]   = DE_COLOR_G(color);
		break;
	}
}

de_uint32 de_bitmap_getpixel(struct deark_bitmap *img, de_int64 x, de_int64 y)
{
	de_int64 pos;

	if(!img) return 0;
	if(!img->bitmap) return 0;
	if(x<0 || y<0 || x>=img->width || y>=img->height) return 0;
	pos = (img->width*img->bytes_per_pixel)*y + img->bytes_per_pixel*x;

	switch(img->bytes_per_pixel) {
	case 4:
		return DE_MAKE_RGBA(img->bitmap[pos], img->bitmap[pos+1],
			img->bitmap[pos+2], img->bitmap[pos+3]);
	case 3:
		return DE_MAKE_RGBA(img->bitmap[pos], img->bitmap[pos+1],
			img->bitmap[pos+2], 0xff);
		break;
	case 2:
		return DE_MAKE_RGBA(img->bitmap[pos], img->bitmap[pos],
			img->bitmap[pos], img->bitmap[pos+1]);
		break;
	case 1:
		return DE_MAKE_RGBA(img->bitmap[pos], img->bitmap[pos],
			img->bitmap[pos], 0xff);
		break;
	}
	return 0;
}

struct deark_bitmap *de_bitmap_create_noinit(deark *c)
{
	struct deark_bitmap *img;
	img = de_malloc(c, sizeof(struct deark_bitmap));
	img->c = c;
	return img;
}

struct deark_bitmap *de_bitmap_create(deark *c, de_int64 width, de_int64 height, int bypp)
{
	struct deark_bitmap *img;
	img = de_bitmap_create_noinit(c);
	img->width = width;
	img->height = height;
	img->bytes_per_pixel = bypp;
	//img->rowspan = img->width * img->bytes_per_pixel;
	return img;
}

void de_bitmap_destroy(struct deark_bitmap *b)
{
	if(b) {
		deark *c = b->c;
		if(b->bitmap) de_free(c, b->bitmap);
		de_free(c, b);
	}
}

de_byte de_get_bits_symbol(dbuf *f, de_int64 bps, de_int64 rowstart, de_int64 index)
{
	de_int64 byte_offset;
	de_byte b;
	de_byte x = 0;

	switch(bps) {
	case 1:
		byte_offset = rowstart + index/8;
		b = dbuf_getbyte(f, byte_offset);
		x = (b >> (7 - index%8)) & 0x01;
		break;
	case 2:
		byte_offset = rowstart + index/4;
		b = dbuf_getbyte(f, byte_offset);
		x = (b >> (2 * (3 - index%4))) & 0x03;
		break;
	case 4:
		byte_offset = rowstart + index/2;
		b = dbuf_getbyte(f, byte_offset);
		x = (b >> (4 * (1 - index%2))) & 0x0f;
		break;
	case 8:
		byte_offset = rowstart + index;
		x = dbuf_getbyte(f, byte_offset);
	}
	return x;
}

// Like de_get_bits_symbol, but with LSB-first bit order
de_byte de_get_bits_symbol_lsb(dbuf *f, de_int64 bps, de_int64 rowstart, de_int64 index)
{
	de_int64 byte_offset;
	de_byte b;
	de_byte x = 0;

	switch(bps) {
	case 1:
		byte_offset = rowstart + index/8;
		b = dbuf_getbyte(f, byte_offset);
		x = (b >> (index%8)) & 0x01;
		break;
	case 2:
		byte_offset = rowstart + index/4;
		b = dbuf_getbyte(f, byte_offset);
		x = (b >> (2 * (index%4))) & 0x03;
		break;
	case 4:
		byte_offset = rowstart + index/2;
		b = dbuf_getbyte(f, byte_offset);
		x = (b >> (4 * (index%2))) & 0x0f;
		break;
	case 8:
		byte_offset = rowstart + index;
		x = dbuf_getbyte(f, byte_offset);
	}
	return x;
}

// Read a symbol (up to 8 bits) that starts at an arbitrary bit position.
// It may span (two) bytes.
de_byte de_get_bits_symbol2(dbuf *f, int nbits, de_int64 bytepos, de_int64 bitpos)
{
	de_byte b0, b1;
	int bits_in_first_byte;
	int bits_in_second_byte;

	bits_in_first_byte = 8-(bitpos%8);

	b0 = dbuf_getbyte(f, bytepos + bitpos/8);

	if(bits_in_first_byte<8) {
		b0 &= (0xff >> (8-bits_in_first_byte)); // Zero out insignificant bits
	}

	if(bits_in_first_byte == nbits) {
		// First byte has all the bits
		return b0;
	}
	else if(bits_in_first_byte >= nbits) {
		// First byte has all the bits
		return b0 >> (bits_in_first_byte - nbits);
	}

	bits_in_second_byte = nbits - bits_in_first_byte;
	b1 = dbuf_getbyte(f, bytepos + bitpos/8 +1);

	return (b0<<bits_in_second_byte) | (b1>>(8-bits_in_second_byte));
}

void de_convert_row_bilevel(dbuf *f, de_int64 fpos, struct deark_bitmap *img,
	de_int64 rownum, unsigned int flags)
{
	de_int64 i;
	de_byte x;
	de_byte b;
	de_byte black, white;

	if(flags & DE_CVTF_WHITEISZERO) {
		white = 0; black = 255;
	}
	else {
		black = 0; white = 255;
	}

	for(i=0; i<img->width; i++) {
		b = dbuf_getbyte(f, fpos + i/8);
		if(flags & DE_CVTF_LSBFIRST)
			x = (b >> (i%8)) & 0x01;
		else
			x = (b >> (7 - i%8)) & 0x01;
		de_bitmap_setpixel_gray(img, i, rownum, x ? white : black);
	}
}

void de_convert_image_bilevel(dbuf *f, de_int64 fpos, de_int64 rowspan,
	struct deark_bitmap *img, unsigned int flags)
{
	de_int64 j;

	for(j=0; j<img->height; j++) {
		de_convert_row_bilevel(f, fpos+j*rowspan, img, j, flags);
	}
}

void de_convert_and_write_image_bilevel(dbuf *f, de_int64 fpos,
	de_int64 width, de_int64 height, de_int64 rowspan, unsigned int cvtflags,
	de_finfo *fi, unsigned int createflags)
{
	struct deark_bitmap *img = NULL;
	deark *c = f->c;

	if(!de_good_image_dimensions(c, width, height)) return;

	img = de_bitmap_create(c, width, height, 1);
	de_convert_image_bilevel(f, fpos, rowspan, img, cvtflags);
	de_bitmap_write_to_file_finfo(img, fi, createflags);
	de_bitmap_destroy(img);
}

// Read a palette of 24-bit RGB colors.
// flags = flags used by dbuf_getRGB()
void de_read_palette_rgb(dbuf *f,
	de_int64 fpos, de_int64 num_entries, de_int64 entryspan,
	de_uint32 *pal, de_int64 ncolors_in_pal,
	unsigned int flags)
{
	de_int64 k;

	if(num_entries > ncolors_in_pal) num_entries = ncolors_in_pal;
	for(k=0; k<num_entries; k++) {
		pal[k] = dbuf_getRGB(f, fpos + k*entryspan, flags);
		de_dbg_pal_entry(f->c, k, pal[k]);
	}
}

void de_convert_image_paletted(dbuf *f, de_int64 fpos,
	de_int64 bpp, de_int64 rowspan, const de_uint32 *pal,
	struct deark_bitmap *img, unsigned int flags)
{
	de_int64 i, j;
	unsigned int palent;

	if(bpp!=1 && bpp!=2 && bpp!=4 && bpp!=8) return;
	if(!de_good_image_dimensions_noerr(f->c, img->width, img->height)) return;

	for(j=0; j<img->height; j++) {
		for(i=0; i<img->width; i++) {
			palent = (unsigned int)de_get_bits_symbol(f, bpp, fpos+j*rowspan, i);
			de_bitmap_setpixel_rgba(img, i, j, pal[palent]);
		}
	}
}

// Paint a solid, solid-color rectangle onto an image.
// (Pixels will be replaced, not merged.)
void de_bitmap_rect(struct deark_bitmap *img,
	de_int64 xpos, de_int64 ypos, de_int64 width, de_int64 height,
	de_uint32 clr, unsigned int flags)
{
	de_int64 i, j;

	for(j=0; j<height; j++) {
		for(i=0; i<width; i++) {
			de_bitmap_setpixel_rgba(img, xpos+i, ypos+j, clr);
		}
	}
}

// Paint or copy (all or part of) srcimg onto dstimg.
// If srcimg and dstimg are the same image, the source and destination
// rectangles must not overlap.
// Flags supported:
//   DE_BITMAPFLAG_MERGE - Merge transparent pixels (partially supported)
void de_bitmap_copy_rect(struct deark_bitmap *srcimg, struct deark_bitmap *dstimg,
	de_int64 srcxpos, de_int64 srcypos, de_int64 width, de_int64 height,
	de_int64 dstxpos, de_int64 dstypos, unsigned int flags)
{
	de_int64 i, j;
	de_uint32 dst_clr, src_clr, clr;
	de_byte src_a;

	for(j=0; j<height; j++) {
		for(i=0; i<width; i++) {
			src_clr = de_bitmap_getpixel(srcimg, srcxpos+i, srcypos+j);
			if(!(flags&DE_BITMAPFLAG_MERGE)) {
				clr = src_clr;
			}
			else {
				src_a = DE_COLOR_A(src_clr);
				if(src_a>0) {
					// TODO: Support partial transparency (of both foreground and
					// background, ideally)
					clr = src_clr;
				}
				else {
					dst_clr = de_bitmap_getpixel(dstimg, dstxpos+i, dstypos+j);
					clr = dst_clr;
				}
			}
			de_bitmap_setpixel_rgba(dstimg, dstxpos+i, dstypos+j, clr);
		}
	}
}

void de_bitmap_apply_mask(struct deark_bitmap *fg, struct deark_bitmap *mask,
	unsigned int flags)
{
	de_int64 i, j;
	de_uint32 clr;
	de_byte a;

	for(j=0; j<fg->height && j<mask->height; j++) {
		for(i=0; i<fg->width && i<mask->width; i++) {
			clr = de_bitmap_getpixel(mask, i, j);
			a = DE_COLOR_K(clr);
			if(flags&DE_BITMAPFLAG_WHITEISTRNS)
				a = 0xff-a;
			de_bitmap_setsample(fg, i, j, 3, a);
		}
	}
}

// If the image is 100% opaque, remove the alpha channel.
// Otherwise do nothing.
// flags:
//  0x1: Make 100% invisible images 100% opaque
//  0x2: Warn if an invisible image was made opaque
void de_optimize_image_alpha(struct deark_bitmap *img, unsigned int flags)
{
	int has_transparency = 0;
	int has_visible_pixels = 0;
	de_int64 i, j;
	de_int64 k;
	de_byte a;

	if(img->bytes_per_pixel!=2 && img->bytes_per_pixel!=4) return;

	for(j=0; j<img->height; j++) {
		for(i=0; i<img->width; i++) {
			a = DE_COLOR_A(de_bitmap_getpixel(img, i, j));
			if(a<255) {
				has_transparency = 1;
			}
			if(a>0) {
				has_visible_pixels = 1;
			}
			if(has_transparency && has_visible_pixels) break;
		}
	}

	if(has_transparency && !has_visible_pixels && (flags&0x1)) {
		if(flags&0x2) {
			de_warn(img->c, "Invisible image detected. Ignoring transparency.\n");
		}
	}
	else if(has_transparency) {
		return;
	}

	// No meaningful transparency found.
	de_dbg3(img->c, "Removing alpha channel from image\n");

	// Note that the format conversion is done in-place. The extra memory used
	// by the alpha channel is not de-allocated.
	for(j=0; j<img->height; j++) {
		for(i=0; i<img->width; i++) {
			for(k=0; k<img->bytes_per_pixel-1; k++) {
				img->bitmap[(j*img->width+i)*(img->bytes_per_pixel-1) + k] =
					img->bitmap[(j*img->width+i)*(img->bytes_per_pixel) + k];
			}
		}
	}

	img->bytes_per_pixel--;
}

// flag 0x1: white-is-min
void de_make_grayscale_palette(de_uint32 *pal, de_int64 num_entries, unsigned int flags)
{
	de_int64 k;
	de_byte b;

	for(k=0; k<num_entries; k++) {
		b = (de_byte)(0.5+ (double)k * (255.0 / (double)(num_entries-1)));
		if(flags&0x1) b = 255-b;
		pal[k] = DE_MAKE_GRAY(b);
	}
}
