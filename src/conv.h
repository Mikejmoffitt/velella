#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "types.h"

//
// Conversion process functions.
//

bool conv_init(Conv *s);
bool conv_validate(Conv *s);
bool conv_entry_add(Conv *s);
//
// Release of conversion resources.
//
void conv_shutdown(Conv *s);
