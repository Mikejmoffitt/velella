#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "format.h"
#include "pal.h"

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
	int depth;                 // Bits per pixel.
	PalFormat pal_format;
	DataFormat data_format;
	
} FrameCfg;

// MD CSP reference struct. This defines a composite sprite frame,
// referring to which sprite(s) within the mapping block area to use.
typedef struct MdCspRef
{
	uint16_t spr_count;  // Sprite quantity used.
	uint16_t spr_index;  // Starting index of MdCspSpr entries.
	uint16_t tile_index;  // Tile offset within data block for this entry.
	uint16_t tile_count;  // Tile count, used for DMA.
} MdCspRef;

#define MDCSP_MAX_REF_COUNT 0x100
#define MDCSP_MAX_SPR_COUNT 0x100

// MD CSP spr struct. This defines how to place one sprite as part of a
// metasprite. One or more of these are referenced by a MdCspRef entry.
typedef struct MdCspSpr
{
	int16_t dx, dy;  // Relative offsets.
	int16_t w, h;    // In tiles.
	uint16_t tile;   // From the start of the VRAM for the CSP Ref.
	int16_t flip_dx, flip_dy;
} MdCspSpr;

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
	struct
	{
		uint16_t size_code;
	} cps_spr;
	struct
	{
		uint16_t size_code;
	} md_spr;
	struct
	{
		MdCspRef ref_dat[MDCSP_MAX_REF_COUNT];
		int ref_count;
		MdCspSpr spr_dat[MDCSP_MAX_SPR_COUNT];
		int spr_count;
		int tile_count;
		int dma_buffer_tiles;  // High score of tile count through all refs
	} md_csp;
	struct
	{
		uint32_t size_code;  // X in the upper word.
	} gcu_spr;
	struct
	{
		// each bit represents a tile within a vertical strip that can be skipped.
		// the index within the table is the X sprite index.
		// this gives a maximum sprite width of 512px.
		uint32_t skip_tbl[32];
		uint32_t start_tile_idx;
	} neo_cspr;

	// The bitmap data.
	uint8_t *chr;
	size_t chr_offs;
	size_t chr_bytes;

	// Mapping.
	size_t map_offs;
	size_t map_bytes;
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

	// Config state from ini for next entry
	char src[256];           // Source image filename.
	char symbol[256];        // Symbol name (section).
	FrameCfg frame_cfg;

	size_t chr_pos;
	size_t map_pos;
} Conv;
