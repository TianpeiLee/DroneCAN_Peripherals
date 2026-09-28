#include "drv_tick.h"

#include "ch32v20x_conf.h"

static uint32_t local_updated_sysclk = 0;

uint32_t get_tick_us(void)
{
    return SysTick->CNT / (local_updated_sysclk / 8 / 1000000);
}

uint32_t get_tick_ms(void)
{
    return SysTick->CNT / (local_updated_sysclk / 8 / 1000);
}

uint32_t get_tick_s(void)
{
    return SysTick->CNT / (local_updated_sysclk / 8);
}

void delay_s(uint32_t n)
{
    uint32_t now = get_tick_s();
    while (get_tick_s() - now < n)
        ;
}

void delay_ms(uint32_t n)
{
    uint32_t now = get_tick_ms();
    while (get_tick_ms() - now < n)
        ;
}

void delay_us(uint32_t n)
{
    uint32_t now = get_tick_us();
    while (get_tick_us() - now < n)
        ;
}

void drv_tick_init(void)
{
    SystemCoreClockUpdate();
    local_updated_sysclk = SystemCoreClock;
    SysTick->SR = ~(1 << 0);
    SysTick->CMP = 0xffffffffffffffff;
    SysTick->CTLR &= ~(1 << 4);
    SysTick->CTLR |= (1 << 5) | (1 << 0);
}
