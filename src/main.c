//
// Velella
// main.c
//
// Copyright 2024 Michael J Moffitt
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "inih/ini.h"
#include "lodepng.h"
#include <ctype.h>
#include "pal.h"
#include "tileread.h"

// =======
// 
// =======
//
// An INI file is (ab)used to act as a script containing conversion information.
//
// A [section] sets the name of a sprite to be worked on. That is, the section
// is adopted as the symbolic name, which will appear in headers and meta
// output files.
// A certain number of required arguments must be set at least once. This includes
// the sprite type, the output CHR filename, output META filename (optional, but
// strongly recommended), output header filename (ditto), sprite size within
// sheet, and most importantly the source filename.
//
// When the line "src" is read, it kicks off the conversion
// using whatever parameters are set. It's OK to start a new sprite section and
// only change parameters that are different from the last one. Similarly, it is
// OK to create a section that only sets parameters and doesn't actually start a
// conversion with "src" to set some metadata.
//
// The output files are derived from the `out` option, giving you:
// * a .chr file for the character data
// * a .pal file containing palette data
// * a .h file for a C header with offsets defined (also GNU AS compatible)
// * a .inc file for an assembly file with offset definitions
//
// Valid options, with example params:
//
// out = res/chr
// tile = 0x1000
// angle = 90
//
// [chr_sprite]
// w = 32
// h = 16
// src = sprite.png

#define TILESIZE_DEFAULT 16
#define DEPTH_DEFAULT 4

//
// Pixel data formats.
//
typedef enum DataFormat
{
	DATA_FORMAT_UNSPECIFIED,
	DATA_FORMAT_SP013,       // Atlus 013 sprite data
	DATA_FORMAT_BG038,       // Atlus 038 background tile data
	DATA_FORMAT_DIRECT,      // Raw tile conversion.
	DATA_FORMAT_COUNT
} DataFormat;

const char *kstring_for_data_format[DATA_FORMAT_COUNT] =
{
	[DATA_FORMAT_UNSPECIFIED] = "{unspecified}",
	[DATA_FORMAT_DIRECT] = "direct",
	[DATA_FORMAT_SP013] = "sp013",
	[DATA_FORMAT_BG038] = "bg038",
};

static DataFormat data_format_for_string(const char *str)
{
	for (int i = 0; i < DATA_FORMAT_COUNT; i++)
	{
		if (strcmp(str, kstring_for_data_format[i]) == 0)
		{
			return i;
		}
	}
	return DATA_FORMAT_UNSPECIFIED;
}

const char *string_for_data_format(DataFormat fmt)
{
	if (fmt < 0 || fmt >= DATA_FORMAT_COUNT) return kstring_for_data_format[DATA_FORMAT_UNSPECIFIED];
	return kstring_for_data_format[fmt];
}

//
// Conversion entry data.
//

// An Entry represents a source image, and a symbol that can be referenced.
// As this tool was born out of a need of converting data from sprite sheets,
// an entry has the concept of "Frames" chopped out of the source texture.
// In addition to frames, a tile size can be defined, which is the minimum-size
// building block used to create the image. It is yet another way to subdivide
// the data.
typedef struct FrameCfg
{
	int src_tex_w, src_tex_h;  // Source texture dimensions (rounded to tile)
	int w, h;                  // Sprite frame dimensions. 0 == whole image.
	int angle;                 // Rotation of the frame.
	uint32_t code;             // Starting code in graphics memory.
	int tilesize;              // Size of one tile.
} FrameCfg;

typedef struct Entry Entry;
struct Entry
{
	int id;
	char symbol[256];  // Symbol name as enumerated
	char symbol_upper[256];
	uint16_t pal[256];
	int pal_size;
	Entry *pal_ref;  // Pointer to pre-existing entry with the same palette.
	int pal_block_offs;  // -1 if not set.

	Entry *next;  // Pointer to the next in the LL.

	FrameCfg frame_cfg;

	// Starting code and code increment per frame / element.
	uint32_t code_per;
	int frames;

