#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

//
// Palette formats.
//
typedef enum PalFormat
{
	PAL_FORMAT_UNSPECIFIED,
	PAL_FORMAT_ATLUS,
	PAL_FORMAT_X68000,
	PAL_FORMAT_MD,     // Packs things for MD/C2; uses extra precision bits.
	PAL_FORMAT_CPS,    // Fade channel left at $F.
	PAL_FORMAT_TOA,    // GCU-based hardware.
	PAL_FORMAT_NEO,    // Neo-Geo.

	PAL_FORMAT_COUNT
} PalFormat;

bool pal_validate_selection(PalFormat fmt);

uint16_t pal_pack_entry(PalFormat fmt, uint8_t r, uint8_t g, uint8_t b);
void pal_pack_set(PalFormat fmt, const uint8_t *srcpal, uint16_t *destpal, size_t count);

PalFormat pal_format_for_string(const char *str);
const char *pal_string_for_format(PalFormat fmt);
