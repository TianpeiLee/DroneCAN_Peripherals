#pragma once

#include <stdint.h>

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color_fmt;

void *drv_ws2812_init(uint32_t lump_num);
void drv_ws2812_draw(color_fmt raw_data, void *buf_pool, uint32_t index, uint32_t num);