	// DataFormat-specific things.
	struct
	{
		uint16_t size_code;
	} sp013;

	// The bitmap data.
	uint8_t *chr;
	size_t chr_bytes;
};

// State for the conversion process.
typedef struct Conv
{
	// Linked list of sprites read
	Entry *entry_head;  // First in the entries link list.
	Entry *entry_tail;  // Pointer to the end of the entries list.
	unsigned int entry_count;

	// Meta config
	char out[256];           // Output base filename.
	DataFormat data_format;  // Arrangement of data.
	PalFormat pal_format;
	int depth;             // Bits per pixel.

	// Config state from ini for next entry
	char src[256];           // Source image filename.
	char symbol[256];        // Symbol name (section).
	FrameCfg frame_cfg;
} Conv;

//
// Entry emission functions.
//
static void entry_emit_meta(const Entry *e, const Conv *conv, FILE *f_inc, int pal_offs)
{
	const FrameCfg *frame_cfg = &e->frame_cfg;
	printf("Entry $%03X \"%s\": %d x %d, %d frames/tiles\n",
	       e->id, e->symbol, e->frame_cfg.w, e->frame_cfg.h, e->frames);

	// Write inc entry
	fprintf(f_inc, "; Entry $%03X \"%s\": %d x %d, %d frames/tiles\n",
	        e->id, e->symbol,
	        frame_cfg->w, frame_cfg->h, e->frames);
	fprintf(f_inc, "%s_PAL_OFFS = $%X\n", e->symbol_upper, pal_offs);
	fprintf(f_inc, "%s_PAL_LEN = $%X\n", e->symbol_upper, e->pal_size);

	switch (conv->data_format)
	{
		case DATA_FORMAT_SP013:
			fprintf(f_inc, "%s_CODE = $%X\n", e->symbol_upper, frame_cfg->code);
			fprintf(f_inc, "%s_CODE_HI = $%X\n", e->symbol_upper, frame_cfg->code >> 16);
			fprintf(f_inc, "%s_CODE_LOW = $%X\n", e->symbol_upper, frame_cfg->code & 0xFFFF);
			fprintf(f_inc, "%s_SRC_TEX_W = %d\n", e->symbol_upper, frame_cfg->src_tex_w);
			fprintf(f_inc, "%s_SRC_TEX_H = %d\n", e->symbol_upper, frame_cfg->src_tex_h);
			fprintf(f_inc, "%s_W = %d\n", e->symbol_upper, frame_cfg->w);
			fprintf(f_inc, "%s_H = %d\n", e->symbol_upper, frame_cfg->h);
			fprintf(f_inc, "%s_SIZE = $%04X\n", e->symbol_upper, e->sp013.size_code);
			fprintf(f_inc, "%s_FRAME_OFFS = $%X\n", e->symbol_upper, e->code_per);
			fprintf(f_inc, "%s_FRAMES = %d\n", e->symbol_upper, e->frames);
			break;

		case DATA_FORMAT_BG038:
			fprintf(f_inc, "%s_CODE8 = $%X\n", e->symbol_upper, frame_cfg->code);
			fprintf(f_inc, "%s_CODE8_HI = $%X\n", e->symbol_upper, frame_cfg->code >> 16);
			fprintf(f_inc, "%s_CODE8_LOW = $%X\n", e->symbol_upper, frame_cfg->code & 0xFFFF);
			fprintf(f_inc, "%s_CODE16 = $%X\n", e->symbol_upper, frame_cfg->code / 4);
			fprintf(f_inc, "%s_CODE16_HI = $%X\n", e->symbol_upper, (frame_cfg->code / 4) >> 16);
			fprintf(f_inc, "%s_CODE16_LOW = $%X\n", e->symbol_upper, (frame_cfg->code / 4) & 0xFFFF);
			fprintf(f_inc, "%s_W = %d\n", e->symbol_upper, frame_cfg->src_tex_w);
			fprintf(f_inc, "%s_H = %d\n", e->symbol_upper, frame_cfg->src_tex_h);
			fprintf(f_inc, "%s_TILESIZE = %d\n", e->symbol_upper, frame_cfg->w);  // "Tilesize" refers to the conversion perspective
			fprintf(f_inc, "%s_TILES_W = %d\n", e->symbol_upper, frame_cfg->src_tex_w / frame_cfg->w);  // whereas the "frame" is used to chop major tiles
			fprintf(f_inc, "%s_TILES_H = %d\n", e->symbol_upper, frame_cfg->src_tex_h / frame_cfg->w);
			break;

		case DATA_FORMAT_DIRECT:
			fprintf(f_inc, "%s_DATA_OFFS = $%X\n", e->symbol_upper, frame_cfg->code*frame_cfg->src_tex_w*frame_cfg->src_tex_h);
			fprintf(f_inc, "%s_FRAME_OFFS = $%X\n", e->symbol_upper, frame_cfg->w*frame_cfg->h);
			fprintf(f_inc, "%s_TILE_OFFS = $%X\n", e->symbol_upper, frame_cfg->tilesize*frame_cfg->tilesize);
			fprintf(f_inc, "%s_W = %d\n", e->symbol_upper, frame_cfg->w);
			fprintf(f_inc, "%s_H = %d\n", e->symbol_upper, frame_cfg->h);
			fprintf(f_inc, "%s_FRAMES = %d\n", e->symbol_upper, e->frames);
			break;

		default:
			break;
	}
	fprintf(f_inc, "\n");
}

