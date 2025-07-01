#include "entry_emit.h"
#include "format.h"
#include <stdlib.h>
#include <string.h>

void entry_emit_meta(const Entry *e, const Conv *conv, FILE *f_inc, int pal_offs, bool c_lang)
{
	const FrameCfg *frame_cfg = &e->frame_cfg;

	const char *k_str_comment = c_lang ? "//" : "; ";
	const char *k_str_hex = c_lang ? "0x" : "$";
	const char *k_str_def = c_lang ? "#define " : "";
	const char *k_str_equ = c_lang ? "" : "= ";

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
			fprintf(f_inc, "%s%s_CODE %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code);
			fprintf(f_inc, "%s%s_CODE_HI %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code >> 16);
			fprintf(f_inc, "%s%s_CODE_LOW %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code & 0xFFFF);
			fprintf(f_inc, "%s%s_SRC_TEX_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_w);
			fprintf(f_inc, "%s%s_SRC_TEX_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_h);
			fprintf(f_inc, "%s%s_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->w);
			fprintf(f_inc, "%s%s_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->h);
			fprintf(f_inc, "%s%s_SIZE %s%s%04X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, e->sp013.size_code);
			fprintf(f_inc, "%s%s_FRAME_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, e->code_per);
			fprintf(f_inc, "%s%s_FRAMES %s%d\n", k_str_def, e->symbol_upper, k_str_equ, e->frames);
			break;

		case DATA_FORMAT_BG038:
			fprintf(f_inc, "%s%s_CODE8 %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code);
			fprintf(f_inc, "%s%s_CODE8_HI %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code >> 16);
			fprintf(f_inc, "%s%s_CODE8_LOW %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code & 0xFFFF);
			fprintf(f_inc, "%s%s_CODE16 %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code / 4);
			fprintf(f_inc, "%s%s_CODE16_HI %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, (frame_cfg->code / 4) >> 16);
			fprintf(f_inc, "%s%s_CODE16_LOW %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, (frame_cfg->code / 4) & 0xFFFF);
			fprintf(f_inc, "%s%s_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_w);
			fprintf(f_inc, "%s%s_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_h);
			fprintf(f_inc, "%s%s_TILESIZE %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->w);  // "Tilesize" refers to the conversion perspective
			fprintf(f_inc, "%s%s_TILES_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_w / frame_cfg->w);  // whereas the "frame" is used to chop major tiles
			fprintf(f_inc, "%s%s_TILES_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_h / frame_cfg->w);
			break;

		case DATA_FORMAT_DIRECT:
			fprintf(f_inc, "%s%s_DATA_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code*frame_cfg->src_tex_w*frame_cfg->src_tex_h);
			fprintf(f_inc, "%s%s_FRAME_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->w*frame_cfg->h);
			fprintf(f_inc, "%s%s_TILE_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->tilesize*frame_cfg->tilesize);
			fprintf(f_inc, "%s%s_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->w);
			fprintf(f_inc, "%s%s_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->h);
			fprintf(f_inc, "%s%s_FRAMES %s%d\n", k_str_def, e->symbol_upper, k_str_equ, e->frames);
			break;

		case DATA_FORMAT_CPS_SPR:
			fprintf(f_inc, "%s%s_CODE %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code);
			fprintf(f_inc, "%s%s_SRC_TEX_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_w);
			fprintf(f_inc, "%s%s_SRC_TEX_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_h);
			fprintf(f_inc, "%s%s_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->w);
			fprintf(f_inc, "%s%s_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->h);
			fprintf(f_inc, "%s%s_SIZE %s%s%02X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, e->cps_spr.size_code);
			fprintf(f_inc, "%s%s_FRAME_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, e->code_per);
			fprintf(f_inc, "%s%s_FRAMES %s%d\n", k_str_def, e->symbol_upper, k_str_equ, e->frames);
			break;

		case DATA_FORMAT_CPS_BG:
			// The code is doubled for 8x8 tiles because they're basically internally padded due to the CPS architecture.
			fprintf(f_inc, "%s%s_CODE %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code*((e->frame_cfg.tilesize == 8) ? 2 : 1));
			fprintf(f_inc, "%s%s_SRC_TEX_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_w);
			fprintf(f_inc, "%s%s_SRC_TEX_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_h);
			fprintf(f_inc, "%s%s_TILESIZE %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->w);  // "Tilesize" refers to the conversion perspective
			fprintf(f_inc, "%s%s_TILES_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_w / frame_cfg->w);  // whereas the "frame" is used to chop major tiles
			fprintf(f_inc, "%s%s_TILES_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_h / frame_cfg->w);
			break;

		case DATA_FORMAT_MD_SPR:
			fprintf(f_inc, "%s%s_CHR_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code*32);
			fprintf(f_inc, "%s%s_CHR_BYTES %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, e->code_per*32*e->frames);
			fprintf(f_inc, "%s%s_CHR_WORDS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, e->code_per*32*e->frames/2);
			fprintf(f_inc, "%s%s_SRC_TEX_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_w);
			fprintf(f_inc, "%s%s_SRC_TEX_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_h);
			fprintf(f_inc, "%s%s_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->w);
			fprintf(f_inc, "%s%s_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->h);
			fprintf(f_inc, "%s%s_SIZE %s%s%02X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, e->md_spr.size_code);
			fprintf(f_inc, "%s%s_FRAME_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, e->code_per);
			fprintf(f_inc, "%s%s_FRAMES %s%d\n", k_str_def, e->symbol_upper, k_str_equ, e->frames);
			break;

		case DATA_FORMAT_MD_BG:
			fprintf(f_inc, "%s%s_CHR_OFFS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, frame_cfg->code*32);
			fprintf(f_inc, "%s%s_CHR_BYTES %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, e->code_per*32*e->frames);
			fprintf(f_inc, "%s%s_CHR_WORDS %s%s%X\n", k_str_def, e->symbol_upper, k_str_equ, k_str_hex, e->code_per*32*e->frames/2);
			fprintf(f_inc, "%s%s_SRC_TEX_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_w);
			fprintf(f_inc, "%s%s_SRC_TEX_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_h);
			fprintf(f_inc, "%s%s_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_w);
			fprintf(f_inc, "%s%s_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_h);
			fprintf(f_inc, "%s%s_TILESIZE %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->w);  // "Tilesize" refers to the conversion perspective
			fprintf(f_inc, "%s%s_TILES_W %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_w / frame_cfg->w);  // whereas the "frame" is used to chop major tiles
			fprintf(f_inc, "%s%s_TILES_H %s%d\n", k_str_def, e->symbol_upper, k_str_equ, frame_cfg->src_tex_h / frame_cfg->w);
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
		case DATA_FORMAT_DIRECT:
			for (size_t i = 0; i < e->chr_bytes; i++)
			{
				const uint8_t px = *chr++;
				fputc(px, f_chr);
			}
			break;

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
			// 16x16 blocsk at a time, emitted as planar data.
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
			for (size_t i = 0; i < e->chr_bytes/2; i++)
			{
				const uint8_t px0 = *chr++;
				const uint8_t px1 = *chr++;

				const uint8_t lowbyte = ((px0 << 4) & 0xF0) | (px1 & 0x0F);

				fputc(lowbyte, f_chr);
			}
			break;

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
			const uint8_t upper = e->pal[i] >> 8;
			const uint8_t lower = e->pal[i] & 0x00FF;
			fputc(upper, f_pal);
			fputc(lower, f_pal);
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

void entry_emit_header_pal_decl(FILE *f, int pal_offs, const char *sym_name, bool c_lang)
{
	if (pal_offs > 0)
	{
		char *sym_buf = sym_underscore_conversion(sym_name);

		if (c_lang)
		{
			fprintf(f, "// Palette block forward declaration.\n");
			fprintf(f, "#ifndef __ASSEMBLER__\n");
			fprintf(f, "extern const uint8_t %s_pal[0x%X];\n", sym_buf, pal_offs);
			fprintf(f, "#else\n");
			fprintf(f, "\t.extern\t%s_pal\n", sym_buf);
			fprintf(f, "#endif  // __ASSEMBLER__\n");
		}

		free(sym_buf);
	}
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
	fprintf(f, "extern const uint8_t %s_chr[0x%lX];\n", sym_buf, bytes);
	fprintf(f, "#else\n");
	fprintf(f, "\t.extern\t%s_chr\n", sym_buf);
	fprintf(f, "#endif  // __ASSEMBLER__\n");
	free(sym_buf);
}
