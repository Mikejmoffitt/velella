#pragma once

#include "types.h"
#include <stdio.h>

//
// Entry emission functions.
//

void entry_emit_meta(const Entry *e, const Conv *conv, FILE *f_inc, int pal_offs, bool c_lang);
void entry_emit_chr(const Entry *e, const Conv *conv, FILE *f_chr1, FILE *f_chr2);
void entry_emit_pal(Entry *e, FILE *f_pal, int *pal_offs);

void entry_emit_header_top(FILE *f, bool c_lang);
void entry_emit_header_divider(FILE *f, bool c_lang);
void entry_emit_header_pal_decl(FILE *f, int pal_offs, const char *sym_name, bool c_lang);
