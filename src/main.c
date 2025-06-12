//
// Velella
// main.c
//
// Copyright 2024-2025 Mike "MOF" Moffitt
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the “Software”), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "inih/ini.h"
#include "lodepng.h"
#include <ctype.h>
#include "pal.h"
#include "tileread.h"
#include "types.h"
#include "entry_emit.h"
#include "format.h"
#include "conv.h"
#include "ini_handler.h"

// =======
// 
// =======
//
// An INI file is (ab)used to act as a script containing conversion information.
//
// A [section] sets the name of a sprite to be worked on. That is, the section
// is adopted as the symbolic name, which will appear in headers and meta
// output files.
// A certain number of required arguments must be set at least once. This includes
// the sprite type, the output CHR filename, output META filename (optional, but
// strongly recommended), output header filename (ditto), sprite size within
// sheet, and most importantly the source filename.
//
// When the line "src" is read, it kicks off the conversion
// using whatever parameters are set. It's OK to start a new sprite section and
// only change parameters that are different from the last one. Similarly, it is
// OK to create a section that only sets parameters and doesn't actually start a
// conversion with "src" to set some metadata.
//
// The output files are derived from the `out` option, giving you:
// * a .chr file for the character data
// * a .pal file containing palette data
// * a .h file for a C header with offsets defined (also GNU AS compatible)
// * a .inc file for an assembly file with offset definitions
//
// Valid options, with example params:
//
// out = res/chr
// tile = 0x1000
// angle = 90
//
// [chr_sprite]
// w = 32
// h = 16
// src = sprite.png

int main(int argc, char **argv)
{
	int ret = -1;
	if (argc < 2)
	{
		printf("Usage: %s CONFIG\n", argv[0]);
		return -1;
	}

	Conv conv;
	if (!conv_init(&conv))
	{
		fprintf(stderr, "Couldn't initialize CONV\n");
		return -1;
	};

	// the actual conversion is kicked off in the INI handler, when `convert` is set.
	ret = ini_parse(argv[1], &ini_handler_func, &conv);
	if (ret)
	{
		fprintf(stderr, "Error parsing \"%s\".\n", argv[0]);
		return -1;
	}

	// Now emit a pile of CHR data
	char fname_buf[512];

	FILE *f_chr = NULL;
	FILE *f_pal = NULL;
	FILE *f_inc = NULL;
	FILE *f_hdr = NULL;

	snprintf(fname_buf, sizeof(fname_buf), "%s.chr", conv.out);
	f_chr = fopen(fname_buf, "wb");
	if (!f_chr)
	{
		fprintf(stderr, "Couldn't open %s for writing\n", fname_buf);
		ret = -1;
		goto done;
	}

	snprintf(fname_buf, sizeof(fname_buf), "%s.pal", conv.out);
	f_pal = fopen(fname_buf, "wb");
	if (!f_pal)
	{
		fprintf(stderr, "Couldn't open %s for writing\n", fname_buf);
		ret = -1;
		goto done;
	}

	snprintf(fname_buf, sizeof(fname_buf), "%s.inc", conv.out);
	f_inc = fopen(fname_buf, "wb");
	if (!f_inc)
	{
		fprintf(stderr, "Couldn't open %s for writing\n", fname_buf);
		ret = -1;
		goto done;
	}

	snprintf(fname_buf, sizeof(fname_buf), "%s.h", conv.out);
	f_hdr = fopen(fname_buf, "wb");
	if (!f_hdr)
	{
		fprintf(stderr, "Couldn't open %s for writing\n", fname_buf);
		ret = -1;
		goto done;
	}

	entry_emit_header_top(f_inc, false);
	entry_emit_header_top(f_hdr, true);

	Entry *e = conv.entry_head;
	int pal_offs = 0;
	while (e)
	{
		printf("Entry $%03X \"%s\": %d x %d, %d frames/tiles\n",
		       e->id, e->symbol, e->frame_cfg.w, e->frame_cfg.h, e->frames);
		entry_emit_pal(e, f_pal, &pal_offs);
		entry_emit_meta(e, &conv, f_inc, e->pal_block_offs, false);
		entry_emit_meta(e, &conv, f_hdr, e->pal_block_offs, true);
		entry_emit_chr(e, f_chr);

		e = e->next;
		if (e)
		{
			entry_emit_header_divider(f_inc, false);
			entry_emit_header_divider(f_hdr, true);
		}
	}

	entry_emit_header_pal_decl(f_hdr, pal_offs, conv.out, true);

done:
	if (f_chr) fclose(f_chr);
	if (f_pal) fclose(f_pal);
	if (f_inc) fclose(f_inc);
	if (f_hdr) fclose(f_hdr);
	conv_shutdown(&conv);

	// Close out data to files

	return ret;
}
