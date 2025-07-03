// Function to claim and erase a region from a sprite.
#pragma once

#include <stdio.h>
#include <stdint.h>

typedef enum ClaimSize
{
	CLAIM_SIZE_NONE = 0,
	CLAIM_SIZE_1x1,
	CLAIM_SIZE_1x2,
	CLAIM_SIZE_1x3,
	CLAIM_SIZE_1x4,
	CLAIM_SIZE_2x1,
	CLAIM_SIZE_2x2,
	CLAIM_SIZE_2x3,
	CLAIM_SIZE_2x4,
	CLAIM_SIZE_3x1,
	CLAIM_SIZE_3x2,
	CLAIM_SIZE_3x3,
	CLAIM_SIZE_3x4,
	CLAIM_SIZE_4x1,
	CLAIM_SIZE_4x2,
	CLAIM_SIZE_4x3,
	CLAIM_SIZE_4x4,
} ClaimSize;

static inline int mdcsp_tiles_for_claim(ClaimSize size);
static inline int mdcsp_w_for_claim(ClaimSize size);
static inline int mdcsp_h_for_claim(ClaimSize size);

// Finds a sprite to clip out of imgdat.
// Returns CLAIM_SIZE_NONE if imgdat is empty.
ClaimSize mdcsp_claim(const uint8_t *imgdat,
                      int iw, int ih,
                      int sx, int sy, int sw, int sh,
                      int *col, int *row);

// -----------------------------------------------------------------------------

static inline int mdcsp_tiles_for_claim(ClaimSize size)
{
	switch (size)
	{
		case CLAIM_SIZE_1x1: return 1;
		case CLAIM_SIZE_1x2: return 2;
		case CLAIM_SIZE_1x3: return 3;
		case CLAIM_SIZE_1x4: return 4;
		case CLAIM_SIZE_2x1: return 2;
		case CLAIM_SIZE_2x2: return 4;
		case CLAIM_SIZE_2x3: return 6;
		case CLAIM_SIZE_2x4: return 8;
		case CLAIM_SIZE_3x1: return 3;
		case CLAIM_SIZE_3x2: return 6;
		case CLAIM_SIZE_3x3: return 9;
		case CLAIM_SIZE_3x4: return 12;
		case CLAIM_SIZE_4x1: return 4;
		case CLAIM_SIZE_4x2: return 8;
		case CLAIM_SIZE_4x3: return 12;
		case CLAIM_SIZE_4x4: return 16;
		default:             return 0;
	}
}

static inline int mdcsp_w_for_claim(ClaimSize size)
{
	switch (size)
	{
		case CLAIM_SIZE_1x1: return 1;
		case CLAIM_SIZE_1x2: return 1;
		case CLAIM_SIZE_1x3: return 1;
		case CLAIM_SIZE_1x4: return 1;
		case CLAIM_SIZE_2x1: return 2;
		case CLAIM_SIZE_2x2: return 2;
		case CLAIM_SIZE_2x3: return 2;
		case CLAIM_SIZE_2x4: return 2;
		case CLAIM_SIZE_3x1: return 3;
		case CLAIM_SIZE_3x2: return 3;
		case CLAIM_SIZE_3x3: return 3;
		case CLAIM_SIZE_3x4: return 3;
		case CLAIM_SIZE_4x1: return 4;
		case CLAIM_SIZE_4x2: return 4;
		case CLAIM_SIZE_4x3: return 4;
		case CLAIM_SIZE_4x4: return 4;
		default:             return 0;
	}
}

static inline int mdcsp_h_for_claim(ClaimSize size)
{
	switch (size)
	{
		case CLAIM_SIZE_1x1: return 1;
		case CLAIM_SIZE_1x2: return 2;
		case CLAIM_SIZE_1x3: return 3;
		case CLAIM_SIZE_1x4: return 4;
		case CLAIM_SIZE_2x1: return 1;
		case CLAIM_SIZE_2x2: return 2;
		case CLAIM_SIZE_2x3: return 3;
		case CLAIM_SIZE_2x4: return 4;
		case CLAIM_SIZE_3x1: return 1;
		case CLAIM_SIZE_3x2: return 2;
		case CLAIM_SIZE_3x3: return 3;
		case CLAIM_SIZE_3x4: return 4;
		case CLAIM_SIZE_4x1: return 1;
		case CLAIM_SIZE_4x2: return 2;
		case CLAIM_SIZE_4x3: return 3;
		case CLAIM_SIZE_4x4: return 4;
		default:             return 0;
	}
}
