#include "conv.h"
#include "pal.h"
#include "tileread.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lodepng.h"

#define TILESIZE_DEFAULT 16
#define DEPTH_DEFAULT 4

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
	s->frame_cfg.depth = DEPTH_DEFAULT;
	s->frame_cfg.tilesize = TILESIZE_DEFAULT;
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
	if (!validate_angle(frame_cfg->angle))
	{
		fprintf(stderr, "[CONV] angle %d NG\n", frame_cfg->angle);
		return false;
	}

	switch (s->frame_cfg.data_format)
	{
		case DATA_FORMAT_UNSPECIFIED:
			fprintf(stderr, "[CONV] No data format specified!\n");
			return false;

		case DATA_FORMAT_DIRECT:
			break;

		case DATA_FORMAT_SP013:
			if (frame_cfg->tilesize != 16)
			{
				fprintf(stderr, "[CONV] WARNING: Tilesize %dpx specified, but "
				        "specified hardware only supports 16px.\n",
				        frame_cfg->tilesize);
				frame_cfg->tilesize = 16;
			}

			if (s->frame_cfg.depth != 4 && s->frame_cfg.depth != 8)
			{
				fprintf(stderr, "[CONV] sp013 supports 4bpp or 8bpp tiles.\n");
				return false;
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

			if (s->frame_cfg.depth != 4 && s->frame_cfg.depth != 8)
			{
				fprintf(stderr, "[CONV] bg038 supports 4bpp or 8bpp tiles.\n");
				return false;
			}
			break;

		case DATA_FORMAT_CPS_SPR:
			if (frame_cfg->tilesize != 16)
			{
				fprintf(stderr, "[CONV] WARNING: Tilesize %dpx specified, but "
				        "specified hardware only supports 16px.\n",
				        frame_cfg->tilesize);
				frame_cfg->tilesize = 16;
			}

			if (s->frame_cfg.depth != 4)
			{
				fprintf(stderr, "[CONV] CPS only supports 4bpp tile data.\n");
				return false;
			}
			break;

		case DATA_FORMAT_CPS_BG:
			if (frame_cfg->tilesize != 8 && frame_cfg->tilesize != 16 && frame_cfg->tilesize != 32)
			{
				fprintf(stderr, "[CONV] Tilesize %dpx NG)", frame_cfg->tilesize);
				return false;
			}
			if (s->frame_cfg.depth != 4)
			{
				fprintf(stderr, "[CONV] CPS only supports 4bpp tile data.\n");
				return false;
			}
			break;

		default:
			fprintf(stderr, "[CONV] Data format %d NG!\n", frame_cfg->data_format);
			break;
	}

	if (!pal_validate_selection(frame_cfg->pal_format))
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

bool conv_entry_add(Conv *s)
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

	// Symbols in the header are meant to be SCREAMING in all caps.
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
	pal_pack_set(frame_cfg->pal_format, state.info_png.color.palette, e->pal, state.info_png.color.palettesize);
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
	switch (frame_cfg->data_format)
	{
		// Background tiles set the tilesize and "frame" width this way in order
		// to support interleaved tile order; "tiles" become "frames" and the
		// tile size is used as the unit that tiles are built from.
		case DATA_FORMAT_BG038:
			if (frame_cfg->tilesize > 8)
			{
				frame_cfg->tilesize = 8;
				frame_cfg->w = 16;
				frame_cfg->h = 16;
			}
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

	const bool yoko = ((frame_cfg->angle == 0) || (frame_cfg->angle == 180));

	// Please see the notes in the convert function about CPS 8x8 tiles.

	// Special format-specific stuff
	switch (frame_cfg->data_format)
	{
		case DATA_FORMAT_SP013:
			e->sp013.size_code = yoko ? ((frame_tiles_x << 8) | frame_tiles_y) :
			                            ((frame_tiles_y << 8) | frame_tiles_x);
			break;

		case DATA_FORMAT_CPS_BG:
			// Special CPS 8x8 tile case: In order to make 8x8 tiles work, at emission
			// time data is duplicated. As far as CPS is concerned, e->code_per is
			// basically "how many 16x16 tiles". The padded 8x8 tile becomes the same
			// as half a tile. So, an uneven width is forbidden, and code_per is then
			// cut in half.
			if (frame_cfg->tilesize == 8)
			{
				if (((frame_tiles_x) % 2) == 1)
				{
					fprintf(stderr, "[CONV] CPS 8x8 tiles must be sourced from a"
					                "file with an even column count.\n");
					free(png);
					free(px);
					return false;
				}
				e->code_per /= 2;
			}
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

			switch (frame_cfg->data_format)
			{
				case DATA_FORMAT_DIRECT:
				case DATA_FORMAT_BG038:
				case DATA_FORMAT_CPS_SPR:
				case DATA_FORMAT_CPS_BG:
					chr_w = tile_read_frame(px, png_w, pngx, pngy, sw_adj, sh_adj, frame_cfg->tilesize, frame_cfg->angle, chr_w);
					break;
					
				case DATA_FORMAT_SP013:
					chr_w = tile_read_frame(px, png_w, pngx, pngy, sw_adj, sh_adj, /*tilesize=*/0, frame_cfg->angle, chr_w);
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

void conv_shutdown(Conv *s)
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

