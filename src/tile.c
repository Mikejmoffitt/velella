#include "tile.h"
#include "crc32.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

//
// Tiles
//

// Copies flipped tiledata from chr into flip_chr.
static inline void chr_flip_copy(const uint8_t *chr, uint8_t *flip_chr,
                                 int tilesize, bool hf, bool vf)
{
	for (int y = 0; y < tilesize; y++)
	{
		for (int x = 0; x < tilesize; x++)
		{
			const int y_idx = (vf ? (tilesize-1-y) : y) * tilesize;
			const int x_idx = (hf ? (tilesize-1-x) : x);
			flip_chr[y_idx + x_idx] = chr[y_idx + x_idx];
		}
	}
}


static bool tile_init(Tile *t, const uint8_t *chr, int tilesize)
{
	*t = (Tile){ 0 };

	const size_t bytes = tilesize * tilesize * sizeof(uint8_t);
	t->tilesize = tilesize;
	t->chr = malloc(bytes);

	if (!t->chr)
	{
		fprintf(stderr, "Failed to allocate %dx%d tile buffer...\n",
		        t->tilesize, t->tilesize);
		return false;
	}

	uint8_t *flip_buf = malloc(bytes);
	if (!flip_buf)
	{
		fprintf(stderr, "Failed to allocate %dx%d flip buffer...\n",
		        t->tilesize, t->tilesize);
		free(t->chr);
		return false;
	}

	memcpy(t->chr, chr, bytes);

	// Hash original data
	t->hash = crc32_bytes(t->chr, bytes);

	// Hash all flip orientations
	chr_flip_copy(t->chr, flip_buf, t->tilesize, /*hf=*/true, /*vf=*/false);
	t->hash_hf = crc32_bytes(flip_buf, bytes);
	chr_flip_copy(t->chr, flip_buf, t->tilesize, /*hf=*/false, /*vf=*/true);
	t->hash_vf = crc32_bytes(flip_buf, bytes);
	chr_flip_copy(t->chr, flip_buf, t->tilesize, /*hf=*/true, /*vf=*/true);
	t->hash_hvf = crc32_bytes(flip_buf, bytes);
	free(flip_buf);
	return true;
}

static void tile_shutdown(Tile *t)
{
	free(t->chr);
}

static bool tile_compare(const Tile *ta, const Tile *tb, uint16_t *attr, TileOptLevel opt)
{
	if (opt >= TILE_OPT_DUPLICATE && ta->hash == tb->hash)
	{
		*attr = 0;
		return true;
	}
	if (opt >= TILE_OPT_DUPLICATE_FLIP)
	{
		uint16_t found_attr = 0;
		if (ta->hash == tb->hash_hf) found_attr = TILE_ATTR_FLIP_H;
		else if (ta->hash == tb->hash_vf) found_attr = TILE_ATTR_FLIP_V;
		else if (ta->hash == tb->hash_hvf) found_attr = TILE_ATTR_FLIP_H | TILE_ATTR_FLIP_V;
		if (found_attr > 0)
		{
			*attr = found_attr;
			return true;
		}
	}
	return false;
}

//
// Tile List
//

// Configure the tile list.
void tile_list_init(TileList *l, int tilesize, TileOptLevel opt)
{
	l->tilesize = tilesize;
	l->opt = opt;
	l->count = 0;
}

// Free all the tiles initialized within the tile list.
void tile_list_shutdown(TileList *l)
{
	for (size_t i = 0; i < l->count; i++)
	{
		tile_shutdown(&l->tiles[i]);
	}
}

// Adds character data to the tile list, and returns a struct specifying
// metadata that can inform the creation of a tilemap or draw instruction.
//
// If optimization has been turned on, and the tile matches one already in the
// list, then the data is not added and the index and appropriate attributes
// associated with the already prsent tile are returned.
//
// Inputs:  tl: TileList structure.
//         chr: 8bpp pixel data, where size is tl->tilesize^2.
//
TileListAddResult tile_list_add(TileList *tl, const uint8_t *chr)
{
	TileListAddResult ret = {-1, 0};
	Tile tile_new;
	if (!tile_init(&tile_new, chr, l->tilesize)) return ret;

	if (l->opt > TILE_OPT_NONE)
	{
		// See if we have this tile already, and if so, free the new tile
		// and return the index of the original copy.
		for (size_t i = 0; i < l->count; i++)
		{
			Tile *old_tile = &l->tiles[i];
			if (tile_compare(tile_new, old_tile, &ret.attr, l->opt))
			{
				ret.idx = i;
				tile_shutdown(tile_new);
				break;
			}
		}
	}

	// Make sure we have space for this.
	if (l->count >= TILE_LIST_MAX)
	{
		tile_shutdown(tile_new);
		return ret;
	}

	// Keep the new tile in the list, and we're done.
	l->tiles[l->count] = tile_new;
	ret.idx = l->count;
	l->count++;
	return ret;

}

void tile_list_write_to_file(const TileList *tl, FILE *f)
{

}
