#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum
{
	TILE_READ_FLAG_X_MAJOR = 0x0001,
	TILE_READ_FLAG_ERASE   = 0x0002,  // Erase the data read from the image data.
	TILE_READ_POS_DIRECT   = 0x0004,  // source x/y specified in pixels, not frame slices.
};

static inline uint8_t *tile_read_tile(uint8_t *px_frame, int src_w,
                                      int tw, int th,
                                      int tw_lim, int th_lim,
                                      int angle,
                                      uint32_t flags, uint8_t *chr_w);
static inline uint8_t *tile_read_frame(uint8_t *px,
                                       int png_w, int png_h,
                                       int png_x, int png_y,
                                       int sw_adj, int sh_adj,
                                       int tilesize, int angle,
                                       int lim_x, int lim_y,
                                       uint32_t flags, uint8_t *chr_w);



// -----------------------------------------------------------------------------------------------------


// #define TILEREAD_DEBUG_OUT


// px_frame: source image data (top left)
// src_w: source image width
// src_h: source image height
// tw: width
// th: height
// angle: rotation angle (90 degree only)
// chr_w: handle to output buffer
// returns new output buffer handle, advanced by pixel count (sw * sh)
static inline uint8_t *tile_read_tile(uint8_t *px_frame, int src_w,
                                      int tw, int th,
                                      int tw_lim, int th_lim,
                                      int angle,
                                      uint32_t flags, uint8_t *chr_w)
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

			if (xoffs >= tw_lim || yoffs >= th_lim)
			{
				*chr_w++ = 0;
#ifdef TILEREAD_DEBUG_OUT
				printf(" ");
#endif  // TILEREAD_DEBUG_OUT
			}
			else
			{
				const unsigned int px_idx = (yoffs*src_w) + xoffs;
				const uint8_t pixel = px_frame[px_idx];
				*chr_w++ = pixel;
				if (flags & TILE_READ_FLAG_ERASE) px_frame[px_idx] = 0;
#ifdef TILEREAD_DEBUG_OUT
				printf("%c", pixel == 0 ? ' ' : '0' + pixel);
#endif  // TILEREAD_DEBUG_OUT
			};

		}

#ifdef TILEREAD_DEBUG_OUT
		printf("\n");
#endif  // TILEREAD_DEBUG_OUT
	}
	return chr_w;
}

static inline void get_tx_ty(int tile_inner, int tile_inner_count,
                             int tile_outer, int tile_outer_count,
                             int angle, bool y_major,
                             int *tx, int *ty)
{
	if (y_major)
	{
		switch (angle)
		{
			default:

			case 0:
				*ty = tile_outer;
				*tx = tile_inner;
				break;

			case 90:
				*tx = tile_inner_count - 1 - tile_inner;
				*ty = tile_inner;
				break;

			case 180:
				*ty = tile_outer_count - 1 - tile_outer;
				*tx = tile_inner_count - 1 - tile_inner;
				break;

			case 270:
				*tx = tile_outer;
				*ty = tile_inner_count - 1 - tile_inner;
				break;
		}
	}
	else
	{
		switch (angle)
		{
			default:

			case 0:
				*ty = tile_inner;
				*tx = tile_outer;
				break;

			case 90:
				*tx = tile_outer_count - 1 - tile_outer;
				*ty = tile_outer;
				break;

			case 180:
				*ty = tile_inner_count - 1 - tile_inner;
				*tx = tile_outer_count - 1 - tile_outer;
				break;

			case 270:
				*tx = tile_inner;
				*ty = tile_outer_count - 1 - tile_outer;
				break;
		}
	}
}

// TODO: for CPS SPR, add a tile skip bool.
//       also consider a separate usage counting function, or let this write back to a tile skip array.
//       and, if chr_w is NULL, run as a read-only test run for tile counting.
static inline uint8_t *tile_read_frame(uint8_t *px,
                                       int png_w, int png_h,
                                       int png_x, int png_y,
                                       int sw_adj, int sh_adj,
                                       int tilesize,
                                       int angle,
                                       int lim_x, int lim_y,
                                       uint32_t flags,
                                       uint8_t *chr_w)
{
	const bool yoko = ((angle == 0) || (angle == 180));

	const bool x_major = (flags & TILE_READ_FLAG_X_MAJOR) ? true : false;
	const bool y_major = (yoko && !x_major) || (!yoko && x_major);

	const bool coords_direct = flags &TILE_READ_POS_DIRECT;
	const int src_x_coef = coords_direct ? 1 : sw_adj;
	const int src_y_coef = coords_direct ? 1 : sh_adj;

	const int tile_outer_count = (tilesize <= 0) ? 1 : (((y_major ? sh_adj : sw_adj)/tilesize));
	const int tile_inner_count = (tilesize <= 0) ? 1 : (((y_major ? sw_adj : sh_adj)/tilesize));

#ifdef TILEREAD_DEBUG_OUT
	printf("read (%d, %d) size %d, %d (ts=%d) outer %d inner %d lim %d, %d\n", png_x*src_x_coef, png_y*src_x_coef, sw_adj, sh_adj, tilesize, tile_outer_count, tile_inner_count, lim_x, lim_y);
#endif  // TILEREAD_DEBUG_OUT

	for (int tile_outer = 0; tile_outer < tile_outer_count; tile_outer++)
	{
		for (int tile_inner = 0; tile_inner < tile_inner_count; tile_inner++)
		{
			int tx, ty;
			get_tx_ty(tile_inner, tile_inner_count,
			          tile_outer, tile_outer_count,
			          angle, y_major, &tx, &ty);

			const int src_y = ((png_y * src_y_coef) + (ty * tilesize));
			const int src_x = ((png_x * src_x_coef) + (tx * tilesize));

			const int base_idx = (src_y * png_w) + src_x;
			uint8_t *px_frame = &px[base_idx];  // Top-left of frame.
			int clip_w = (tilesize <= 0) ? sw_adj : tilesize;
			int clip_h = (tilesize <= 0) ? sh_adj : tilesize;

			int clip_lim_w = clip_w;
			int clip_lim_h = clip_h;

#ifdef TILEREAD_DEBUG_OUT
			printf("src %d, %d : %dx%d --> ", src_x, src_y, clip_lim_w, clip_lim_h);
#endif  // TILEREAD_DEBUG_OUT
			if (lim_x >= 0 && src_x + clip_lim_w > lim_x) clip_lim_w = lim_x - src_x;
			if (lim_y >= 0 && src_y + clip_lim_h > lim_y) clip_lim_h = lim_y - src_y;
#ifdef TILEREAD_DEBUG_OUT
			printf(" %d, %d : %dx%d\n", src_x, src_y, clip_lim_w, clip_lim_h);
#endif  // TILEREAD_DEBUG_OUT


#ifdef TILEREAD_DEBUG_OUT
			printf("  read idx %d, %d --> %d, %d\n", tx, ty, src_x, src_y);
#endif  // TILEREAD_DEBUG_OUT
			chr_w = tile_read_tile(px_frame, png_w, clip_w, clip_h, clip_lim_w, clip_lim_h, angle, flags, chr_w);
		}
	}
	return chr_w;
}