static void entry_emit_chr(const Entry *e, const Conv *conv, FILE *f_chr1, FILE *f_chr2)
{
	// Dump CHR data into CHR file(s)
	uint8_t *chr = e->chr;
	switch (conv->data_format)
	{
		case DATA_FORMAT_SP013:
			for (size_t i = 0; i < e->chr_bytes/2; i++)
			{
				const uint8_t fetchpx0 = *chr++;
				const uint8_t fetchpx1 = *chr++;

				// Inverted data order for SP013.
				const uint8_t px0 = fetchpx1;
				const uint8_t px1 = fetchpx0;

				const uint8_t lowbyte = ((px0 << 4) & 0xF0) | (px1 & 0x0F);
				const uint8_t hibyte = (px0 & 0xF0) | ((px1 >> 4) & 0x0F);

				fputc(lowbyte, f_chr1);
				if (f_chr2) fputc(hibyte, f_chr2);  // TODO: good option for selecting plane split
			}
			break;

		case DATA_FORMAT_BG038:
			for (size_t i = 0; i < e->chr_bytes/2; i++)
			{
				const uint8_t px0 = *chr++;
				const uint8_t px1 = *chr++;

				const uint8_t lowbyte = ((px0 << 4) & 0xF0) | (px1 & 0x0F);
				const uint8_t hibyte = (px0 & 0xF0) | ((px1 >> 4) & 0x0F);

				// Put main plane on low bytes, upper 4bpp on even bytes
				fputc(lowbyte, f_chr1);
				if (conv->depth == 8) fputc(hibyte, f_chr1);
			}
			break;

		case DATA_FORMAT_DIRECT:
			for (size_t i = 0; i < e->chr_bytes; i++)
			{
				const uint8_t px = *chr++;
				fputc(px, f_chr1);
			}
			break;

		default:
			break;

	}
}

//
// Conversion process functions.
//

static bool validate_angle(int angle)
{
	switch (angle)
	{
		case 0:
		case 90:
		case 180:
		case 270:
			return true;
		default:
			return false;
	}
}

bool conv_init(Conv *s)
{
	memset(s, 0, sizeof(*s));
	s->depth = DEPTH_DEFAULT;
	return true;
}

