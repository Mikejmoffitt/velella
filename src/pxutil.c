#include "pxutil.h"
#include <stdio.h>
#include <stdlib.h>


// -----------------------------------------------------------------------------
// Data Manipulation
// -----------------------------------------------------------------------------

// Rotates a tile in place.
void pxutil_rotate_tile(uint8_t *imgdat, int x, int y, int line_w,
                        int tsize, int angle)
{
	if (angle == 0) return;
	uint8_t *imgcopy = malloc(sizeof(uint8_t) * tsize * tsize);
	if (!imgcopy)
	{
		fprintf(stderr, "Couldn't allocate for tile rotation.\n");
		return;
	}

	switch (angle % 360)
	{
		case 0:
			break;
		case 90:
			for (int dy = 0; dy < tsize; dy++)
			{
				for (int dx = 0; dx < tsize; dx++)
				{
					const int sy = tsize - dx - 1;
					const int sx = dy;
					imgcopy[dy*tsize + dx] = imgdat[((y+sy)*line_w) + (x+sx)];
				}
			}
			break;
		case 270:
			for (int dy = 0; dy < tsize; dy++)
			{
				for (int dx = 0; dx < tsize; dx++)
				{
					const int sy = dx;
					const int sx = tsize - dy - 1;
					imgcopy[dy*tsize + dx] = imgdat[((y+sy)*line_w) + (x+sx)];
				}
			}
			break;
		case 180:
			for (int dy = 0; dy < tsize; dy++)
			{
				for (int dx = 0; dx < tsize; dx++)
				{
					const int sy = tsize - dy - 1;
					const int sx = tsize - dx - 1;
					imgcopy[dy*tsize + dx] = imgdat[((y+sy)*line_w) + (x+sx)];
				}
			}
			break;
		default:
			fprintf(stderr, "Unsupported rotation angle %d\n", angle);
			break;
	}

	for (int cy = 0; cy < tsize; cy++)
	{
		for (int cx = 0; cx < tsize; cx++)
		{
			imgdat[(cy+y)*line_w + x + cx] = imgcopy[cy*tsize + cx];
		}
	}

	free(imgcopy);
}


// -----------------------------------------------------------------------------
// Data Packing
// -----------------------------------------------------------------------------

bool pxutil_pack_planar(const uint8_t *in, int planes,
                        uint32_t order, bool reverse, uint8_t *out)
{
	if (planes >= 8)
	{
		fprintf(stderr, "More than eight bitplanes are not supported.\n");
		return false;
	}
	for (int plane = 0; plane < planes; plane++)
	{
		const int plane_bit = order & 0x7;
		if (plane_bit >= planes)
		{
			fprintf(stderr, "Plane bit %d used despite plane count of %d\n",
			       plane_bit, planes);
			return false;
		}

		out[plane] = 0;
		const uint8_t testbits = (1 << plane_bit);
	//	printf("plane %d: test $%X\n", plane, testbits);

		for (int i = 0; i < 8; i++)
		{
			if (!(in[i] & testbits)) continue;

			if (reverse) out[plane] |= (0x01 << i);
			else out[plane] |= (0x80 >> i);
		}
		order = order >> 4;
	}
	//printf("result buffer {%02X %02X %02X %02X %02X %02X %02X %02X}\n", out[0], out[1], out[2], out[3], out[4], out[5], out[6], out[7]);
	return true;
}

// Pass a pointer to eight pixels and linear data comes out.
//
// in: pointer to linear array of 8 pixels (one byte per)
// depth: bits per pixel (power of two)
// reverse: if true, data is emitted horizontally flipped
// out: pointer to destination buffer (1 * depth in size)
bool pxutil_pack_linear(const uint8_t *in, unsigned int depth,
                        bool reverse, uint8_t *out)
{
	if (depth >= 8)
	{
		fprintf(stderr, "8bpp depth is not supported.\n");
		return false;
	}
	if (((depth & (depth - 1)) != 0) || depth <= 0)
	{
		fprintf(stderr, "Only power of two depths are supported.\n");
		return false;
	}

	int out_idx = 0;
	const int out_bit_init = 7 - depth;
	int out_bit = out_bit_init;
	out[out_idx] = 0;
	for (int i = 0; i < 8; i++)
	{
		const int in_idx = reverse ? (7 - i) : i;
		const int data_mask = (1 << depth) - 1;
		const int px = in[in_idx] & data_mask;
		out[out_idx] |= (px << out_bit);
		out_bit -= depth;
		if (out_bit < 0)
		{
			out_bit = out_bit_init;
			out_idx++;
			out[out_idx] = 0;
		}
	}
	return true;
}
