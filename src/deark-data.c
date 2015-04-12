// This file is part of Deark, by Jason Summers.
// This software is in the public domain. See the file COPYING for details.

// deark-data.c
//
// Data lookup and conversion.

#include "deark-config.h"

#include "deark-private.h"

static const char *g_hexchars = "0123456789abcdef";

char de_get_hexchar(int n)
{
	if(n>=0 && n<16) return g_hexchars[n];
	return '0';
}

de_byte de_decode_hex_digit(de_byte x, int *errorflag)
{
	if(errorflag) *errorflag = 0;
	if(x>='0' && x<='9') return x-48;
	if(x>='A' && x<='F') return x-55;
	if(x>='a' && x<='f') return x-87;
	if(errorflag) *errorflag = 1;
	return 0;
}

static const de_uint16 cp437table[256] = {
	0x00a0,0x263a,0x263b,0x2665,0x2666,0x2663,0x2660,0x2022,0x25d8,0x25cb,0x25d9,0x2642,0x2640,0x266a,0x266b,0x263c,
	0x25ba,0x25c4,0x2195,0x203c,0x00b6,0x00a7,0x25ac,0x21a8,0x2191,0x2193,0x2192,0x2190,0x221f,0x2194,0x25b2,0x25bc,
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x005c,0x005d,0x005e,0x005f,
	0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006a,0x006b,0x006c,0x006d,0x006e,0x006f,
	0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007a,0x007b,0x007c,0x007d,0x007e,0x2302,
	0x00c7,0x00fc,0x00e9,0x00e2,0x00e4,0x00e0,0x00e5,0x00e7,0x00ea,0x00eb,0x00e8,0x00ef,0x00ee,0x00ec,0x00c4,0x00c5,
	0x00c9,0x00e6,0x00c6,0x00f4,0x00f6,0x00f2,0x00fb,0x00f9,0x00ff,0x00d6,0x00dc,0x00a2,0x00a3,0x00a5,0x20a7,0x0192,
	0x00e1,0x00ed,0x00f3,0x00fa,0x00f1,0x00d1,0x00aa,0x00ba,0x00bf,0x2310,0x00ac,0x00bd,0x00bc,0x00a1,0x00ab,0x00bb,
	0x2591,0x2592,0x2593,0x2502,0x2524,0x2561,0x2562,0x2556,0x2555,0x2563,0x2551,0x2557,0x255d,0x255c,0x255b,0x2510,
	0x2514,0x2534,0x252c,0x251c,0x2500,0x253c,0x255e,0x255f,0x255a,0x2554,0x2569,0x2566,0x2560,0x2550,0x256c,0x2567,
	0x2568,0x2564,0x2565,0x2559,0x2558,0x2552,0x2553,0x256b,0x256a,0x2518,0x250c,0x2588,0x2584,0x258c,0x2590,0x2580,
	0x03b1,0x00df,0x0393,0x03c0,0x03a3,0x03c3,0x00b5,0x03c4,0x03a6,0x0398,0x03a9,0x03b4,0x221e,0x03c6,0x03b5,0x2229,
	0x2261,0x00b1,0x2265,0x2264,0x2320,0x2321,0x00f7,0x2248,0x00b0,0x2219,0x00b7,0x221a,0x207f,0x00b2,0x25a0,0x00a0
};