bool conv_validate(Conv *s)
{
	if (s->symbol[0] == '\0')
	{
		fprintf(stderr, "[CONV] symbol not set!\n");
		return false;
	}
	if (s->src[0] == '\0')
	{
		fprintf(stderr, "[CONV] src not set!\n");
		return false;
	}
	if (s->out[0] == '\0')
	{
		fprintf(stderr, "[CONV] output not set!\n");
		return false;
	}

	FrameCfg *frame_cfg = &s->frame_cfg;
	if (s->depth != 4 && s->depth != 8)
	{
		fprintf(stderr, "[CONV] depth (%d) must be 4 or 8!\n", s->depth);
		return false;
	}
	if (!validate_angle(frame_cfg->angle))
	{
		fprintf(stderr, "[CONV] angle %d NG\n", frame_cfg->angle);
		return false;
	}

	switch (s->data_format)
	{
		case DATA_FORMAT_UNSPECIFIED:
			fprintf(stderr, "[CONV] No data format specified!\n");
			return false;

		case DATA_FORMAT_SP013:
			// unspecified is ok for SP013.
			if (frame_cfg->tilesize == 0)
			{
				frame_cfg->tilesize = 16;
			}
			// Point out mistaken sizes.
			else if (frame_cfg->tilesize != 16)
			{
				fprintf(stderr, "[CONV] WARNING: Tilesize %dpx specified, but "
				        "specified hardware only supports 16px.\n",
				        frame_cfg->tilesize);
				frame_cfg->tilesize = 16;
			}
			break;

		case DATA_FORMAT_BG038:
			// Limit to supported formats.
			if (frame_cfg->tilesize != 8 && frame_cfg->tilesize != 16)
			{
				fprintf(stderr, "[CONV] Tilesize %dpx unsupported; "
				        "BG038 only supports 8px or 16px.\n",
				         frame_cfg->tilesize);
				return false;
			}
			break;

		case DATA_FORMAT_DIRECT:
			break;

		default:
			fprintf(stderr, "[CONV] Data format %d NG!\n", s->data_format);
			break;
	}

	if (!pal_validate_selection(s->pal_format))
	{
		fprintf(stderr, "[CONV] Palette format NG!\n");
		return false;
	}

	if (frame_cfg->tilesize <= 0 || (frame_cfg->tilesize % 8 != 0))
	{
		fprintf(stderr, "[CONV] WARNING: Tilesize %d NG; defaulting to 16\n",
		        frame_cfg->tilesize);
		frame_cfg->tilesize = 16;
	}

	if (frame_cfg->w % frame_cfg->tilesize != 0)
	{
		fprintf(stderr, "[CONV] WARNING: W (%d) not a multiple of %d\n", frame_cfg->w, frame_cfg->tilesize);
	}
	if (frame_cfg->h % frame_cfg->tilesize != 0)
	{
		fprintf(stderr, "[CONV] WARNING: H (%d) not a multiple of %d\n", frame_cfg->h, frame_cfg->tilesize);
	}
	return true;
}

