#pragma once

#include <stdint.h>

//
// Pixel data formats.
//
typedef enum DataFormat
{
	DATA_FORMAT_UNSPECIFIED,
	DATA_FORMAT_DIRECT,      // Raw tile conversion.
	DATA_FORMAT_SP013,       // Atlus 013 sprite data
	DATA_FORMAT_BG038,       // Atlus 038 background tile data
	DATA_FORMAT_CPS_SPR,     // CPS/CPS2 sprites.
	DATA_FORMAT_CPS_BG,      // CPS/CPS2 background tiles.
	DATA_FORMAT_MD_SPR,      // Megadrive direct sprite data.
	DATA_FORMAT_MD_BG,       // Megadrive direct background data.
	DATA_FORMAT_MD_CSP,      // Megadrive composite sprite data.
	DATA_FORMAT_MD_CBG,      // Megadrive composite background data.
	DATA_FORMAT_TOA_TXT,     // Toaplan text tilemap layer.
	DATA_FORMAT_TOA_GCU_SPR, // Toaplan GCU sprites.
	DATA_FORMAT_NEO_SPR,     // Neo-Geo sprites (direct).
	DATA_FORMAT_NEO_CSPR,    // Neo-Geo sprites (composite).
	DATA_FORMAT_NEO_FIX,     // Neo-Geo FIX layer.
	DATA_FORMAT_COUNT
} DataFormat;

DataFormat data_format_for_string(const char *str);
const char *string_for_data_format(DataFormat fmt);