static const de_uint16 petscii1table[256] = {
	0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0x000d,0x000e,0xfffd,
	0xfffd,0xfffd,0xfffd,0xfffd,0x007f,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,
	0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002a,0x002b,0x002c,0x002d,0x002e,0x002f,
	0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003a,0x003b,0x003c,0x003d,0x003e,0x003f,
	0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004a,0x004b,0x004c,0x004d,0x004e,0x004f,
	0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005a,0x005b,0x00a3,0x005d,0x2191,0x2190,
	0x2500,0x2660,0x2502,0x2500,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0x256e,0x2570,0x256f,0xfffd,0x2572,0x2571,0xfffd,
	0xfffd,0x25cf,0xfffd,0x2665,0xfffd,0x256d,0x2573,0x25cb,0x2663,0xfffd,0x2666,0x253c,0xfffd,0x2502,0x03c0,0x25e5,
	0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0x000a,0x000f,0xfffd,
	0xfffd,0xfffd,0xfffd,0x000c,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0x0008,0xfffd,0xfffd,
	0x00a0,0x258c,0x2584,0x2594,0x2581,0x258e,0x2592,0xfffd,0xfffd,0x25e4,0xfffd,0x251c,0x2597,0x2514,0x2510,0x2582,
	0x250c,0x2534,0x252c,0x2524,0x258e,0x258d,0xfffd,0xfffd,0xfffd,0x2583,0xfffd,0x2596,0x259d,0x2518,0x2598,0x259a,
	0x2500,0x2660,0x2502,0x2500,0xfffd,0xfffd,0xfffd,0xfffd,0xfffd,0x256e,0x2570,0x256f,0xfffd,0x2572,0x2571,0xfffd,
	0xfffd,0x25cf,0xfffd,0x2665,0xfffd,0x256d,0x2573,0x25cb,0x2663,0xfffd,0x2666,0x253c,0xfffd,0x2502,0x03c0,0x25e5,
	0x00a0,0x258c,0x2584,0x2594,0x2581,0x258e,0x2592,0xfffd,0xfffd,0x25e4,0xfffd,0x251c,0x2597,0x2514,0x2510,0x2582,
	0x250c,0x2534,0x252c,0x2524,0x258e,0x258d,0xfffd,0xfffd,0xfffd,0x2583,0xfffd,0x2596,0x259d,0x2518,0x2598,0x03c0
};

// Code page 437, with screen code graphics characters.
de_int32 de_cp437g_to_unicode(deark *c, int a)
{
	if(a>=0 && a<=0xff) return (de_int32)cp437table[a];
	return 0xfffd;
}

// Code page 437, with control characters.
de_int32 de_cp437c_to_unicode(deark *c, int a)
{
	if(a>=0 && a<=0x7f) return a;
	if(a>=0x80 && a<=0xff) return (de_int32)cp437table[a];
	return 0xfffd;
}

// Encode a Unicode char in UTF-8.
// Caller supplies utf8buf[4].
// Sets *p_utf8len to the number of bytes used (1-4).
void de_uchar_to_utf8(de_int32 u1, de_byte *utf8buf, de_int64 *p_utf8len)
{
	de_uint32 u = (de_uint32)u1;

	if(u>0x10ffff) u=0xfffd;

	if(u<=0x7f) {
		*p_utf8len = 1;
		utf8buf[0] = (de_byte)u;
	}
	else if(u>=0x80 && u<=0x7ff) {
		*p_utf8len = 2;
		utf8buf[0] = 0xc0 | (u>>6);
		utf8buf[1] = 0x80 | (u&0x3f);
	}
	else if(u>=0x800 && u<=0xffff) {
		*p_utf8len = 3;
		utf8buf[0] = 0xe0 | (u>>12);
		utf8buf[1] = 0x80 | ((u>>6)&0x3f);
		utf8buf[2] = 0x80 | (u&0x3f);
	}
	else {
		*p_utf8len = 4;
		utf8buf[0] = 0xf0 | (u>>18);
		utf8buf[1] = 0x80 | ((u>>12)&0x3f);
		utf8buf[2] = 0x80 | ((u>>6)&0x3f);
		utf8buf[3] = 0x80 | (u&0x3f);
	}
}

// Write a unicode code point to a file, encoded as UTF-8.
void dbuf_write_uchar_as_utf8(dbuf *outf, de_int32 u)
{
	de_byte utf8buf[4];
	de_int64 utf8len;

	de_uchar_to_utf8(u, utf8buf, &utf8len);
	dbuf_write(outf, utf8buf, utf8len);
}

// Given a buffer, return 1 if it has no bytes 0x80 or higher.
int de_is_ascii(const de_byte *buf, de_int64 buflen)
{
	de_int64 i;

	for(i=0; i<buflen; i++) {
		if(buf[i]>=128) return 0;
	}
	return 1;
}

