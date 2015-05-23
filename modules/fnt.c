// This file is part of Deark, by Jason Summers.
// This software is in the public domain. See the file COPYING for details.

// Windows FNT font format

#include <deark-config.h>
#include <deark-modules.h>
#include "fmtutil.h"

typedef struct localctx_struct {
	de_int64 fnt_version;
	de_int64 nominal_char_width;
	de_int64 char_height;
	de_int64 hdrsize;
	//de_int64 char_table_size;

	de_byte first_char;
	de_byte last_char;
	de_int64 num_chars_indexed;
	de_int64 num_chars_stored;

	de_int64 char_entry_size;
	de_int64 detected_max_width;

	de_int64 dfPoints;
	de_int64 dfFace; // Offset of font face name

	de_finfo *fi;
} lctx;

// Find the widest character.
static void do_prescan_chars(deark *c, lctx *d)
{
	de_int64 i;
	de_int64 pos;
	de_int64 char_width;

	for(i=0; i<d->num_chars_indexed; i++) {
		pos = d->hdrsize + d->char_entry_size*i;
		char_width = de_getui16le(pos);

		if(char_width > d->detected_max_width) {
			d->detected_max_width = char_width;
		}
	}
	de_dbg(c, "detected max width: %d\n", (int)d->detected_max_width);
}

// create bitmap_font object
static void do_make_image(deark *c, lctx *d)
{
	struct de_bitmap_font *font = NULL;
	de_int64 i;
	de_int64 pos;

	font = de_malloc(c, sizeof(struct de_bitmap_font));
	font->nominal_width = (int)d->nominal_char_width;
	font->nominal_height = (int)d->char_height;
	font->num_chars = d->num_chars_indexed;
	font->char_array = de_malloc(c, font->num_chars * sizeof(struct de_bitmap_font_char));

	for(i=0; i<d->num_chars_indexed; i++) {
		de_int64 char_width;
		de_int64 char_offset;
		de_int64 num_tiles;
		de_int64 tile;
		de_int64 row;

		pos = d->hdrsize + d->char_entry_size*i;
		char_width = de_getui16le(pos);
		if(d->char_entry_size==6)
			char_offset = de_getui32le(pos+2);
		else
			char_offset = de_getui16le(pos+2);
		de_dbg2(c, "char[%d] width=%d offset=%d\n", (int)(d->first_char + i), (int)char_width, (int)char_offset);

		num_tiles = (char_width+7)/8;

		font->char_array[i].codepoint = (de_int32)d->first_char + (de_int32)i;
		font->char_array[i].width = (int)char_width;
		font->char_array[i].height = (int)d->char_height;
		font->char_array[i].rowspan = num_tiles;
		font->char_array[i].bitmap = de_malloc(c, d->char_height * num_tiles);

		for(row=0; row<d->char_height; row++) {
			for(tile=0; tile<num_tiles; tile++) {
				font->char_array[i].bitmap[row * font->char_array[i].rowspan + tile] =
					de_getbyte(char_offset + tile*d->char_height + row);
			}
		}
	}

	de_fmtutil_bitmap_font_to_image(c, font, d->fi);

	if(font) {
		if(font->char_array) {
			for(i=0; i<font->num_chars; i++) {
				de_free(c, font->char_array[i].bitmap);
			}
			de_free(c, font->char_array);
		}
		de_free(c, font);
	}
}

static void read_face_name(deark *c, lctx *d)
{
	char buf[50];
	char buf2[50];

	if(d->dfFace<1) return;

	if(c->filenames_from_file) {
		// The facename is terminated with a NUL byte.
		// There seems to be no defined limit to its length, but Windows font face
		// names traditionally have to be quite short.
		dbuf_read_sz(c->infile, d->dfFace, buf, sizeof(buf));
		de_snprintf(buf2, sizeof(buf2), "%s-%d", buf, (int)d->dfPoints);
	}
	else {
		de_snprintf(buf2, sizeof(buf2), "%d", (int)d->dfPoints);
	}

	d->fi = de_finfo_create(c);
	de_finfo_set_name_from_sz(c, d->fi, buf2, DE_ENCODING_ASCII);
}

