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

	// The bitmap data.
	uint8_t *chr;
	size_t chr_bytes;

	// Mapping.
	int mapping_offs;
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
} Conv;
