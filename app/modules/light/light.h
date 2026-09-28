#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "canard.h"
#include "drv_ws2812.h"

// 单个 WS2812：index 为该灯在灯串中的物理位置，color 为当前颜色
typedef struct
{
    uint8_t index;
    color_fmt color;
} ws2812_led_t;

// 分配 WS2812 驱动缓冲并初始化灯组
bool light_init(void);

// DroneCAN LightsCommand 处理
void light_handle_lights_command(CanardInstance *ins, CanardRxTransfer *transfer);
