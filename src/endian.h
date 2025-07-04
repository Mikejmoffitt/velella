// Little utilities for working with endiannes

#pragma once

#include <stdio.h>
#include <stdint.h>

// Motorola 68000 uses big-endian data.
static inline void fwrite_uint16be(uint16_t val, FILE *f)
{
	uint8_t buf[2];
	buf[0] = (val >> 8) & 0xFF;
	buf[1] = val & 0xFF;
	fwrite(buf, 1, sizeof(buf), f);
	fflush(f);
}

static inline void fwrite_int16be(int16_t val, FILE *f)
{
	fputc((val >> 8) & 0xFF, f);
	fputc(val & 0xFF, f);
}

static inline void fwrite_uint32be(uint32_t val, FILE *f)
{
	fwrite_uint16be((val >> 16) & 0xFFFF, f);
	fwrite_uint16be(val & 0xFFFF, f);
}
