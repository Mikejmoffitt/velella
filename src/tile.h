//
// Structure and functions for tiles and tilemaps, used for optimizing
// tilesets and generating mappings.
//
#pragma once

#define TILE_SIZE_MAX 32

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct Tile
{
	// Pixel data, simplified to 8bpp (one byte per dot).
	uint8_t *chr;
	size_t bytes;
	int tilesize;  // tile dimensions in pixels.
	// Hashes for the tile in all four orientations.
	uint32_t hash;
	uint32_t hash_hf;
	uint32_t hash_vf;
	uint32_t hash_hvf;
} Tile;

bool tile_init(Tile *t, const uint8_t *chr, int tilesize);
void tile_shutdown(Tile *t);
