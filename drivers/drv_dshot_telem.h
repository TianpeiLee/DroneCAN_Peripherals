#pragma once

#include <stdbool.h>
#include <stdint.h>

#define DSHOT_TELEM_MAX_SAMPLES 224

void drv_dshot_telem_init(void);

void drv_dshot_telem_arm(void);

void drv_dshot_telem_disarm(void);

void drv_dshot_telem_poll(void);

bool drv_dshot_telem_take_raw(const uint16_t **samples, uint16_t *count);

uint8_t drv_dshot_telem_over_sample(void);

uint8_t drv_dshot_telem_runs(uint8_t ch, const uint16_t *samples, uint16_t count, uint8_t *runs, uint8_t max_runs,
                             uint8_t *initial_level);

int drv_dshot_telem_decode(uint8_t ch, const uint16_t *samples, uint16_t count, uint16_t *value12);

uint32_t drv_dshot_telem_erpm(uint16_t value12);
