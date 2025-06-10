#pragma once

#include <stdint.h>
#include "types.h"

DataFormat data_format_for_string(const char *str);
const char *string_for_data_format(DataFormat fmt);
