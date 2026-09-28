#include "drv_dshot_telem.h"

#include "ch32v20x_conf.h"

#include "drv_dshot.h"
#include "drv_tick.h"

// 3x 过采样，可直接套用 Betaflight decode_bb 的 (run+1)/3 游程还原
#define TELEM_OVERSAMPLE 3
// 采样窗口：覆盖「约 30us 等待 + 约 28us 回传帧」并留余量
#define TELEM_WINDOW_US 100u
// 采样超时保护
#define TELEM_SAMPLE_TIMEOUT_US 250u

// ch0..3 = PA0..PA3，ch4..7 = PA8..PA11
static const uint16_t telem_pin_bit[DSHOT_CHANNELS] = {(uint16_t)(1u << 8),  (uint16_t)(1u << 9), (uint16_t)(1u << 10),
                                                       (uint16_t)(1u << 11), (uint16_t)(1u << 0), (uint16_t)(1u << 1),
                                                       (uint16_t)(1u << 2),  (uint16_t)(1u << 3)};

static uint16_t telem_samples[DSHOT_TELEM_MAX_SAMPLES];
static uint16_t telem_active_samples;
static uint16_t telem_sample_count;
static volatile bool telem_sampling;
static volatile bool telem_ready;
static volatile uint32_t telem_arm_ts;
static uint32_t telem_ticks_per_us;

static inline uint32_t telem_now(void)
{
    return get_tick_us();
}

static uint32_t tim4_clock_hz(void)
{
    RCC_ClocksTypeDef c;
    RCC_GetClocksFreq(&c);
    uint32_t clk = c.PCLK1_Frequency;
    if (c.PCLK1_Frequency != c.HCLK_Frequency)
        clk = c.PCLK1_Frequency * 2;
    return clk;
}

uint8_t drv_dshot_telem_over_sample(void)
{
    return TELEM_OVERSAMPLE;
}

void drv_dshot_telem_init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    SystemCoreClockUpdate();
    telem_ticks_per_us = SystemCoreClock / 8 / 1000000;
    if (telem_ticks_per_us == 0)
        telem_ticks_per_us = 1;

    // 遥测位周期 = 4/5 × DShot 位周期；采样周期 = 遥测位周期 / 过采样倍数
    const uint32_t bit_ns = drv_dshot_bit_period_ns();
    const uint32_t sample_ns = (bit_ns * 4u / 5u) / TELEM_OVERSAMPLE;

    const uint32_t clk = tim4_clock_hz();
    uint32_t period = (uint32_t)((uint64_t)clk * sample_ns / 1000000000ull);
    if (period == 0)
        period = 1;
    period -= 1;

    TIM_TimeBaseInitTypeDef t = {.TIM_Prescaler = 0,
                                 .TIM_CounterMode = TIM_CounterMode_Up,
                                 .TIM_Period = period,
                                 .TIM_ClockDivision = TIM_CKD_DIV1,
                                 .TIM_RepetitionCounter = 0};
    TIM_TimeBaseInit(TIM4, &t);
    TIM_DMACmd(TIM4, TIM_DMA_Update, ENABLE);

    uint32_t n = TELEM_WINDOW_US * 1000u / sample_ns;
    if (n > DSHOT_TELEM_MAX_SAMPLES)
        n = DSHOT_TELEM_MAX_SAMPLES;
    if (n == 0)
        n = 1;
    telem_active_samples = (uint16_t)n;

    DMA_InitTypeDef d = {.DMA_PeripheralBaseAddr = (uint32_t)&GPIOA->INDR,
                         .DMA_MemoryBaseAddr = (uint32_t)telem_samples,
                         .DMA_DIR = DMA_DIR_PeripheralSRC,
                         .DMA_BufferSize = telem_active_samples,
                         .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
                         .DMA_MemoryInc = DMA_MemoryInc_Enable,
                         .DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord,
                         .DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord,
                         .DMA_Mode = DMA_Mode_Normal,
                         .DMA_Priority = DMA_Priority_VeryHigh,
                         .DMA_M2M = DMA_M2M_Disable};
    DMA_Init(DMA1_Channel7, &d);
    DMA_ClearITPendingBit(DMA1_IT_TC7);
    DMA_ITConfig(DMA1_Channel7, DMA_IT_TC, ENABLE);
    NVIC_SetPriority(DMA1_Channel7_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Channel7_IRQn);
    drv_dshot_telem_disarm();
}

void drv_dshot_telem_arm(void)
{
    TIM_Cmd(TIM4, DISABLE);
    DMA_Cmd(DMA1_Channel7, DISABLE);
    DMA_ClearFlag(DMA1_FLAG_TC7 | DMA1_FLAG_GL7);
    DMA_SetCurrDataCounter(DMA1_Channel7, telem_active_samples);
    TIM_SetCounter(TIM4, 0);

    telem_ready = false;
    telem_sample_count = 0;
    telem_arm_ts = telem_now();
    telem_sampling = true;

    DMA_Cmd(DMA1_Channel7, ENABLE);
    TIM_Cmd(TIM4, ENABLE);
}

void drv_dshot_telem_disarm(void)
{
    telem_sampling = false;
    TIM_Cmd(TIM4, DISABLE);
    DMA_Cmd(DMA1_Channel7, DISABLE);
}

void drv_dshot_telem_poll(void)
{
    if (!telem_sampling)
        return;

    const uint16_t remaining = DMA_GetCurrDataCounter(DMA1_Channel7);
    const bool done = (remaining == 0) ||
                      ((int32_t)(telem_now() - telem_arm_ts) > (int32_t)(TELEM_SAMPLE_TIMEOUT_US * telem_ticks_per_us));
    if (!done)
        return;

    TIM_Cmd(TIM4, DISABLE);
    DMA_Cmd(DMA1_Channel7, DISABLE);
    telem_sampling = false;
    telem_sample_count = (uint16_t)(telem_active_samples - remaining);
    telem_ready = true;
}

