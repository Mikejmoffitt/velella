#pragma once

#include "types.h"
#include <stdio.h>

//
// Entry emission functions.
//

void entry_emit_meta(const Entry *e, const Conv *conv, FILE *f_inc, int pal_offs, bool c_lang);
void entry_emit_chr(const Entry *e, const Conv *conv, FILE *f_chr1, FILE *f_chr2);
