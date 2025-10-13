#include "entry_emit.h"
#include "format.h"
#include <stdlib.h>
#include <string.h>
#include "endian.h"
#include "mdcsp_mapping.h"
#include "pxutil.h"

static const char *get_str_comment(bool c_lang)
{
	return c_lang ? "//" : "; ";
}

static const char *get_str_hex(bool c_lang)
{
	return c_lang ? "0x" : "$ ";
}

static const char *get_str_def(bool c_lang)
{
	return c_lang ? "#define " : "";
}

static const char *get_str_equ(bool c_lang)
{
	return c_lang ? "" : "= ";
}

static inline void emit_code(const Entry *e, bool c_lang, const char *suffix, uint32_t data, FILE *f)
{
	fprintf(f, "%s%s_CODE%s %s%s%X\n",
	        get_str_def(c_lang),
	        e->symbol_upper, suffix,
	        get_str_equ(c_lang),
	        get_str_hex(c_lang), data);
}

static inline void emit_frame_size(const Entry *e, bool c_lang, FILE *f)
{
	const FrameCfg *frame_cfg = &e->frame_cfg;
	const char *k_str_def = get_str_def(c_lang);
	const char *k_str_equ = get_str_equ(c_lang);
	fprintf(f, "%s%s_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->w);
	fprintf(f, "%s%s_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->h);
}

static inline void emit_src_tex_size(const Entry *e, bool c_lang, FILE *f)
{
	const FrameCfg *frame_cfg = &e->frame_cfg;
	const char *k_str_def = get_str_def(c_lang);
	const char *k_str_equ = get_str_equ(c_lang);
	fprintf(f, "%s%s_SRC_TEX_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_w);
	fprintf(f, "%s%s_SRC_TEX_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_h);
}

static inline void emit_tile_count_bg(const Entry *e, bool c_lang, FILE *f)
{
	const FrameCfg *frame_cfg = &e->frame_cfg;
	const char *k_str_def = get_str_def(c_lang);
	const char *k_str_equ = get_str_equ(c_lang);
	// "Tilesize" refers to the conversion perspectivem, whereas the "frame" is used to chop major tiles.
	fprintf(f, "%s%s_TILESIZE %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->w);
	fprintf(f, "%s%s_TILES_W %s%d\n",  k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_w / frame_cfg->w);
	fprintf(f, "%s%s_TILES_H %s%d\n",  k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_h / frame_cfg->h);
}

static inline void emit_frame_metrics(const Entry *e, bool c_lang, uint32_t frame_offs, FILE *f)
{
	const FrameCfg *frame_cfg = &e->frame_cfg;
	const char *k_str_def = get_str_def(c_lang);
	const char *k_str_equ = get_str_equ(c_lang);
	const char *k_str_hex = get_str_hex(c_lang);
	if (frame_offs > 1)
	{
		fprintf(f, "%s%s_FRAME_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_offs);
	}
	fprintf(f, "%s%s_FRAMES %s%d\n", k_str_def, e->symbol_upper, k_str_equ, e->frames);
}

static inline void emit_chr_metrics(const Entry *e, bool c_lang, FILE *f)
{
	const FrameCfg *frame_cfg = &e->frame_cfg;
	const char *k_str_def = get_str_def(c_lang);
	const char *k_str_equ = get_str_equ(c_lang);
	const char *k_str_hex = get_str_hex(c_lang);
	fprintf(f, "%s%s_CHR_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, (uint32_t)e->chr_offs);
	fprintf(f, "%s%s_CHR_BYTES %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, (uint32_t)e->chr_bytes/2);
	fprintf(f, "%s%s_CHR_WORDS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, (uint32_t)e->chr_bytes/4);
}

static inline void emit_size(const Entry *e, bool c_lang, uint32_t size_code, FILE *f)
{
	const char *k_str_def = get_str_def(c_lang);
	const char *k_str_equ = get_str_equ(c_lang);
	const char *k_str_hex = get_str_hex(c_lang);
	fprintf(f, "%s%s_SIZE %s%s%04X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, size_code);
}

void entry_emit_meta(const Entry *e, FILE *f_inc, int pal_offs, bool c_lang)
{
	const FrameCfg *frame_cfg = &e->frame_cfg;

	const char *k_str_comment = get_str_comment(c_lang);
	const char *k_str_def = get_str_def(c_lang);
	const char *k_str_equ = get_str_equ(c_lang);
	const char *k_str_hex = get_str_hex(c_lang);

	// Write inc entry
	fprintf(f_inc, "%s Entry $%03X \"%s\": format \"%s\" %d x %d, %d frames/tiles\n",
	        k_str_comment, e->id, e->symbol,
	        string_for_data_format(e->frame_cfg.data_format),
	        frame_cfg->w, frame_cfg->h, e->frames);
	fprintf(f_inc, "%s%s_PAL_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, pal_offs);
	fprintf(f_inc, "%s%s_PAL_LEN %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, e->pal_size);

	switch (e->frame_cfg.data_format)
	{
		case DATA_FORMAT_SP013:
			emit_code(e,               c_lang, "",    frame_cfg->code, f_inc);
			emit_code(e,               c_lang, "_HI", frame_cfg->code >> 16, f_inc);
			emit_code(e,               c_lang, "_LO", frame_cfg->code & 0xFFFF, f_inc);
			emit_src_tex_size(e,       c_lang, f_inc);
			emit_frame_size(e,         c_lang, f_inc);
			emit_size(e,               c_lang, e->sp013.size_code, f_inc);
			emit_frame_metrics(e,      c_lang, e->code_per, f_inc);
			break;

		case DATA_FORMAT_BG038:
			emit_code(e,               c_lang, "8",    frame_cfg->code, f_inc);
			emit_code(e,               c_lang, "8_HI", frame_cfg->code >> 16, f_inc);
			emit_code(e,               c_lang, "8_LO", frame_cfg->code & 0xFFFF, f_inc);
			emit_code(e,               c_lang, "16",   (frame_cfg->code>>2), f_inc);
			emit_code(e,               c_lang, "16_HI",(frame_cfg->code>>2) >> 16, f_inc);
			emit_code(e,               c_lang, "16_LO",(frame_cfg->code>>2) & 0xFFFF, f_inc);
			emit_src_tex_size(e,       c_lang, f_inc);
			emit_tile_count_bg(e,      c_lang, f_inc);
			break;

		case DATA_FORMAT_DIRECT:
			emit_code(e,               c_lang, "", frame_cfg->code, f_inc);
			emit_chr_metrics(e,        c_lang, f_inc);
//			fprintf(f_inc, "%s%s_DATA_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code*frame_cfg->src_tex_w*frame_cfg->src_tex_h);

			fprintf(f_inc, "%s%s_TILE_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->tilesize*frame_cfg->tilesize);
			emit_frame_size(e,         c_lang, f_inc);
			emit_frame_metrics(e,      c_lang, frame_cfg->w*frame_cfg->h, f_inc);
			break;

		case DATA_FORMAT_CPS_SPR:
			emit_code(e,               c_lang,"",  frame_cfg->code, f_inc);
			emit_src_tex_size(e,       c_lang, f_inc);
			emit_frame_size(e,         c_lang, f_inc);
			emit_size(e,               c_lang, e->cps_spr.size_code, f_inc);
			emit_frame_metrics(e,      c_lang, e->code_per, f_inc);
			break;

		case DATA_FORMAT_CPS_BG:
			// The code is doubled for 8x8 tiles because they're basically internally padded due to the CPS architecture.
			emit_code(e,               c_lang,"",  frame_cfg->code*((e->frame_cfg.tilesize == 8) ? 2 : 1), f_inc);
			emit_src_tex_size(e,       c_lang, f_inc);
			emit_tile_count_bg(e,      c_lang, f_inc);
			break;

		case DATA_FORMAT_MD_SPR:
			emit_code(e,               c_lang,"",  frame_cfg->code, f_inc);
			emit_chr_metrics(e,        c_lang, f_inc);
			emit_src_tex_size(e,       c_lang, f_inc);
			emit_frame_size(e,         c_lang, f_inc);
			emit_size(e,               c_lang, e->md_spr.size_code, f_inc);
			emit_frame_metrics(e,      c_lang, e->code_per, f_inc);
			break;

		case DATA_FORMAT_MD_BG:
		case DATA_FORMAT_TOA_TXT:
		case DATA_FORMAT_NEO_FIX:
			emit_code(e,               c_lang, "", frame_cfg->code, f_inc);
			emit_chr_metrics(e,        c_lang, f_inc);
			emit_src_tex_size(e,       c_lang, f_inc);
			emit_frame_size(e,         c_lang, f_inc);
			emit_tile_count_bg(e,      c_lang, f_inc);
			break;

		case DATA_FORMAT_MD_CSP:
			emit_code(e,               c_lang, "",  frame_cfg->code, f_inc);
			emit_chr_metrics(e,        c_lang, f_inc);
			emit_src_tex_size(e,       c_lang, f_inc);
			emit_frame_size(e,         c_lang, f_inc);
			fprintf(f_inc, "%s%s_MAP_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, (uint32_t)e->map_offs);
			emit_frame_metrics(e,      c_lang, 0, f_inc);
			break;

		case DATA_FORMAT_TOA_GCU_SPR:
			emit_code(e,               c_lang, "",    frame_cfg->code, f_inc);
			emit_code(e,               c_lang, "_HI", frame_cfg->code >> 16, f_inc);
			emit_code(e,               c_lang, "_LO", frame_cfg->code & 0xFFFF, f_inc);
			emit_src_tex_size(e,       c_lang, f_inc);
			emit_frame_size(e,         c_lang, f_inc);
			emit_size(e,               c_lang, e->gcu_spr.size_code, f_inc);
			emit_frame_metrics(e,      c_lang, e->code_per, f_inc);
			break;

		case DATA_FORMAT_TOA_GCU_BG:
			emit_code(e,               c_lang, "",    frame_cfg->code>>2, f_inc);
			emit_src_tex_size(e,       c_lang, f_inc);
			emit_tile_count_bg(e,      c_lang, f_inc);
			break;

		case DATA_FORMAT_NEO_SPR:
			emit_code(e,               c_lang, "",    frame_cfg->code, f_inc);
			emit_code(e,               c_lang, "_MSB", frame_cfg->code >> 16, f_inc);
			emit_code(e,               c_lang, "_MSB_ATTR", (frame_cfg->code >> 16) << 4, f_inc);
			emit_src_tex_size(e,       c_lang, f_inc);
			emit_frame_size(e,         c_lang, f_inc);
			fprintf(f_inc, "%s%s_TILES_W %s%d\n",  k_str_def, e->symbol_upper, k_str_equ, frame_cfg->w / frame_cfg->tilesize);
			fprintf(f_inc, "%s%s_TILES_H %s%d\n",  k_str_def, e->symbol_upper, k_str_equ, frame_cfg->h / frame_cfg->tilesize);
			emit_frame_metrics(e,      c_lang, e->code_per, f_inc);
			break;

		default:
			break;
	}
	fprintf(f_inc, "\n");
}

void entry_emit_chr(const Entry *e, FILE *f_chr)
{
	// Dump CHR data into CHR file(s)
	uint8_t *chr = e->chr;
	switch (e->frame_cfg.data_format)
	{
		// 8bpp as-is
		case DATA_FORMAT_DIRECT:
			for (size_t i = 0; i < e->chr_bytes; i++)
			{
				const uint8_t px = *chr++;
				fputc(px, f_chr);
			}
			break;

		// sp013 special 4bpp/8bpp hybrid
		case DATA_FORMAT_SP013:
			for (size_t i = 0; i < e->chr_bytes/2; i++)
			{
				const uint8_t fetchpx0 = *chr++;
				const uint8_t fetchpx1 = *chr++;

				// Inverted data order for SP013.
				const uint8_t px0 = fetchpx1;
				const uint8_t px1 = fetchpx0;

				const uint8_t lowbyte = ((px0 << 4) & 0xF0) | (px1 & 0x0F);
				const uint8_t hibyte = (px0 & 0xF0) | ((px1 >> 4) & 0x0F);

				fputc(lowbyte, f_chr);
				if (e->frame_cfg.depth == 8) fputc(hibyte, f_chr);
			}
			break;

		case DATA_FORMAT_BG038:
			for (size_t i = 0; i < e->chr_bytes/2; i++)
			{
				const uint8_t px0 = *chr++;
				const uint8_t px1 = *chr++;

				const uint8_t lowbyte = ((px0 << 4) & 0xF0) | (px1 & 0x0F);
				const uint8_t hibyte = (px0 & 0xF0) | ((px1 >> 4) & 0x0F);

				// Put main plane on low bytes, upper 4bpp on even bytes
				fputc(lowbyte, f_chr);
				if (e->frame_cfg.depth == 8) fputc(hibyte, f_chr);
			}
			break;

		case DATA_FORMAT_CPS_BG:
			if (e->frame_cfg.tilesize == 8)
			{
				// CPS-B only selects data from the "even" graphics for 8x8 tiles.
				// Basically, we emit an 8x8 planar tile, followed by a blank tile.
				for (size_t i = 0; i < e->chr_bytes/(8*8); i++)
				{
					for (size_t j = 0; j < 8; j++)
					{
						uint8_t even[4] = {0};
						for (size_t k = 0; k < 8; k++)
						{
							for (int bit = 0; bit < 4; bit++)
							{
								even[bit] = even[bit] << 1;
								even[bit] |= ((chr[(j*8)+k]   & (1<<bit)) ? 1 : 0);
							}
						}
						for (int bit = 0; bit < 4; bit++) fputc(~even[bit], f_chr);
						for (int bit = 0; bit < 4; bit++) fputc(~even[bit], f_chr);
					}
					chr += 8*8;
				}
				break;
			}
			else if (e->frame_cfg.tilesize == 32)
			{
				break;
			}
			__attribute__((fallthrough));

		case DATA_FORMAT_CPS_SPR:
			// spreads across 3-6, we have low 2bpp even tiles, low 2bpp odd tiles, high 2bpp even tiles, high 2bpp odd tiles
			// 16x16 blocks at a time, emitted as planar data.
			// for every 16x16 sprite:
//			printf("CHR bytes: %lu\n", e->chr_bytes);
//			printf("16x16 tile count: %lu\n", e->chr_bytes/(16*16));
			for (size_t i = 0; i < e->chr_bytes/(16*16); i++)
			{
				for (size_t j = 0; j < 16; j++)
				{
					uint8_t even[4] = {0};
					uint8_t odd[4] = {0};
					for (size_t k = 0; k < 8; k++)
					{
						for (int bit = 0; bit < 4; bit++)
						{
							even[bit] = even[bit] << 1;
							odd[bit]  = odd[bit] << 1;
							even[bit] |= ((chr[(j*16)+k]   & (1<<bit)) ? 1 : 0);
							odd[bit]  |= ((chr[(j*16)+k+8] & (1<<bit)) ? 1 : 0);
						}
					}

					for (int bit = 0; bit < 4; bit++) fputc(~even[bit], f_chr);
					for (int bit = 0; bit < 4; bit++) fputc(~odd[bit], f_chr);
				}
				chr += 16*16;
			}
			break;

		case DATA_FORMAT_MD_SPR:
		case DATA_FORMAT_MD_BG:
		case DATA_FORMAT_MD_CSP:
		case DATA_FORMAT_TOA_TXT:
			for (size_t i = 0; i < e->chr_bytes/2; i++)
			{
				const uint8_t px0 = *chr++;
				const uint8_t px1 = *chr++;

				const uint8_t lowbyte = ((px0 << 4) & 0xF0) | (px1 & 0x0F);

				fputc(lowbyte, f_chr);
			}
			break;

		// 4bpp planar
		case DATA_FORMAT_TOA_GCU_SPR:
		case DATA_FORMAT_TOA_GCU_BG:
			for (size_t i = 0; i < (e->chr_bytes)/(8); i++)
			{
				const uint8_t *chr_row = &chr[i*8];

				for (size_t plane = 0; plane < 4; plane++)
				{
					uint8_t row_out = 0;
					const uint8_t mask = 1 << plane;
					for (size_t col = 0; col < 8; col++)
					{
						row_out = row_out << 1;
						if (chr_row[col] & mask) row_out |= 0x01;
					}
					fputc(row_out, f_chr);
				};
			}
			break;

		// Linear, in a funny order
		case DATA_FORMAT_NEO_FIX:
			for (size_t i = 0; i < e->chr_bytes/(8*8); i++)
			{
				const uint8_t *chr_tile = &chr[i*8*8];
				static const int column_pair_order_tbl[4] = {2, 3, 0, 1};
				for (int colset = 0; colset < 4; colset++)
				{
					for (int row = 0; row < 8; row++)
					{
						const int source_x_offset = column_pair_order_tbl[colset]*2;

						const uint8_t px_lo = (chr_tile[(row*8) + source_x_offset +1] & 0xF) << 4;
						const uint8_t px_hi = (chr_tile[(row*8) + source_x_offset] & 0xF);
						fputc(px_lo | px_hi, f_chr);
					}
				}
			}
			break;

		// Planar
		case DATA_FORMAT_NEO_SPR:
			for (size_t i = 0; i < e->chr_bytes/(16*16); i++)  // per tile
			{
				const uint8_t *chr_tile16 = &chr[i*16*16];
				for (int tx = 1; tx >= 0; tx--)
				{
					for (int y = 0; y < 16; y++)
					{
						const uint8_t *chr_row = &chr_tile16[(tx*8)+(y*16)];
						uint8_t row_buffer[4];  // sized for 8 px @ 4bpp
						pxutil_pack_planar(chr_row, 4, 0x3120, true, row_buffer);
					fwrite(row_buffer, 1, sizeof(row_buffer), f_chr);
					}
				}
			}
			break;

		default:
			break;
	}
}

void entry_emit_pal(Entry *e, FILE *f_pal, int *pal_offs)
{
	// If this entry references another's palette, copy the entry offs, and
	// do not add palette data.
	if (e->pal_ref)
	{
		Entry *f = e->pal_ref;
		e->pal_block_offs = f->pal_block_offs;
	}
	else
	{
		// Otherwise, mark palette offs by the output position and write
		// unique palette data.
		e->pal_block_offs = *pal_offs;
		for (int i = 0; i < e->pal_size; i++)
		{
			fwrite_uint16be(e->pal[i], f_pal);
		}
		*pal_offs += e->pal_size * sizeof(uint16_t);
	}
}

void entry_emit_header_top(FILE *f, bool c_lang)
{
	if (c_lang)
	{
		fprintf(f, "#pragma once\n");
		fprintf(f, "#ifndef __ASSEMBLER__\n");
		fprintf(f, "#include <stdint.h>\n");
		fprintf(f, "#endif  // __ASSEMBLER__\n");
		fprintf(f, "\n");
	}
	const char *str_comment = c_lang ? "//" : ";";
	fprintf(f, "%s ┌───────────────────────────────────────────────────┐\n", str_comment);
	fprintf(f, "%s │ This file was automatically generated by Velella. │\n", str_comment);
	fprintf(f, "%s └───────────────────────────────────────────────────┘\n", str_comment);
	fprintf(f, "\n");
}

void entry_emit_header_divider(FILE *f, bool c_lang)
{
	const char *str_comment = c_lang ? "//" : ";";
	fprintf(f, "%s ─────────────────────────────────────────────────────\n", str_comment);
}

// Replace slashes in name with underscores
char *sym_underscore_conversion(const char *sym_name)
{
	char *sym_buf = malloc(strlen(sym_name)+1);
	strcpy(sym_buf, sym_name);
	char *sym_buf_walk = sym_buf;
	while (*sym_buf_walk)
	{
		if (*sym_buf_walk == '/') *sym_buf_walk = '_';
		sym_buf_walk++;
	}
	return sym_buf;
}

void entry_emit_header_data_decl(FILE *f, size_t pal_offs, size_t map_offs,
                                 const char *sym_name, bool c_lang)
{
	char *sym_buf = sym_underscore_conversion(sym_name);
	if (pal_offs > 0)
	{
		if (c_lang)
		{
			fprintf(f, "// Palette block forward declaration.\n");
			fprintf(f, "#ifndef __ASSEMBLER__\n");
			fprintf(f, "extern const uint16_t %s_pal[0x%X/2];\n", sym_buf, (uint32_t)pal_offs);
			fprintf(f, "#else\n");
			fprintf(f, "\t.extern\t%s_pal  // %d bytes\n", sym_buf, (uint32_t)pal_offs);
			fprintf(f, "#endif  // __ASSEMBLER__\n");
			fprintf(f, "#define k_%s_pal_bytes (%d)\n", sym_buf, (uint32_t)pal_offs);
			fprintf(f, "\n");
			fprintf(f, "// Palette access macro by resource name.\n");
			fprintf(f, "#ifndef __ASSEMBLER__\n");
			fprintf(f, "#define vel_get_%s_pal(_resname) &%s_pal[_resname##_PAL_OFFS/2]\n", sym_buf, sym_buf);
			fprintf(f, "#else\n");
			fprintf(f, "#define vel_get_%s_pal(_resname) (%s_pal+_resname##_PAL_OFFS)\n", sym_buf, sym_buf);
			fprintf(f, "#endif\n");
			fprintf(f, "\n");
		}
	}
	if (map_offs > 0)
	{
		if (c_lang)
		{
			fprintf(f, "// Mapping block forward declaration.\n");
			fprintf(f, "#ifndef __ASSEMBLER__\n");
			fprintf(f, "extern const uint16_t %s_map[0x%X/2];\n", sym_buf, (uint32_t)map_offs);
			fprintf(f, "#else\n");
			fprintf(f, "\t.extern\t%s_map  // %d bytes\n", sym_buf, (uint32_t)map_offs);
			fprintf(f, "#endif  // __ASSEMBLER__\n");
			fprintf(f, "#define k_%s_map_bytes (%d)\n", sym_buf, (uint32_t)map_offs);
			fprintf(f, "\n");
			fprintf(f, "// Mapping access macro by resource name.\n");
			fprintf(f, "#ifndef __ASSEMBLER__\n");
			fprintf(f, "#define vel_get_%s_map(_resname) &%s_map[_resname##_MAP_OFFS/2]\n", sym_buf, sym_buf);
			fprintf(f, "#else\n");
			fprintf(f, "#define vel_get_%s_map(_resname) (%s_map+_resname##_MAP_OFFS)\n", sym_buf, sym_buf);
			fprintf(f, "#endif\n");
			fprintf(f, "\n");
		}
	}
	free(sym_buf);
}

// Type declarations
void entry_emit_type_decl(FILE *f, DataFormat fmt, bool c_lang)
{
	switch (fmt)
	{
		case DATA_FORMAT_CPS_SPR:
			if (c_lang)
			{
				fprintf(f, "#ifndef __ASSEMBLER__\n");
				fprintf(f, "typedef struct VelCpsObj\n"
				           "{\n"
				           "\tuint16_t code;  // Tile code\n"
				           "\tuint16_t size;  // Size attribute bits\n"
				           "\tuint8_t ts[0];  // Variable bitfield of tiles to skip indexed by row.\n"
				           "} VelCpsObj;\n");
				fprintf(f, "#else\n");
				fprintf(f, "\t.struct\t0\n"
				           "VelCpsObj.code: ds.w 1  // Tile code\n"
				           "VelCpsObj.size: ds.w 1  // Size attribute bits\n"
				           "VelCpsObj.ts:           // Variable bitfield of tiles to skip indexed by row.\n"
				           "\n");
				fprintf(f, "#endif // __ASSEMBLER__\n");
			}
			break;

		default:
			break;
	}
}

void entry_emit_header_chr_size(FILE *f, const char *sym_name, size_t bytes)
{
	char *sym_buf = sym_underscore_conversion(sym_name);
	fprintf(f, "// Character data block forward declaration.\n");
	fprintf(f, "#ifndef __ASSEMBLER__\n");
	fprintf(f, "extern const uint16_t %s_chr[0x%lX/2];\n", sym_buf, bytes);
	fprintf(f, "#else\n");
	fprintf(f, "\t.extern\t%s_chr\n", sym_buf);
	fprintf(f, "#endif  // __ASSEMBLER__\n");
	fprintf(f, "\n");
	fprintf(f, "// Mapping access macro by resource name.\n");
	fprintf(f, "#ifndef __ASSEMBLER__\n");
	fprintf(f, "#define vel_get_%s_chr(_resname) &%s_chr[_resname##_CHR_OFFS/2]\n", sym_buf, sym_buf);
	fprintf(f, "#else\n");
	fprintf(f, "#define vel_get_%s_chr(_resname) (%s_chr+_resname##_CHR_OFFS)\n", sym_buf, sym_buf);
	fprintf(f, "#endif\n");
	fprintf(f, "\n");
	free(sym_buf);
}

void entry_emit_map(const Entry *e, FILE *f_map)
{
	switch (e->frame_cfg.data_format)
	{
		case DATA_FORMAT_MD_CSP:
			mdcsp_emit_mapping(e, f_map);
			break;
		default:
			break;
	}
}
