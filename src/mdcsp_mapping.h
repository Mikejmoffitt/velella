// Composite sprite mappings.

/*

Header format:
$00    2   ref count (available images)
$02    2   spr list offs (longword)
$04    2   required fixed vram buffer words (for fixed usage)
$06    2   required dma vram buffer words (based on tile count of biggest frame)
$08    ..  ref list
...    ..  spr list

Ref format:
$00    2   hw sprite count
$02    2   spr list offs
$04    2   tile index (within CHR data associated)
$06    2   tile words (DMA size, if used in such a mode)

Spr format:
$00    2   dy
$02    1   size
$03    1   padding
$04    2   attr (relative to ref tile index base + vram load pos)
$06    2   dx
$08    2   flip dy
$0A    2   padding
$0C    2   padding
$0E    2   flip dx

*/

#pragma once

#include "types.h"
#include <stdio.h>
#include "endian.h"

#define MDCSP_HEADER_BYTES 0x08
#define MDCSP_REF_BYTES    0x08
#define MDCSP_SPR_BYTES    0x10

// Returns bytes used for mapping (no writing)
static inline size_t mdcsp_bytes_for_mapping(int ref_count, int spr_count)
{
	return MDCSP_HEADER_BYTES + (ref_count*MDCSP_REF_BYTES) + (spr_count*MDCSP_SPR_BYTES);
}

// Returns bytes used for mapping.
static inline size_t mdcsp_emit_mapping(const Entry *e, FILE *f)
{
	const uint16_t sprlist_offs = MDCSP_HEADER_BYTES + (e->md_csp.ref_count*MDCSP_REF_BYTES);
	const uint16_t fixed_buffer_words = (e->chr_bytes / 2) / (sizeof(uint16_t));
	const uint16_t vram_buffer_words = e->md_csp.dma_buffer_tiles * (32 / sizeof(uint16_t));

	fwrite_uint16be(e->md_csp.ref_count, f);
	fwrite_uint16be(sprlist_offs, f);
	fwrite_uint16be(fixed_buffer_words, f);
	fwrite_uint16be(vram_buffer_words, f);

	// Ref list.
	for (int i = 0; i < e->md_csp.ref_count; i++)
	{
		const MdCspRef *ref = &e->md_csp.ref_dat[i];
		fwrite_uint16be(ref->spr_count, f);
		fwrite_uint16be(ref->spr_index * 16, f);  // * sizeof(spr_def)
		fwrite_uint16be(ref->tile_index, f);  // Tile offset within tile data.
		fwrite_uint16be(ref->tile_count * (32 / sizeof(uint16_t)), f);  // DMA size in words.
	}
	// Sprite list.
	for (int i = 0; i < e->md_csp.spr_count; i++)
	{
		const MdCspSpr *spr = &e->md_csp.spr_dat[i];
		fwrite_int16be(spr->dy, f);
		const uint16_t sizebits = ((spr->h-1)) | ((spr->w-1) << 2);
		fwrite_uint16be(sizebits << 8, f);
		fwrite_uint16be(spr->tile, f);
		fwrite_int16be(spr->dx, f);
		fwrite_int16be(spr->flip_dy, f);
		fwrite_uint16be(0, f);
		fwrite_uint16be(0, f);
		fwrite_int16be(spr->flip_dx, f);
	}

	return mdcsp_bytes_for_mapping(e->md_csp.ref_count, e->md_csp.spr_count);
}
