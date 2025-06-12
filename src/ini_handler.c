#include "ini_handler.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "types.h"
#include "conv.h"
#include "format.h"
#include "pal.h"

int ini_handler_func(void *user, const char *section, const char *name, const char *value)
{
	Conv *s = (Conv *)user;

	// New section - copy in name
	if (strcmp(section, s->symbol) != 0)
	{
		strncpy(s->symbol, section, sizeof(s->symbol));
		s->symbol[sizeof(s->symbol)-1] = '\0';
	}

	// Setting the source is what actually kicks off the conversion for this entry.
	if (strcmp("src", name) == 0)
	{
		strncpy(s->src, value, sizeof(s->src));
		s->src[sizeof(s->src)-1] = '\0';
		if (!conv_entry_add(s)) return 0;
	}
	else if (strcmp("out", name) == 0)
	{
		strncpy(s->out, value, sizeof(s->out));
		s->out[sizeof(s->out)-1] = '\0';
	}
	else if (strcmp("code", name) == 0)
	{
		s->frame_cfg.code = strtoul(value, NULL, 0);
	}
	else if (strcmp("angle", name) == 0)
	{
		s->frame_cfg.angle = strtoul(value, NULL, 0);
	}
	else if (strcmp("depth", name) == 0)
	{
		s->frame_cfg.depth = strtoul(value, NULL, 0);
	}
	else if (strcmp("w", name) == 0)
	{
		s->frame_cfg.w = strtoul(value, NULL, 0);
	}
	else if (strcmp("h", name) == 0)
	{
		s->frame_cfg.h = strtoul(value, NULL, 0);
	}
	else if (strcmp("tilesize", name) == 0)
	{
		s->frame_cfg.tilesize = strtoul(value, NULL, 0);
	}
	else if (strcmp("format", name) == 0)
	{
		s->frame_cfg.data_format = data_format_for_string(value);
		if (s->frame_cfg.data_format == DATA_FORMAT_UNSPECIFIED)
		{
			printf("ERROR: Unhandled data format %s\n", value);
			return 0;
		}
	}
	else if (strcmp("pal", name) == 0)
	{
		s->frame_cfg.pal_format = pal_format_for_string(value);
		if (s->frame_cfg.pal_format == PAL_FORMAT_UNSPECIFIED)
		{
			printf("ERROR: Unhandled palette format %s\n", value);
			return 0;
		}
	}
	else
	{
		printf("WARNING: Unhandled directive \"%s\"\n", name);
		return 0;
	}
	return 1;
}
