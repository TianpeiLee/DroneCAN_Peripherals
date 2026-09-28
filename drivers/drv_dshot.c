#include "drv_dshot.h"

#include "ch32v20x_conf.h"

#include "drv_dshot_telem.h"

#define DShot_300_PERIOD (0.00000333f)
#define DShot_300_T1H (0.00000250f)
#define DShot_300_T0H (0.00000125f)

#define DShot_600_PERIOD (0.00000166f)
#define DShot_600_T1H (0.000001250f)
#define DShot_600_T0H (0.000000625f)

#define DShot_1200_PERIOD (0.000000833f)
#define DShot_1200_T1H (0.000000625f)
#define DShot_1200_T0H (0.000000313f)

#define DSHOT_DMA_BURST DSHOT_CHANNELS_PER_TIMER
#define DSHOT_FRAME_BITS 20
#define DSHOT_DMA_SIZE (DSHOT_FRAME_BITS * DSHOT_DMA_BURST)

#define DSHOT_BIDIR_ENABLE 1

#define DSHOT_PIN_MASK                                                                                                \
    (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11)

#define DSHOT_TX_GROUP0_PINS (GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3)
#define DSHOT_TX_GROUP1_PINS (GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11)

static uint16_t dshot_dma_buf_tim1[DSHOT_DMA_SIZE];
static uint16_t dshot_dma_buf_tim2[DSHOT_DMA_SIZE];

static uint16_t dshot_t1h_tim1;
static uint16_t dshot_t0h_tim1;
static uint16_t dshot_t1h_tim2;
static uint16_t dshot_t0h_tim2;

static uint16_t dshot_stage_value[DSHOT_CHANNELS];
static uint8_t dshot_stage_telem[DSHOT_CHANNELS];

static uint32_t dshot_bit_period_ns;

#if DSHOT_BIDIR_ENABLE
static volatile uint8_t dshot_tx_done;
#endif

uint32_t drv_dshot_bit_period_ns(void)
{
    return dshot_bit_period_ns;
}

static uint16_t dshot_build_frame(uint16_t value, uint8_t request_telemetry)
{
    const uint16_t packet = (uint16_t)((value << 1) | (request_telemetry ? 1u : 0u));

    uint16_t csum = packet ^ (packet >> 4) ^ (packet >> 8);
    csum &= 0x0Fu;

#if DSHOT_BIDIR_ENABLE
    csum = (~csum) & 0x0Fu;
#endif

    return (uint16_t)((packet << 4) | csum);
}

static void dshot_fill_buffer(uint16_t *buf, const uint16_t *frames, uint16_t t1h, uint16_t t0h)
{
    for (uint8_t bit = 0; bit < DSHOT_FRAME_BITS; bit++)
    {
        for (uint8_t c = 0; c < DSHOT_CHANNELS_PER_TIMER; c++)
        {
            uint16_t ccr = 0;
            if (bit < 16)
            {
                ccr = ((frames[c] >> (15 - bit)) & 1u) ? t1h : t0h;
            }
            buf[bit * DSHOT_CHANNELS_PER_TIMER + c] = ccr;
        }
    }
}

