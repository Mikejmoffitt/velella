#include "conv.h"
#include "pal.h"
#include "tileread.h"
#include "tile.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "lodepng.h"
#include "mdcsp_claim.h"
#include "mdcsp_mapping.h"

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

	// Tilesize and depth checks.
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
				return false;
			}

			if (frame_cfg->depth != 4 && frame_cfg->depth != 8)
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

			if (frame_cfg->depth != 4 && frame_cfg->depth != 8)
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
				return false;
			}

			if (frame_cfg->depth != 4)
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
			if (frame_cfg->depth != 4)
			{
				fprintf(stderr, "[CONV] CPS only supports 4bpp tile data.\n");
				return false;
			}
			break;

		case DATA_FORMAT_TOA_GCU_BG:
			if (frame_cfg->tilesize != 16)
			{
				fprintf(stderr, "[CONV] GCU background only supports 16x16 tiles.n");
				return false;
			}
			break;

		case DATA_FORMAT_TOA_GCU_SPR:
			if (frame_cfg->w > 128 || frame_cfg->w > 128)
			{
				fprintf(stderr, "[CONV] Max GCU sprite size is 128x128.\n");
				return false;
			}
			[[fallthrough]];
		case DATA_FORMAT_MD_SPR:
		case DATA_FORMAT_MD_BG:
		case DATA_FORMAT_TOA_TXT:
			if ((frame_cfg->w && frame_cfg->w < 8) ||
			    (frame_cfg->h && frame_cfg->h < 8))
			{
				fprintf(stderr, "[CONV] Frame w/h must be at least the tilesize.\n");
				return false;
			}
			if (frame_cfg->tilesize != 8)
			{
				fprintf(stderr, "[CONV] Only 8x8 tiles are supported for this format.\n");
				return false;
			}
			[[fallthrough]];
		case DATA_FORMAT_MD_CSP:
		case DATA_FORMAT_MD_CBG:
		case DATA_FORMAT_NEO_FIX:
		case DATA_FORMAT_NEO_SPR:
		case DATA_FORMAT_NEO_CSPR:
			if (frame_cfg->depth != 4)
			{
				fprintf(stderr, "[CONV] Only 4bpp tile data is supported for this format.\n");
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
		fprintf(stderr, "[CONV] Tilesize %d not a power of twoNG; defaulting to 16\n",
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

	e->chr_offs = s->chr_pos;
	e->map_offs = s->map_pos;

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
			frame_cfg->w = frame_cfg->tilesize;
			frame_cfg->h = frame_cfg->tilesize;
			frame_cfg->tilesize = 8;
			break;

		case DATA_FORMAT_TOA_GCU_BG:
			frame_cfg->w = frame_cfg->tilesize;
			frame_cfg->h = frame_cfg->tilesize;
			frame_cfg->tilesize = 8;

		default:
			break;
	}

	// A size < 0 means the whole image width/height is used for a frame.
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
			e->sp013.size_code = yoko ? ((frame_tiles_x << 8) | frame_tiles_y)
			                          : ((frame_tiles_y << 8) | frame_tiles_x);
			break;

		case DATA_FORMAT_CPS_SPR:
			e->cps_spr.size_code = yoko ? ((frame_tiles_y-1)<<4) | (frame_tiles_x-1)
			                            : ((frame_tiles_x-1)<<4) | (frame_tiles_y-1);
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
					fprintf(stderr, "[CONV] CPS 8x8 tiles must be sourced from a "
					                "file with an even column count.\n");
					free(png);
					free(px);
					return false;
				}
				e->code_per /= 2;
			}
			break;

		case DATA_FORMAT_MD_SPR:
			e->md_spr.size_code = yoko ? ((frame_tiles_x-1)<<2) | (frame_tiles_y-1)
			                           : ((frame_tiles_y-1)<<2) | (frame_tiles_x-1);
			break;

		case DATA_FORMAT_MD_CSP:
			if (frame_cfg->angle != 0)
			{
				fprintf(stderr, "[CONV] MD composites do not yet support rotation.\n");
				free(png);
				free(px);
				return false;
			}
			// code_per is set at each iteration of the sprite claim routine.
			break;

		// Composite background (optimized tilemap and chr data) does the fofllowing
		// before emitting CHR data ordinarily:
		// * Chop texture into tiles
		// * Create layout mapping of tiles
		// * Compare tile hashes to identify redundancies and make replacement list
		// * Create optimized layout data and reduced tileset
		//
		// The optimized layout data is added to the entry as metadata to be emitted
		// and the reduced tileset replaces the texture from the PNG from this point.
		// That reduced tileset is then passed through the ordinary pipeline.
		case DATA_FORMAT_MD_CBG:
			if (frame_cfg->angle != 0)
			{
				fprintf(stderr, "[CONV] MD composites do not yet support rotation.\n");
				free(png);
				free(px);
				return false;
			}
			// code_per is set at each iteration of the sprite claim routine.
			break;

		case DATA_FORMAT_TOA_GCU_SPR:
			// As the GCU has fixed point positioning and the size code has
			// unused upper bits, there is a clever techinque Toaplan employed
			// to bake centering offset information into the size code.
			// The resulting value can just be added to the X and Y positions.
			if (frame_cfg->center)
			{
				uint16_t widthcode = yoko ? (frame_tiles_x-1) : (frame_tiles_y-1);
				uint16_t heightcode = yoko ? (frame_tiles_y-1) : (frame_tiles_x-1);
				const int16_t offs_x = yoko ? (((frame_tiles_x*8)/2)*128) : (((frame_tiles_y*8)/2)*128);
				const int16_t offs_y = yoko ? (((frame_tiles_y*8)/2)*128) : (((frame_tiles_x*8)/2)*128);
				uint16_t code_x;
				uint16_t code_y;

				switch (frame_cfg->angle)
				{
					default:
					case 0:
						code_x = -offs_x | widthcode;
						code_y = -offs_y | heightcode;
						break;

					case 90:
						code_y = offs_x | widthcode;
						code_x = offs_y | heightcode;
						break;

					case 180:
						code_x = offs_x | widthcode;
						code_y = offs_y | heightcode;
						break;

					case 270:
						code_y = -offs_x | widthcode;
						code_x = -offs_y | heightcode;
						break;
				}
				e->gcu_spr.size_code = (code_x<<16) | code_y;
				
			}
			else
			{
				e->gcu_spr.size_code = yoko ? (((frame_tiles_x-1) << 16) | (frame_tiles_y-1))
				                            : (((frame_tiles_y-1) << 16) | (frame_tiles_x-1));
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

	// TODO: For CPS, iterate through the image, create a bitmap of tile skips,
	// and get the actual used chr size before allocating.

	const int chr_bytes_per = sw_adj * sh_adj;
	const size_t expected_chr_bytes = (frame_count_x * frame_count_y) * chr_bytes_per;
	e->chr_bytes = 0;
	e->chr = malloc(expected_chr_bytes);
	if (!e->chr)
	{
		fprintf(stderr, "[ENTRY $%03X] Couldn't allocate CHR %lu bytes\n", e->id,
		        expected_chr_bytes);
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
			const int png_src_y = outer;
			const int png_src_x = inner;

			switch (frame_cfg->data_format)
			{
				case DATA_FORMAT_DIRECT:
				case DATA_FORMAT_BG038:
				case DATA_FORMAT_CPS_SPR:  // TODO: For CPS SPR, pass in a tile skip flag.
				case DATA_FORMAT_CPS_BG:
				case DATA_FORMAT_MD_BG:
				case DATA_FORMAT_TOA_GCU_SPR:
				case DATA_FORMAT_TOA_GCU_BG:
				case DATA_FORMAT_NEO_FIX:
					chr_w = tile_read_frame(px,
					                        png_w, png_h,
					                        png_src_x, png_src_y,
					                        sw_adj, sh_adj,
					                        frame_cfg->tilesize,
					                        frame_cfg->angle,
					                        -1, -1,
					                        0, chr_w);
					e->chr_bytes += chr_bytes_per;
					break;
					
				case DATA_FORMAT_SP013:
					chr_w = tile_read_frame(px,
					                        png_w, png_h,
					                        png_src_x, png_src_y,
					                        sw_adj, sh_adj,
					                        /*tilesize=*/1,
					                        frame_cfg->angle,
					                        -1, -1,
					                        0,  chr_w);
					e->chr_bytes += chr_bytes_per;
					break;

				case DATA_FORMAT_MD_SPR:
				case DATA_FORMAT_TOA_TXT:
					chr_w = tile_read_frame(px,
					                        png_w, png_h,
					                        png_src_x, png_src_y,
					                        sw_adj, sh_adj,
					                        frame_cfg->tilesize,
					                        frame_cfg->angle,
					                        -1, -1,
					                        TILE_READ_FLAG_X_MAJOR, chr_w);
					e->chr_bytes += chr_bytes_per;
					break;

				case DATA_FORMAT_MD_CSP:
					// This is mostly a rewrite of claim() from png2csp.
					{
						static const int k_tile_bytes = 8*8*sizeof(uint8_t);
						static const int k_md_static_spr_offs = 128;
						const int base_spr_index = e->md_csp.spr_count;
						int spr_in_sprite = 0;
						const int base_tile_index = e->md_csp.tile_count;
						int tiles_for_sprite = 0;

						// TODO: Support more than center origin, using origin_for_sp()
						const int ox = sw_adj/2;
						const int oy = sh_adj/2;

						// Clip region determined by claim
						int clip_x, clip_y;

						// The relative x/y is used to bake in the 128px offset.
						int last_vx = -k_md_static_spr_offs;
						int last_vy = -k_md_static_spr_offs;
						int last_fvx = -k_md_static_spr_offs;
						int last_fvy = -k_md_static_spr_offs;

						ClaimSize claim_size;
						while ((claim_size = mdcsp_claim(px, png_src_x*sw_adj, png_src_y*sh_adj,
						                                 sw_adj, sh_adj,
						                                 png_w, png_h,
						                                 &clip_x, &clip_y)))
						{
							spr_in_sprite++;
							//
							// Store the PCG data for one claim's worth of tiles.
							//
							uint8_t tile_data[k_tile_bytes * 4 * 4];  // Up to one 32x32px sprite (16 tiles).
							int tiles_clipped = 0;

							const int tiles_w = mdcsp_w_for_claim(claim_size);
							const int tiles_h = mdcsp_h_for_claim(claim_size);

							//printf("F%02d Sp%02d: %d, %d %dx%d\n", e->md_csp.ref_count, e->md_csp.spr_count, clip_x, clip_y, tiles_w, tiles_h);

							// Stop it from scooping data from the adjacent frame.
							const int limx = png_src_x*sw_adj + sw_adj;
							const int limy = png_src_y*sh_adj + sh_adj;

							const int clip_w = tiles_w*frame_cfg->tilesize;
							const int clip_h = tiles_h*frame_cfg->tilesize;

							// Copy tiles into local tile_data cache.

							// TODO: Handle rotation for non-0 degree config? maybe it works?
							uint8_t *tile_data_w = tile_data;

							tile_read_frame(px,
							                png_w, png_h,
							                clip_x, clip_y,
							                clip_w, clip_h,
							                frame_cfg->tilesize,
							                frame_cfg->angle,
							                limx, limy,
							                TILE_READ_FLAG_X_MAJOR|TILE_READ_FLAG_ERASE|TILE_READ_POS_DIRECT,
							                tile_data_w);

							tiles_clipped += mdcsp_tiles_for_claim(claim_size);

							// Copy the claimed tiles into CHR.
							memcpy(chr_w, tile_data, k_tile_bytes * tiles_clipped);
							e->md_csp.tile_count += tiles_clipped;

							chr_w += k_tile_bytes * tiles_clipped;

							// Record the hardware sprite entry.
							const int vx = ((clip_x % sw_adj) - ox);
							const int vy = ((clip_y % sh_adj) - oy);

							int fvx = -1 * ((clip_x % sw_adj) - ox);
							int fvy = -1 * ((clip_y % sh_adj) - oy);
							//		fvx -= (fvx - ox);
							fvx -= tiles_w * frame_cfg->tilesize;
							//		fvy -= (fvy - oy);
							fvy -= tiles_h * frame_cfg->tilesize;

							MdCspSpr *spr = &e->md_csp.spr_dat[e->md_csp.spr_count];
							e->md_csp.spr_count++;

							spr->dx = vx - last_vx;
							spr->dy = vy - last_vy;
							spr->w = tiles_w;
							spr->h = tiles_h;
							spr->tile = tiles_for_sprite;
							spr->flip_dx = fvx - last_fvx;
							spr->flip_dy = fvy - last_fvy;

							last_vx = vx;
							last_vy = vy;
							last_fvx = fvx;
							last_fvy = fvy;

							tiles_for_sprite += tiles_clipped;
							if (e->md_csp.dma_buffer_tiles < tiles_for_sprite)
							{
								e->md_csp.dma_buffer_tiles = tiles_for_sprite;
							}
						}
						// Once all tiles have been claimed, make a ref entry for the sprites.
						// printf("F%02d: %d sprites, %d tiles\n", e->md_csp.ref_count, spr_in_sprite, tiles_for_sprite);

						e->chr_bytes += tiles_for_sprite * k_tile_bytes;  // in 8bpp terms.
						MdCspRef *ref = &e->md_csp.ref_dat[e->md_csp.ref_count];
						ref->spr_count = spr_in_sprite;  // Hardware sprite count.
						ref->spr_index = base_spr_index;
						ref->tile_index = base_tile_index;
						ref->tile_count = tiles_for_sprite;

						e->code_per = tiles_for_sprite;

						e->md_csp.ref_count++;

						//
					}
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

	// Close out any mapping data and advance

	switch (frame_cfg->data_format)
	{
		case DATA_FORMAT_MD_CSP:
			e->map_bytes = mdcsp_bytes_for_mapping(e->md_csp.ref_count, e->md_csp.spr_count);
			break;

		default:
			break;
	}
	s->map_pos += e->map_bytes;

	switch (frame_cfg->depth)
	{
		case 1:
			s->chr_pos += e->chr_bytes/8;
			break;
		case 2:
			s->chr_pos += e->chr_bytes/4;
			break;
		case 4:
			s->chr_pos += e->chr_bytes/2;
			break;
		case 8:
			s->chr_pos += e->chr_bytes;
			break;
	}

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
		tile_list_shutdown(e->tile_list);
		free(e);
		e = next;
	}
}