bool drv_dshot_telem_take_raw(const uint16_t **samples, uint16_t *count)
{
    if (!telem_ready)
        return false;

    *samples = telem_samples;
    *count = telem_sample_count;
    telem_ready = false;

    return true;
}

uint8_t drv_dshot_telem_runs(uint8_t ch, const uint16_t *samples, uint16_t count, uint8_t *runs, uint8_t max_runs,
                             uint8_t *initial_level)
{
    if (ch >= DSHOT_CHANNELS || count == 0 || max_runs == 0)
        return 0;

    const uint16_t mask = telem_pin_bit[ch];
    uint8_t level = (samples[0] & mask) ? 1u : 0u;
    *initial_level = level;

    uint8_t nruns = 0;
    uint16_t run = 1;

    for (uint16_t i = 1; i < count; i++)
    {
        const uint8_t l = (samples[i] & mask) ? 1u : 0u;
        if (l == level)
        {
            run++;
            continue;
        }

        if (nruns < max_runs)
            runs[nruns] = (run > 255u) ? 255u : (uint8_t)run;
        nruns++;
        level = l;
        run = 1;
    }

    if (nruns < max_runs)
        runs[nruns] = (run > 255u) ? 255u : (uint8_t)run;
    nruns++;

    return (nruns > max_runs) ? max_runs : nruns;
}

void DMA1_Channel7_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void DMA1_Channel7_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC7) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC7);
        TIM_Cmd(TIM4, DISABLE);
        DMA_Cmd(DMA1_Channel7, DISABLE);
        telem_sample_count = telem_active_samples;
        telem_sampling = false;
        telem_ready = true;
    }
}

// ---------------------------------------------------------------------------
// 解码：移植 Betaflight dshot_bitbang_decode.c
// ---------------------------------------------------------------------------

#define TELEM_DECODE_MIN ((21 - 2) * TELEM_OVERSAMPLE)
#define TELEM_DECODE_MAX ((21 + 2) * TELEM_OVERSAMPLE)

// 5bit GCR -> 4bit nibble；0x80 表示非法
static const uint8_t telem_gcr_decode[32] = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 9,    10,
                                             11,   0x80, 13,   14,   15,   0x80, 0x80, 2,    3,    0x80, 5,
                                             6,    7,    0x80, 0,    8,    1,    0x80, 4,    12,   0x80};

static int telem_decode_value(uint32_t value, uint16_t *out)
{
    // 丢弃起始位
    value &= 0xFFFFFu;

    const uint8_t a = telem_gcr_decode[value & 0x1Fu];
    const uint8_t b = telem_gcr_decode[(value >> 5) & 0x1Fu];
    const uint8_t c = telem_gcr_decode[(value >> 10) & 0x1Fu];
    const uint8_t d = telem_gcr_decode[(value >> 15) & 0x1Fu];
    if (((a | b | c | d) & 0x80u) != 0u)
        return -1;

    const uint16_t decoded = (uint16_t)((uint16_t)a | ((uint16_t)b << 4) | ((uint16_t)c << 8) | ((uint16_t)d << 12));

    uint16_t csum = decoded;
    csum ^= (uint16_t)(csum >> 8);
    csum ^= (uint16_t)(csum >> 4);
    if ((csum & 0x0Fu) != 0x0Fu)
        return -1;

    *out = (uint16_t)(decoded >> 4);
    return 0;
}

int drv_dshot_telem_decode(uint8_t ch, const uint16_t *samples, uint16_t count, uint16_t *value12)
{
    if (ch >= DSHOT_CHANNELS || count < TELEM_DECODE_MIN)
        return -1;

    const uint16_t mask = telem_pin_bit[ch];
    const uint16_t *p = samples;
    const uint16_t *endP = p + (count - TELEM_DECODE_MIN);

    // 跳过前导空闲高，找第一个低电平（应答帧起始）
    while (p < endP)
    {
        if (!(*p++ & mask))
            break;
    }
    if (p >= endP)
        return -1;

    const uint16_t *oldP = p;
    uint32_t bits = 0;
    uint32_t value = 0;
    uint16_t lastValue = 0;

    uint16_t remaining = (uint16_t)(count - (uint16_t)(p - samples));
    if (remaining > TELEM_DECODE_MAX)
        remaining = TELEM_DECODE_MAX;
    endP = p + remaining;

    // 每个电平段（长度 len 个位）还原成 1 后跟 len-1 个 0（NRZI 差分）
    while (endP > p)
    {
        if ((*p++ & mask) != lastValue)
        {
            if (endP > p)
            {
                int len = (int)((p - oldP + 1) / TELEM_OVERSAMPLE);
                if (len < 1)
                    len = 1;
                bits += (uint32_t)len;
                value <<= len;
                value |= 1u << (len - 1);
                oldP = p;
                lastValue = *(p - 1) & mask;
            }
        }
    }

    if (bits < 18u || bits > 21u)
        return -1;

    // 末段长度需要反推（反向 DShot 最后一个位为高）
    const int nlen = (int)(21u - bits);
    if (nlen > 0)
    {
        value <<= nlen;
        value |= 1u << (nlen - 1);
    }

    return telem_decode_value(value, value12);
}

uint32_t drv_dshot_telem_erpm(uint16_t value12)
{
    if (value12 == 0x0FFFu)
        return 0;

    const uint32_t period = (uint32_t)(value12 & 0x01FFu) << ((value12 & 0xFE00u) >> 9);
    if (period == 0)
        return 0;

    return ((600000u + period / 2u) / period) * 100u;
}