static bool conv_entry_add(Conv *s)
{
	if (!conv_validate(s)) return false;

	Entry *e = NULL;

	// If this is the first one, create the head
	if (!s->entry_head)
	{
		s->entry_head = calloc(sizeof(*e), 1);
		s->entry_head->id = 0;
		e = s->entry_head;
		s->entry_tail = s->entry_head;
	}
	else
	{
		e = calloc(sizeof(*e), 1);
		e->id = s->entry_tail->id + 1;
		s->entry_tail->next = e;
		s->entry_tail = e;
	}

	strncpy(e->symbol, s->symbol, sizeof(e->symbol));
	for (int i = 0; i < strlen(s->symbol); i++)
	{
		e->symbol_upper[i] = toupper(e->symbol[i]);
	}
	e->frame_cfg = s->frame_cfg;

	FrameCfg *frame_cfg = &e->frame_cfg;

	//
	// Load image data into 8bpp buffer.
	//
	const char *fname = s->src;
	unsigned int png_w;
	unsigned int png_h;
	size_t png_fsize;
	uint8_t *png;
	int error = lodepng_load_file(&png, &png_fsize, fname);
	LodePNGState state;
	if (error)
	{
		fprintf(stderr, "[ENTRY $%03X: %s] LodePNG error %u: %s\n", e->id, fname, error,
		        lodepng_error_text(error));
		return false;
	}
	lodepng_state_init(&state);
	state.info_raw.colortype = LCT_PALETTE;
	state.info_raw.bitdepth = 8;
	uint8_t *px;
	error = lodepng_decode(&px, &png_w, &png_h, &state, png, png_fsize);
	if (error)
	{
		fprintf(stderr, "[ENTRY $%03X: %s] LodePNG error %u: %s\n", e->id, fname, error,
		        lodepng_error_text(error));
		free(png);
		return false;
	}

	//
	// Make native palette data (host endianness)
	//
	e->pal_size = state.info_png.color.palettesize;
	pal_pack_set(s->pal_format, state.info_png.color.palette, e->pal, state.info_png.color.palettesize);
	e->pal_ref = NULL;
	
	// See if another entry has the same palette, and get a reference to it if so.
	Entry *f = s->entry_head;
	while (f && (f != e))
	{
		bool match = true;
		// Palette size of other is >= this one's palette, and data matches
		if (f->pal_size >= e->pal_size)
		{
			for (int i = 0; i < e->pal_size; i++)
			{
				if (e->pal[i] == f->pal[i]) continue;
				match = false;
				break;
			}
		}
		else
		{
			match = false;
		}

		if (match)
		{
			e->pal_ref = f;
			break;
		}

		f = f->next;
	}

	//
	// Set size and sprite count information.
	//
	switch (s->data_format)
	{
		// BG038 sets up the tilesize and "frame" width in order to support the
		// interleaved tile order the hardware expects.
		case DATA_FORMAT_BG038:
			if (frame_cfg->tilesize > 8)
			{
				frame_cfg->tilesize = 8;
				frame_cfg->w = 16;
				frame_cfg->h = 16;
			}
			break;

		// SP013 supports only 16px.
		case DATA_FORMAT_SP013:
			frame_cfg->tilesize = 16;
			break;
		default:
			break;
	}

	// A size < 0 means the whole image width/height is used.
	frame_cfg->src_tex_w = png_w;
	frame_cfg->src_tex_h = png_h;
	if (frame_cfg->w <= 0) frame_cfg->w = png_w;
	if (frame_cfg->h <= 0) frame_cfg->h = png_h;

	// Count tiles for one sprite frame.
	const int frame_tiles_x = frame_cfg->w / frame_cfg->tilesize;
	const int frame_tiles_y = frame_cfg->h / frame_cfg->tilesize;
	e->code_per = frame_tiles_x * frame_tiles_y;
	// Effective frame height by rounding down to tile count.
	const int sw_adj = (frame_cfg->tilesize*frame_tiles_x);
	const int sh_adj = (frame_cfg->tilesize*frame_tiles_y);

	memset(&e->sp013, 0, sizeof(e->sp013));

	const bool yoko = ((frame_cfg->angle == 0) || (frame_cfg->angle == 180));

	// Special format-specific stuff
	switch (s->data_format)
	{
		case DATA_FORMAT_SP013:
			e->sp013.size_code = yoko ? ((frame_tiles_x << 8) | frame_tiles_y) :
			                            ((frame_tiles_y << 8) | frame_tiles_x);
			break;
		case DATA_FORMAT_BG038:
			memset(&e->sp013, 0, sizeof(e->sp013));
			break;

		default:
			break;
	}

	//
	// Based on image dimensions allocate CHR space and set chr_bytes.
	//

	// sprite quantities within the sheet
	const int frame_count_x = png_w / sw_adj;
	const int frame_count_y = png_h / sh_adj;

	const int chr_bytes_per = sw_adj * sh_adj;
	e->chr_bytes = (frame_count_x * frame_count_y) * chr_bytes_per;
	e->chr = malloc(e->chr_bytes);
	if (!e->chr)
	{
		fprintf(stderr, "[ENTRY $%03X] Couldn't allocate CHR %lu bytes\n", e->id,
		        e->chr_bytes);
		free(png);
		free(px);
		return false;
	}


	//
	// Copy image data as 8bpp CHR data.
	//
	int frame_no = 0;
	uint8_t *chr_w = e->chr;

	// Frames are taken from top to bottom, left to right.
	const int png_outer = frame_count_y;
	const int png_inner = frame_count_x;

	for (int outer = 0; outer < png_outer; outer++)
	{
		for (int inner = 0; inner < png_inner; inner++)
		{
			const int pngy = outer;
			const int pngx = inner;

			switch (s->data_format)
			{
				case DATA_FORMAT_DIRECT:
					chr_w = tile_read_frame(px, png_w, pngx, pngy, sw_adj, sh_adj, frame_cfg->tilesize, frame_cfg->angle, chr_w);
					break;
				case DATA_FORMAT_SP013:
					chr_w = tile_read_frame(px, png_w, pngx, pngy, sw_adj, sh_adj, /*tilesize=*/0, frame_cfg->angle, chr_w);
					break;
				case DATA_FORMAT_BG038:
					chr_w = tile_read_frame(px, png_w, pngx, pngy, sw_adj, sh_adj, frame_cfg->tilesize, frame_cfg->angle, chr_w);
					break;
				default:
					break;
			}
			frame_no++;
			s->frame_cfg.code += e->code_per;  // Move base code value forward
			                                   // for the next entry.
		}
	}
	e->frames = frame_no;

	free(png);
	free(px);

	s->entry_count++;
	return true;
}

