#include "format.h"
#include <stdio.h>
#include <string.h>

static const char *kstring_for_data_format[DATA_FORMAT_COUNT] =
{
	[DATA_FORMAT_UNSPECIFIED] = "{unspecified}",
	[DATA_FORMAT_DIRECT]      = "direct",
	[DATA_FORMAT_SP013]       = "sp013",
	[DATA_FORMAT_BG038]       = "bg038",
	[DATA_FORMAT_CPS_SPR]     = "cps_spr",
	[DATA_FORMAT_CPS_BG]      = "cps_bg",
	[DATA_FORMAT_MD_SPR]      = "md_spr",
	[DATA_FORMAT_MD_BG]       = "md_bg",
	[DATA_FORMAT_MD_CSP]      = "md_csp",
	[DATA_FORMAT_MD_CBG]      = "md_cbg",
	[DATA_FORMAT_TOA_TXT]     = "toa_txt",
	[DATA_FORMAT_TOA_GCU_SPR] = "toa_gcu_spr",
	[DATA_FORMAT_NEO_FIX]     = "neo_fix",
	[DATA_FORMAT_NEO_SPR]     = "neo_spr",
	[DATA_FORMAT_NEO_CSPR]    = "neo_cspr",
};

DataFormat data_format_for_string(const char *str)
{
	for (int i = 0; i < DATA_FORMAT_COUNT; i++)
	{
		if (strcmp(str, kstring_for_data_format[i]) == 0)
		{
			return i;
		}
	}
	return DATA_FORMAT_UNSPECIFIED;
}

const char *string_for_data_format(DataFormat fmt)
{
	if (fmt < 0 || fmt >= DATA_FORMAT_COUNT) return kstring_for_data_format[DATA_FORMAT_UNSPECIFIED];
	return kstring_for_data_format[fmt];
}
