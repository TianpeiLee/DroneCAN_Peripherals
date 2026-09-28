#pragma once

#include <stdint.h>

void delay_s(uint32_t n);
void delay_ms(uint32_t n);
void delay_us(uint32_t n);

void drv_tick_init(void);

uint32_t get_tick_us(void);
uint32_t get_tick_ms(void);
uint32_t get_tick_s(void);