static int do_read_header(deark *c, lctx *d)
{
	de_int64 dfType;
	de_byte dfCharSet;
	de_int64 dfPixWidth;
	de_int64 dfPixHeight;
	de_int64 dfMaxWidth;
	int is_vector = 0;
	int retval = 0;

	d->fnt_version = de_getui16le(0);
	de_dbg(c, "dfVersion: 0x%04x\n", (int)d->fnt_version);

	if(d->fnt_version<0x0200) {
		de_err(c, "This version of FNT is not supported\n");
		goto done;
	}

	if(d->fnt_version==0x0300)
		d->hdrsize = 148;
	else
		d->hdrsize = 118;

	dfType = de_getui16le(66);
	de_dbg(c, "dfType: 0x%04x\n", (int)dfType);

	is_vector = (dfType&0x1)?1:0;
	de_dbg(c, "Font type: %s\n", is_vector?"vector":"bitmap");
	if(is_vector) {
		de_err(c, "This is a vector font. Not supported.\n");
		goto done;
	}

	d->dfPoints = de_getui16le(68);
	de_dbg(c, "dfPoints: %d\n", (int)d->dfPoints);

	dfPixWidth = de_getui16le(86);
	de_dbg(c, "dfPixWidth: %d\n", (int)dfPixWidth);
	dfPixHeight = de_getui16le(88);
	de_dbg(c, "dfPixHeight: %d\n", (int)dfPixHeight);

	dfCharSet = de_getbyte(85);
	de_dbg(c, "charset: 0x%02x\n", (int)dfCharSet);

	dfMaxWidth = de_getui16le(93);
	de_dbg(c, "dfMaxWidth: %d\n", (int)dfMaxWidth);

	if(dfPixWidth!=dfMaxWidth && dfPixWidth!=0) {
		de_warn(c, "dfMaxWidth (%d) does not equal dfPixWidth (%d)\n",
			(int)dfMaxWidth, (int)dfPixWidth);
	}

	d->first_char = de_getbyte(95);
	d->last_char = de_getbyte(96);
	de_dbg(c, "first char: %d, last char: %d\n", (int)d->first_char, (int)d->last_char);

	if(d->fnt_version >= 0x0200) {
		d->dfFace = de_getui32le(105);
	}

	d->num_chars_indexed = (de_int64)d->last_char - d->first_char + 1;
	// There is an extra character at the end of the table that is an
	// "absolute-space" character, and is guaranteed to be blank.
	d->num_chars_stored = d->num_chars_indexed + 1;

	if(d->fnt_version==0x0300) {
		d->char_entry_size = 6;
	}
	else {
		d->char_entry_size = 4;
	}

	//d->char_table_size = d->char_entry_size * d->num_chars_stored;

	do_prescan_chars(c, d);

	if(d->detected_max_width < dfMaxWidth) {
		// dfMaxWidth setting is larger than necessary.
		d->nominal_char_width = d->detected_max_width;
	}
	else {
		d->nominal_char_width = dfMaxWidth;
	}
	d->char_height = dfPixHeight;

	retval = 1;
done:
	return retval;
}

static void de_run_fnt(deark *c, const char *params)
{
	lctx *d = NULL;

	de_dbg(c, "In fnt module\n");
	d = de_malloc(c, sizeof(lctx));
	if(!do_read_header(c, d)) goto done;
	read_face_name(c, d);
	do_make_image(c, d);
done:
	de_finfo_destroy(c, d->fi);
	de_free(c, d);
}

static int de_identify_fnt(deark *c)
{
	de_int64 ver;

	// TODO: Better format detection.
	if(de_input_file_has_ext(c, "fnt")) {
		ver = de_getui16le(0);
		if(ver==0x0100 || ver==0x0200 || ver==0x0300)
			return 10;
	}
	return 0;
}

void de_module_fnt(deark *c, struct deark_module_info *mi)
{
	mi->id = "fnt";
	mi->run_fn = de_run_fnt;
	mi->identify_fn = de_identify_fnt;
}
