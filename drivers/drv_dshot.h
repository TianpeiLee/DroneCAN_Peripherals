#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DSHOT_CHANNELS 8
#define DSHOT_CHANNELS_PER_TIMER 4

#define DSHOT_VALUE_MIN 48
#define DSHOT_VALUE_MAX 2047

typedef enum
{
    DShot_Type_300,
    DShot_Type_600,
    DShot_Type_1200
} DShot_Type_e;

void drv_dshot_init(DShot_Type_e type);

uint32_t drv_dshot_bit_period_ns(void);

void drv_dshot_set(uint8_t ch, uint16_t value, bool request_telemetry);

void drv_dshot_set_telemetry(uint8_t ch, bool request_telemetry);

void drv_dshot_send(void);
