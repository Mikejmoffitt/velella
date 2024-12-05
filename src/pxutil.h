#ifndef PXUTIL_H
#define PXUTIL_H

#include <stdint.h>
#include <stdbool.h>


// -----------------------------------------------------------------------------
// Data Manipulation
// -----------------------------------------------------------------------------


//
// Rotates tile data in-place.
//
// imgdat: pointer to pixel data with one byte per pixel
// x: tile left coord
// y: tile top coord
// line_w: source pixel data width
// tsize: size of tile in pixels
// angle: target rotation angle (orthagonal only please)
void pxutil_rotate_tile(uint8_t *imgdat,
                        int x, int y,
                        int line_w, int tsize,
                        int angle);


// -----------------------------------------------------------------------------
// Data Packing
// -----------------------------------------------------------------------------

// Pass a pointer to eight pixels and planar data comes out.
//
// in: pointer to linear array of 8 pixels (one byte per)
// planes: number of bitplanes (max 8, 4 is typical)
// order: bitplane order in reverse, as hex (e.g. for 0, 2, 1, 3 use 0x3120)
// reverse: if true, bitplane data is emitted horizontally flipped
// out: pointer to destination buffer (1 * planes in size)
bool pxutil_pack_planar(const uint8_t *in, int planes,
                        uint32_t order, bool reverse, uint8_t *out);

// Pass a pointer to eight pixels and linear data comes out.
//
// in: pointer to linear array of 8 pixels (one byte per)
// depth: bits per pixel (power of two)
// reverse: if true, data is emitted horizontally flipped
// out: pointer to destination buffer (1 * depth in size)
bool pxutil_pack_linear(const uint8_t *in, unsigned int depth,
                        bool reverse, uint8_t *out);


#endif  // UTIL_H