void drv_dshot_init(DShot_Type_e type)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_TIM1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

        GPIO_InitTypeDef GPIO_InitStruct = {.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_8 |
                                                    GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11,
                                        .GPIO_Speed = GPIO_Speed_50MHz,
                                        .GPIO_Mode = GPIO_Mode_AF_PP};
    GPIO_Init(GPIOA, &GPIO_InitStruct);

    RCC_ClocksTypeDef RCC_Clocks;
    SystemCoreClockUpdate();
    RCC_GetClocksFreq(&RCC_Clocks);

    uint32_t tim1_clock = RCC_Clocks.PCLK2_Frequency;
    if (RCC_Clocks.PCLK2_Frequency != RCC_Clocks.HCLK_Frequency)
        tim1_clock = RCC_Clocks.PCLK2_Frequency * 2;

    uint32_t tim2_clock = RCC_Clocks.PCLK1_Frequency;
    if (RCC_Clocks.PCLK1_Frequency != RCC_Clocks.HCLK_Frequency)
        tim2_clock = RCC_Clocks.PCLK1_Frequency * 2;

    float period;
    float t1h;
    float t0h;
    switch (type)
    {
    case DShot_Type_300:
        period = DShot_300_PERIOD;
        t1h = DShot_300_T1H;
        t0h = DShot_300_T0H;
        break;
    case DShot_Type_600:
        period = DShot_600_PERIOD;
        t1h = DShot_600_T1H;
        t0h = DShot_600_T0H;
        break;
    default:
        period = DShot_1200_PERIOD;
        t1h = DShot_1200_T1H;
        t0h = DShot_1200_T0H;
        break;
    }

    dshot_t1h_tim1 = (uint16_t)(t1h * (float)tim1_clock);
    dshot_t0h_tim1 = (uint16_t)(t0h * (float)tim1_clock);
    dshot_t1h_tim2 = (uint16_t)(t1h * (float)tim2_clock);
    dshot_t0h_tim2 = (uint16_t)(t0h * (float)tim2_clock);

    dshot_bit_period_ns = (uint32_t)(period * 1000000000.0f);

    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStruct = {.TIM_ClockDivision = TIM_CKD_DIV1,
                                                      .TIM_CounterMode = TIM_CounterMode_Up,
                                                      .TIM_RepetitionCounter = 0,
                                                      .TIM_Prescaler = 0,
                                                      .TIM_Period = (uint16_t)(period * (float)tim1_clock)};

    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStruct);

    TIM_TimeBaseInitStruct.TIM_Prescaler = 0;
    TIM_TimeBaseInitStruct.TIM_Period = (uint16_t)(period * (float)tim2_clock);
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStruct);

    TIM_OCInitTypeDef TIM_OCInitStruct = {.TIM_OCMode = TIM_OCMode_PWM1,
                                          .TIM_OutputState = TIM_OutputState_Enable,
                                          .TIM_OutputNState = TIM_OutputNState_Disable,
                                          .TIM_Pulse = 0,
#if DSHOT_BIDIR_ENABLE
                                          .TIM_OCPolarity = TIM_OCPolarity_Low,
#else
                                          .TIM_OCPolarity = TIM_OCPolarity_High,
#endif
                                          .TIM_OCNPolarity = TIM_OCNPolarity_High,
                                          .TIM_OCIdleState = TIM_OCIdleState_Reset,
                                          .TIM_OCNIdleState = TIM_OCNIdleState_Reset};

    TIM_OC1Init(TIM1, &TIM_OCInitStruct);
    TIM_OC2Init(TIM1, &TIM_OCInitStruct);
    TIM_OC3Init(TIM1, &TIM_OCInitStruct);
    TIM_OC4Init(TIM1, &TIM_OCInitStruct);
                TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, DISABLE);
    TIM_CtrlPWMOutputs(TIM1, ENABLE);

    TIM_OC1Init(TIM2, &TIM_OCInitStruct);
    TIM_OC2Init(TIM2, &TIM_OCInitStruct);
    TIM_OC3Init(TIM2, &TIM_OCInitStruct);
    TIM_OC4Init(TIM2, &TIM_OCInitStruct);
    TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM2, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2, DISABLE);

    TIM_DMAConfig(TIM1, TIM_DMABase_CCR1, TIM_DMABurstLength_4Transfers);
    TIM_DMACmd(TIM1, TIM_DMA_Update, ENABLE);

    TIM_DMAConfig(TIM2, TIM_DMABase_CCR1, TIM_DMABurstLength_4Transfers);
    TIM_DMACmd(TIM2, TIM_DMA_Update, ENABLE);

    DMA_InitTypeDef DMA_InitStruct = {.DMA_PeripheralBaseAddr = (uint32_t)&TIM1->DMAADR,
                                      .DMA_MemoryBaseAddr = (uint32_t)dshot_dma_buf_tim1,
                                      .DMA_DIR = DMA_DIR_PeripheralDST,
                                      .DMA_BufferSize = DSHOT_DMA_SIZE,
                                      .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
                                      .DMA_MemoryInc = DMA_MemoryInc_Enable,
                                      .DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
                                      .DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord,
                                      .DMA_Mode = DMA_Mode_Normal,
                                      .DMA_Priority = DMA_Priority_High,
                                      .DMA_M2M = DMA_M2M_Disable};

    DMA_Init(DMA1_Channel5, &DMA_InitStruct);

    DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&TIM2->DMAADR;
    DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)dshot_dma_buf_tim2;
    DMA_Init(DMA1_Channel2, &DMA_InitStruct);

        DMA_ClearITPendingBit(DMA1_IT_TC5);
    DMA_ClearITPendingBit(DMA1_IT_TC2);
    DMA_ITConfig(DMA1_Channel5, DMA_IT_TC, ENABLE);
    DMA_ITConfig(DMA1_Channel2, DMA_IT_TC, ENABLE);
    NVIC_SetPriority(DMA1_Channel5_IRQn, 2);
    NVIC_SetPriority(DMA1_Channel2_IRQn, 2);
