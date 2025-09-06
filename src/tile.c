#include "tile.h"
#include "crc32.h"

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

static void chr_flip_copy(const uint8_t *chr, uint8_t *flip_chr, int tilesize, bool hf, bool vf)
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

bool tile_init(Tile *t, const uint8_t *chr, int tilesize)
{
	memset(t, 0, sizeof(*t));

	t->bytes = tilesize * tilesize * sizeof(uint8_t);
	t->tilesize = tilesize;
	t->chr = malloc(t->bytes);

	if (!t->chr)
	{
		fprintf(stderr, "Failed to allocate %dx%d tile buffer...\n",
		        t->tilesize, t->tilesize);
		return false;
	}

	memcpy(t->chr, chr, t->bytes);

	// Hash original data
	t->hash = crc32_bytes(t->chr, t->bytes);

	// Hash all flip orientations
	uint8_t *flip_buf = malloc(t->bytes);
	if (!flip_buf)
	{
		fprintf(stderr, "Failed to allocate %dx%d flip buffer...\n",
		        t->tilesize, t->tilesize);
		free(t->chr);
		return false;
	}

	chr_flip_copy(t->chr, flip_buf, t->tilesize, /*hf=*/true, /*vf=*/false);
	t->hash_hf = crc32_bytes(flip_buf, t->bytes);
	chr_flip_copy(t->chr, flip_buf, t->tilesize, /*hf=*/false, /*vf=*/true);
	t->hash_vf = crc32_bytes(flip_buf, t->bytes);
	chr_flip_copy(t->chr, flip_buf, t->tilesize, /*hf=*/true, /*vf=*/true);
	t->hash_hvf = crc32_bytes(flip_buf, t->bytes);
	free(flip_buf);
	return true;
}

void tile_shutdown(Tile *t)
{
	free(t->chr);
}
