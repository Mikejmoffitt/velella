#pragma once

#include <stdint.h>

uint8_t *tile_read_frame(const uint8_t *px, int png_w, int png_x, int png_y, int sw_adj, int sh_adj, int tilesize, int angle, uint8_t *chr_w);
