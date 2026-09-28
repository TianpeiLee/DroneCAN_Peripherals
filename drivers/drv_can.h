#pragma once

#include <stdbool.h>
#include <stdint.h>

bool drv_can_init(void);
bool drv_can_transmit(uint32_t id, const uint8_t *data, uint8_t len);
bool drv_can_receive(uint32_t *id, uint8_t *data, uint8_t *len);
