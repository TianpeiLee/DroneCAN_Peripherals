#include "esc.h"

#include "drv_dshot.h"
#include "drv_dshot_telem.h"
#include "drv_tick.h"

#include "dronecan_msgs.h"
#include "dronecan_node.h"

#include <math.h>
#include <stdio.h>

uint64_t esc_index = 0;
uint64_t esc_pole_pairs = 6;
uint64_t esc_dshot_rate_hz = 500;
bool use_dshot = true;
bool esc_edt_enable = true;

#define ESC_CMD_MAX 8191

#define TELEM_RUNS_MAX 32
#define ESC_STATUS_PERIOD_MS 50u
#define TELEM_STALE_MS 1000u
#define ESC_CMD_TIMEOUT_MS 500u

// EDT 前缀（bit11:8）-> 类型（prefix>>1）
enum
{
    EDT_TYPE_TEMPERATURE = 1,
    EDT_TYPE_VOLTAGE = 2,
    EDT_TYPE_CURRENT = 3,
    EDT_TYPE_DEBUG1 = 4,
    EDT_TYPE_DEBUG2 = 5,
    EDT_TYPE_STRESS = 6,
    EDT_TYPE_STATUS = 7,
};

// 每通道原始采集打印次数预算（调试用）
static uint8_t esc_dump_budget[DSHOT_CHANNELS];

typedef struct
{
    uint32_t rpm;
    float voltage;
    float current;
    float temperature;
    uint8_t status_flags;
    uint32_t error_count;
    uint32_t last_rx_ms;
    bool valid;
    bool has_voltage;
    bool has_current;
    bool has_temperature;
} esc_status_t;

static esc_status_t esc_status[DSHOT_CHANNELS];

static uint32_t esc_next_tx_us;               
static uint16_t esc_cmd_value[DSHOT_CHANNELS];
static bool esc_armed;                        
static uint32_t esc_last_cmd_ms;              

static bool esc_edt_confirmed;
static uint8_t esc_edt_burst;
static uint8_t esc_edt_attempts;
static uint32_t esc_edt_last_attempt_ms;

void esc_init(void)
{
    drv_dshot_init(DShot_Type_600);
    drv_dshot_telem_init();

    for (uint8_t ch = 0; ch < DSHOT_CHANNELS; ch++)
    {
        esc_dump_budget[ch] = 3;
        esc_status[ch] = (esc_status_t){0};
        esc_cmd_value[ch] = 0;
    }

    esc_next_tx_us = 0;
    esc_armed = false;
    esc_last_cmd_ms = 0;

    esc_edt_confirmed = false;
    esc_edt_burst = 0;
    esc_edt_attempts = 0;
    esc_edt_last_attempt_ms = 0;
}

static uint16_t esc_cmd_to_dshot(int16_t v)
{
    if (v <= 0)
        return 0;

    const uint32_t span = (uint32_t)(DSHOT_VALUE_MAX - DSHOT_VALUE_MIN);
    return (uint16_t)(DSHOT_VALUE_MIN + ((uint32_t)v * span) / (uint32_t)ESC_CMD_MAX);
}

// EDT typed frame：前缀为偶数且非 0。返回类型并取出 8bit 数据
static bool esc_edt_parse(uint16_t value12, uint8_t *type_index, uint8_t *data)
{
    const uint8_t prefix = (uint8_t)((value12 >> 8) & 0x0F);
    if ((prefix & 0x01u) != 0u || prefix == 0u)
        return false;

    *type_index = (uint8_t)(prefix >> 1);
    *data = (uint8_t)(value12 & 0xFF);
    return true;
}

void esc_handle_raw_command(CanardInstance *ins, CanardRxTransfer *transfer)
{
    (void)ins;

    struct uavcan_equipment_esc_RawCommand cmd;

    if (uavcan_equipment_esc_RawCommand_decode(transfer, &cmd))
    {
        return;
    }

    bool any_throttle = false;
    for (uint8_t ch = 0; ch < cmd.cmd.len; ch++)
    {
        const int16_t v = cmd.cmd.data[ch];
        esc_cmd_value[ch] = esc_cmd_to_dshot(v);
        if (v > 0)
            any_throttle = true;
    }

    if (cmd.cmd.len > 0)
        esc_armed = any_throttle;

    esc_last_cmd_ms = get_tick_ms();
}

static void esc_dshot_tick(uint32_t now_ms)
{
    const bool cmd_fresh = (now_ms - esc_last_cmd_ms) < ESC_CMD_TIMEOUT_MS;
    const bool disarmed = (!cmd_fresh) || (!esc_armed);

    // EDT 使能：未确认前，未解锁时以 500ms 周期连发 6 帧命令 13
    if (esc_edt_enable && !esc_edt_confirmed && disarmed)
    {
        if (esc_edt_burst == 0 && (esc_edt_attempts == 0 || (now_ms - esc_edt_last_attempt_ms) >= EDT_RETRY_MS))
        {
            esc_edt_burst = EDT_BURST_FRAMES;
            esc_edt_last_attempt_ms = now_ms;
            esc_edt_attempts++;
        }

        if (esc_edt_burst > 0)
        {
            for (uint8_t ch = 0; ch < DSHOT_CHANNELS; ch++)
                drv_dshot_set(ch, EDT_CMD_ENABLE, true);
            drv_dshot_send();
            esc_edt_burst--;
            return;
        }
    }

    for (uint8_t ch = 0; ch < DSHOT_CHANNELS; ch++)
    {
        drv_dshot_set(ch, cmd_fresh ? esc_cmd_value[ch] : 0u, true);
    }

    drv_dshot_send();
}

