//
// Structure and functions for tiles and tilemaps, used for optimizing
// tilesets and generating mappings.
//
#pragma once

#define TILE_SIZE_MAX 32

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define TILE_LIST_MAX 1024

#define TILE_ATTR_FLIP_H 0x0001
#define TILE_ATTR_FLIP_V 0x0002

typedef enum TileOptLevel
{
	TILE_OPT_NONE,
	TILE_OPT_DUPLICATE,
	TILE_OPT_DUPLICATE_FLIP
} TileOptLevel;

typedef struct Tile
{
	// Pixel data, simplified to 8bpp (one byte per dot).
	uint8_t *chr;
	int16_t tilesize;  // tile dimensions in pixels.
	int16_t index;    // Index within character bank.

	// Hashes for the tile in all four orientations.
	uint32_t hash;
	uint32_t hash_hf;
	uint32_t hash_vf;
	uint32_t hash_hvf;
} Tile;

typedef struct TileList
{
	Tile tiles[TILE_LIST_MAX];
	int tilesize;
	TileOptLevel opt;
	size_t count;
} TileList;

typedef struct TileListAddResult
{
	int16_t idx;   // Index within tile list. -1 if no good.
	uint16_t attr;  // flip flags.
} TileListAddResult;


// Configure the tile list.
void tile_list_init(TileList *l, int tilesize, TileOptLevel opt);

// Free all the tiles initialized within the tile list.
void tile_list_shutdown(TileList *l);

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
TileListAddResult tile_list_add(TileList *tl, const uint8_t *chr);

void tile_list_write_to_file(const TileList *tl, FILE *f);