//
// Release of conversion resources.
//
static void conv_shutdown(Conv *s)
{
	Entry *e = s->entry_head;
	while (e)
	{
		if (e->chr) free(e->chr);
		Entry *next = e->next;
		free(e);
		e = next;
	}
}

//
// Handler for the (ab)used INI parsing functions.
//
static int handler(void *user, const char *section, const char *name, const char *value)
{
	Conv *s = (Conv *)user;

	// New section - copy in name
	if (strcmp(section, s->symbol) != 0)
	{
		strncpy(s->symbol, section, sizeof(s->symbol));
		s->symbol[sizeof(s->symbol)-1] = '\0';
	}

	// parameter settings
	if (strcmp("src", name) == 0)
	{
		strncpy(s->src, value, sizeof(s->src));
		s->src[sizeof(s->src)-1] = '\0';
		if (!conv_entry_add(s)) return 0;
	}
	else if (strcmp("out", name) == 0)
	{
		strncpy(s->out, value, sizeof(s->out));
		s->out[sizeof(s->out)-1] = '\0';
	}
	else if (strcmp("code", name) == 0)
	{
		s->frame_cfg.code = strtoul(value, NULL, 0);
	}
	else if (strcmp("angle", name) == 0)
	{
		s->frame_cfg.angle = strtoul(value, NULL, 0);
	}
	else if (strcmp("depth", name) == 0)
	{
		s->depth = strtoul(value, NULL, 0);
	}
	else if (strcmp("w", name) == 0)
	{
		s->frame_cfg.w = strtoul(value, NULL, 0);
	}
	else if (strcmp("h", name) == 0)
	{
		s->frame_cfg.h = strtoul(value, NULL, 0);
	}
	else if (strcmp("tilesize", name) == 0)
	{
		s->frame_cfg.tilesize = strtoul(value, NULL, 0);
	}
	else if (strcmp("format", name) == 0)
	{
		s->data_format = data_format_for_string(value);
		if (s->data_format == DATA_FORMAT_UNSPECIFIED)
		{
			printf("ERROR: Unhandled data format %s\n", value);
			return 0;
		}
	}
	else if (strcmp("pal", name) == 0)
	{
		s->pal_format = pal_format_for_string(value);
		if (s->pal_format == PAL_FORMAT_UNSPECIFIED)
		{
			printf("ERROR: Unhandled palette format %s\n", value);
			return 0;
		}
	}
	else
	{
		printf("WARNING: Unhandled directive \"%s\"\n", name);
		return 0;
	}
	return 1;
}




