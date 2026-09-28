#include "drv_ws2812.h"

#include <malloc.h>
#include <string.h>

#include "ch32v20x_conf.h"

#define PERIOD_4_WS2812 (0.00000125f)
#define SPARE_MEM_4_WS2812 ((uint32_t)(0.000200f / PERIOD_4_WS2812))

static uint16_t hi_zro_2812;
static uint16_t lo_zro_2812;

static uint32_t lump_num_local;

void *drv_ws2812_init(uint32_t lump_num)
{
    RCC_ClocksTypeDef RCC_Clocks;
    RCC_GetClocksFreq(&RCC_Clocks);
    SystemCoreClockUpdate();
    uint32_t tim_clock = RCC_Clocks.PCLK1_Frequency;
    if (RCC_Clocks.HCLK_Frequency != RCC_Clocks.PCLK1_Frequency)
        tim_clock = RCC_Clocks.PCLK1_Frequency * 2;

    lump_num_local = lump_num;
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure = {
        .GPIO_Mode = GPIO_Mode_AF_PP, .GPIO_Pin = GPIO_Pin_6, .GPIO_Speed = GPIO_Speed_50MHz};

    GPIO_Init(GPIOA, &GPIO_InitStructure);

    const uint16_t tim_prescaler = 1;
    const float tim_tick_period = 1.0f / ((float)tim_clock / (float)(tim_prescaler + 1));

    TIM_TimeBaseInitTypeDef TIM_InitStruct = {.TIM_ClockDivision = TIM_CKD_DIV1,
                                              .TIM_CounterMode = TIM_CounterMode_Up,
                                              .TIM_Prescaler = tim_prescaler,
                                              .TIM_RepetitionCounter = 0,
                                              .TIM_Period = (uint16_t)(PERIOD_4_WS2812 / tim_tick_period - 1)};

    TIM_OCInitTypeDef TIM_OCInitStruct = {.TIM_OCMode = TIM_OCMode_PWM1,
                                          .TIM_OutputState = TIM_OutputState_Enable,
                                          .TIM_OutputNState = TIM_OutputNState_Disable,
                                          .TIM_Pulse = (uint16_t)(PERIOD_4_WS2812 / tim_tick_period),
                                          .TIM_OCPolarity = TIM_OCPolarity_High,
                                          .TIM_OCNPolarity = TIM_OCNPolarity_High,
                                          .TIM_OCIdleState = TIM_OCIdleState_Reset,
                                          .TIM_OCNIdleState = TIM_OCNIdleState_Reset};

    hi_zro_2812 = (uint16_t)(TIM_InitStruct.TIM_Period * 0.64f);
    lo_zro_2812 = (uint16_t)(TIM_InitStruct.TIM_Period * 0.26f);
    void *addr = malloc(lump_num * 3 * 8 + SPARE_MEM_4_WS2812 + 2);
    if (addr == NULL)
        return NULL;
    memset(addr, 0xff, lump_num * 3 * 8 + SPARE_MEM_4_WS2812 + 2);

    memset(addr, 0, SPARE_MEM_4_WS2812);

    TIM_TimeBaseInit(TIM3, &TIM_InitStruct);
    TIM_OC1Init(TIM3, &TIM_OCInitStruct);

    TIM_ClearFlag(TIM3, TIM_FLAG_Update);
    TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);

    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, DISABLE);
    TIM_CtrlPWMOutputs(TIM3, ENABLE);

    TIM_DMACmd(TIM3, TIM_DMA_Update, ENABLE);

    DMA_InitTypeDef DMA_InitStruct = {.DMA_BufferSize = lump_num * 3 * 8 + SPARE_MEM_4_WS2812 + 2,
                                      .DMA_DIR = DMA_DIR_PeripheralDST,
                                      .DMA_M2M = DMA_M2M_Disable,
                                      .DMA_MemoryBaseAddr = (uint32_t)addr,
                                      .DMA_MemoryDataSize = DMA_MemoryDataSize_Byte,
                                      .DMA_MemoryInc = DMA_MemoryInc_Enable,
                                      .DMA_Mode = DMA_Mode_Normal,
                                      .DMA_PeripheralBaseAddr = (uint32_t)&TIM3->CH1CVR,
                                      .DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
                                      .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
                                      .DMA_Priority = DMA_Priority_High};

    DMA_Init(DMA1_Channel3, &DMA_InitStruct);
    DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
    DMA_Cmd(DMA1_Channel3, ENABLE);

    return addr + SPARE_MEM_4_WS2812;
}

typedef struct
{
    uint8_t g[8];
    uint8_t r[8];
    uint8_t b[8];
} alted_color_data;

void drv_ws2812_draw(color_fmt raw_data, void *buf_pool, uint32_t index, uint32_t num)
{
    DMA_Cmd(DMA1_Channel3, DISABLE);
    TIM_Cmd(TIM3, DISABLE);
    alted_color_data *p = (alted_color_data *)buf_pool;

    for (size_t i = index; i < index + num; i++)
    {
        for (size_t j = 0; j < 8; j++)
        {
            p[i].r[j] = ((raw_data.r >> (7 - j)) & 1) ? (uint8_t)hi_zro_2812 : (uint8_t)lo_zro_2812;
            p[i].g[j] = ((raw_data.g >> (7 - j)) & 1) ? (uint8_t)hi_zro_2812 : (uint8_t)lo_zro_2812;
            p[i].b[j] = ((raw_data.b >> (7 - j)) & 1) ? (uint8_t)hi_zro_2812 : (uint8_t)lo_zro_2812;
        }
    }

    DMA_SetCurrDataCounter(DMA1_Channel3, lump_num_local * 3 * 8 + SPARE_MEM_4_WS2812 + 2);
    TIM_SetCounter(TIM3, 0);
    DMA_Cmd(DMA1_Channel3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}