static void esc_telem_apply(uint8_t ch, uint16_t value12, uint32_t now_ms)
{
    esc_status_t *st = &esc_status[ch];
    const uint32_t pole_pairs = (esc_pole_pairs != 0) ? (uint32_t)esc_pole_pairs : 1u;

    uint8_t type_index;
    uint8_t data;

    if (!esc_edt_parse(value12, &type_index, &data))
    {
        st->rpm = drv_dshot_telem_erpm(value12) / pole_pairs;
        st->valid = true;
        st->last_rx_ms = now_ms;
        return;
    }

    esc_edt_confirmed = true;
    st->valid = true;
    st->last_rx_ms = now_ms;

    switch (type_index)
    {
    case EDT_TYPE_TEMPERATURE:
        st->temperature = (float)data;
        st->has_temperature = true;
        
        break;
    case EDT_TYPE_VOLTAGE:
        st->voltage = (float)data * 0.25f;
        st->has_voltage = true;
        break;
    case EDT_TYPE_CURRENT:
        st->current = (float)data;
        st->has_current = true;
        break;
    case EDT_TYPE_STATUS:
        st->status_flags = data;
        break;
    default:
        break;
    }
}

static void esc_telem_consume(const uint16_t *samples, uint16_t count, uint32_t now_ms)
{
    for (uint8_t ch = 0; ch < DSHOT_CHANNELS; ch++)
    {
        uint16_t value12;
        if (drv_dshot_telem_decode(ch, samples, count, &value12) == 0)
        {
            esc_telem_apply(ch, value12, now_ms);
            if (esc_dump_budget[ch] > 0)
            {
                uint8_t type_index;
                uint8_t data;
                esc_dump_budget[ch]--;
                if (esc_edt_parse(value12, &type_index, &data))
                {
                    printf("TELEM ch=%u EDT type=%u data=%u\n", ch, type_index, data);
                }
                else
                {
                    printf("TELEM ch=%u val=0x%03X erpm=%lu rpm=%lu\n", ch, value12,
                           (unsigned long)drv_dshot_telem_erpm(value12), (unsigned long)esc_status[ch].rpm);
                }
            }
            continue;
        }

        uint8_t runs[TELEM_RUNS_MAX];
        uint8_t initial_level = 0;
        const uint8_t n = drv_dshot_telem_runs(ch, samples, count, runs, TELEM_RUNS_MAX, &initial_level);
        if (n < 6)
            continue;

        esc_status[ch].error_count++;

        if (esc_dump_budget[ch] > 0)
        {
            esc_dump_budget[ch]--;
            printf("TELEM ch=%u DEC-FAIL n=%u init=%u runs:", ch, n, initial_level);
            for (uint8_t i = 0; i < n; i++)
                printf(" %u", runs[i]);
            printf("\n");
        }
    }
}

static void esc_publish(uint32_t now_ms)
{
    static uint32_t last_ms;
    static uint8_t transfer_id[DSHOT_CHANNELS];

    if (now_ms - last_ms < ESC_STATUS_PERIOD_MS)
        return;
    last_ms = now_ms;

    for (uint8_t ch = 0; ch < DSHOT_CHANNELS; ch++)
    {
        if (!esc_status[ch].valid)
            continue;

        const bool fresh = (now_ms - esc_status[ch].last_rx_ms) < TELEM_STALE_MS;

        struct uavcan_equipment_esc_Status msg = {
            .error_count = esc_status[ch].error_count,
            .voltage = (fresh && esc_status[ch].has_voltage) ? esc_status[ch].voltage : NAN,
            .current = (fresh && esc_status[ch].has_current) ? esc_status[ch].current : NAN,
            .temperature = (fresh && esc_status[ch].has_temperature) ? (esc_status[ch].temperature + 273.15f) : NAN,
            .rpm = fresh ? (int32_t)esc_status[ch].rpm : 0,
            .power_rating_pct = 0,
            .esc_index = ch,
        };

        uint8_t buffer[UAVCAN_EQUIPMENT_ESC_STATUS_MAX_SIZE];
        const uint32_t len = uavcan_equipment_esc_Status_encode(&msg, buffer);

        canardBroadcast(&canard, UAVCAN_EQUIPMENT_ESC_STATUS_SIGNATURE, UAVCAN_EQUIPMENT_ESC_STATUS_ID,
                        &transfer_id[ch], CANARD_TRANSFER_PRIORITY_LOW, buffer, len);
    }
}

void esc_update(void)
{
    const uint32_t now_ms = get_tick_ms();
    const uint32_t now_us = get_tick_us();

    const uint32_t rate = (esc_dshot_rate_hz > 0) ? (uint32_t)esc_dshot_rate_hz : 500u;
    const uint32_t period_us = 1000000u / rate;
    if ((int32_t)(now_us - esc_next_tx_us) >= 0)
    {
        esc_next_tx_us = now_us + period_us;
        esc_dshot_tick(now_ms);
    }

    drv_dshot_telem_poll();

    const uint16_t *samples;
    uint16_t count;
    if (drv_dshot_telem_take_raw(&samples, &count))
    {
        esc_telem_consume(samples, count, now_ms);
    }

    esc_publish(now_ms);
}