static const de_uint32 vga256pal[256] = {
	0x000000,0x0000aa,0x00aa00,0x00aaaa,0xaa0000,0xaa00aa,0xaa5500,0xaaaaaa,
	0x555555,0x5555ff,0x55ff55,0x55ffff,0xff5555,0xff55ff,0xffff55,0xffffff,
	0x000000,0x141414,0x202020,0x2d2d2d,0x393939,0x454545,0x515151,0x616161,
	0x717171,0x828282,0x929292,0xa2a2a2,0xb6b6b6,0xcacaca,0xe3e3e3,0xffffff,
	0x0000ff,0x4100ff,0x7d00ff,0xbe00ff,0xff00ff,0xff00be,0xff007d,0xff0041,
	0xff0000,0xff4100,0xff7d00,0xffbe00,0xffff00,0xbeff00,0x7dff00,0x41ff00,
	0x00ff00,0x00ff41,0x00ff7d,0x00ffbe,0x00ffff,0x00beff,0x007dff,0x0041ff,
	0x7d7dff,0x9e7dff,0xbe7dff,0xdf7dff,0xff7dff,0xff7ddf,0xff7dbe,0xff7d9e,
	0xff7d7d,0xff9e7d,0xffbe7d,0xffdf7d,0xffff7d,0xdfff7d,0xbeff7d,0x9eff7d,
	0x7dff7d,0x7dff9e,0x7dffbe,0x7dffdf,0x7dffff,0x7ddfff,0x7dbeff,0x7d9eff,
	0xb6b6ff,0xc6b6ff,0xdbb6ff,0xebb6ff,0xffb6ff,0xffb6eb,0xffb6db,0xffb6c6,
	0xffb6b6,0xffc6b6,0xffdbb6,0xffebb6,0xffffb6,0xebffb6,0xdbffb6,0xc6ffb6,
	0xb6ffb6,0xb6ffc6,0xb6ffdb,0xb6ffeb,0xb6ffff,0xb6ebff,0xb6dbff,0xb6c6ff,
	0x000071,0x1c0071,0x390071,0x550071,0x710071,0x710055,0x710039,0x71001c,
	0x710000,0x711c00,0x713900,0x715500,0x717100,0x557100,0x397100,0x1c7100,
	0x007100,0x00711c,0x007139,0x007155,0x007171,0x005571,0x003971,0x001c71,
	0x393971,0x453971,0x553971,0x613971,0x713971,0x713961,0x713955,0x713945,
	0x713939,0x714539,0x715539,0x716139,0x717139,0x617139,0x557139,0x457139,
	0x397139,0x397145,0x397155,0x397161,0x397171,0x396171,0x395571,0x394571,
	0x515171,0x595171,0x615171,0x695171,0x715171,0x715169,0x715161,0x715159,
	0x715151,0x715951,0x716151,0x716951,0x717151,0x697151,0x617151,0x597151,
	0x517151,0x517159,0x517161,0x517169,0x517171,0x516971,0x516171,0x515971,
	0x000041,0x100041,0x200041,0x310041,0x410041,0x410031,0x410020,0x410010,
	0x410000,0x411000,0x412000,0x413100,0x414100,0x314100,0x204100,0x104100,
	0x004100,0x004110,0x004120,0x004131,0x004141,0x003141,0x002041,0x001041,
	0x202041,0x282041,0x312041,0x392041,0x412041,0x412039,0x412031,0x412028,
	0x412020,0x412820,0x413120,0x413920,0x414120,0x394120,0x314120,0x284120,
	0x204120,0x204128,0x204131,0x204139,0x204141,0x203941,0x203141,0x202841,
	0x2d2d41,0x312d41,0x352d41,0x3d2d41,0x412d41,0x412d3d,0x412d35,0x412d31,
	0x412d2d,0x41312d,0x41352d,0x413d2d,0x41412d,0x3d412d,0x35412d,0x31412d,
	0x2d412d,0x2d4131,0x2d4135,0x2d413d,0x2d4141,0x2d3d41,0x2d3541,0x2d3141,
	0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000
};