int main(int argc, char **argv)
{
	int ret = -1;
	if (argc < 2)
	{
		printf("Usage: %s CONFIG\n", argv[0]);
		return -1;
	}

	Conv conv;
	if (!conv_init(&conv))
	{
		fprintf(stderr, "Couldn't initialize CONV\n");
		return -1;
	};

	// the actual conversion is kicked off in the INI handler, when `convert` is set.
	ret = ini_parse(argv[1], &handler, &conv);
	if (ret)
	{
		fprintf(stderr, "Error parsing \"%s\".\n", argv[0]);
		return -1;
	}

	// Now emit a pile of CHR data
	char fname_buf[512];

	FILE *f_chr1 = NULL;
	FILE *f_chr2 = NULL;
	FILE *f_pal = NULL;
	FILE *f_inc = NULL;

	// SP013 special case where 8bpp data is split into upper and lower files
	if (conv.depth == 8 && conv.data_format == DATA_FORMAT_SP013)
	{
		snprintf(fname_buf, sizeof(fname_buf), "%s.ch1", conv.out);
		f_chr1 = fopen(fname_buf, "wb");
		if (!f_chr1)
		{
			fprintf(stderr, "Couldn't open %s for writing\n", fname_buf);
			ret = -1;
			goto done;
		}

		snprintf(fname_buf, sizeof(fname_buf), "%s.ch2", conv.out);
		f_chr2 = fopen(fname_buf, "wb");
		if (!f_chr2)
		{
			fprintf(stderr, "Couldn't open %s for writing\n", fname_buf);
			ret = -1;
			goto done;
		}
	}
	else
	{
		snprintf(fname_buf, sizeof(fname_buf), "%s.chr", conv.out);
		f_chr1 = fopen(fname_buf, "wb");
		if (!f_chr1)
		{
			fprintf(stderr, "Couldn't open %s for writing\n", fname_buf);
			ret = -1;
			goto done;
		}
	}

	snprintf(fname_buf, sizeof(fname_buf), "%s.pal", conv.out);
	f_pal = fopen(fname_buf, "wb");
	if (!f_pal)
	{
		fprintf(stderr, "Couldn't open %s for writing\n", fname_buf);
		ret = -1;
		goto done;
	}

	snprintf(fname_buf, sizeof(fname_buf), "%s.inc", conv.out);
	f_inc = fopen(fname_buf, "wb");
	if (!f_inc)
	{
		fprintf(stderr, "Couldn't open %s for writing\n", fname_buf);
		ret = -1;
		goto done;
	}
	fprintf(f_inc, "; ┌───────────────────────────────────────────────────┐\n");
	fprintf(f_inc, "; │ This file was automatically generated by Velella. │\n");
	fprintf(f_inc, "; └───────────────────────────────────────────────────┘\n");
	fprintf(f_inc, "\n");

	Entry *e = conv.entry_head;
	int pal_offs = 0;
	while (e)
	{
		// If this entry references another's palette, copy the entry offs, and
		// do not add palette data.
		if (e->pal_ref)
		{
			Entry *f = e->pal_ref;
			e->pal_block_offs = f->pal_block_offs;
		}
		else
		{
			// Otherwise, mark palette offs by the output position and write
			// unique palette data.
			e->pal_block_offs = pal_offs;
			for (int i = 0; i < e->pal_size; i++)
			{
				const uint8_t upper = e->pal[i] >> 8;
				const uint8_t lower = e->pal[i] & 0x00FF;
				fputc(upper, f_pal);
				fputc(lower, f_pal);
			}
			pal_offs += e->pal_size * sizeof(uint16_t);
		}

		entry_emit_meta(e, &conv, f_inc, e->pal_block_offs);
		entry_emit_chr(e, &conv, f_chr1, f_chr2);

		e = e->next;
		if (e)
		{
			fprintf(f_inc, "; ──────────────────────────────────────────────────────────────────────────────\n");
		}
	}

done:
	if (f_chr1) fclose(f_chr1);
	if (f_chr2) fclose(f_chr2);
	if (f_pal) fclose(f_pal);
	if (f_inc) fclose(f_inc);
	conv_shutdown(&conv);

	// Close out data to files

	return ret;
}
