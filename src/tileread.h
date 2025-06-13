#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static inline uint8_t *tile_read_frame(const uint8_t *px, int png_w, int png_x, int png_y, int sw_adj, int sh_adj, int tilesize, int angle, uint8_t *chr_w);



// -----------------------------------------------------------------------------------------------------



// #define TILEREAD_DEBUG_OUT


// px_frame: source image data (top left)
// src_w: source image width
// tw: width
// th: height
// angle: rotation angle (90 degree only)
// chr_w: handle to output buffer
// returns new output buffer handle, advanced by pixel count (sw * sh)
static inline uint8_t *read_tile_px(const uint8_t *px_frame, int src_w, int tw, int th, int angle, uint8_t *chr_w)
{
	const bool yoko = ((angle == 0) || (angle == 180));
	const int touter_lim = yoko ? th : tw;
	const int tinner_lim = yoko ? tw : th;

	// The inner and outer iteration order is based on the orientation
	// of the sprite, as in general the hardware works in yoko terms.

	// Tile outer iteration
	for (int to = 0; to < touter_lim; to++)  // h in yoko, w in tate
	{
		// Tile inner iteration
		for (int ti = 0; ti < tinner_lim; ti++)  // w in yoko, h in tate
		{
			// Index calculation.
			int yoffs = 0;
			int xoffs = 0;
			switch (angle)
			{
				case 0:
					yoffs = to;
					xoffs = ti;
					break;
				case 270:
					xoffs = to;
					yoffs = th - 1 - ti;
					break;
				case 180:
					yoffs = th - 1 - to;
					xoffs = tw - 1 - ti;
					break;
				case 90:
					xoffs = tw - 1 - to;
					yoffs = ti;
					break;
				default:
					break;
			}

			const uint8_t pixel = px_frame[(yoffs*src_w) + xoffs];
			*chr_w++ = pixel;

#ifdef TILEREAD_DEBUG_OUT
			printf("%c", pixel == 0 ? ' ' : '0' + pixel);
#endif  // TILEREAD_DEBUG_OUT

		}

#ifdef TILEREAD_DEBUG_OUT
		printf("\n");
#endif  // TILEREAD_DEBUG_OUT
	}
	return chr_w;
}

// TODO: for CPS SPR, add a tile skip bool.
//       also consider a separate usage counting function, or let this write back to a tile skip array.
//       and, if chr_w is NULL, run as a read-only test run for tile counting.
static inline uint8_t *tile_read_frame(const uint8_t *px,
                                       int png_w, int png_x, int png_y,
                                       int sw_adj, int sh_adj,
                                       int tilesize,
                                       int angle,
                                       uint8_t *chr_w)  // CHR data to write to.
{
	const bool yoko = ((angle == 0) || (angle == 180));

	const int tile_outer_count = (tilesize <= 0) ? 1 : (((yoko ? sh_adj : sw_adj)/tilesize));
	const int tile_inner_count = (tilesize <= 0) ? 1 : (((yoko ? sw_adj : sh_adj)/tilesize));

#ifdef TILEREAD_DEBUG_OUT
	printf("read (%d, %d) size %d, %d (ts=%d) outer %d inner %d\n", png_x*sw_adj, png_y*sh_adj, sw_adj, sh_adj, tilesize, tile_outer_count, tile_inner_count);
#endif  // TILEREAD_DEBUG_OUT

	for (int tile_outer = 0; tile_outer < tile_outer_count; tile_outer++)
	{
		for (int tile_inner = 0; tile_inner < tile_inner_count; tile_inner++)
		{
			int tx, ty;
			switch (angle)
			{
				default:

				case 0:
					ty = tile_outer;
					tx = tile_inner;
					break;

				case 90:
					tx = tile_inner_count - 1 - tile_inner;
					ty = tile_inner;
					break;

				case 180:
					ty = tile_outer_count - 1 - tile_outer;
					tx = tile_inner_count - 1 - tile_inner;
					break;

				case 270:
					tx = tile_outer;
					ty = tile_inner_count - 1 - tile_inner;
					break;
			}

			const int src_y = ((png_y * sh_adj) + (ty * tilesize));
			const int src_x = ((png_x * sw_adj) + (tx * tilesize));

			const int base_idx = (src_y * png_w) + src_x;;
			const uint8_t *px_frame = &px[base_idx];  // Top-left of frame.
			const int clip_w = (tilesize <= 0) ? sw_adj : tilesize;
			const int clip_h = (tilesize <= 0) ? sh_adj : tilesize;
#ifdef TILEREAD_DEBUG_OUT
			printf("  read idx %d, %d --> %d, %d\n", tx, ty, src_x, src_y);
#endif  // TILEREAD_DEBUG_OUT
			chr_w = read_tile_px(px_frame, png_w, clip_w, clip_h, angle, chr_w);
		}
	}
	return chr_w;
}