static const de_uint32 ega64pal[64] = {
	0x000000,0x0000aa,0x00aa00,0x00aaaa,0xaa0000,0xaa00aa,0xaaaa00,0xaaaaaa,
	0x000055,0x0000ff,0x00aa55,0x00aaff,0xaa0055,0xaa00ff,0xaaaa55,0xaaaaff,
	0x005500,0x0055aa,0x00ff00,0x00ffaa,0xaa5500,0xaa55aa,0xaaff00,0xaaffaa,
	0x005555,0x0055ff,0x00ff55,0x00ffff,0xaa5555,0xaa55ff,0xaaff55,0xaaffff,
	0x550000,0x5500aa,0x55aa00,0x55aaaa,0xff0000,0xff00aa,0xffaa00,0xffaaaa,
	0x550055,0x5500ff,0x55aa55,0x55aaff,0xff0055,0xff00ff,0xffaa55,0xffaaff,
	0x555500,0x5555aa,0x55ff00,0x55ffaa,0xff5500,0xff55aa,0xffff00,0xffffaa,
	0x555555,0x5555ff,0x55ff55,0x55ffff,0xff5555,0xff55ff,0xffff55,0xffffff
};

static const de_uint32 pc16pal[16] = {
	0x000000,0x0000aa,0x00aa00,0x00aaaa,0xaa0000,0xaa00aa,0xaa5500,0xaaaaaa,
	0x555555,0x5555ff,0x55ff55,0x55ffff,0xff5555,0xff55ff,0xffff55,0xffffff
};


de_uint32 de_palette_vga256(int index)
{
	if(index>=0 && index<256) {
		return vga256pal[index];
	}
	return 0;
}

de_uint32 de_palette_ega64(int index)
{

	if(index>=0 && index<64) {
		return ega64pal[index];
	}
	return 0;
}

de_uint32 de_palette_pc16(int index)
{
	if(index>=0 && index<16) {
		return pc16pal[index];
	}
	return 0;
}

static const de_uint32 pcpaint_cga_pals[6][4] = {
	{ 0x000000, 0x00aaaa, 0xaa00aa, 0xaaaaaa }, // palette 1 low
	{ 0x000000, 0x00aa00, 0xaa0000, 0xaa5500 }, // palette 0 low
	{ 0x000000, 0x00aaaa, 0xaa0000, 0xaaaaaa }, // 3rd palette low
	{ 0x000000, 0x55ffff, 0xff55ff, 0xffffff }, // palette 1 high
	{ 0x000000, 0x55ff55, 0xff5555, 0xffff55 }, // palette 0 high
	{ 0x000000, 0x55ffff, 0xff5555, 0xffffff }  // 3rd palette high
};

de_uint32 de_palette_pcpaint_cga4(int palnum, int index)
{
	if(palnum<0 || palnum>5) palnum=2;
	if(index>=0 && index<4) {
		return pcpaint_cga_pals[palnum][index];
	}
	return 0;
}

void de_color_to_css(de_uint32 color, char *buf, int buflen)
{
	de_byte r, g, b;

	buf[0] = '#';
	r = DE_COLOR_R(color);
	g = DE_COLOR_G(color);
	b = DE_COLOR_B(color);

	if(r%17==0 && g%17==0 && b%17==0) {
		// Can use short form.
		buf[1] = g_hexchars[r/17];
		buf[2] = g_hexchars[g/17];
		buf[3] = g_hexchars[b/17];
		buf[4] = '\0';
		return;
	}

	buf[1] = g_hexchars[r/16];
	buf[2] = g_hexchars[r%16];
	buf[3] = g_hexchars[g/16];
	buf[4] = g_hexchars[g%16];
	buf[5] = g_hexchars[b/16];
	buf[6] = g_hexchars[b%16];
	buf[7] = '\0';
}

de_byte de_palette_sample_6_to_8bit(de_byte samp)
{
	if(samp>=63) return 255;
	return (de_byte)(0.5+((((double)samp)/63.0)*255.0));
}

de_uint32 de_rgb565_to_888(de_uint32 n)
{
	de_byte cr, cg, cb;
	cr = (de_byte)(n>>11);
	cg = (de_byte)((n>>5)&0x3f);
	cb = (de_byte)(n&0x1f);
	cr = (de_byte)(0.5+((double)cr)*(255.0/31.0));
	cg = (de_byte)(0.5+((double)cg)*(255.0/63.0));
	cb = (de_byte)(0.5+((double)cb)*(255.0/31.0));
	return DE_MAKE_RGB(cr, cg, cb);
}

