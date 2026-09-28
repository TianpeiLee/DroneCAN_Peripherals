#include "stdint.h"

__attribute__((section(".text.sysinit"), noinline, optimize("O0"))) void __init_data_section()
{
    extern uint32_t __data_start__[];
    extern uint32_t __data_end__[];
    extern uint32_t __data_lma__[];

    uint32_t *dst = __data_start__;
    uint32_t *src = __data_lma__;
    uint32_t *end = __data_end__;
    while (src < end)
        *dst++ = *src++;
}

__attribute__((section(".text.sysinit"), noinline, optimize("O0"))) void __init_bss_section()
{
    extern uint32_t __bss_start__[];
    extern uint32_t __bss_end__[];

    uint32_t *src = __bss_start__;
    uint32_t *end = __bss_end__;
    while (src < end)
        *src++ = 0;
}

#include <stdio.h>
__attribute__((used)) void *_sbrk(ptrdiff_t incr)
{
    extern char __heap_start__[];
    extern char __heap_end__[];
    static char *curbrk = __heap_start__;

    if ((curbrk + incr < __heap_start__) || (curbrk + incr > __heap_end__))
        return NULL - 1;

    curbrk += incr;
    return curbrk - incr;
}