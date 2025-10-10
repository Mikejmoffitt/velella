#include "pal.h"
#include <string.h>
#include <stdio.h>

static const char *kstr_for_formats[PAL_FORMAT_COUNT] =
{
	[PAL_FORMAT_UNSPECIFIED] = "{unspecified}",
	[PAL_FORMAT_ATLUS] = "atlus",
	[PAL_FORMAT_X68000] = "x68000",
	[PAL_FORMAT_MD] = "md",
	[PAL_FORMAT_CPS] = "cps",
	[PAL_FORMAT_TOA] = "toa",
	[PAL_FORMAT_NEO] = "neo",
};

bool pal_validate_selection(PalFormat fmt)
{
	switch (fmt)
	{
		case PAL_FORMAT_UNSPECIFIED:
			fprintf(stderr, "[pal] No palette format specified!\n");
			return false;
		default:
			break;
	}
	return true;
}

uint16_t pal_pack_entry(PalFormat fmt, uint8_t r, uint8_t g, uint8_t b)
{
	switch (fmt)
	{
		case PAL_FORMAT_ATLUS:
			r = r >> 3;
			g = g >> 3;
			b = b >> 3;
			return (r << 5) | (g << 10) | b;

		case PAL_FORMAT_X68000:
			r = r >> 3;
			g = g >> 3;
			b = b >> 3;
			return (r << 6) | (g << 11) | (b << 1);

		case PAL_FORMAT_MD:
			return ((r >> 3 & 1) ? 0x1000 : 0x0000) |
			       ((g >> 3 & 1) ? 0x2000 : 0x0000) |
			       ((b >> 3 & 1) ? 0x4000 : 0x0000) |
			       (((r >> 4) & 0x0F)) |
			       (((g >> 4) & 0x0F) << 4) |
			       (((b >> 4) & 0x0F) << 8);

		case PAL_FORMAT_CPS:
			r = r >> 4;
			g = g >> 4;
			b = b >> 4;
			return 0xF000 | (r << 8) | (g << 4) | (b);

		case PAL_FORMAT_TOA:
			r = r >> 3;
			g = g >> 3;
			b = b >> 3;
			return (g << 5) | (b << 10) | r;

		case PAL_FORMAT_NEO:
			{
				// Logic taken from the neo geo development wiki. What in the world
				const int luma = (int)(((54.213f*r) + (182.376f*g) + (18.411f*b))) & 1;
				r = r >> 3;
				g = g >> 3;
				b = b >> 3;

				return (luma ? 0x0000 : 0x8000) | 
				       ((r & 0x01) << 14) |
				       ((g & 0x01) << 13) |
				       ((b & 0x01) << 12) |
				       ((r >> 1) << 8) |
				       ((g >> 1) << 4) |
				       ((b >> 1));
				// TODO: Considerations for the dark bit
			}
			break;

		default:
			fprintf(stderr, "[pal] Unhandled palette type %d\n", fmt);
			return 0;
	}
	return 0;
}

void pal_pack_set(PalFormat fmt, const uint8_t *srcpal, uint16_t *destpal, size_t count)
{
	// On CPS, we invert the palette read order so we can pretend index 0 is
	// the transparent key index.
	switch (fmt)
	{
		case PAL_FORMAT_CPS:
			for (size_t i = 0; i < count; i++)
			{
				const int idx_inner = i % 16;
				const int offs = i * 4;
				const uint8_t r = srcpal[offs + 0];
				const uint8_t g = srcpal[offs + 1];
				const uint8_t b = srcpal[offs + 2];
				destpal[15 - idx_inner] = pal_pack_entry(fmt, r, g, b);
			}
			break;
		default:
			for (size_t i = 0; i < count; i++)
			{
				const int offs = i * 4;
				const uint8_t r = srcpal[offs + 0];
				const uint8_t g = srcpal[offs + 1];
				const uint8_t b = srcpal[offs + 2];
				destpal[i] = pal_pack_entry(fmt, r, g, b);
			}
			break;
	}
}

PalFormat pal_format_for_string(const char *str)
{
	for (int i = 0; i < PAL_FORMAT_COUNT; i++)
	{
		if (strcmp(str, kstr_for_formats[i]) == 0) return i;
	}
	return PAL_FORMAT_UNSPECIFIED;
}

const char *pal_string_for_format(PalFormat fmt)
{
	if (fmt >= PAL_FORMAT_COUNT) return kstr_for_formats[PAL_FORMAT_UNSPECIFIED];
	return kstr_for_formats[fmt];
}
