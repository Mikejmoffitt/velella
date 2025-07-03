#pragma once

#include "types.h"
#include <stdio.h>

//
// Entry emission functions.
//

void entry_emit_meta(const Entry *e, FILE *f_inc, int pal_offs, bool c_lang);
void entry_emit_chr(const Entry *e, FILE *f_chr);
void entry_emit_pal(Entry *e, FILE *f_pal, int *pal_offs);
void entry_emit_map(const Entry *e, FILE *f_map);

void entry_emit_header_top(FILE *f, bool c_lang);
void entry_emit_header_divider(FILE *f, bool c_lang);

// Type declarations
void entry_emit_type_decl(FILE *f, DataFormat fmt, bool c_lang);

// Palette declarations
void entry_emit_header_data_decl(FILE *f, size_t pal_offs, size_t map_offs,
                                const char *sym_name, bool c_lang);

void entry_emit_header_chr_size(FILE *f, const char *sym_name, size_t bytes);