de_uint32 de_bgr555_to_888(de_uint32 n)
{
	de_byte cr, cg, cb;
	cb = (de_byte)((n>>10)&0x1f);
	cg = (de_byte)((n>>5)&0x1f);
	cr = (de_byte)(n&0x1f);
	cb = (de_byte)(0.5+((double)cb)*(255.0/31.0));
	cg = (de_byte)(0.5+((double)cg)*(255.0/31.0));
	cr = (de_byte)(0.5+((double)cr)*(255.0/31.0));
	return DE_MAKE_RGB(cr, cg, cb);
}

// s1 is not NUL terminated, but s2 will be.
// s2_size includes the NUL terminator.
void de_make_printable_ascii(de_byte *s1, de_int64 s1_len,
	char *s2, de_int64 s2_size, unsigned int conv_flags)
{
	de_int64 i;
	de_int64 s2_pos = 0;
	char ch;

	for(i=0; i<s1_len; i++) {
		if(s1[i]=='\0' && (conv_flags & DE_CONVFLAG_STOP_AT_NUL)) {
			break;
		}

		if(s1[i]>=32 && s1[i]<=126) {
			ch = (char)s1[i];
		}
		else {
			ch = '_';
		}

		if(s2_pos < s2_size-1) {
			s2[s2_pos++] = ch;
		}
	}

	s2[s2_pos] = '\0';
}

de_int32 de_char_to_valid_fn_char(deark *c, de_int32 ch)
{
	if(ch>=32 && ch<=126 && ch!='/' && ch!='\\' && ch!=':'
		&& ch!='*' && ch!='?' && ch!='\"' && ch!='<' &&
		ch!='>' && ch!='|')
	{
		// These are the valid ASCII characters in Windows filenames.
		// TODO: We could behave differently on different platforms.
		return ch;
	}
	else if(ch>=160 && ch<=0x10ffff) {
		// For now, we don't support Unicode filenames in ZIP files.
		if(c->output_style==DE_OUTPUTSTYLE_DIRECT) {
			// TODO: A lot of Unicode characters probably don't belong in filenames.
			// Maybe we need a whitelist or blacklist.
			return ch;
		}
	}
	return '_';
}

// de_ucstring is a Unicode (utf-32) string object.
de_ucstring *ucstring_create(deark *c)
{
	de_ucstring *s;
	s = de_malloc(c, sizeof(de_ucstring));
	s->c = c;
	return s;
}

void ucstring_destroy(de_ucstring *s)
{
	deark *c;
	if(s) {
		c = s->c;
		de_free(c, s->str);
		de_free(c, s);
	}
}

void ucstring_append_char(de_ucstring *s, de_int32 ch)
{
	de_int64 new_len;
	de_int64 new_alloc;

	if(s->len >= 100000000) {
		return;
	}
	new_len = s->len + 1;
	if(new_len > s->alloc) {
		new_alloc = s->alloc * 2;
		if(new_alloc<32) new_alloc=32;

		s->str = de_realloc(s->c, s->str, s->alloc * sizeof(de_int32), new_alloc * sizeof(de_int32));
		s->alloc = new_alloc;
	}

	s->str[s->len] = ch;
	s->len++;
	return;
}

de_int32 de_petscii_char_to_utf32(de_byte ch)
{
	de_int32 uchar;
	uchar = (de_int32)petscii1table[(int)ch];
	return uchar;
}

void de_write_codepoint_to_html(deark *c, dbuf *f, de_int32 ch)
{
	int e; // How to encode this codepoint

	if(ch=='&' || ch=='<' || ch=='>') {
		e = 1; // HTML entity
	}
	else if(ch>=32 && ch<=126) {
		e = 2; // raw byte
	}
	else if(c->ascii_html) {
		e = 1; // HTML entity
	}
	else {
		e = 3; // UTF-8
	}

	if(e==2) {
		dbuf_writebyte(f, (de_byte)ch);
	}
	else if(e==3) {
		dbuf_write_uchar_as_utf8(f, ch);
	}
	else {
		dbuf_fprintf(f, "&#%d;", (int)ch);
	}
}