#if DSHOT_BIDIR_ENABLE
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    NVIC_EnableIRQ(DMA1_Channel2_IRQn);
#endif

    for (uint8_t ch = 0; ch < DSHOT_CHANNELS; ch++)
    {
        dshot_stage_value[ch] = 0;
        dshot_stage_telem[ch] = 0;
    }

    TIM_SetCounter(TIM2, 0);
    TIM_SetCounter(TIM1, 0);
}

void drv_dshot_set(uint8_t ch, uint16_t value, bool request_telemetry)
{
    if (ch >= DSHOT_CHANNELS)
        return;

    if (value > DSHOT_VALUE_MAX)
        value = DSHOT_VALUE_MAX;

    dshot_stage_value[ch] = value;
    dshot_stage_telem[ch] = request_telemetry ? 1 : 0;
}

void drv_dshot_set_telemetry(uint8_t ch, bool request_telemetry)
{
    if (ch >= DSHOT_CHANNELS)
        return;

    dshot_stage_telem[ch] = request_telemetry ? 1 : 0;
}

static void dshot_pins_mode(uint16_t pin_mask, GPIOMode_TypeDef mode)
{
    GPIO_InitTypeDef g = {.GPIO_Pin = pin_mask, .GPIO_Speed = GPIO_Speed_50MHz, .GPIO_Mode = mode};
    GPIO_Init(GPIOA, &g);
}

#if DSHOT_BIDIR_ENABLE
static void dshot_tx_group_release(uint16_t pins, uint8_t group_bit)
{
    dshot_tx_done |= group_bit;
    dshot_pins_mode(pins, GPIO_Mode_IN_FLOATING);
    if (dshot_tx_done == 0x03u)
        drv_dshot_telem_arm();
}

void DMA1_Channel5_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel5_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC5) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC5);
        DMA_Cmd(DMA1_Channel5, DISABLE);
        dshot_tx_group_release(DSHOT_TX_GROUP1_PINS, 0x02u);
    }
}

void DMA1_Channel2_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel2_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC2) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC2);
        DMA_Cmd(DMA1_Channel2, DISABLE);
        dshot_tx_group_release(DSHOT_TX_GROUP0_PINS, 0x01u);
    }
}
#endif

void drv_dshot_send(void)
{
        drv_dshot_telem_disarm();

    uint16_t frames1[DSHOT_CHANNELS_PER_TIMER];
    uint16_t frames2[DSHOT_CHANNELS_PER_TIMER];

    for (uint8_t c = 0; c < DSHOT_CHANNELS_PER_TIMER; c++)
    {
        frames1[c] = dshot_build_frame(dshot_stage_value[c], dshot_stage_telem[c]);
        frames2[c] = dshot_build_frame(dshot_stage_value[DSHOT_CHANNELS_PER_TIMER + c],
                                       dshot_stage_telem[DSHOT_CHANNELS_PER_TIMER + c]);
    }

    dshot_fill_buffer(dshot_dma_buf_tim1, frames1, dshot_t1h_tim1, dshot_t0h_tim1);
    dshot_fill_buffer(dshot_dma_buf_tim2, frames2, dshot_t1h_tim2, dshot_t0h_tim2);

    TIM_Cmd(TIM1, DISABLE);
    TIM_Cmd(TIM2, DISABLE);
    DMA_Cmd(DMA1_Channel5, DISABLE);
    DMA_Cmd(DMA1_Channel2, DISABLE);
    DMA_ClearITPendingBit(DMA1_IT_TC5);
    DMA_ClearITPendingBit(DMA1_IT_TC2);

    DMA_SetCurrDataCounter(DMA1_Channel5, DSHOT_DMA_SIZE);
    DMA_SetCurrDataCounter(DMA1_Channel2, DSHOT_DMA_SIZE);

    TIM_SetCounter(TIM1, 0);
    TIM_SetCounter(TIM2, 0);

#if DSHOT_BIDIR_ENABLE
    dshot_tx_done = 0;
#endif

    DMA_Cmd(DMA1_Channel5, ENABLE);
    DMA_Cmd(DMA1_Channel2, ENABLE);

    dshot_pins_mode(DSHOT_PIN_MASK, GPIO_Mode_AF_PP);
    TIM_Cmd(TIM1, ENABLE);
    TIM_Cmd(TIM2, ENABLE);
}
